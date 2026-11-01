#pragma once
// minimal ws server on 127.0.0.1 for the payload bridge
// js payload opens ws://127.0.0.1:45678 and pushes
// esp_data json here. data is parsed with the same
// ws_server::parse_log_line and lands in g_log_players.
// implements: TCP accept loop, HTTP upgrade handshake (RFC 6455), text frames
// (single-frame, no masking from server side), close frames

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
#include <cstdio>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <cstdint>
#include "network/ws_bridge.hpp"

#pragma comment(lib, "ws2_32.lib")

namespace ws_bridge_server {

    inline constexpr uint16_t kPort = 45678;
    inline std::atomic<bool> g_running{false};
    inline std::thread g_thread;
    inline SOCKET g_listen = INVALID_SOCKET;

    inline std::string base64_encode(const uint8_t* data, size_t len) {
        static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        out.reserve(((len + 2) / 3) * 4);
        for (size_t i = 0; i < len; i += 3) {
            uint32_t n = data[i] << 16;
            if (i + 1 < len) n |= data[i + 1] << 8;
            if (i + 2 < len) n |= data[i + 2];
            out.push_back(tbl[(n >> 18) & 63]);
            out.push_back(tbl[(n >> 12) & 63]);
            out.push_back(i + 1 < len ? tbl[(n >> 6) & 63] : '=');
            out.push_back(i + 2 < len ? tbl[n & 63] : '=');
        }
        return out;
    }

    inline std::string sha1_base64(const std::string& input) {
        // SHA-1 of the handshake key + GUID, base64 (required by RFC 6455)
        // implemented via CryptAPI (bcrypt)
        HCRYPTPROV prov = 0;
        HCRYPTHASH hash = 0;
        std::string out;
        if (!CryptAcquireContextW(&prov, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return out;
        if (CryptCreateHash(prov, CALG_SHA1, 0, 0, &hash)) {
            if (CryptHashData(hash, reinterpret_cast<const BYTE*>(input.data()), static_cast<DWORD>(input.size()), 0)) {
                BYTE digest[20] = {};
                DWORD len = 20;
                if (CryptGetHashParam(hash, HP_HASHVAL, digest, &len, 0)) {
                    out = base64_encode(digest, len);
                }
            }
            CryptDestroyHash(hash);
        }
        CryptReleaseContext(prov, 0);
        return out;
    }

    // returns: 1 = full frame in payload_out, 0 = connection closed, -1 = would-block (no data)
    inline int handle_frame(SOCKET s, std::string& payload_out, bool& closed) {
        uint8_t hdr[2] = {};
        int r = recv(s, reinterpret_cast<char*>(hdr), 2, 0);
        if (r < 0) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) { closed = false; return -1; }
            closed = true; return 0;
        }
        if (r == 0) { closed = true; return 0; }
        if (r == 1) { // need second byte
            int r2 = recv(s, reinterpret_cast<char*>(hdr) + 1, 1, 0);
            if (r2 < 0) {
                if (WSAGetLastError() == WSAEWOULDBLOCK) { closed = false; return -1; }
                closed = true; return 0;
            }
            if (r2 == 0) { closed = true; return 0; }
        }
        const bool fin = (hdr[0] & 0x80) != 0;
        const uint8_t opcode = hdr[0] & 0x0F;
        const bool masked = (hdr[1] & 0x80) != 0;
        uint64_t len = hdr[1] & 0x7F;

        if (len == 126) {
            uint8_t ext[2] = {};
            int r2 = recv(s, reinterpret_cast<char*>(ext), 2, 0);
            if (r2 < 0) { if (WSAGetLastError() == WSAEWOULDBLOCK) { closed = false; return -1; } closed = true; return 0; }
            if (r2 != 2) { closed = true; return 0; }
            len = (ext[0] << 8) | ext[1];
        } else if (len == 127) {
            uint8_t ext[8] = {};
            int r2 = recv(s, reinterpret_cast<char*>(ext), 8, 0);
            if (r2 < 0) { if (WSAGetLastError() == WSAEWOULDBLOCK) { closed = false; return -1; } closed = true; return 0; }
            if (r2 != 8) { closed = true; return 0; }
            len = 0;
            for (int i = 0; i < 8; i++) len = (len << 8) | ext[i];
        }

        if (len > 8 * 1024 * 1024) { closed = true; return 0; }

        uint8_t mask[4] = {};
        if (masked) {
            int r2 = recv(s, reinterpret_cast<char*>(mask), 4, 0);
            if (r2 < 0) { if (WSAGetLastError() == WSAEWOULDBLOCK) { closed = false; return -1; } closed = true; return 0; }
            if (r2 != 4) { closed = true; return 0; }
        }

        std::string data;
        data.resize(len);
        size_t got = 0;
        while (got < len) {
            int n = recv(s, &data[got], static_cast<int>(len - got), 0);
            if (n < 0) {
                if (WSAGetLastError() == WSAEWOULDBLOCK) { closed = false; return -1; }
                closed = true; return 0;
            }
            if (n == 0) { closed = true; return 0; }
            got += n;
        }
        if (masked) {
            for (size_t i = 0; i < len; i++) data[i] ^= mask[i & 3];
        }

        if (opcode == 0x8) { // close
            closed = true;
            return 0;
        }
        if (opcode == 0x9) { // ping -> pong
            uint8_t pong[2] = { 0x8A, 0x00 };
            send(s, reinterpret_cast<char*>(pong), 2, 0);
            return -1;
        }
        if (opcode == 0x1 && fin) { // text frame complete
            payload_out = std::move(data);
            return 1;
        }
        return -1;
    }

    inline bool window_focused_now() {
        // Robust for windowed-borderless: WM_ACTIVATEAPP/WM_SIZE are unreliable
        // when the game is minimized in borderless mode, but the foreground
        // window always changes. Fall back to the WndProc-tracked flag.
        HWND fg = GetForegroundWindow();
        HWND game = ws_server::g_window;
        if (fg && game) {
            return fg == game || GetAncestor(fg, GA_ROOT) == GetAncestor(game, GA_ROOT);
        }
        return ws_server::g_window_focused.load(std::memory_order_acquire);
    }

    inline void handle_client(SOCKET s) {
        std::string req;
        char buf[1024];
        bool got_header = false;
        while (!got_header) {
            int n = recv(s, buf, sizeof(buf), 0);
            if (n <= 0) { closesocket(s); return; }
            req.append(buf, n);
            if (req.find("\r\n\r\n") != std::string::npos) got_header = true;
            if (req.size() > 16384) { closesocket(s); return; }
        }
        std::string key;
        size_t pos = req.find("Sec-WebSocket-Key:");
        if (pos != std::string::npos) {
            pos += 19;
            while (pos < req.size() && (req[pos] == ' ' || req[pos] == '\t')) pos++;
            size_t end = req.find("\r\n", pos);
            key = req.substr(pos, end - pos);
        }
        if (key.empty()) { closesocket(s); return; }
        const std::string accept = sha1_base64(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        std::string resp =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
        if (send(s, resp.data(), static_cast<int>(resp.size()), 0) <= 0) { closesocket(s); return; }

        // Passive: payload pushes ESP_DATA on its own 500ms timer. No ask frames.
        while (g_running) {
            std::string payload;
            bool closed = false;
            const int fr = handle_frame(s, payload, closed);
            if (fr == 1) {
                if (!payload.empty()) {
                    ws_server::parse_log_line(payload);
                }
                continue;
            }
            if (fr == 0) { closesocket(s); return; }
            Sleep(10); // would-block
        }
        closesocket(s);
    }

    inline void server_thread() {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;
        g_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (g_listen == INVALID_SOCKET) { WSACleanup(); return; }
        BOOL opt = TRUE;
        setsockopt(g_listen, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(kPort);
        if (::bind(g_listen, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            closesocket(g_listen); g_listen = INVALID_SOCKET; WSACleanup(); return;
        }
        if (listen(g_listen, 4) != 0) {
            closesocket(g_listen); g_listen = INVALID_SOCKET; WSACleanup(); return;
        }
        while (g_running) {
            SOCKET c = accept(g_listen, nullptr, nullptr);
            if (c == INVALID_SOCKET) continue;
            std::thread(handle_client, c).detach();
        }
        closesocket(g_listen);
        g_listen = INVALID_SOCKET;
        WSACleanup();
    }

    inline void start() {
        if (g_running.exchange(true)) return;
        g_thread = std::thread(server_thread);
    }

    inline void stop() {
        g_running = false;
        if (g_listen != INVALID_SOCKET) {
            closesocket(g_listen);
            g_listen = INVALID_SOCKET;
        }
        if (g_thread.joinable()) g_thread.join();
    }
}
