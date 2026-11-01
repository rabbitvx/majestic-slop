import glob, os, json, re

log_dir = os.path.expandvars(r"%APPDATA%\majestic-launcher\Multiplayer\logs")
logs = sorted(glob.glob(os.path.join(log_dir, "client_*.log")), key=os.path.getmtime)
newest = logs[-1]
print(f"Лог: {os.path.basename(newest)}  ({os.path.getsize(newest)/1024/1024:.1f} MB)")
with open(newest, 'r', encoding='utf-8', errors='replace') as f:
    lines = f.readlines()

def parse(l):
    idx = l.find('{')
    if idx == -1: return None
    try: return json.loads(l[idx:])
    except: return None

for tag in ['DIAG_READY', 'DIAG_LISTENERS', 'DIAG_FRACTION']:
    hits = [parse(l) for l in lines if tag in l]
    hits = [h for h in hits if h]
    print(f"{tag}: {len(hits)}")

dl = [parse(l) for l in lines if 'DIAG_LISTENERS' in l]
dl = [d for d in dl if d]
if dl:
    d = dl[0]
    print(f"\n=== DIAG_LISTENERS ===")
    for store in ['local', 'remote']:
        v = d.get(store)
        print(f"\n{store} ({len(v) if isinstance(v, list) else 'obj'}):")
        if isinstance(v, list):
            for e in v:
                print(f"  {e}")
        elif isinstance(v, dict):
            for k, cnt in sorted(v.items()):
                print(f"  {k}: {cnt}")
        else:
            print(f"  {v}")

df = [parse(l) for l in lines if 'DIAG_FRACTION' in l]
df = [d for d in df if d]
if df:
    d = df[0]
    print(f"\n=== DIAG_FRACTION (local player) ===")
    print(f"player: {d.get('player')}")
    probe = d.get('probe') or {}
    print(f"найдено ключей: {len(probe)}")
    for k, v in probe.items():
        print(f"  {k} = {json.dumps(v, ensure_ascii=False)[:70]}")

esp = [l for l in lines if 'ESP_DATA' in l]
print(f"\nESP_DATA: {len(esp)}")
