# Dumps majestic-client.dll (runtime-decrypted image) from GTA5.exe memory.
# Usage: run while the game is in-game (client loaded); writes majestic_client_dump.bin next to this script.
import ctypes, ctypes.wintypes as wt, struct, sys, os, subprocess

PROCESS_VM_READ = 0x0010
PROCESS_QUERY_INFORMATION = 0x0400

def enable_debug():
    class LUID(ctypes.Structure):
        _fields_ = [("LowPart", wt.DWORD), ("HighPart", ctypes.c_long)]
    class TOKEN_PRIVILEGES(ctypes.Structure):
        _fields_ = [("PrivilegeCount", wt.DWORD), ("Luid", LUID), ("Attributes", wt.DWORD)]
    hToken = wt.HANDLE()
    if not ctypes.windll.advapi32.OpenProcessToken(ctypes.windll.kernel32.GetCurrentProcess(),
            0x0020 | 0x0008, ctypes.byref(hToken)):
        print("OpenProcessToken failed", ctypes.get_last_error()); return
    luid = LUID()
    ctypes.windll.advapi32.LookupPrivilegeValueW(None, "SeDebugPrivilege", ctypes.byref(luid))
    tp = TOKEN_PRIVILEGES(1, luid, 2)
    ctypes.windll.advapi32.AdjustTokenPrivileges(hToken, False, ctypes.byref(tp), 0, None, None)
    ctypes.windll.kernel32.CloseHandle(hToken)
    print("SeDebugPrivilege enabled")

enable_debug()

# Find GTA5 pid
out = subprocess.run(['tasklist','/FI','IMAGENAME eq GTA5.exe','/FO','CSV','/NH'],capture_output=True,text=True).stdout
pid = None
for line in out.strip().split('\n'):
    if 'GTA5' in line:
        pid = int(line.strip().strip('"').split('","')[1].strip('"'))
        break
if not pid:
    print("GTA5 not running"); sys.exit(1)
print(f"GTA5 PID: {pid}")

psapi = ctypes.WinDLL('psapi.dll')
k32 = ctypes.windll.kernel32
h = k32.OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, pid)
if not h:
    print("OpenProcess FAILED", ctypes.get_last_error()); sys.exit(1)

# Enumerate modules, find majestic-client.dll
hMods = (ctypes.c_void_p * 4096)()
cb = wt.DWORD()
psapi.EnumProcessModules(h, hMods, ctypes.sizeof(hMods), ctypes.byref(cb))
count = cb.value // 8
target = None
for i in range(count):
    name = ctypes.create_unicode_buffer(512)
    mod = ctypes.c_void_p(hMods[i])
    psapi.GetModuleFileNameExW(h, mod, name, 512)
    if 'majestic-client.dll' in name.value.lower():
        class MI(ctypes.Structure):
            _fields_ = [('base', ctypes.c_void_p), ('size', wt.DWORD), ('ep', ctypes.c_void_p)]
        mi = MI()
        psapi.GetModuleInformation(h, mod, ctypes.byref(mi), ctypes.sizeof(MI))
        target = (name.value, mi.base, mi.size)
        break
if not target:
    print("majestic-client.dll NOT loaded (are you in-game?)"); k32.CloseHandle(h); sys.exit(1)
base = target[1]; mod_size = target[2]
print(f"majestic-client.dll @ 0x{base:X} size 0x{mod_size:X} ({mod_size/1024/1024:.1f} MB)")

class MBI(ctypes.Structure):
    _fields_ = [("BaseAddress", ctypes.c_void_p),
                ("AllocationBase", ctypes.c_void_p),
                ("AllocationProtect", wt.DWORD),
                ("PartitionId", ctypes.c_ulong),
                ("RegionSize", ctypes.c_size_t),
                ("State", wt.DWORD),
                ("Protect", wt.DWORD),
                ("Type", wt.DWORD)]

# Walk memory within the module allocation
addr = base
max_end = base
regions = []
while addr < base + mod_size + 0x1000:
    mbi = MBI()
    r = ctypes.windll.kernel32.VirtualQueryEx(h, ctypes.c_void_p(addr), ctypes.byref(mbi), ctypes.sizeof(MBI))
    if not r:
        break
    end = addr + mbi.RegionSize
    if mbi.State == 0x1000:
        regions.append((addr, end, mbi.Protect))
        max_end = max(max_end, end)
    if mbi.AllocationBase and mbi.AllocationBase != base:
        if mbi.AllocationBase > base + 0x10000:
            break
    addr = end

total_size = max_end - base
print(f"Module extent: 0x{base:X} - 0x{max_end:X} ({total_size/1024/1024:.1f} MB), regions: {len(regions)}")

out = bytearray(total_size)
ok_read = 0
for (s, e, prot) in regions:
    if prot & 0x100:
        continue
    size = e - s
    chunk = ctypes.create_string_buffer(size)
    br = ctypes.c_size_t(0)
    if ctypes.windll.kernel32.ReadProcessMemory(h, ctypes.c_void_p(s), chunk, size, ctypes.byref(br)):
        out[s - base : s - base + br.value] = chunk.raw[:br.value]
        ok_read += br.value

print(f"Read {ok_read/1024/1024:.1f} MB / {total_size/1024/1024:.1f} MB")

outpath = os.path.join(os.path.dirname(os.path.abspath(__file__)), "majestic_client_dump.bin")
with open(outpath, 'wb') as f:
    f.write(out)
print(f"Dumped to {outpath}")
k32.CloseHandle(h)
