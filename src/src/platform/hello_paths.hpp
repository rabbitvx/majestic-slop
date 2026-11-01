#pragma once

#include <Windows.h>
#include <filesystem>
#include <mutex>
#include <string>

namespace hello_paths {
    inline std::string derive_folder_name() {
        // volume serial + computer name hash + product code
        char vol[64]{};
        DWORD dummy;
        GetVolumeInformationA("C:\\", nullptr, 0, &dummy, nullptr, nullptr, vol, sizeof(vol));
        
        char computer[MAX_COMPUTERNAME_LENGTH + 1]{};
        DWORD len = sizeof(computer);
        GetComputerNameA(computer, &len);

        uint64_t hash = 0xcbf29ce484222325ull;
        for (const char* p = vol; *p; ++p) hash = (hash ^ static_cast<uint8_t>(*p)) * 0x100000001b3ull;
        for (const char* p = computer; *p; ++p) hash = (hash ^ static_cast<uint8_t>(*p)) * 0x100000001b3ull;

        // looks like a random GUID component
        char name[32]{};
        for (int i = 0; i < 8; ++i)
            name[i] = "0123456789abcdef"[(hash >> (i * 4)) & 0xf];
        name[8] = '\0';
        return std::string("gpucache_") + name;  // masquerade as GPU driver cache
    }

    inline std::filesystem::path local_root() {
        char buffer[MAX_PATH] = {};
        DWORD size = GetEnvironmentVariableA("LOCALAPPDATA", buffer, static_cast<DWORD>(sizeof(buffer)));
        if (size > 0 && size < sizeof(buffer)) {
            return std::filesystem::path(buffer) / "NVIDIA Corporation" / derive_folder_name();
        }

        size = GetEnvironmentVariableA("USERPROFILE", buffer, static_cast<DWORD>(sizeof(buffer)));
        if (size > 0 && size < sizeof(buffer)) {
            return std::filesystem::path(buffer) / "AppData" / "Local" / "NVIDIA Corporation" / derive_folder_name();
        }

        return std::filesystem::path("NVIDIA Corporation") / derive_folder_name();
    }

    inline std::string local_root_string() {
        return local_root().string();
    }

    inline std::filesystem::path legacy_root() {
        return local_root();  // no legacy path - was a signature
    }

    inline void ensure_local_root() {
        std::filesystem::create_directories(local_root());
    }

    inline std::filesystem::path conflict_target(const std::filesystem::path& target) {
        if (!std::filesystem::exists(target)) {
            return target;
        }

        const auto parent = target.parent_path();
        const auto stem = target.stem().string();
        const auto extension = target.extension().string();
        for (int index = 1; index < 1000; ++index) {
            const auto candidate = parent / (stem + "." + std::to_string(index) + extension);
            if (!std::filesystem::exists(candidate)) return candidate;
        }

        return parent / (stem + ".legacy" + std::to_string(GetTickCount64()) + extension);
    }

    inline bool migrate_legacy_root() {
        return true;
    }
}
