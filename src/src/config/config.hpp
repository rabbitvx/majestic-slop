#pragma once
#include "core/imports.h"
#include "config/interface.hpp"
#include "config/local_backend.hpp"
#include "ui/menu/notify_bridge.hpp"
#include "platform/build_profile.hpp"
#include "platform/hello_paths.hpp"
#include "platform/vmp.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <functional>
#include <utility>

namespace config {

	std::recursive_mutex config_mutex;

	const std::string configs_folder = hello_paths::local_root_string();
	std::vector<std::string> local_configs;
	std::map<std::string, std::string> config_list;
	std::string active_config_name;
	std::string status_message;
	bool dirty = false;
	bool save_thread_running = false;
	std::function<void()> before_serialize_hook;
	std::function<void()> after_load_hook;

	void push_status_notification(const std::string& message, hello_notify::status type, bool show_notification) {
		if (!show_notification || message.empty()) {
			return;
		}

		hello_notify::push(message, type);
	}

	void set_runtime_hooks(std::function<void()> before_serialize, std::function<void()> after_load) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		before_serialize_hook = std::move(before_serialize);
		after_load_hook = std::move(after_load);
	}

	void run_before_serialize_hook() {
		std::function<void()> hook;
		{
			std::lock_guard<std::recursive_mutex> lock(config_mutex);
			hook = before_serialize_hook;
		}
		if (hook) hook();
	}

	void run_after_load_hook() {
		std::function<void()> hook;
		{
			std::lock_guard<std::recursive_mutex> lock(config_mutex);
			hook = after_load_hook;
		}
		if (hook) hook();
	}

	map<pair<string, string>, map<string, string>> string_cache;
	map<pair<string, string>, map<DWORD, string>> dword_cache;
	map<pair<string, string>, map<string, Vector3>> vector3_cache;
	map<pair<string, string>, string> normal_cache;

	namespace fast {
		double pow10(int n) {
			double ret = 1.0; double r = 10.0;
			if (n < 0) { n = -n; r = 0.1; }
			while (n) { if (n & 1) ret *= r; r *= r; n >>= 1; }
			return ret;
		}
		int atoi(const char* str) {
			int val = 0;
			while (*str) { val = val * 10 + (*str++ - '0'); }
			return val;
		}
		double atof(const char* num) {
			if (!num || !*num) return 0;
			int sign = 1; double intPart = 0, fracPart = 0;
			bool hasFrac = false, hasExp = false;
			if (*num == '-') { ++num; sign = -1; } else if (*num == '+') ++num;
			while (*num) {
				if (*num >= '0' && *num <= '9') intPart = intPart * 10 + (*num - '0');
				else if (*num == '.') { hasFrac = true; ++num; break; }
				else if (*num == 'e') { hasExp = true; ++num; break; }
				else return sign * intPart;
				++num;
			}
			if (hasFrac) {
				double fracExp = 0.1;
				while (*num) {
					if (*num >= '0' && *num <= '9') { fracPart += fracExp * (*num - '0'); fracExp *= 0.1; }
					else if (*num == 'e') { hasExp = true; ++num; break; }
					else return sign * (intPart + fracPart);
					++num;
				}
			}
			double expPart = 1.0;
			if (*num && hasExp) {
				int expSign = 1;
				if (*num == '-') { expSign = -1; ++num; } else if (*num == '+') ++num;
				int e = 0;
				while (*num && *num >= '0' && *num <= '9') { e = e * 10 + *num - '0'; ++num; }
				expPart = pow10(expSign * e);
			}
			return sign * (intPart + fracPart) * expPart;
		}
	}

	float get(string category, string key, float defaultVal) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		auto k = make_pair(category, key);
		if (normal_cache[k].length()) return (float)fast::atof(normal_cache[k].c_str());
		return defaultVal;
	}
	int get(string category, string key, int defaultVal) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		auto k = make_pair(category, key);
		if (normal_cache[k].length()) return fast::atoi(normal_cache[k].c_str());
		return defaultVal;
	}
	string get(string category, string key, string defaultVal) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		auto k = make_pair(category, key);
		if (normal_cache[k].length()) return normal_cache[k];
		return defaultVal;
	}

	map<string, string> get(string category, string key, map<string, string> defaultValue) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		auto k = make_pair(category, key);
		if (string_cache[k].size()) return string_cache[k];
		return defaultValue;
	}
	map<DWORD, string> get(string category, string key, map<DWORD, string> defaultValue) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		auto k = make_pair(category, key);
		if (dword_cache[k].size()) return dword_cache[k];
		return defaultValue;
	}
	map<string, Vector3> get(string category, string key, map<string, Vector3> defaultValue) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		auto k = make_pair(category, key);
		if (vector3_cache[k].size()) return vector3_cache[k];
		return defaultValue;
	}

	bool set(string category, string name, string setting) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		normal_cache[make_pair(category, name)] = setting;
		dirty = true;
		return true;
	}
	bool set(string category, string key, map<string, string> newValue) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		string_cache[make_pair(category, key)] = newValue;
		dirty = true;
		return true;
	}
	bool set(string category, string key, map<DWORD, string> newValue) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		dword_cache[make_pair(category, key)] = newValue;
		dirty = true;
		return true;
	}
	bool set(string category, string key, map<string, Vector3> newValue) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		vector3_cache[make_pair(category, key)] = newValue;
		dirty = true;
		return true;
	}

	bool update(float newValue, string category, string key, float defaultVal) {
		if (newValue != config::get(category, key, defaultVal))
			return config::set(category, key, to_string(newValue));
		return false;
	}
	bool update(int newValue, string category, string key, int defaultVal) {
		if (newValue != config::get(category, key, defaultVal))
			return config::set(category, key, to_string(newValue));
		return false;
	}
	bool update(string newValue, string category, string key, string defaultVal) {
		if (newValue != config::get(category, key, defaultVal))
			return config::set(category, key, newValue);
		return false;
	}
	bool update(map<string, string> newValue, string category, string key, map<string, string> defaultValue) {
		if (newValue != config::get(category, key, defaultValue))
			return config::set(category, key, newValue);
		return false;
	}
	bool update(map<DWORD, string> newValue, string category, string key, map<DWORD, string> defaultValue) {
		if (newValue != config::get(category, key, defaultValue))
			return config::set(category, key, newValue);
		return false;
	}
	bool update(map<string, Vector3> newValue, string category, string key, map<string, Vector3> defaultValue) {
		if (newValue != config::get(category, key, defaultValue))
			return config::set(category, key, newValue);
		return false;
	}

	void refresh() {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		normal_cache.clear();
		dword_cache.clear();
		string_cache.clear();
		vector3_cache.clear();
	}

	std::string serialize_to_json() {
		run_before_serialize_hook();

		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		json j;

		json nc;
		for (auto& [k, v] : normal_cache) {
			if (!v.empty())
				nc[k.first + "/" + k.second] = v;
		}
		j["normal"] = nc;

		json sc;
		for (auto& [k, v] : string_cache) {
			if (!v.empty()) {
				json m;
				for (auto& [mk, mv] : v) m[mk] = mv;
				sc[k.first + "/" + k.second] = m;
			}
		}
		j["string_map"] = sc;

		json dc;
		for (auto& [k, v] : dword_cache) {
			if (!v.empty()) {
				json m;
				for (auto& [mk, mv] : v) m[std::to_string(mk)] = mv;
				dc[k.first + "/" + k.second] = m;
			}
		}
		j["dword_map"] = dc;

		json vc;
		for (auto& [k, v] : vector3_cache) {
			if (!v.empty()) {
				json m;
				for (auto& [mk, mv] : v)
					m[mk] = to_string(mv.x) + "|" + to_string(mv.y) + "|" + to_string(mv.z);
				vc[k.first + "/" + k.second] = m;
			}
		}
		j["vector3_map"] = vc;

		return j.dump();
	}

	void deserialize_from_json(const std::string& data) {
		map<pair<string, string>, map<string, string>> next_string_cache;
		map<pair<string, string>, map<DWORD, string>> next_dword_cache;
		map<pair<string, string>, map<string, Vector3>> next_vector3_cache;
		map<pair<string, string>, string> next_normal_cache;

		try {
			json j = json::parse(data);

			if (j.contains("normal")) {
				for (auto& [k, v] : j["normal"].items()) {
					size_t sep = k.find('/');
					if (sep != std::string::npos)
						next_normal_cache[make_pair(k.substr(0, sep), k.substr(sep + 1))] = v.get<std::string>();
				}
			}

			if (j.contains("string_map")) {
				for (auto& [k, v] : j["string_map"].items()) {
					size_t sep = k.find('/');
					if (sep == std::string::npos) continue;
					map<string, string> m;
					for (auto& [mk, mv] : v.items()) m[mk] = mv.get<std::string>();
					next_string_cache[make_pair(k.substr(0, sep), k.substr(sep + 1))] = m;
				}
			}

			if (j.contains("dword_map")) {
				for (auto& [k, v] : j["dword_map"].items()) {
					size_t sep = k.find('/');
					if (sep == std::string::npos) continue;
					map<DWORD, string> m;
					for (auto& [mk, mv] : v.items()) m[(DWORD)stoll(mk)] = mv.get<std::string>();
					next_dword_cache[make_pair(k.substr(0, sep), k.substr(sep + 1))] = m;
				}
			}

			if (j.contains("vector3_map")) {
				for (auto& [k, v] : j["vector3_map"].items()) {
					size_t sep = k.find('/');
					if (sep == std::string::npos) continue;
					map<string, Vector3> m;
					for (auto& [mk, mv] : v.items()) {
						string pd = mv.get<std::string>();
						float x = (float)fast::atof(pd.substr(0, pd.find("|")).c_str());
						pd.erase(0, pd.find("|") + 1);
						float y = (float)fast::atof(pd.substr(0, pd.find("|")).c_str());
						pd.erase(0, pd.find("|") + 1);
						float z = (float)fast::atof(pd.c_str());
						m[mk] = Vector3(x, y, z);
					}
					next_vector3_cache[make_pair(k.substr(0, sep), k.substr(sep + 1))] = m;
				}
			}
		}
		catch (...) {
			return;
		}

		{
			std::lock_guard<std::recursive_mutex> lock(config_mutex);
			normal_cache.swap(next_normal_cache);
			dword_cache.swap(next_dword_cache);
			string_cache.swap(next_string_cache);
			vector3_cache.swap(next_vector3_cache);
		}
		run_after_load_hook();
	}

	bool save_to_file(const std::string& name, bool show_notification) {
		const std::string data = serialize_to_json();
		const auto result = local_backend::save_config(name, data);
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		status_message = result.status_message;
		if (result.ok) {
			active_config_name = name;
			dirty = false;
			push_status_notification(status_message, hello_notify::status::success, show_notification);
			return true;
		}
		push_status_notification(status_message, hello_notify::status::error, show_notification);
		return false;
	}

	bool save_to_file(const std::string& name) {
		return save_to_file(name, true);
	}

	bool load_from_file(const std::string& name, bool show_notification) {
		const auto result = local_backend::load_config(name);
		if (!result.ok) {
			std::lock_guard<std::recursive_mutex> lock(config_mutex);
			status_message = result.status_message;
			push_status_notification(status_message, hello_notify::status::error, show_notification);
			return false;
		}
		deserialize_from_json(result.data);
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		status_message = result.status_message;
		active_config_name = name;
		dirty = false;
		push_status_notification(status_message, hello_notify::status::success, show_notification);
		return true;
	}

	bool load_from_file(const std::string& name) {
		return load_from_file(name, true);
	}

	bool list_configs();

	bool delete_config(const std::string& name, bool show_notification) {
		const auto result = local_backend::delete_config(name);
		if (!result.ok) {
			std::lock_guard<std::recursive_mutex> lock(config_mutex);
			status_message = result.status_message;
			push_status_notification(status_message, hello_notify::status::error, show_notification);
			return false;
		}
		{
			std::lock_guard<std::recursive_mutex> lock(config_mutex);
			if (active_config_name == name) {
				active_config_name.clear();
			}
		}
		list_configs();
		{
			std::lock_guard<std::recursive_mutex> lock(config_mutex);
			status_message = result.status_message;
			push_status_notification(status_message, hello_notify::status::success, show_notification);
		}
		return true;
	}

	bool delete_config(const std::string& name) {
		return delete_config(name, true);
	}

	bool list_configs() {
		const auto result = local_backend::list_configs();
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		config_list = result.configs;
		local_configs.clear();
		for (const auto& [name, path] : result.configs) {
			(void)path;
			local_configs.push_back(name);
		}
		status_message = result.status_message;
		return result.ok;
	}
	void load_config(std::string path) {
		try {
			std::filesystem::path p(path);
			std::string name = p.stem().string();
			load_from_file(name);
		} catch (...) {}
	}

	void new_config(std::string config_name) {
		if (config_name.empty()) config_name = "new_config";
		save_to_file(config_name);
		list_configs();
	}

	void flush_to_disk() {
		if (!dirty) return;
		if (active_config_name.empty()) return;
		save_to_file(active_config_name, false);
	}

	void mark_dirty() { dirty = true; }

	snapshot current_snapshot() {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		return snapshot{ config_list, active_config_name, status_message };
	}

	void set_status_message(std::string message) {
		std::lock_guard<std::recursive_mutex> lock(config_mutex);
		status_message = std::move(message);
	}

	void start_save_thread() {
		save_thread_running = false;
	}

	bool load() {
		list_configs();
		start_save_thread();
		return true;
	}
}
