import ctypes, ctypes.wintypes as wt, struct, subprocess, re

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
        print(f"GTA5.exe @ 0x{gta_base:X} size 0x{gta_size:X}")
        break

def rmem(addr, size):
    buf = ctypes.create_string_buffer(size)
    br = ctypes.c_size_t(0)
    if k32.ReadProcessMemory(h, ctypes.c_void_p(addr), buf, size, ctypes.byref(br)):
        return buf.raw[:br.value]
    return None

def scan(pattern_hex, name):
    pat = bytes.fromhex(pattern_hex.replace('?','0').replace(' ',''))
    mask = ''.join('?' if c == '?' else 'x' for c in pattern_hex.split())
    # build pattern with None wildcards
    toks = pattern_hex.split()
    plen = len(toks)
    p = [None if t == '??' else int(t,16) for t in toks]
    chunk = 0x400000
    for off in range(0, min(gta_size, 0x6000000), chunk):  # scan first 96MB (code)
        sz = min(chunk, min(gta_size, 0x6000000) - off)
        data = rmem(gta_base + off, sz)
        if not data: continue
        for i in range(len(data) - plen):
            ok = True
            for j in range(plen):
                if p[j] is not None and data[i+j] != p[j]:
                    ok = False; break
            if ok:
                print(f"  {name}: НАЙДЕН @ 0x{gta_base + off + i:X} (RVA 0x{off+i:X})")
                return gta_base + off + i
    print(f"  {name}: НЕ НАЙДЕН")
    return None

print("\n=== Проверка паттернов ===")
sp = scan("41 8B CF C7 85 ?? ?? ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? B2 01", "shapeTestPattern")
if sp:
    start = sp - 0xBE
    print(f"  _START_SHAPE_TEST_CAPSULE = 0x{start:X}")
    # читаем первые байты для подтверждения
    head = rmem(start, 24)
    if head:
        print(f"  байты: {head.hex(' ')}")
scan("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 56 41 57 48 8B EC 48 83 EC 60 33 DB", "_GET_RAYCAST_RESULT")

k32.CloseHandle(h)
