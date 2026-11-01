# Legacy single-shot dump: copies the committed regions of a module out of a live process.
# Usage: dumper.py <pid> <module_base_hex> [out_path]
#        out_path defaults to <script dir>/majestic_dump.bin
import ctypes, ctypes.wintypes as wt, struct, sys, os, time

PROCESS_VM_READ = 0x0010
PROCESS_QUERY_INFORMATION = 0x0400

if len(sys.argv) < 3:
    print(__doc__ or "usage: dumper.py <pid> <module_base_hex> [out_path]")
    sys.exit(2)

try:
    pid = int(sys.argv[1], 0)
    base = int(sys.argv[2], 16)
except ValueError:
    print("pid must be decimal/0x-prefixed, base must be hex (e.g. 0x7FFC00000000)")
    sys.exit(2)

outpath = sys.argv[3] if len(sys.argv) > 3 else os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "majestic_dump.bin")

# Enable SeDebugPrivilege
def enable_debug():
    class LUID(ctypes.Structure):
        _fields_ = [("LowPart", wt.DWORD), ("HighPart", ctypes.c_long)]
    class TOKEN_PRIVILEGES(ctypes.Structure):
        _fields_ = [("PrivilegeCount", wt.DWORD), ("Luid", LUID), ("Attributes", wt.DWORD)]
    hToken = wt.HANDLE()
    if not ctypes.windll.advapi32.OpenProcessToken(ctypes.windll.kernel32.GetCurrentProcess(),
            0x0020 | 0x0008, ctypes.byref(hToken)):
        print("OpenProcessToken failed", ctypes.get_last_error())
        return
    luid = LUID()
    ctypes.windll.advapi32.LookupPrivilegeValueW(None, "SeDebugPrivilege", ctypes.byref(luid))
    tp = TOKEN_PRIVILEGES(1, luid, 2)  # SE_PRIVILEGE_ENABLED
    ctypes.windll.advapi32.AdjustTokenPrivileges(hToken, False, ctypes.byref(tp), 0, None, None)
    ctypes.windll.kernel32.CloseHandle(hToken)
    print("SeDebugPrivilege enabled")

enable_debug()

h = ctypes.windll.kernel32.OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, pid)
if not h:
    print("OpenProcess FAILED, error", ctypes.get_last_error())
    sys.exit(1)
print("OpenProcess OK, handle", hex(h))

# Query module memory layout
class MBI(ctypes.Structure):
    _fields_ = [("BaseAddress", ctypes.c_void_p),
                ("AllocationBase", ctypes.c_void_p),
                ("AllocationProtect", wt.DWORD),
                ("PartitionId", ctypes.c_ulong),
                ("RegionSize", ctypes.c_size_t),
                ("State", wt.DWORD),
                ("Protect", wt.DWORD),
                ("Type", wt.DWORD)]

# Walk memory from base to find module extent
addr = base
max_end = base
regions = []
while addr < base + 0x9000000:
    mbi = MBI()
    r = ctypes.windll.kernel32.VirtualQueryEx(h, ctypes.c_void_p(addr), ctypes.byref(mbi), ctypes.sizeof(MBI))
    if not r:
        break
    end = addr + mbi.RegionSize
    if mbi.State == 0x1000:  # MEM_COMMIT
        regions.append((addr, end, mbi.Protect))
        max_end = max(max_end, end)
    # Stop when we leave the module allocation
    if mbi.AllocationBase and mbi.AllocationBase != base:
        if mbi.AllocationBase > base + 0x10000:
            break
    addr = end
    if addr > base + 0x9000000:
        break

total_size = max_end - base
print(f"Module extent: 0x{base:X} - 0x{max_end:X} ({total_size/1024/1024:.1f} MB)")
print(f"Regions: {len(regions)}")

# Read whole module
buf = ctypes.create_string_buffer(total_size)
bytes_read = ctypes.c_size_t(0)
# Read region by region (contiguous read may fail on guard pages)
out = bytearray(total_size)
ok_read = 0
for (s, e, prot) in regions:
    if prot & 0x100:  # PAGE_GUARD
        continue
    size = e - s
    chunk = ctypes.create_string_buffer(size)
    br = ctypes.c_size_t(0)
    if ctypes.windll.kernel32.ReadProcessMemory(h, ctypes.c_void_p(s), chunk, size, ctypes.byref(br)):
        out[s - base : s - base + br.value] = chunk.raw[:br.value]
        ok_read += br.value

print(f"Read {ok_read/1024/1024:.1f} MB / {total_size/1024/1024:.1f} MB")

with open(outpath, 'wb') as f:
    f.write(out)

print(f"Dumped to {outpath}")
ctypes.windll.kernel32.CloseHandle(h)
