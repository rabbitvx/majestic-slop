#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <utility>

#include "platform/build_profile.hpp"
#include "platform/hello_paths.hpp"

namespace runtime_debug {
	inline thread_local const char* last_section = "unknown";
	inline thread_local DWORD last_exception_code = 0;
	inline thread_local void* last_exception_addr = nullptr;
	inline thread_local const char* last_native_source = nullptr;
	inline thread_local uint64_t last_native_hash = 0;
	inline thread_local void* last_native_handler = nullptr;
	inline thread_local uint32_t last_native_arg_count = 0;

	inline void clear_native_context() {
		last_native_source = nullptr;
		last_native_hash = 0;
		last_native_handler = nullptr;
		last_native_arg_count = 0;
	}

	struct scoped_section {
		const char* previous;

		explicit scoped_section(const char* section) : previous(last_section) {
			last_section = section ? section : "unknown";
		}

		~scoped_section() {
			last_section = previous ? previous : "unknown";
		}

		void set(const char* section) {
			last_section = section ? section : "unknown";
		}

		scoped_section(const scoped_section&) = delete;
		scoped_section& operator=(const scoped_section&) = delete;
	};


	inline constexpr bool enabled = false;
	inline constexpr bool file_log_enabled = false;

	inline void ensure_log_dir() {
		if constexpr (file_log_enabled) {
			hello_paths::ensure_local_root();
		}
	}

	inline void append_file_line(const char* text) {
		if constexpr (!file_log_enabled) {
			return;
		}
		else {
			if (!text) return;

			ensure_log_dir();
			const std::string log_path = (hello_paths::local_root() / "hello.log").string();
			HANDLE file = CreateFileA(log_path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file == INVALID_HANDLE_VALUE) return;

			SYSTEMTIME st{};
			GetLocalTime(&st);

			char line[2304];
			int len = snprintf(
				line,
				sizeof(line),
				"[%04u-%02u-%02u %02u:%02u:%02u.%03u][tid:%lu] %s\r\n",
				st.wYear,
				st.wMonth,
				st.wDay,
				st.wHour,
				st.wMinute,
				st.wSecond,
				st.wMilliseconds,
				GetCurrentThreadId(),
				text);

			if (len > 0) {
				DWORD written = 0;
				WriteFile(file, line, static_cast<DWORD>(len < static_cast<int>(sizeof(line)) ? len : sizeof(line) - 1), &written, nullptr);
			}

			CloseHandle(file);
		}
	}

	inline void init() {
		ensure_log_dir();
	}

	template <typename... Args>
	inline void log(const char* fmt, Args&&... args) {
		if constexpr (!enabled) {
			return;
		}
		else {
			if (!fmt) return;

			char buffer[2048];
			snprintf(buffer, sizeof(buffer), fmt, std::forward<Args>(args)...);

			OutputDebugStringA("[debug] ");
			OutputDebugStringA(buffer);
			OutputDebugStringA("\n");
			append_file_line(buffer);
		}
	}

	inline void install() {}
}

#define HELLO_RUNTIME_LOG(...) ((void)0)
