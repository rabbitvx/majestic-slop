const WS_KEY = "__14f7b2_bridge";

if (typeof globalThis[WS_KEY] !== "undefined") {
    try { globalThis[WS_KEY].unload(); } catch (_) {}
}

let tickId = null;
let alive = true;
let serverInfoSent = false;

function safe(fn) {
    try { return fn(); } catch (_) { return null; }
}

let ws = null;
let wsReconnectDelay = 2000;
const WS_URLS = ["ws://127.0.0.1:45678", "ws://localhost:45678"];
let wsUrlIdx = 0;

function wsSend(text) {
    try {
        if (ws && ws.readyState === 1) {
            ws.send(text);
            return true;
        }
    } catch (_) {}
    return false;
}

function wsConnect() {
    if (!alive) return;
    try {
        const WS_URL = WS_URLS[wsUrlIdx % WS_URLS.length];
        ws = new alt.WebSocketClient(WS_URL);
        ws.on("open", () => {
            wsReconnectDelay = 2000;
        });
        ws.on("error", () => {
            wsUrlIdx++;
            try { ws.close(); } catch (_) {}
            ws = null;
            if (alive) setTimeout(wsConnect, wsReconnectDelay);
        });
        ws.on("close", () => {
            wsUrlIdx++;
            ws = null;
            if (alive) setTimeout(wsConnect, wsReconnectDelay);
        });
        try { ws.start(); } catch (_) {}
    } catch (_) {
        if (alive) setTimeout(wsConnect, wsReconnectDelay);
    }
}
wsConnect();

const FRACTION_NAMES = {
    1: "LSPD", 2: "EMS", 3: "Sheriff", 4: "SANG", 5: "GOV",
    6: "WN", 7: "FIB", 8: "Ballas", 9: "Vagos", 10: "Families",
    11: "Bloods", 12: "Marabunta"
};

function fractionName(id) {
    return FRACTION_NAMES[id] || "";
}

const SERVER_CODE_TO_ID = {
    RU1: "new york", RU2: "detroit", RU3: "chicago", RU4: "san francisco",
    RU5: "atlanta", RU6: "san diego", RU7: "los angeles", RU8: "miami",
    RU9: "las vegas", RU10: "washington", RU11: "dallas", RU12: "boston",
    RU13: "houston", RU14: "seattle", RU15: "phoenix", RU16: "denver",
    RU17: "portland", RU18: "orlando", RU19: "memphis"
};

function detectServer() {
    if (serverInfoSent) return;
    try {
        let code = "";
        try { code = String(globalThis.server || "").trim().toUpperCase().replace(/\s+/g, ""); } catch (_) {}
        let server = SERVER_CODE_TO_ID[code] || "";
        let recognized = server.length > 0;
        if (!recognized) {
            const ip = safe(() => alt.getServerIp && alt.getServerIp()) || "";
            const port = safe(() => alt.getServerPort && alt.getServerPort()) || 0;
            if (ip) { server = ip + ":" + port; recognized = true; }
        }
        if (server) {
            wsSend(JSON.stringify({ type: "SERVER_INFO", server: server, recognized: recognized }));
            serverInfoSent = true;
        }
    } catch (_) {}
}

// Per-player meta cache: getStreamSyncedMeta is expensive (blocks the JS loop,
// which also delays the majestic voice manager). Static fields (id, login, level,
// admin, family, fraction, flags) are cached and only refreshed every REFRESH_MS;
// position/health/weapon are read live each tick because they change constantly.
const META_REFRESH_MS = 2000;
const metaCache = new Map(); // key: scriptID -> { stamp, fields }

function meta(p, key, cache) {
    const now = Date.now();
    if (now - cache.stamp > META_REFRESH_MS) {
        cache.stamp = now;
        cache.f.id = safe(() => p.getStreamSyncedMeta("id")) || 0;
        cache.f.login = safe(() => p.getStreamSyncedMeta("login")) || "";
        cache.f.level = safe(() => p.getStreamSyncedMeta("level")) || 0;
        cache.f.admin = safe(() => p.getStreamSyncedMeta("admin")) || 0;
        cache.f.family = safe(() => p.getStreamSyncedMeta("family")) || 0;
        cache.f.member = safe(() => p.getStreamSyncedMeta("member")) || 0;
        cache.f.isDead = safe(() => p.getStreamSyncedMeta("isDead")) || false;
        cache.f.isAfk = safe(() => p.getStreamSyncedMeta("afkSprite")) || false;
        cache.f.isMedia = safe(() => p.getStreamSyncedMeta("media")) || false;
        cache.f.isTester = safe(() => p.getStreamSyncedMeta("tester")) || false;
        cache.f.cuffed = safe(() => p.getStreamSyncedMeta("cuffed")) || false;
        cache.f.wantedLevel = safe(() => p.getStreamSyncedMeta("wantedLevel")) || 0;
        cache.f.hasWearArmor = safe(() => p.getStreamSyncedMeta("hasWearArmor")) || false;
    }
    return cache.f[key] !== undefined ? cache.f[key] : 0;
}

function collectPlayers() {
    if (!alive) return;
    const list = [];
    const all = safe(() => alt.Player.all);
    if (!all) return;

    for (const p of all) {
        if (!p || !p.valid) continue;
        const scriptId = safe(() => p.scriptID) || 0;
        if (scriptId <= 0) continue;
        const pos = safe(() => p.pos);

        let cache = metaCache.get(scriptId);
        if (!cache) {
            cache = { stamp: 0, f: {} };
            metaCache.set(scriptId, cache);
        }

        const login = meta(p, "login", cache);
        const staticId = meta(p, "id", cache);
        const level = meta(p, "level", cache);
        if (!login && !staticId && !level) continue;

        const fraction_id = meta(p, "member", cache);
        list.push({
            handle:     scriptId,
            // p.id is NOT unique in this client (multiple players share id values), so key the match on the ped handle (scriptID) which equals pointer_to_handle in GTA memory
            netID:      scriptId,
            name:       login || safe(() => p.name) || "Unknown",
            social:     safe(() => p.name) || "",
            login:      login,
            staticID:   staticId,
            level:      level,
            adminLevel: meta(p, "admin", cache),
            isAdmin:    meta(p, "admin", cache) > 0,
            family:     meta(p, "family", cache),
            isDead:     meta(p, "isDead", cache),
            isAfk:      meta(p, "isAfk", cache),
            isMedia:    meta(p, "isMedia", cache),
            isTester:   meta(p, "isTester", cache),
            hp:         safe(() => p.health) || 100,
            armor:      safe(() => p.armour) || 0,
            weaponHash: safe(() => p.currentWeapon) || 0,
            posX:       pos ? pos.x : 0,
            posY:       pos ? pos.y : 0,
            posZ:       pos ? pos.z : 0,
            fraction:   fractionName(fraction_id),
            fractionID: fraction_id,
            familyID:   meta(p, "family", cache),
            cuffed:     meta(p, "cuffed", cache),
            wantedLevel: meta(p, "wantedLevel", cache),
            hasWearArmor: meta(p, "hasWearArmor", cache),
            seq:        Date.now()
        });
    }

    // Drop cache entries for players that are no longer present (prevents unbounded growth).
    if (metaCache.size > all.length + 16) {
        const live = new Set();
        for (const p of all) {
            const sid = safe(() => p && p.scriptID) || 0;
            if (sid) live.add(sid);
        }
        for (const key of metaCache.keys()) {
            if (!live.has(key)) metaCache.delete(key);
        }
    }
    return list;
}

function tick() {
    if (!alive) return;
    detectServer();
    const objects = collectPlayers();
    if (objects && objects.length > 0) {
        const msg = JSON.stringify({ type: "ESP_DATA", objects: objects });
        wsSend(msg);
    }
}
// 500ms interval: fast ESP flag updates (original behaviour).
tickId = safe(() => alt.setInterval(tick, 500));

const api = {
    unload() {
        alive = false;
        if (tickId !== null) { safe(() => alt.clearInterval(tickId)); tickId = null; }
        if (ws) { try { ws.close(); } catch (_) {} ws = null; }
        metaCache.clear();
        if (globalThis[WS_KEY] === api) safe(() => delete globalThis[WS_KEY]);
    }
};

globalThis[WS_KEY] = api;
