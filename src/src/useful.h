#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <shlobj.h>
#include <ctime>
#include <dxgi.h>
#include <atomic>
#include <mutex>
#include <cstdio>
#include "xor.h"
#include "platform/build_profile.hpp"

using namespace std;

namespace logs {
	inline std::mutex g_log_mutex;

	inline void ensure_log_dir() {
	}

	inline string make_prefix() {
		SYSTEMTIME st{};
		GetLocalTime(&st);
		char buffer[128];
		sprintf_s(
			buffer,
			"[%04u-%02u-%02u %02u:%02u:%02u.%03u][tid:%lu] ",
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
			GetCurrentThreadId()
		);
		return string(buffer);
	}

	inline void directWrite(const string& line) {
		(void)line; // nop no traces
	}

	inline void add(string msg) {
		directWrite(make_prefix() + msg + "\r\n");
	}

	inline void clear() {
	}

	inline void addLog(string msg) {
		add(std::move(msg));
	}

	inline void addHResult(string label, HRESULT hr) {
		char buffer[64];
		sprintf_s(buffer, "0x%08X", static_cast<unsigned int>(hr));
		add(label + " " + buffer);
	}

	inline void logSystemInfo() {
		add("pid=" + std::to_string(GetCurrentProcessId()));
	}
}
