# majestic-slop

Vibecoded slop for **GTA V / Majestic RP (alt:V)**: a single C++17 DLL (built as `dxilconv.dll`) that hooks D3D11
`Present` + the GTA script thread.

Everything is one shared library, header-heavy, compiled with MSVC or the OLLVM-patched `clang-cl`
shipped by `build_ollvm.*`. There is no server component, no licence backend, and no telemetry.

> **Status:** detected. Several subsystems are stubs (see [Known gaps](#known-gaps)); the source is
> published as-is and is not supported.

---

## Features

Six menu tabs, `aimbot` / `visuals{players,other}` / `misc` / `settings` / `playerlist` / `hashes`
(`src/src/ui/menu/menu.cpp:3864-3893`). Every control is bound to a config key and is also exposed as a
`__builtin` menu item (`<tab>/<card>/<label>`) so script UI can override its value, tooltip, visibility
or disabled state.

### Aimbot

| Option | Notes | Config key |
|---|---|---|
| enabled (with hotkey) | bindable hold / toggle / always-on | `aimbot/vector_enable`, `binds/vector_aim_key`, `binds/vector_aim_mode` |
| bone | multi-select: head, neck, chest, spine, pelvis | `aimbot/vector_aim_bones` |
| fov | 0–500 px, drawn as a circle | `aimbot/vector_radius`, `aimbot/vector_show_radius` |
| priority | `screen distance` / `3d distance` | `aimbot/vector_priority` |
| switch delay | 0–2000 ms before retargeting | `aimbot/target_switch_delay` |
| smoothness | 0–100 %, with per-weapon-class overrides (`sniper_`, `rifle_`, `sg_`, `smg_`, `pistol_`) | `aimbot/vector_smoothness` |
| target timeout | 0–2000 ms | `aimbot/target_timeout` |
| ignore friends | skips players marked as friend | `aimbot/ignore_friends` |
| visible only | ray/line-of-sight check before aiming | `aimbot/visible_only` |
| ignore my family / ignore other families | plus a managed list of family ids | `aimbot/ignore_family`, `aimbot/ignore_other_families`, `aimbot/ignore_families` |
| ignore my fraction / ignore other fractions | GOV, Marabunta, SANG, EMS, LSPD, FIB, WN, Ballas, Vagos, Sheriff, Families, Bloods | `aimbot/ignore_fraction`, `aimbot/ignore_other_fractions`, `aimbot/ignore_fractions` |

Aim math lives in `src/src/features/aimbot/aim.hpp` (skeleton-driven target points, smoothing, bone
selection by weapon class). `aimbot/force_shot` exists in the UI but has no reader.

### Visuals → players (ESP)

| Option | Notes | Config key |
|---|---|---|
| enabled | bindable | `visual/enable`, `visual/esp_key`, `visual/esp_key_mode` |
| box | styles `default` / `corner` / `filled`, radius, thickness, visible + invisible + fill colours | `visual/draw_box`, `visual/draw_box_style`, `visual/box_radius`, `visual/box_thickness` |
| skeleton | visible/invisible colours, thickness, `preview invisible` | `visual/draw_skeleton`, `visual/skel_*` |
| name / relation / fraction | alt:V nickname, friend/enemy/family/fraction tags with auto-detection and per-tag colours | `visual/altv_nickname`, `visual/altv_relation`, `visual/altv_faction` |
| health | `none` / `hp only` / `hp and armor` / `adaptive`, static colour, armour colour | `visual/health_mode`, `visual/healthbar_*`, `visual/armorbar_color_*` |
| weapon | `icon` (embedded PUA glyph font) and/or `text`, english/russian naming | `visual/draw_weapon_icon`, `visual/draw_weapon_text`, `visual/esp_weapon_text_language` |
| flags | distance, static id, admin, tester, level, dead, afk, media, cuffed, wanted, armor, social club, visible | `visual/altv_*`, `visual/draw_distance` |
| render distance | 1–300 m | `hack/max_range` |
| admins around | indicator when staff are nearby | `visual/admins_around_indicator` |
| dim dead players | | `visual/dim_dead_players` |
| cheap render text | cheaper text path for large player counts | `visual/cheap_render_text` |

Per-element layout is fully user-editable: **drag** the 3D preview to rotate, **scroll** over an element
to resize it, **right-click** an element to open its popup (font `mini`/`bold`, case, colour, size,
`show value` for bars). Layout is persisted per element (`esp_pos_*`, `visual/esp_size_*`).
The preview is a real off-screen render: `happ.glb` + `bones.glb` are compiled into the DLL and drawn
into a 512×512 render target (`src/src/ui/preview/model_preview.cpp`).

### Visuals → other (weapons & world)

| Option | Notes | Config key |
|---|---|---|
| weapon detection | enabled, widget | `visual/weapons_enabled`, `visual/weapons_widget` |
| offscreen arrows | arrow size, icon size, distance %, per-weapon filter list, select all / clear | `visual/weapon_arrows`, `visual/weapon_arrow_*`, `visual/weapon_arrow_weapons` |
| highlight player | highlights players carrying selected weapons on ESP | `visual/weapons_highlight`, `visual/weapons_highlighted` |
| per-weapon settings | custom name, colour, draw line, optional sound alert | `visual/weapons_highlight_custom*`, `weapons_highlight_lines`, `visual/weapons_sound` |
| world ESP | pickups/objects with `text` / `icon` / `distance`, distance limit, icon size, colour | `visual/world_esp*` |

Weapons are keyed by joaat hash and mapped to glyphs from the embedded `majestic_weapon_icons` font
(`src/src/features/visuals/esp/weapon_icon_map.hpp`, `src/src/assets/majestic_weapon_icons_font.hpp`).
A heavy-sniper banner (`HEAVY WEAPON DETECTED`) fires from `esp/alerts.hpp`.

### Misc

| Option | Config key |
|---|---|
| custom fov + value (30–120) | `hacks/custom_fov`, `hacks/custom_fov_value` |
| aspect ratio + value (0.5–3.0) | `hacks/custom_aspect`, `hacks/custom_aspect_value` |
| radar (zoomable, `Radarbox` window) — config only | `visual/radar`, `visual/radar_zoom` |
| waypoints + colour — config only | `misc/waypoints_enable`, `misc/waypoints_draw`, `misc/waypoint_color_*` |
| vehicle fast-stop — bind only, no UI | `hacks/veh_fast_stop_enabled`, `binds/veh_fast_stop_key`, `binds/veh_fast_stop_mode` |

### Settings

| Option | Notes | Config key |
|---|---|---|
| watermark | HUD name, fps, date | `hack/watermark` |
| keybinds / indicators | on-screen bind list, `ADMINS: n` counter | `hud/keybinds`, `hud/indicators` |
| accent colour | RGB | `hud/accent_r/g/b` |
| menu key | default `END` | `menu/menu_key` |
| panic key | hold / toggle / always-on; snapshots and disables 30 ESP keys while active, restores on release | `misc/panic_enabled`, `binds/panic_key`, `binds/panic_mode` |
| require teleport approval | teleport needs RMB confirmation while preview key is held | `misc/require_teleport_approval` |
| unload | ordered teardown, unhooks everything, `FreeLibraryAndExitThread` | — |
| configs | local JSON list: load / save / delete / refresh | `%LOCALAPPDATA%\NVIDIA Corporation\gpucache_<8 hex>\*.json` |

### Playerlist

Live player list from the game thread plus alt:V metadata: avatar, display name, static/dynamic id,
alt:V name, family id, fraction, admin level. Rows are tinted by friend/enemy/family/fraction marks.
Per-player actions: `mark as friend` (ignored by aimbot, `FRIEND` tag in ESP), `mark as enemy`,
`ignore this family`.

### Hashes

Object/pickup hash browser (`0x<HASH> (<name>)`) with per-hash overrides used by world/pickup ESP:
display name, custom colour, draw line, draw box, reset. Master toggle `visual/pickup_esp`
(bindable via `visual/pickup_esp_key`), `visual/pickup_show_all`, `visual/pickup_ignore_local_held_objects`.

---

## How it works

| Stage | Implementation |
|---|---|
| Injection | `dxilconv.dll` is injected into `GTA5.exe`; `DllMain` caches the module base, then spawns an init thread (`src/src/app/entry.cpp`) |
| Init | session seeding → `config::load()` → AOB scan (`pointers::findAllPatterns`, ~20 signatures) → hooks → payload executor → 500 ms watchdog |
| Rendering | kiero D3D11 vtable hooks: `Present` (idx 8) and `ResizeBuffers` (idx 13), only on the swapchain whose window class is `grcWindow`; ImGui DX11 + Win32 backends, one full-screen transparent backbuffer window |
| Game logic | MinHook on `gta_script_thread_tick`; script-hash allow-list filters the thread, then a Win32 **fiber** pump runs `game::game_tick()` once per frame |
| Input | polled (`GetAsyncKeyState` / `GetCursorPos`) rather than the WndProc hook — the WndProc patch is present but never installed |
| Native calls | two parallel invokers: `native::invoker` (hash → `eax` via the 6.4k-entry `native/maps.hpp`, then the client's native handler) and a self-contained `native_invoke.hpp`; both fix up vector args via `gta_fix_context_vector` |
| ESP data | done in-process from `ReplayInterface`/`World` (player list, 24 bones, visibility) and merged with alt:V metadata |
| Telemetry (optional) | local WebSocket **server** on `127.0.0.1:45678` (RFC 6455, hand-rolled) receiving `ESP_DATA` and `SERVER_INFO` from a JS payload inside `majestic-client.dll` |
| JS payload | no JS engine of its own: AOB-scans `majestic-client.dll` for the client's script buffer and replaces it with `// payload guard` + `payload.js` + original source, so majestic's own engine runs it (`src/src/executor/`) |
| Config | typed in-memory cache (`normal` / `string_map` / `dword_map` / `vector3_map`) serialised to JSON; files live under `%LOCALAPPDATA%\NVIDIA Corporation\gpucache_<8 hex>\` (name derived from volume label + host, FNV-1a) |
| Teardown | player control re-enabled, aimbot shut down, WS server closed, fov/aspect restored, hooks removed, module unloaded |

The overlay draws only into the game window; the majestic client's own swapchains are skipped.

---

## Build

Requirements: Windows x64, Visual Studio 2022 (MSVC + Windows SDK), CMake ≥ 3.20, Ninja or MSBuild.

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output: `src/build/dxilconv.dll` (static CRT, `/O1 /Ob2`, `/LTCG /OPT:REF /OPT:ICF`).

Options: `-DHELLO_ENABLE_RELEASE_LOGS=ON`, `-DHELLO_ENABLE_CRASH_LOGS=ON` (both off by default;
all logging is compiled out).

### OLLVM pipeline

`build_ollvm.ps1` (or the `.bat` twin) downloads stock **LLVM 16.0.6** (~400 MB, installed into `./ollvm`),
downloads the OLLVM-patched `clang-cl.7z` from `wwh1004/ollvm-16`, overlays it onto `ollvm/bin`, then
configures CMake with that compiler/linker (`build_ollvm/`).

```powershell
.\build_ollvm.ps1            # full build
.\build_ollvm.ps1 -NoBuild   # configure only
```

Caveat: `HELLO_USE_OLLVM` is passed by the scripts but `src/CMakeLists.txt` neither declares the option
nor adds `-mllvm -fla/-bcf/-sub/...` flags, so the passes are only active if the patched compiler enables
them by default.

---

## Repository layout

```
CMakeLists.txt          root -> src/CMakeLists.txt
src/src/app             DllMain / entry
src/src/runtime         init orchestration, session state, task start-up
src/src/hooks.hpp       Present, ResizeBuffers, script-thread detours, fiber pump
src/src/core            imports (god header, pattern scanner), memory, integrity bypass
src/src/native          signature database, native hash->id maps, invoker
src/src/features        aimbot, misc (tick/binds), visuals (esp, radar, weapon icons)
src/src/render          D3D11/ImGui renderer
src/src/ui/menu         menu, widgets, search, colorpicker, notifications
src/src/ui/preview      off-screen 3D model preview
src/src/network         WebSocket server, payload bridge, nickname cache
src/src/executor        payload.js + in-memory script-buffer injection
src/src/config          typed config cache + local JSON backend
src/thirdparty          imgui, kiero, minhook, nlohmann_json, simplemath, cgltf/stb, freetype
tools/vmpdump           research scripts for dumping the VMProtect-protected client
```

`ollvm/`, `ollvm_patch/`, `build*/`, `src/build/`, `*.zip`, `*.exe` under `tools/vmpdump` are ignored —
they are downloaded or produced by the build scripts.

`tools/vmpdump` is not part of the runtime: `dump_client.py` finds `GTA5.exe`, enables
`SeDebugPrivilege`, walks `majestic-client.dll`'s committed regions after the VMProtect stub has decrypted
them and writes a flat dump; `dumper.py` is the earlier manual variant and takes the target explicitly:

```
dumper.py <pid> <module_base_hex> [out_path]     # out_path defaults to <script dir>/majestic_dump.bin
```

`check_*.py` are probes used to validate the ESP JSON schema, bone ordering and native/raycast hashes;
they auto-detect the game process. No script contains machine-specific paths — dumps land next to the
script unless you pass a path.

---

## Known gaps

Present in the tree, but inert by design or unfinished — do not expect them to work:

- No licence/auth backend: `runtime_context::read()` returns stubs, the `.rdata$zy`/`.rdata$zz` blobs and
  TLS gate are never consumed (`magic` is 0), `HELLO_VMP_*` expands to nothing.
- WebSocket bridge is **inbound only**: `send_config`, `send_execute`, `set_menu_item_value` and friends are
  empty stubs; the log-tailing path and the `NtWriteFile` hook are never started.
- `runtime_tasks::start_ws_server()` is empty; the executor starts the server itself.
- Aimbot `force_shot` is UI-only. Menu controls registered as builtin items can collide when two controls
  in one card share a visible label.
- Several UI builders are dead code (unused `player_esp_options`, `weapon_options`, `vector_aim_options`,
  `cfg_bind_state`, `k_bone_items`).
- `src/src/ui/ui.hpp` and `src/src/core.cpp` are empty scaffolding.
- `src/src/features/visuals/esp.hpp` is an include hub, not a feature file — the ESP code is under `esp/`.

---

## Notes

- Version-locked constants that break on client updates: client RVAs `0x3F9734` / `0x3F96B2`
  (`src/src/core/integrity_bypass.hpp`), the JS buffer AOB (`src/src/executor/executor.hpp`), the
  script-thread hash allow-list (`src/src/hooks.hpp:490`), and the native hash map
  (`src/src/native/maps.hpp`).
- The DLL is named `dxilconv.dll` and its data directory masquerades as an NVIDIA driver cache
  (`%LOCALAPPDATA%\NVIDIA Corporation\gpucache_<hash>`).
- Use at your own risk. Nothing here is sanctioned by, affiliated with, or endorsed by Rockstar Games,
  majestic RP or alt:V; using it on a live server violates their rules and will get you banned.
