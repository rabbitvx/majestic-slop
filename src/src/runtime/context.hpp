#pragma once

#include <Windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "platform/build_profile.hpp"
#include "platform/vmp.hpp"

namespace runtime_context {
    inline constexpr unsigned char marker_byte(size_t index) {
        constexpr unsigned char encoded[16] {
            0x14, 0x19, 0x0e, 0x08, 0x0e, 0x19, 0x02, 0x68,
            0x37, 0xcb, 0xb8, 0x10, 0x42, 0xe4, 0x29, 0x55,
        };
        return static_cast<unsigned char>(encoded[index] ^ 0x5a);
    }

#pragma pack(push, 1)
    struct patch_context {
        unsigned char marker[16];
        unsigned char masked_key[32];
        unsigned char encrypted_payload[240];
    };
#pragma pack(pop)

#pragma section(".rdata$zy", read, write)
    __declspec(allocate(".rdata$zy")) inline volatile patch_context g_patch_context {
        { 0x4e, 0x43, 0x54, 0x52, 0x54, 0x43, 0x58, 0x32, 0x6d, 0x91, 0xe2, 0x4a, 0x18, 0xbe, 0x73, 0x0f },
        {},
        {},
    };

    struct context_view {
        std::string product_session;
        std::string product_code;
        std::string tg_id;
        std::string hwid;
        std::array<unsigned char, 32> context_seed{};
    };

    inline std::string read_fixed(const volatile char* source, size_t size) {
        std::string out;
        out.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            const char ch = source[i];
            if (ch == '\0') break;
            out.push_back(ch);
        }
        return out;
    }

    inline bool seed_is_empty(const std::array<unsigned char, 32>& seed) {
        unsigned char mixed = 0;
        for (unsigned char byte : seed) {
            mixed |= byte;
        }
        return mixed == 0;
    }

    template <size_t Size>
    inline void decode_label(unsigned char (&out)[Size], const unsigned char (&encoded)[Size]) {
        for (size_t i = 0; i < Size; ++i) out[i] = static_cast<unsigned char>(encoded[i] ^ 0x6b);
    }

    inline bool marker_matches() {
        for (size_t i = 0; i < 16; ++i) {
            if (g_patch_context.marker[i] != marker_byte(i)) return false;
        }
        return true;
    }

    inline bool patch_data_present() {
        unsigned char mixed = 0;
        for (size_t i = 0; i < sizeof(g_patch_context.masked_key); ++i) {
            mixed |= g_patch_context.masked_key[i];
        }
        for (size_t i = 0; i < sizeof(g_patch_context.encrypted_payload); ++i) {
            mixed |= g_patch_context.encrypted_payload[i];
        }
        return mixed != 0;
    }

    inline uint32_t rotr32(uint32_t value, int bits) {
        return (value >> bits) | (value << (32 - bits));
    }

    inline void sha256(const unsigned char* data, size_t size, unsigned char out[32]) {
        uint32_t h[8] = { 0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au, 0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u };
        static constexpr uint32_t k[64] = {
            0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
            0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
            0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
            0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
            0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
            0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
            0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
            0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
        };
        const uint64_t bit_len = static_cast<uint64_t>(size) * 8ull;
        std::vector<unsigned char> msg(data, data + size);
        msg.push_back(0x80);
        while ((msg.size() % 64) != 56) msg.push_back(0);
        for (int i = 7; i >= 0; --i) msg.push_back(static_cast<unsigned char>(bit_len >> (i * 8)));
        for (size_t offset = 0; offset < msg.size(); offset += 64) {
            uint32_t w[64]{};
            for (int i = 0; i < 16; ++i) w[i] = (uint32_t(msg[offset + i * 4]) << 24) | (uint32_t(msg[offset + i * 4 + 1]) << 16) | (uint32_t(msg[offset + i * 4 + 2]) << 8) | msg[offset + i * 4 + 3];
            for (int i = 16; i < 64; ++i) {
                const uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
                const uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }
            uint32_t a=h[0], b=h[1], c=h[2], d=h[3], e=h[4], f=h[5], g=h[6], hh=h[7];
            for (int i = 0; i < 64; ++i) {
                const uint32_t s1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
                const uint32_t ch = (e & f) ^ (~e & g);
                const uint32_t temp1 = hh + s1 + ch + k[i] + w[i];
                const uint32_t s0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
                const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                const uint32_t temp2 = s0 + maj;
                hh=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
            }
            h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
        }
        for (int i = 0; i < 8; ++i) {
            out[i * 4] = static_cast<unsigned char>(h[i] >> 24);
            out[i * 4 + 1] = static_cast<unsigned char>(h[i] >> 16);
            out[i * 4 + 2] = static_cast<unsigned char>(h[i] >> 8);
            out[i * 4 + 3] = static_cast<unsigned char>(h[i]);
        }
        SecureZeroMemory(msg.data(), msg.size());
    }

    inline void decrypt_payload(const volatile patch_context* context, unsigned char payload[240]) {
        HELLO_VMP_SCOPE_ULTRA("runtime_context.decrypt_payload");
        unsigned char mask_material[32]{};
        unsigned char mask_input[32]{};
        static constexpr unsigned char encoded_mask_label[15]{ 0x08, 0x1f, 0x13, 0x46, 0x00, 0x0e, 0x12, 0x46, 0x06, 0x0a, 0x18, 0x00, 0x44, 0x1d, 0x5a };
        unsigned char mask_label[sizeof(encoded_mask_label)]{};
        decode_label(mask_label, encoded_mask_label);
        memcpy(mask_input, mask_label, sizeof(mask_label));
        for (size_t i = 0; i < 16; ++i) mask_input[sizeof(mask_label) + i] = context->marker[i];
        sha256(mask_input, sizeof(mask_label) + 16, mask_material);
        SecureZeroMemory(mask_label, sizeof(mask_label));
        unsigned char key[32]{};
        for (size_t i = 0; i < 32; ++i) key[i] = context->masked_key[i] ^ mask_material[i];
        for (size_t i = 0; i < 240; ++i) payload[i] = context->encrypted_payload[i];
        for (size_t offset = 0; offset < 240; offset += 32) {
            unsigned char block_input[64]{};
            static constexpr unsigned char encoded_label[14]{ 0x08, 0x1f, 0x13, 0x46, 0x1b, 0x0a, 0x12, 0x07, 0x04, 0x0a, 0x0f, 0x44, 0x1d, 0x5a };
            unsigned char label[sizeof(encoded_label)]{};
            decode_label(label, encoded_label);
            memcpy(block_input, label, sizeof(label));
            memcpy(block_input + sizeof(label), key, 32);
            block_input[sizeof(label) + 32] = static_cast<unsigned char>(offset);
            block_input[sizeof(label) + 33] = static_cast<unsigned char>(offset >> 8);
            unsigned char block[32]{};
            sha256(block_input, sizeof(label) + 34, block);
            for (size_t i = 0; i < 32 && offset + i < 240; ++i) payload[offset + i] ^= block[i];
            SecureZeroMemory(label, sizeof(label));
            SecureZeroMemory(block, sizeof(block));
            SecureZeroMemory(block_input, sizeof(block_input));
        }
        SecureZeroMemory(key, sizeof(key));
        SecureZeroMemory(mask_material, sizeof(mask_material));
        SecureZeroMemory(mask_input, sizeof(mask_input));
    }

    inline bool read(context_view& out) { // I could just remove you, you know?
        HELLO_VMP_SCOPE_VIRTUALIZATION("runtime_context.read");
        out.product_session = "dummy_session_placeholder_32b";
        out.product_code    = "altv";
        out.tg_id           = "dummy_tg_id_placeholder_32byte";
        out.hwid            = "dummy_hwid_placeholder_128_bytes________________________________________________________________________________________________________";
        for (size_t i = 0; i < out.context_seed.size(); ++i)
            out.context_seed[i] = static_cast<unsigned char>(i + 1);
        return true;
    }
}
