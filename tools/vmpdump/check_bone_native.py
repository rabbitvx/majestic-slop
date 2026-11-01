import ctypes, ctypes.wintypes as wt, subprocess

out = subprocess.run(['tasklist','/FI','IMAGENAME eq GTA5.exe','/FO','CSV','/NH'],capture_output=True,text=True).stdout
pid = None
for line in out.strip().split(chr(10)):
    if 'GTA5' in line:
        pid = int(line.strip().strip('"').split('","')[1].strip('"'))
        break
if not pid:
    print("GTA5 не запущен"); exit()
print(f"PID: {pid}")

psapi = ctypes.WinDLL('psapi.dll')
k32 = ctypes.WinDLL('kernel32.dll')
h = k32.OpenProcess(0x0410, False, pid)
hMods = (ctypes.c_void_p * 2048)()
cb = wt.DWORD()
psapi.EnumProcessModules(h, hMods, ctypes.sizeof(hMods), ctypes.byref(cb))
count = cb.value // 8
gta_base = None; gta_size = 0
for i in range(count):
    name = ctypes.create_unicode_buffer(260)
    mod = ctypes.c_void_p(hMods[i])
    psapi.GetModuleFileNameExW(h, mod, name, 260)
    fn = name.value.lower()
    if 'gta5' in fn and fn.endswith('.exe'):
        class MI(ctypes.Structure):
            _fields_ = [('base', ctypes.c_void_p), ('size', wt.DWORD), ('ep', ctypes.c_void_p)]
        mi = MI()
        psapi.GetModuleInformation(h, mod, ctypes.byref(mi), ctypes.sizeof(MI))
        gta_base, gta_size = mi.base, mi.size
        print(f"GTA5 @ 0x{gta_base:X} size 0x{gta_size:X}")
        break

def rmem(addr, size):
    buf = ctypes.create_string_buffer(size)
    br = ctypes.c_size_t(0)
    if k32.ReadProcessMemory(h, ctypes.c_void_p(addr), buf, size, ctypes.byref(br)):
        return buf.raw[:br.value]
    return None

def scan(toks_hex, name):
    p = [None if t == '??' else int(t,16) for t in toks_hex]
    plen = len(p)
    chunk = 0x400000
    limit = min(gta_size, 0x6000000)
    for off in range(0, limit, chunk):
        sz = min(chunk, limit - off)
        data = rmem(gta_base + off, sz)
        if not data: continue
        for i in range(len(data) - plen):
            ok = True
            for j in range(plen):
                if p[j] is not None and data[i+j] != p[j]:
                    ok = False; break
            if ok:
                print(f"  {name}: НАЙДЕН @ 0x{gta_base+off+i:X}")
                return
    print(f"  {name}: НЕ НАЙДЕН")

print("=== GetBoneFunc паттерн ===")
scan("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 60 48 8B 01 41 8B E8 48 8B F2 48 8B F9 33 DB".split(), "GetBoneFunc")
k32.CloseHandle(h)
