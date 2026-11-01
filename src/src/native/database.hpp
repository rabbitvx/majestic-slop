#pragma once
#include <cstdint>
#include <Windows.h>
#include "platform/debug.hpp"
#include "core/imports.h"
#include "native/maps.hpp"

namespace native {
	namespace type {
		using any = std::uint32_t;
		using hash = std::int32_t;
		using uhash = std::uint32_t;
		using entity = std::int32_t;
		using player = std::int32_t;
		using ped = entity;
		using vehicle = entity;
		using cam = std::int32_t;
		using object = entity;
		using pickup = object;
		using blip = std::int32_t;
		using camera = entity;
		using scrhandle = entity;
	}

	namespace nsdk {
#pragma warning(push)
#pragma warning(disable:4324)
		class scrNativeCallContext
		{
		protected:
			void* m_pReturn;
			uint32_t m_nArgCount;
			void* m_pArgs;

			uint32_t m_nDataCount;

			alignas(uintptr_t) uint8_t m_vectorSpace[192];

		public:
			template<typename T>
			inline T GetArgument(int idx) {
				intptr_t* arguments = (intptr_t*)m_pArgs;

				return *(T*)&arguments[idx];
			}

			template<typename T>
			inline void SetArgument(int idx, T value) {
				intptr_t* arguments = (intptr_t*)m_pArgs;

				*(T*)&arguments[idx] = value;
			}

			template<typename T>
			inline void SetResult(int idx, T value) {
				intptr_t* returnValues = (intptr_t*)m_pReturn;

				*(T*)&returnValues[idx] = value;
			}

			inline int GetArgumentCount() {
				return m_nArgCount;
			}

			template<typename T>
			inline T GetResult(int idx) {
				intptr_t* returnValues = (intptr_t*)m_pReturn;

				return *(T*)&returnValues[idx];
			}

			inline void* GetArgumentBuffer() {
				return m_pArgs;
			}

			inline void SetVectorResults() {
				auto a1 = (uintptr_t)this;

				uint64_t result;

				for (; *(uint64_t*)(a1 + 24); *(uint64_t*)(*(uint64_t*)(a1 + 8i64 * *(signed int*)(a1 + 24) + 32) + 16i64) = result) {
					--* (uint64_t*)(a1 + 24);
					**(uint64_t**)(a1 + 8i64 * *(signed int*)(a1 + 24) + 32) = *(uint64_t*)(a1 + 16 * (*(signed int*)(a1 + 24) + 4i64));
					*(uint64_t*)(*(uint64_t*)(a1 + 8i64 * *(signed int*)(a1 + 24) + 32) + 8i64) = *(uint64_t*)(a1
						+ 16i64
						* *(signed int*)(a1 + 24)
						+ 68);
					result = *(unsigned int*)(a1 + 16i64 * *(signed int*)(a1 + 24) + 72);
				}
				--* (uint64_t*)(a1 + 24);
			}
		};
#pragma warning(pop)
		class NativeContext:
			public scrNativeCallContext
		{
		private:
			enum
			{
				MaxNativeParams = 32,
				ArgSize = 8,
			};

			uint8_t m_TempStack[MaxNativeParams * ArgSize];

		public:
			inline NativeContext() {
				m_pArgs = &m_TempStack;
				m_pReturn = &m_TempStack;
				m_nArgCount = 0;
				m_nDataCount = 0;

				memset(m_TempStack, 0, sizeof(m_TempStack));
			}

			template <typename T>
			void push_arg(T&& value) {
				static_assert(sizeof(T) <= sizeof(std::uint64_t));
				*reinterpret_cast<std::remove_cv_t<std::remove_reference_t<T>>*>(reinterpret_cast<std::uint64_t*>(m_pArgs) + (m_nArgCount++)) = std::forward<T>(value);
			}

			template <typename T>
			inline void Push(T value) {
				if constexpr (sizeof(T) > ArgSize) {
					throw "Argument has an invalid size";
				} else if constexpr (sizeof(T) < ArgSize) {
					*reinterpret_cast<uintptr_t*>(m_TempStack + ArgSize * (uintptr_t)m_nArgCount) = 0;
				}

				*reinterpret_cast<T*>(m_TempStack + ArgSize * (uintptr_t)m_nArgCount) = value;
				m_nArgCount++;
			}

			inline void Reverse() {
				uintptr_t tempValues[MaxNativeParams];
				uintptr_t* args = (uintptr_t*)m_pArgs;

				for (int i = 0; i < (signed int)m_nArgCount; i++) {
					int target = (signed int)m_nArgCount - i - 1;

					tempValues[target] = args[i];
				}

				memcpy(m_TempStack, tempValues, sizeof(m_TempStack));
			}

			template <typename T>
			inline void SetReturnResult(T res) {
				*reinterpret_cast<T*>(m_pReturn) = res;
				*reinterpret_cast<T*>(m_TempStack) = res;
			}

			template <typename T>
			inline T GetResult() {
				return *reinterpret_cast<T*>(m_TempStack);
			}

			inline uint64_t* GetResultAddr() {
				return reinterpret_cast<uint64_t*>(m_TempStack);
			}

			inline uint8_t* GetResultByte() {
				return reinterpret_cast<uint8_t*>(m_TempStack);
			}
		};

		typedef void(__cdecl* native_handler_t)(scrNativeCallContext* context);
	}

	namespace invoker {

		inline nsdk::native_handler_t find_native_handler(uint64_t hash) {
			if (auto eax = native::maps::mapper::get()[hash])

				if (Game.game_mod_base == 1) {
					return (nsdk::native_handler_t)ragemp_fetch_handler(eax);
				} else {
					return (nsdk::native_handler_t)gta_get_native_handler(gta_native_registration_table, eax);
				}

			return 0;
		}

		inline void invoke_internal(nsdk::NativeContext* context, uint64_t hash) {

			auto fn = find_native_handler(hash);
			runtime_debug::last_native_handler = reinterpret_cast<void*>(fn);

			if (fn != 0)
				fn(context);

		}

		template <typename type, typename ...args_t>
		inline type invoke(uintptr_t hash, args_t&& ...args) {
			nsdk::NativeContext cxt;

			(cxt.Push(args), ...);
			runtime_debug::last_native_source = "native::invoker";
			runtime_debug::last_native_hash = hash;
			runtime_debug::last_native_handler = nullptr;
			runtime_debug::last_native_arg_count = static_cast<uint32_t>(sizeof...(args_t));

			__try {
				invoke_internal(&cxt, hash);

				if (gta_fix_context_vector) {
					gta_fix_context_vector(&cxt);
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				HELLO_RUNTIME_LOG(
					"SEH: native::invoker hash=0x%llX handler=%p code=0x%08X",
					static_cast<unsigned long long>(hash),
					runtime_debug::last_native_handler,
					GetExceptionCode()
				);

				if constexpr (!std::is_same_v<type, void>) {
					return type{};
				}
				else {
					return;
				}
			}

			if constexpr (!std::is_same_v<type, void>)
				return cxt.GetResult<type>();
		}

	}

#pragma warning(push)
#pragma warning(disable:4505)
	namespace player {
		static type::ped get_player_ped(type::player player) {
			if (player < 0) {
				return 0;
			}

			return invoker::invoke<type::ped>(0x43a66c31c68491c0, player);
		}
		static type::entity get_player_ped_script_index(type::player player) {
			return invoker::invoke<type::entity>(0x50fac3a3e030a6e1, player);
		}
		static void set_player_model(type::player player, type::hash modelhash) {
			invoker::invoke<void>(0x00a1cadd00108836, player, modelhash);
		}
		static void change_player_ped(type::player player, type::ped ped, bool p, bool resetdamage) {
			invoker::invoke<void>(0x048189fac643deee, player, ped, p, resetdamage);
		}
		static void get_player_rgb_colour(type::player player, int* r, int* g, int* b) {
			invoker::invoke<void>(0xe902ef951dce178f, player, r, g, b);
		}
		static bool get_number_of_players() {
			return invoker::invoke<bool>(0x407c7f91ddb46c16);
		}
		static int get_player_team(type::player player) {
			return invoker::invoke<int>(0x37039302f4e0a008, player);
		}
		static void set_player_team(type::player player, int team) {
			invoker::invoke<void>(0x0299fa38396a4940, player, team);
		}
		static const char* get_player_name(type::player player) {
			return invoker::invoke<const char*>(0x6d0de6a7b5da71f8, player);
		}
		static bool get_wanted_level_radius(bool player) {
			return invoker::invoke<bool>(0x085deb493be80812, player);
		}
		static PVector3 get_player_wanted_centre_position(type::player player) {
			return invoker::invoke<PVector3>(0x0c92ba89f1af26f8, player);
		}
		static void set_player_wanted_centre_position(type::player player, float x, float y, float z) {
			invoker::invoke<void>(0x520e541a97a13354, player, x, y, z);
		}
		static int get_wanted_level_threshold(int wantedlevel) {
			return invoker::invoke<int>(0xfdd179eaf45b556c, wantedlevel);
		}
		static void set_player_wanted_level(type::player player, int wantedlevel, bool disablenomission) {
			invoker::invoke<void>(0x39ff19c64ef7da5b, player, wantedlevel, disablenomission);
		}
		static void set_player_wanted_level_no_drop(type::player player, int wantedlevel, bool p2) {
			invoker::invoke<void>(0x340e61de7f471565, player, wantedlevel, p2);
		}
		static void set_player_wanted_level_now(type::player player, bool p1) {
			invoker::invoke<void>(0xe0a7d1e497ffcd6f, player, p1);
		}
		static bool are_player_flashing_stars_about_to_drop(type::player player) {
			return invoker::invoke<bool>(0xafaf86043e5874e9, player);
		}
		static bool are_player_stars_greyed_out(type::player player) {
			return invoker::invoke<bool>(0x0a6eb355ee14a2db, player);
		}
		static void set_dispatch_cops_for_player(type::player player, bool toggle) {
			invoker::invoke<void>(0xdb172424876553f4, player, toggle);
		}
		static bool is_player_wanted_level_greater(type::player player, int wantedlevel) {
			return invoker::invoke<bool>(0x238db2a2c23ee9ef, player, wantedlevel);
		}
		static void clear_player_wanted_level(type::player player) {
			invoker::invoke<void>(0xb302540597885499, player);
		}
		static bool is_player_dead(type::player player) {
			return invoker::invoke<bool>(0x424d4687fa1e5652, player);
		}
		static bool is_player_pressing_horn(type::player player) {
			return invoker::invoke<bool>(0xfa1e2bf8b10598f9, player);
		}
		static void set_player_control(type::player player, bool toggle, int flags) {
			invoker::invoke<void>(0x8d32347d6d4c40a2, player, toggle, flags);
		}
		static int get_player_wanted_level(type::player player) {
			return invoker::invoke<int>(0xe28e54788ce8f12d, player);
		}
		static void set_max_wanted_level(int maxwantedlevel) {
			invoker::invoke<void>(0xaa5f02db48d704b9, maxwantedlevel);
		}
		static void set_police_radar_blips(type::player test) {
			invoker::invoke<void>(0x43286d561b72b8bf, test);
		}
		static void set_police_ignore_player(type::player player, bool toggle) {
			invoker::invoke<void>(0x32c62aa929c2da6a, player, toggle);
		}
		static bool is_player_playing(type::player player) {
			return invoker::invoke<bool>(0x5e9564d8246b909a, player);
		}
		static void set_everyone_ignore_player(type::player player, bool toggle) {
			invoker::invoke<void>(0x8eeda153ad141ba4, player, toggle);
		}
		static void set_all_random_peds_flee(type::player* player, type::any toggle) {
			invoker::invoke<void>(0x056e0fe8534c2949, player, toggle);
		}
		static void set_all_random_peds_flee_this_frame(type::player player) {
			invoker::invoke<void>(0x471d2ff42a94b4f2, player);
		}
		static void _0xde45d1a1ef45ee61(type::player player, bool toggle) {
			invoker::invoke<void>(0xde45d1a1ef45ee61, player, toggle);
		}
		static void _0xc3376f42b1faccc6(type::player player) {
			invoker::invoke<void>(0xc3376f42b1faccc6, player);
		}
		static void set_ignore_low_priority_shocking_events(type::player player, bool toggle) {
			invoker::invoke<void>(0x596976b02b6b5700, player, toggle);
		}
		static void set_wanted_level_multiplier(float multiplier) {
			invoker::invoke<void>(0x020e5f00cda207ba, multiplier);
		}
		static void set_wanted_level_difficulty(type::player player, float difficulty) {
			invoker::invoke<void>(0x9b0bb33b04405e7a, player, difficulty);
		}
		static void reset_wanted_level_difficulty(type::player player) {
			invoker::invoke<void>(0xb9d0dd990dc141dd, player);
		}
		static void start_firing_amnesty(int duration) {
			invoker::invoke<void>(0xbf9bd71691857e48, duration);
		}
		static void report_crime(type::player player, int crimetype, int wantedlvlthresh) {
			invoker::invoke<void>(0xe9b09589827545e7, player, crimetype, wantedlvlthresh);
		}
		static void _switch_crime_type(type::player player, int p1) {
			invoker::invoke<void>(0x9a987297ed8bd838, player, p1);
		}
		static void _0xbc9490ca15aea8fb(type::player player) {
			invoker::invoke<void>(0xbc9490ca15aea8fb, player);
		}
		static void _0x4669b3ed80f24b4e(type::player player) {
			invoker::invoke<void>(0x4669b3ed80f24b4e, player);
		}
		static void _0xad73ce5a09e42d12(type::player player) {
			invoker::invoke<void>(0xad73ce5a09e42d12, player);
		}
		static void _0x36f1b38855f2a8df(type::player player) {
			invoker::invoke<void>(0x36f1b38855f2a8df, player);
		}
		static void _0xdc64d2c53493ed12(type::player player) {
			invoker::invoke<void>(0xdc64d2c53493ed12, player);
		}
		static void _0xb45eff719d8427a6(float p0) {
			invoker::invoke<void>(0xb45eff719d8427a6, p0);
		}
		static void _0x0032a6dba562c518() {
			invoker::invoke<void>(0x0032a6dba562c518);
		}
		static bool can_player_start_mission(type::player player) {
			return invoker::invoke<bool>(0xde7465a27d403c06, player);
		}
		static bool is_player_ready_for_cutscene(type::player player) {
			return invoker::invoke<bool>(0x908cbecc2caa3690, player);
		}
		static bool is_player_targetting_entity(type::player player, type::entity entity) {
			return invoker::invoke<bool>(0x7912f7fc4f6264b6, player, entity);
		}
		static bool get_player_target_entity(type::player player, type::entity* entity) {
			return invoker::invoke<bool>(0x13ede1a5dbf797c9, player, entity);
		}
		static bool is_player_free_aiming(type::player player) {
			return invoker::invoke<bool>(0x2e397fd2ecd37c87, player);
		}
		static bool is_player_free_aiming_at_entity(type::player player, type::entity entity) {
			return invoker::invoke<bool>(0x3c06b5c839b38f7b, player, entity);
		}
		static bool get_entity_player_is_free_aiming_at(type::player player, type::entity* entity) {
			return invoker::invoke<bool>(0x2975c866e6713290, player, entity);
		}
		static void set_player_lockon_range_override(type::player player, float range) {
			invoker::invoke<void>(0x29961d490e5814fd, player, range);
		}
		static void set_player_can_do_drive_by(type::player player, bool toggle) {
			invoker::invoke<void>(0x6e8834b52ec20c77, player, toggle);
		}
		static void set_player_can_be_hassled_by_gangs(type::player player, bool toggle) {
			invoker::invoke<void>(0xd5e460ad7020a246, player, toggle);
		}
		static void set_player_can_use_cover(type::player player, bool toggle) {
			invoker::invoke<void>(0xd465a8599dff6814, player, toggle);
		}
		static int get_max_wanted_level() {
			return invoker::invoke<int>(0x462e0db9b137dc5f);
		}
		static bool is_player_targetting_anything(type::player player) {
			return invoker::invoke<bool>(0x78cfe51896b6b8a4, player);
		}
		static void set_player_sprint(type::player player, bool toggle) {
			invoker::invoke<void>(0xa01b8075d8b92df4, player, toggle);
		}
		static void reset_player_stamina(type::player player) {
			invoker::invoke<void>(0xa6f312fcce9c1dfe, player);
		}
		static void restore_player_stamina(type::player player, float p1) {
			invoker::invoke<void>(0xa352c1b864cafd33, player, p1);
		}
		static float get_player_sprint_stamina_remaining(type::player player) {
			return invoker::invoke<float>(0x3f9f16f8e65a7ed7, player);
		}
		static float get_player_sprint_time_remaining(type::player player) {
			return invoker::invoke<float>(0x1885bc9b108b4c99, player);
		}
		static float get_player_underwater_time_remaining(type::player player) {
			return invoker::invoke<float>(0xa1fcf8e6af40b731, player);
		}
		static int get_player_group(type::player player) {
			return invoker::invoke<int>(0x0d127585f77030af, player);
		}
		static int get_player_max_armour(type::any vector) {
			return invoker::invoke<int>(0x92659b4ce1863cb3, vector);
		}
		static bool is_player_control_on(type::player player) {
			return invoker::invoke<bool>(0x49c32d60007afa47, player);
		}
		static bool _is_player_cam_control_disabled() {
			return invoker::invoke<bool>(0x7c814d2fb49f40c0);
		}
		static bool is_player_script_control_on(type::player player) {
			return invoker::invoke<bool>(0x8a876a65283dd7d7, player);
		}
		static bool is_player_climbing(type::player player) {
			return invoker::invoke<bool>(0x95e8f73dc65efb9c, player);
		}
		static bool is_player_being_arrested(type::player player, bool atarresting) {
			return invoker::invoke<bool>(0x388a47c51abdac8e, player, atarresting);
		}
		static void reset_player_arrest_state(type::player player) {
			invoker::invoke<void>(0x2d03e13c460760d6, player);
		}
		static type::vehicle get_players_last_vehicle() {
			return invoker::invoke<type::vehicle>(0xb6997a7eb3f5c8c0);
		}
		static type::player get_player_index() {
			return invoker::invoke<type::player>(0xa5edc40ef369b48d);
		}
		static type::player int_to_playerindex(int value) {
			return invoker::invoke<type::player>(0x41bd2a6b006af756, value);
		}
		static int int_to_participantindex(type::player value) {
			return invoker::invoke<int>(0x9ec6603812c24710, value);
		}
		static int get_time_since_player_hit_vehicle(type::player player) {
			return invoker::invoke<int>(0x5d35ecf3a81a0ee0, player);
		}
		static int get_time_since_player_hit_ped(type::player player) {
			return invoker::invoke<int>(0xe36a25322dc35f42, player);
		}
		static int get_time_since_player_drove_on_pavement(type::player player) {
			return invoker::invoke<int>(0xd559d2be9e37853b, player);
		}
		static int get_time_since_player_drove_against_traffic(type::player player) {
			return invoker::invoke<int>(0xdb89591e290d9182, player);
		}
		static type::player is_player_free_for_ambient_task(type::player player) {
			return invoker::invoke<type::player>(0xdccfd3f106c36ab4, player);
		}
		static type::player player_id() {
			return invoker::invoke<type::player>(0x4f8644af03d0e0d6);
		}
		static type::ped player_ped_id() {
			return invoker::invoke<type::ped>(0xd80958fc74e988a6);
		}
		static int network_player_id_to_int() {
			return invoker::invoke<int>(0xee68096f9f37341e);
		}
		static bool has_force_cleanup_occurred(int cleanupflags) {
			return invoker::invoke<bool>(0xc968670bface42d9, cleanupflags);
		}
		static void force_cleanup(int cleanupflags) {
			invoker::invoke<void>(0xbc8983f38f78ed51, cleanupflags);
		}
		static void force_cleanup_for_all_threads_with_this_name(const char* name, int cleanupflags) {
			invoker::invoke<void>(0x4c68ddddf0097317, name, cleanupflags);
		}
		static void force_cleanup_for_thread_with_this_id(int id, int cleanupflags) {
			invoker::invoke<void>(0xf745b37630df176b, id, cleanupflags);
		}
		static type::vehicle* get_cause_of_most_recent_force_cleanup() {
			return invoker::invoke<type::vehicle*>(0x9a41cf4674a12272);
		}
		static void set_player_may_only_enter_this_vehicle(type::player player, type::vehicle vehicle) {
			invoker::invoke<void>(0x8026ff78f208978a, player, vehicle);
		}
		static void set_player_may_not_enter_any_vehicle(type::player* player) {
			invoker::invoke<void>(0x1de37bbf9e9cc14a, player);
		}
		static int give_achievement_to_player(int achievement) {
			return invoker::invoke<int>(0xbec7076d64130195, achievement);
		}
		static bool _set_achievement_progression(int achid, int progression) {
			return invoker::invoke<bool>(0xc2afffdabbdc2c5c, achid, progression);
		}
		static int _get_achievement_progression(int achid) {
			return invoker::invoke<int>(0x1c186837d0619335, achid);
		}
		static bool has_achievement_been_passed(int achievement) {
			return invoker::invoke<bool>(0x867365e111a3b6eb, achievement);
		}
		static bool is_player_online() {
			return invoker::invoke<bool>(0xf25d331dc2627bbc);
		}
		static bool is_player_logging_in_np() {
			return invoker::invoke<bool>(0x74556e1420867eca);
		}
		static void display_system_signin_ui(bool unk) {
			invoker::invoke<void>(0x94dd7888c10a979e, unk);
		}
		static bool is_system_ui_being_displayed() {
			return invoker::invoke<bool>(0x5d511e3867c87139);
		}
		static void set_player_invincible(type::player player, bool toggle) {
			invoker::invoke<void>(0x239528eacdc3e7de, player, toggle);
		}
		static bool get_player_invincible(type::player player) {
			return invoker::invoke<bool>(0xb721981b2b939e07, player);
		}
		static void _0xcac57395b151135f(type::player player, bool p1) {
			invoker::invoke<void>(0xcac57395b151135f, player, p1);
		}
		static void remove_player_helmet(type::player player, bool p2) {
			invoker::invoke<void>(0xf3ac26d3cc576528, player, p2);
		}
		static void give_player_ragdoll_control(type::player player, bool toggle) {
			invoker::invoke<void>(0x3c49c870e66f0a28, player, toggle);
		}
		static void set_player_lockon(type::player player, bool toggle) {
			invoker::invoke<void>(0x5c8b2f450ee4328e, player, toggle);
		}
		static void set_player_targeting_mode(int targetmode) {
			invoker::invoke<void>(0xb1906895227793f3, targetmode);
		}
		static void _0x5702b917b99db1cd(int p0) {
			invoker::invoke<void>(0x5702b917b99db1cd, p0);
		}
		static bool _0xb9cf1f793a9f1bf1() {
			return invoker::invoke<bool>(0xb9cf1f793a9f1bf1);
		}
		static void clear_player_has_damaged_at_least_one_ped(type::player player) {
			invoker::invoke<void>(0xf0b67a4de6ab5f98, player);
		}
		static bool has_player_damaged_at_least_one_ped(type::player player) {
			return invoker::invoke<bool>(0x20ce80b0c2bf4acc, player);
		}
		static void clear_player_has_damaged_at_least_one_non_animal_ped(type::player player) {
			invoker::invoke<void>(0x4aacb96203d11a31, player);
		}
		static bool has_player_damaged_at_least_one_non_animal_ped(type::player player) {
			return invoker::invoke<bool>(0xe4b90f367bd81752, player);
		}
		static void set_air_drag_multiplier_for_players_vehicle(type::ped* player, type::vehicle multiplier) {
			invoker::invoke<void>(0xca7dc8329f0a1e9e, player, multiplier);
		}
		static void set_swim_multiplier_for_player(type::ped player, float multiplier) {
			invoker::invoke<void>(0xa91c6f0ff7d16a13, player, multiplier);
		}
		static void set_run_sprint_multiplier_for_player(type::player player, float multiplier) {
			invoker::invoke<void>(0x6db47aa77fd94e09, player, multiplier);
		}
		static int get_time_since_last_arrest() {
			return invoker::invoke<int>(0x5063f92f07c2a316);
		}
		static int get_time_since_last_death() {
			return invoker::invoke<int>(0xc7034807558ddfca);
		}
		static void assisted_movement_close_route() {
			invoker::invoke<void>(0xaebf081ffc0a0e5e);
		}
		static void assisted_movement_flush_route() {
			invoker::invoke<void>(0x8621390f0cdcfe1f);
		}
		static void set_player_forced_aim(type::player player, bool toggle) {
			invoker::invoke<void>(0x0fee4f80ac44a726, player, toggle);
		}
		static void set_player_forced_zoom(type::player player, bool toggle) {
			invoker::invoke<void>(0x75e7d505f2b15902, player, toggle);
		}
		static void set_player_force_skip_aim_intro(type::player player, bool toggle) {
			invoker::invoke<void>(0x7651bc64ae59e128, player, toggle);
		}
		static void disable_player_firing(type::player player, bool toggle) {
			invoker::invoke<void>(0x5e6cc07646bbeab8, player, toggle);
		}
		static void _0xb885852c39cc265d() {
			invoker::invoke<void>(0xb885852c39cc265d);
		}
		static void set_disable_ambient_melee_move(type::player player, bool toggle) {
			invoker::invoke<void>(0x2e8aabfa40a84f8c, player, toggle);
		}
		static void set_player_max_armour(type::player player, int value) {
			invoker::invoke<void>(0x77dfccf5948b8c71, player, value);
		}
		static void special_ability_deactivate(type::player player) {
			invoker::invoke<void>(0xd6a953c6d1492057, player);
		}
		static void special_ability_deactivate_fast(type::player player) {
			invoker::invoke<void>(0x9cb5ce07a3968d5a, player);
		}
		static void special_ability_reset(type::player player) {
			invoker::invoke<void>(0x375f0e738f861a94, player);
		}
		static void _0xc9a763d8fe87436a(type::player player) {
			invoker::invoke<void>(0xc9a763d8fe87436a, player);
		}
		static void special_ability_charge_small(type::player player, bool p1, bool p2) {
			invoker::invoke<void>(0x2e7b9b683481687d, player, p1, p2);
		}
		static void special_ability_charge_medium(type::player player, type::vehicle p1, type::vehicle p2) {
			invoker::invoke<void>(0xf113e3aa9bc54613, player, p1, p2);
		}
		static void special_ability_charge_large(type::player player, int p1, type::hash p2) {
			invoker::invoke<void>(0xf733f45fa4497d93, player, p1, p2);
		}
		static void special_ability_charge_continuous(type::player player, type::ped p2) {
			invoker::invoke<void>(0xed481732dff7e997, player, p2);
		}
		static void special_ability_charge_absolute(type::player player, int p1, bool p2) {
			invoker::invoke<void>(0xb7b0870eb531d08d, player, p1, p2);
		}
		static void special_ability_charge_normalized(type::player player, float normalizedvalue, bool p2) {
			invoker::invoke<void>(0xa0696a65f009ee18, player, normalizedvalue, p2);
		}
		static void special_ability_fill_meter(type::player player, bool p1) {
			invoker::invoke<void>(0x3daca8ddc6fd4980, player, p1);
		}
		static void special_ability_deplete_meter(type::player player, bool p1) {
			invoker::invoke<void>(0x1d506dbbbc51e64b, player, p1);
		}
		static void special_ability_lock(type::hash playermodel) {
			invoker::invoke<void>(0x6a09d0d590a47d13, playermodel);
		}
		static void special_ability_unlock(type::hash playermodel) {
			invoker::invoke<void>(0xf145f3be2efa9a3b, playermodel);
		}
		static bool is_special_ability_unlocked(type::hash playermodel) {
			return invoker::invoke<bool>(0xc6017f6a6cdfa694, playermodel);
		}
		static bool is_special_ability_active(type::ped player) {
			return invoker::invoke<bool>(0x3e5f7fc85d854e15, player);
		}
		static bool is_special_ability_meter_full(type::player player) {
			return invoker::invoke<bool>(0x05a1fe504b7f2587, player);
		}
		static void enable_special_ability(type::player* player, bool toggle) {
			invoker::invoke<void>(0x181ec197daefe121, player, toggle);
		}
		static bool is_special_ability_enabled(type::player player) {
			return invoker::invoke<bool>(0xb1d200fe26aef3cb, player);
		}
		static void set_special_ability_multiplier(float multiplier) {
			invoker::invoke<void>(0xa49c426ed0ca4ab7, multiplier);
		}
		static void _0xffee8fa29ab9a18e(type::player player) {
			invoker::invoke<void>(0xffee8fa29ab9a18e, player);
		}
		static bool _0x5fc472c501ccadb3(type::player player) {
			return invoker::invoke<bool>(0x5fc472c501ccadb3, player);
		}
		static bool _0xf10b44fd479d69f3(type::player player, int p1) {
			return invoker::invoke<bool>(0xf10b44fd479d69f3, player, p1);
		}
		static bool _is_player_within_test_capsule(type::player player, float p1) {
			return invoker::invoke<bool>(0xdd2620b7b9d16ff1, player, p1);
		}
		static void start_player_teleport(type::player player, float x, float y, float z, float heading, bool keepvehicle, bool keepvelocity, bool fadeinout) {
			invoker::invoke<void>(0xad15f075a4da0fde, player, x, y, z, heading, keepvehicle, keepvelocity, fadeinout);
		}
		static bool _has_player_teleport_finished(type::player player) {
			return invoker::invoke<bool>(0xe23d5873c2394c61, player);
		}
		static void stop_player_teleport() {
			invoker::invoke<void>(0xc449eded9d73009c);
		}
		static bool is_player_teleport_active() {
			return invoker::invoke<bool>(0x02b15662d7f8886f);
		}
		static float get_player_current_stealth_noise(type::player player) {
			return invoker::invoke<float>(0x2f395d61f3a1f877, player);
		}
		static void set_player_health_recharge_multiplier(type::player player, float regenrate) {
			invoker::invoke<void>(0x5db660b38dd98a31, player, regenrate);
		}
		static void set_player_weapon_damage_modifier(type::player player, type::pickup* damageamount) {
			invoker::invoke<void>(0xce07b9f7817aada3, player, damageamount);
		}
		static void set_player_weapon_defense_modifier(type::player player, float modifier) {
			invoker::invoke<void>(0x2d83bc011ca14a3c, player, modifier);
		}
		static void set_player_melee_weapon_damage_modifier(type::player player, float modifier) {
			invoker::invoke<void>(0x4a3dc7eccc321032, player, modifier);
		}
		static void set_player_melee_weapon_defense_modifier(type::player player, float modifier) {
			invoker::invoke<void>(0xae540335b4abc4e2, player, modifier);
		}
		static void set_player_vehicle_damage_modifier(type::player player, float damageamount) {
			invoker::invoke<void>(0xa50e117cddf82f0c, player, damageamount);
		}
		static void set_player_vehicle_defense_modifier(type::player player, float modifier) {
			invoker::invoke<void>(0x4c60e6efdaff2462, player, modifier);
		}
		static void set_player_parachute_tint_index(type::ped ped, type::vehicle* vehicleindex) {
			invoker::invoke<void>(0xa3d0e54541d9a5e5, ped, vehicleindex);
		}
		static void get_player_parachute_tint_index(type::player player, int* tintindex) {
			invoker::invoke<void>(0x75d3f7a1b0d9b145, player, tintindex);
		}
		static void set_player_reserve_parachute_tint_index(type::player player, int index) {
			invoker::invoke<void>(0xaf04c87f5dc1df38, player, index);
		}
		static void get_player_reserve_parachute_tint_index(type::player player, int* index) {
			invoker::invoke<void>(0xd5a016bc3c09cf40, player, index);
		}
		static void set_player_parachute_pack_tint_index(type::player player, int tintindex) {
			invoker::invoke<void>(0x93b0fb27c9a04060, player, tintindex);
		}
		static void get_player_parachute_pack_tint_index(type::player player, int* tintindex) {
			invoker::invoke<void>(0x6e9c742f340ce5a2, player, tintindex);
		}
		static void set_player_has_reserve_parachute(type::vehicle player) {
			invoker::invoke<void>(0x7ddab28d31fac363, player);
		}
		static bool get_player_has_reserve_parachute(type::player player) {
			return invoker::invoke<bool>(0x5ddfe2ff727f3ca3, player);
		}
		static void set_player_can_leave_parachute_smoke_trail(type::player player, bool enabled) {
			invoker::invoke<void>(0xf401b182dba8af53, player, enabled);
		}
		static void set_player_parachute_smoke_trail_color(type::player player, int r, int g, int b) {
			invoker::invoke<void>(0x8217fd371a4625cf, player, r, g, b);
		}
		static void get_player_parachute_smoke_trail_color(type::player player, int* r, int* g, int* b) {
			invoker::invoke<void>(0xef56dbabd3cd4887, player, r, g, b);
		}
		static void set_player_reset_flag_prefer_rear_seats(type::player player, int flags) {
			invoker::invoke<void>(0x11d5f725f0e780e0, player, flags);
		}
		static void set_player_noise_multiplier(type::player player, float multiplier) {
			invoker::invoke<void>(0xdb89ef50ff25fce9, player, multiplier);
		}
		static void set_player_sneaking_noise_multiplier(type::player player, float multiplier) {
			invoker::invoke<void>(0xb2c1a29588a9f47c, player, multiplier);
		}
		static bool can_ped_hear_player(type::player player, type::ped ped) {
			return invoker::invoke<bool>(0xf297383aa91dca29, player, ped);
		}
		static void simulate_player_input_gait(type::player control, float amount, int gaittype, float speed, bool p4, bool p5) {
			invoker::invoke<void>(0x477d5d63e63eca5d, control, amount, gaittype, speed, p4, p5);
		}
		static void reset_player_input_gait(type::player player) {
			invoker::invoke<void>(0x19531c47a2abd691, player);
		}
		static void set_auto_give_parachute_when_enter_plane(type::player player, bool toggle) {
			invoker::invoke<void>(0x9f343285a00b4bb6, player, toggle);
		}
		static void _0xd2b315b6689d537d(type::player player, bool p1) {
			invoker::invoke<void>(0xd2b315b6689d537d, player, p1);
		}
		static void set_player_stealth_perception_modifier(type::player player, float value) {
			invoker::invoke<void>(0x4e9021c1fcdd507a, player, value);
		}
		static bool _0x690a61a6d13583f6(type::ped p0) {
			return invoker::invoke<bool>(0x690a61a6d13583f6, p0);
		}
		static void _0x9edd76e87d5d51ba(type::player player) {
			invoker::invoke<void>(0x9edd76e87d5d51ba, player);
		}
		static void set_player_simulate_aiming(type::player player, bool toggle) {
			invoker::invoke<void>(0xc54c95da968ec5b5, player, toggle);
		}
		static void set_player_cloth_pin_frames(type::player player, bool toggle) {
			invoker::invoke<void>(0x749faddf97dfe930, player, toggle);
		}
		static void set_player_cloth_package_index(int index) {
			invoker::invoke<void>(0x9f7bba2ea6372500, index);
		}
		static void set_player_cloth_lock_counter(int value) {
			invoker::invoke<void>(0x14d913b777dff5da, value);
		}
		static void player_attach_virtual_bound(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7) {
			invoker::invoke<void>(0xed51733dc73aed51, p0, p1, p2, p3, p4, p5, p6, p7);
		}
		static void player_detach_virtual_bound() {
			invoker::invoke<void>(0x1dd5897e2fa6e7c9);
		}
		static bool has_player_been_spotted_in_stolen_vehicle(type::player player) {
			return invoker::invoke<bool>(0xd705740bb0a1cf4c, player);
		}
		static bool _0x38d28da81e4e9bf9(type::player player) {
			return invoker::invoke<bool>(0x38d28da81e4e9bf9, player);
		}
		static bool _0xbc0753c9ca14b506(type::player player, int p1, bool p2) {
			return invoker::invoke<bool>(0xbc0753c9ca14b506, player, p1, p2);
		}
		static void _expand_world_limits(float x, float y, float z) {
			invoker::invoke<void>(0x5006d96c995a5827, x, y, z);
		}
		static bool is_player_riding_train(type::player player) {
			return invoker::invoke<bool>(0x4ec12697209f2196, player);
		}
		static bool has_player_left_the_world(type::player player) {
			return invoker::invoke<bool>(0xd55ddfb47991a294, player);
		}
		static void _0xff300c7649724a0b(type::player player, bool p1) {
			invoker::invoke<void>(0xff300c7649724a0b, player, p1);
		}
		static void set_player_parachute_variation_override(type::player player, int p1, type::any p2, type::any p3, bool p4) {
			invoker::invoke<void>(0xd9284a8c0d48352c, player, p1, p2, p3, p4);
		}
		static void clear_player_parachute_variation_override(type::player player) {
			invoker::invoke<void>(0x0f4cc924cf8c7b21, player);
		}
		static void set_player_parachute_model_override(type::player player, type::hash model) {
			invoker::invoke<void>(0x977db4641f6fc3db, player, model);
		}
		static void clear_player_parachute_model_override(type::player player) {
			invoker::invoke<void>(0x8753997eb5f6ee3f, player);
		}
		static void set_player_parachute_pack_model_override(type::player player, type::hash model) {
			invoker::invoke<void>(0xdc80a4c2f18a2b64, player, model);
		}
		static void clear_player_parachute_pack_model_override(type::player player) {
			invoker::invoke<void>(0x10c54e4389c12b42, player);
		}
		static void disable_player_vehicle_rewards(type::player player) {
			invoker::invoke<void>(0xc142be3bb9ce125f, player);
		}
		static void _0x2f7ceb6520288061(bool p0) {
			invoker::invoke<void>(0x2f7ceb6520288061, p0);
		}
		static void _0x5dc40a8869c22141(bool p0, type::scrhandle p1) {
			invoker::invoke<void>(0x5dc40a8869c22141, p0, p1);
		}
		static bool _0x65faee425de637b0(type::player player) {
			return invoker::invoke<bool>(0x65faee425de637b0, player);
		}
		static void _0x5501b7a5cdb79d37(type::player player) {
			invoker::invoke<void>(0x5501b7a5cdb79d37, player);
		}
		static type::any* _0x56105e599cab0efa(type::hash p7) {
			return invoker::invoke<type::any*>(0x56105e599cab0efa, p7);
		}
	}

	namespace entity {
		static bool does_entity_exist(type::entity entity) {
			return invoker::invoke<bool>(0x7239b21a38f536ba, entity);
		}
		static bool does_entity_belong_to_this_script(type::entity entity, bool p2) {
			return invoker::invoke<bool>(0xdde6df5ae89981d2, entity, p2);
		}
		static bool does_entity_have_drawable(type::entity entity) {
			return invoker::invoke<bool>(0x060d6e96f8b8e48d, entity);
		}
		static bool does_entity_have_physics(type::entity entity) {
			return invoker::invoke<bool>(0xda95ea3317cc5064, entity);
		}
		static bool has_entity_anim_finished(type::entity entity, const char* animdict, const char* animname, int p3) {
			return invoker::invoke<bool>(0x20b711662962b472, entity, animdict, animname, p3);
		}
		static bool has_entity_been_damaged_by_any_object(type::entity entity) {
			return invoker::invoke<bool>(0x95eb9964ff5c5c65, entity);
		}
		static bool has_entity_been_damaged_by_any_ped(type::entity entity) {
			return invoker::invoke<bool>(0x605f5a140f202491, entity);
		}
		static bool has_entity_been_damaged_by_any_vehicle(type::entity entity) {
			return invoker::invoke<bool>(0xdfd5033fdba0a9c8, entity);
		}
		static bool has_entity_been_damaged_by_entity(type::entity entity1, type::entity entity2, bool p2) {
			return invoker::invoke<bool>(0xc86d67d52a707cf8, entity1, entity2, p2);
		}
		static bool has_entity_clear_los_to_entity(type::entity entity1, type::entity entity2, int tracetype) {
			return invoker::invoke<bool>(0xfcdff7b72d23a1ac, entity1, entity2, tracetype);
		}
		static bool has_entity_clear_los_to_entity_in_front(type::entity entity1, type::entity entity2) {
			return invoker::invoke<bool>(0x0267d00af114f17a, entity1, entity2);
		}
		static bool has_entity_collided_with_anything(type::entity entity) {
			return invoker::invoke<bool>(0x8bad02f0368d9e14, entity);
		}
		static type::hash get_last_material_hit_by_entity(type::entity entity) {
			return invoker::invoke<type::hash>(0x5c3d0a935f535c4c, entity);
		}
		static PVector3 get_collision_normal_of_last_hit_for_entity(type::entity entity) {
			return invoker::invoke<PVector3>(0xe465d4ab7ca6ae72, entity);
		}
		static void force_entity_ai_and_animation_update(type::entity entity) {
			invoker::invoke<void>(0x40fdedb72f8293b2, entity);
		}
		static float get_entity_anim_current_time(type::entity entity, const char* animdict, const char* animname) {
			return invoker::invoke<float>(0x346d81500d088f42, entity, animdict, animname);
		}
		static float get_entity_anim_total_time(type::entity entity, const char* animdict, const char* animname) {
			return invoker::invoke<float>(0x50bd2730b191e360, entity, animdict, animname);
		}
		static float _get_anim_duration(const char* animdict, const char* animname) {
			return invoker::invoke<float>(0xfeddf04d62b8d790, animdict, animname);
		}
		static type::entity get_entity_attached_to(type::entity entity) {
			return invoker::invoke<type::entity>(0x48c2bed9180fe123, entity);
		}
		static PVector3 get_entity_coords(type::entity entity, bool alive) {
			return invoker::invoke<PVector3>(0x3fef770d40960d5a, entity, alive);
		}
		static PVector3 get_entity_forward_vector(type::entity entity) {
			return invoker::invoke<PVector3>(0x0a794a5a57f8df91, entity);
		}
		static float get_entity_forward_x(type::entity entity) {
			return invoker::invoke<float>(0x8bb4ef4214e0e6d5, entity);
		}
		static float get_entity_forward_y(type::entity entity) {
			return invoker::invoke<float>(0x866a4a5fae349510, entity);
		}
		static float get_entity_heading(type::entity entity) {
			return invoker::invoke<float>(0xe83d4f9ba2a38914, entity);
		}
		static float _get_entity_physics_heading(type::entity entity) {
			return invoker::invoke<float>(0x846bf6291198a71e, entity);
		}
		static int get_entity_health(type::entity entity) {
			return invoker::invoke<int>(0xeef059fad016d209, entity);
		}
		static int get_entity_max_health(type::entity entity) {
			return invoker::invoke<int>(0x15d757606d170c3c, entity);
		}
		static void set_entity_max_health(type::entity entity, int value) {
			invoker::invoke<void>(0x166e7cf68597d8b5, entity, value);
		}
		static float get_entity_height(type::entity entity, float x, float y, float z, bool attop, bool inworldcoords) {
			return invoker::invoke<float>(0x5a504562485944dd, entity, x, y, z, attop, inworldcoords);
		}
		static float get_entity_height_above_ground(type::entity entity) {
			return invoker::invoke<float>(0x1dd55701034110e5, entity);
		}
		static void get_entity_matrix(type::entity entity, PVector3* rightvector, PVector3* forwardvector, PVector3* upvector, PVector3* position) {
			invoker::invoke<void>(0xecb2fc7235a7d137, entity, rightvector, forwardvector, upvector, position);
		}
		static type::hash get_entity_model(type::entity entity) {
			return invoker::invoke<type::hash>(0x9f47b058362c84b5, entity);
		}
		static PVector3 get_offset_from_entity_given_world_coords(type::entity entity, float posx, float posy, float posz) {
			return invoker::invoke<PVector3>(0x2274bc1c4885e333, entity, posx, posy, posz);
		}
		static PVector3 get_offset_from_entity_in_world_coords(type::entity entity, float offsetx, float offsety, float offsetz) {
			return invoker::invoke<PVector3>(0x1899f328b0e12848, entity, offsetx, offsety, offsetz);
		}
		static float get_entity_pitch(type::entity entity) {
			return invoker::invoke<float>(0xd45dc2893621e1fe, entity);
		}
		static void get_entity_quaternion(type::entity entity, float* x, float* y, float* z, float* w) {
			invoker::invoke<void>(0x7b3703d2d32dfa18, entity, x, y, z, w);
		}
		static float get_entity_roll(type::entity entity) {
			return invoker::invoke<float>(0x831e0242595560df, entity);
		}
		static PVector3 get_entity_rotation(type::entity entity, int rotationorder) {
			return invoker::invoke<PVector3>(0xafbd61cc738d9eb9, entity, rotationorder);
		}
		static PVector3 get_entity_rotation_velocity(type::entity entity) {
			return invoker::invoke<PVector3>(0x213b91045d09b983, entity);
		}
		static const char* get_entity_script(type::entity entity, type::scrhandle* script) {
			return invoker::invoke<const char*>(0xa6e9c38db51d7748, entity, script);
		}
		static float get_entity_speed(type::entity entity) {
			return invoker::invoke<float>(0xd5037ba82e12416f, entity);
		}
		static PVector3 get_entity_speed_vector(type::entity entity, bool relative) {
			return invoker::invoke<PVector3>(0x9a8d700a51cb7b0d, entity, relative);
		}
		static float get_entity_upright_value(type::entity entity) {
			return invoker::invoke<float>(0x95eed5a694951f9f, entity);
		}
		static PVector3 get_entity_velocity(type::entity entity) {
			return invoker::invoke<PVector3>(0x4805d2b1d8cf94a9, entity);
		}
		static type::object get_object_index_from_entity_index(type::entity entity) {
			return invoker::invoke<type::object>(0xd7e3b9735c0f89d6, entity);
		}
		static type::ped get_ped_index_from_entity_index(type::entity entity) {
			return invoker::invoke<type::ped>(0x04a2a40c73395041, entity);
		}
		static type::vehicle get_vehicle_index_from_entity_index(type::entity entity) {
			return invoker::invoke<type::vehicle>(0x4b53f92932adfac0, entity);
		}
		static PVector3 get_world_position_of_entity_bone(type::entity entity, int boneindex) {
			return invoker::invoke<PVector3>(0x44a8fcb8ed227738, entity, boneindex);
		}
		static type::player get_nearest_player_to_entity(type::entity entity) {
			return invoker::invoke<type::player>(0x7196842cb375cdb3, entity);
		}
		static type::player get_nearest_player_to_entity_on_team(type::entity entity, int team) {
			return invoker::invoke<type::player>(0x4dc9a62f844d9337, entity, team);
		}
		static int get_entity_type(type::entity entity) {
			return invoker::invoke<int>(0x8acd366038d14505, entity);
		}
		static int _get_entity_population_type(type::entity entity) {
			return invoker::invoke<int>(0xf6f5161f4534edff, entity);
		}
		static bool is_an_entity(int handle) {
			return invoker::invoke<bool>(0x731ec8a916bd11a1, handle);
		}
		static bool is_entity_a_ped(type::entity entity) {
			return invoker::invoke<bool>(0x524ac5ecea15343e, entity);
		}
		static bool is_entity_a_mission_entity(type::entity entity) {
			return invoker::invoke<bool>(0x0a7b270912999b3c, entity);
		}
		static bool is_entity_a_vehicle(type::entity entity) {
			return invoker::invoke<bool>(0x6ac7003fa6e5575e, entity);
		}
		static bool is_entity_an_object(type::entity entity) {
			return invoker::invoke<bool>(0x8d68c8fd0faca94e, entity);
		}
		static bool is_entity_at_coord(type::entity entity, float xpos, float ypos, float zpos, float xsize, float ysize, float zsize, bool p7, bool p8, int p9) {
			return invoker::invoke<bool>(0x20b60995556d004f, entity, xpos, ypos, zpos, xsize, ysize, zsize, p7, p8, p9);
		}
		static bool is_entity_at_entity(type::entity entity1, type::entity entity2, float xsize, float ysize, float zsize, bool p5, bool p6, int p7) {
			return invoker::invoke<bool>(0x751b70c3d034e187, entity1, entity2, xsize, ysize, zsize, p5, p6, p7);
		}
		static bool is_entity_attached(type::entity entity) {
			return invoker::invoke<bool>(0xb346476ef1a64897, entity);
		}
		static bool is_entity_attached_to_any_object(type::entity entity) {
			return invoker::invoke<bool>(0xcf511840ceede0cc, entity);
		}
		static bool is_entity_attached_to_any_ped(type::entity entity) {
			return invoker::invoke<bool>(0xb1632e9a5f988d11, entity);
		}
		static bool is_entity_attached_to_any_vehicle(type::entity entity) {
			return invoker::invoke<bool>(0x26aa915ad89bfb4b, entity);
		}
		static bool is_entity_attached_to_entity(type::entity from, type::entity to) {
			return invoker::invoke<bool>(0xefbe71898a993728, from, to);
		}
		static bool is_entity_dead(type::entity entity) {
			return invoker::invoke<bool>(0x5f9532f3b5cc2551, entity);
		}
		static bool is_entity_in_air(type::entity entity) {
			return invoker::invoke<bool>(0x886e37ec497200b6, entity);
		}
		static bool is_entity_in_angled_area(type::entity entity, float originx, float originy, float originz, float edgex, float edgey, float edgez, float angle, bool p8, bool p9, type::any p10) {
			return invoker::invoke<bool>(0x51210ced3da1c78a, entity, originx, originy, originz, edgex, edgey, edgez, angle, p8, p9, p10);
		}
		static bool is_entity_in_area(type::entity entity, float x1, float y1, float z1, float x2, float y2, float z2, bool p7, bool p8, type::any p9) {
			return invoker::invoke<bool>(0x54736aa40e271165, entity, x1, y1, z1, x2, y2, z2, p7, p8, p9);
		}
		static bool is_entity_in_zone(type::entity entity, const char* zone) {
			return invoker::invoke<bool>(0xb6463cf6af527071, entity, zone);
		}
		static bool is_entity_in_water(type::entity entity) {
			return invoker::invoke<bool>(0xcfb0a0d8edd145a3, entity);
		}
		static float get_entity_submerged_level(type::entity entity) {
			return invoker::invoke<float>(0xe81afc1bc4cc41ce, entity);
		}
		static void _set_used_by_player(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x694e00132f2823ed, entity, toggle);
		}
		static bool is_entity_on_screen(type::entity entity) {
			return invoker::invoke<bool>(0xe659e47af827484b, entity);
		}
		static bool is_entity_playing_anim(type::entity entity, const char* animdict, const char* animname, int taskflag) {
			return invoker::invoke<bool>(0x1f0b79228e461ec9, entity, animdict, animname, taskflag);
		}
		static bool is_entity_static(type::entity entity) {
			return invoker::invoke<bool>(0x1218e6886d3d8327, entity);
		}
		static bool is_entity_touching_entity(type::entity entity, type::entity targetentity) {
			return invoker::invoke<bool>(0x17ffc1b2ba35a494, entity, targetentity);
		}
		static bool is_entity_touching_model(type::entity entity, type::hash modelhash) {
			return invoker::invoke<bool>(0x0f42323798a58c8c, entity, modelhash);
		}
		static bool is_entity_upright(type::entity entity, float angle) {
			return invoker::invoke<bool>(0x5333f526f6ab19aa, entity, angle);
		}
		static bool is_entity_upsidedown(type::entity entity) {
			return invoker::invoke<bool>(0x1dbd58820fa61d71, entity);
		}
		static bool is_entity_visible(type::entity entity) {
			return invoker::invoke<bool>(0x47d6f43d77935c75, entity);
		}
		static bool is_entity_visible_to_script(type::entity entity) {
			return invoker::invoke<bool>(0xd796cb5ba8f20e32, entity);
		}
		static bool is_entity_occluded(type::entity entity) {
			return invoker::invoke<bool>(0xe31c2c72b8692b64, entity);
		}
		static bool would_entity_be_occluded(type::hash entitymodelhash, float x, float y, float z, bool p4) {
			return invoker::invoke<bool>(0xee5d2a122e09ec42, entitymodelhash, x, y, z, p4);
		}
		static bool is_entity_waiting_for_world_collision(type::entity entity) {
			return invoker::invoke<bool>(0xd05bff0c0a12c68f, entity);
		}
		static void apply_force_to_entity_center_of_mass(type::entity entity, int forcetype, float x, float y, float z, bool p5, bool isdirectionrel, bool isforcerel, bool p8) {
			invoker::invoke<void>(0x18ff00fc7eff559e, entity, forcetype, x, y, z, p5, isdirectionrel, isforcerel, p8);
		}
		static void apply_force_to_entity(type::entity entity, int forceflags, float x, float y, float z, float offx, float offy, float offz, int boneindex, bool isdirectionrel, bool ignoreupvec, bool isforcerel, bool p12, bool p13) {
			invoker::invoke<void>(0xc5f68be9613e2d18, entity, forceflags, x, y, z, offx, offy, offz, boneindex, isdirectionrel, ignoreupvec, isforcerel, p12, p13);
		}
		static void attach_entity_to_entity(type::entity entity1, type::entity entity2, int boneindex, float xpos, float ypos, float zpos, float xrot, float yrot, float zrot, bool p9, bool usesoftpinning, bool collision, bool isped, int vertexindex, bool fixedrot) {
			invoker::invoke<void>(0x6b9bbd38ab0796df, entity1, entity2, boneindex, xpos, ypos, zpos, xrot, yrot, zrot, p9, usesoftpinning, collision, isped, vertexindex, fixedrot);
		}
		static void attach_entity_to_entity_physically(type::entity entity1, type::entity entity2, int boneindex1, int boneindex2, float xpos1, float ypos1, float zpos1, float xpos2, float ypos2, float zpos2, float xrot, float yrot, float zrot, float breakforce, bool fixedrot, bool p15, bool collision, bool teleport, int p18) {
			invoker::invoke<void>(0xc3675780c92f90f9, entity1, entity2, boneindex1, boneindex2, xpos1, ypos1, zpos1, xpos2, ypos2, zpos2, xrot, yrot, zrot, breakforce, fixedrot, p15, collision, teleport, p18);
		}
		static void process_entity_attachments(type::entity entity) {
			invoker::invoke<void>(0xf4080490adc51c6f, entity);
		}
		static int get_entity_bone_index_by_name(type::entity entity, const char* bonename) {
			return invoker::invoke<int>(0xfb71170b7e76acba, entity, bonename);
		}
		static void clear_entity_last_damage_entity(type::entity entity) {
			invoker::invoke<void>(0xa72cd9ca74a5ecba, entity);
		}
		static void delete_entity(type::entity* entity) {
			invoker::invoke<void>(0xae3cbe5bf394c9c9, entity);
		}
		static void detach_entity(type::entity entity, bool p1, bool collision) {
			invoker::invoke<void>(0x961ac54bf0613f5d, entity, p1, collision);
		}
		static void freeze_entity_position(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x428ca6dbd1094446, entity, toggle);
		}
		static void _set_entity_something(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x3910051ccecdb00c, entity, toggle);
		}
		static bool play_entity_anim(type::entity entity, const char* animname, const char* animdict, float p3, bool loop, bool stayinanim, bool p6, float delta, type::any bitset) {
			return invoker::invoke<bool>(0x7fb218262b810701, entity, animname, animdict, p3, loop, stayinanim, p6, delta, bitset);
		}
		static bool play_synchronized_entity_anim(type::entity entity, int sceneid, const char* animname, const char* animdict, float speed, float speedmult, int flag, float flag2) {
			return invoker::invoke<bool>(0xc77720a12fe14a86, entity, sceneid, animname, animdict, speed, speedmult, flag, flag2);
		}
		static bool play_synchronized_map_entity_anim(float posx, float posy, float posz, float radius, type::object prop, int sceneid, const char* animname, const char* animdict, float playbackrate, float unkflag, bool unkbool, float unkflag2) {
			return invoker::invoke<bool>(0xb9c54555ed30fbc4, posx, posy, posz, radius, prop, sceneid, animname, animdict, playbackrate, unkflag, unkbool, unkflag2);
		}
		static bool stop_synchronized_map_entity_anim(float posx, float posy, float posz, float radius, type::object object, float playbackrate) {
			return invoker::invoke<bool>(0x11e79cab7183b6f5, posx, posy, posz, radius, object, playbackrate);
		}
		static type::any stop_entity_anim(type::entity entity, const char* animation, const char* animgroup, float p3) {
			return invoker::invoke<type::any>(0x28004f88151e03e0, entity, animation, animgroup, p3);
		}
		static bool stop_synchronized_entity_anim(type::entity entity, float p1, bool p2) {
			return invoker::invoke<bool>(0x43d3807c077261e3, entity, p1, p2);
		}
		static bool has_anim_event_fired(type::entity entity, type::hash actionhash) {
			return invoker::invoke<bool>(0xeaf4cd9ea3e7e922, entity, actionhash);
		}
		static bool find_anim_event_phase(const char* animdictionary, const char* animname, const char* p2, type::any* p3, type::any* p4) {
			return invoker::invoke<bool>(0x07f1be2bccaa27a7, animdictionary, animname, p2, p3, p4);
		}
		static void set_entity_anim_current_time(type::entity entity, const char* animdictionary, const char* animname, float time) {
			invoker::invoke<void>(0x4487c259f0f70977, entity, animdictionary, animname, time);
		}
		static void set_entity_anim_speed(type::entity entity, const char* animdictionary, const char* animname, float speedmultiplier) {
			invoker::invoke<void>(0x28d1a16553c51776, entity, animdictionary, animname, speedmultiplier);
		}
		static void set_entity_as_mission_entity(type::entity entity, bool p1, bool p2) {
			invoker::invoke<void>(0xad738c3085fe7e11, entity, p1, p2);
		}
		static void set_entity_as_no_longer_needed(type::entity* entity) {
			invoker::invoke<void>(0xb736a491e64a32cf, entity);
		}
		static void set_ped_as_no_longer_needed(type::ped* ped) {
			invoker::invoke<void>(0x2595dd4236549ce3, ped);
		}
		static void set_vehicle_as_no_longer_needed(type::vehicle* vehicle) {
			invoker::invoke<void>(0x629bfa74418d6239, vehicle);
		}
		static void set_object_as_no_longer_needed(type::object* object) {
			invoker::invoke<void>(0x3ae22deb5ba5a3e6, object);
		}
		static void set_entity_can_be_damaged(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x1760ffa8ab074d66, entity, toggle);
		}
		static void set_entity_can_be_damaged_by_relationship_group(type::entity entity, bool bcanbedamaged, int relgroup) {
			invoker::invoke<void>(0xe22d8fde858b8119, entity, bcanbedamaged, relgroup);
		}
		static void set_entity_can_be_targeted_without_los(type::entity entity, bool toggle) {
			invoker::invoke<void>(0xd3997889736fd899, entity, toggle);
		}
		static void set_entity_collision(type::entity entity, bool toggle, bool keepphysics) {
			invoker::invoke<void>(0x1a9205c1b9ee827f, entity, toggle, keepphysics);
		}
		static bool _get_entity_collison_disabled(type::entity entity) {
			return invoker::invoke<bool>(0xccf1e97befdae480, entity);
		}
		static void _set_entity_collision_2(type::entity entity, bool p1, bool p2) {
			invoker::invoke<void>(0x9ebc85ed0fffe51c, entity, p1, p2);
		}
		static void set_entity_coords(type::entity entity, PVector3 coords, bool xaxis, bool yaxis, bool zaxis, bool cleararea) {
			invoker::invoke<void>(0x06843da7060a026b, entity, coords.x, coords.y, coords.z, xaxis, yaxis, zaxis, cleararea);
		}
		static void _set_entity_coords_2(type::entity entity, float xpos, float ypos, float zpos, bool xaxis, bool yaxis, bool zaxis, bool cleararea) {
			invoker::invoke<void>(0x621873ece1178967, entity, xpos, ypos, zpos, xaxis, yaxis, zaxis, cleararea);
		}
		static void set_entity_coords_no_offset(type::entity entity, float xpos, float ypos, float zpos, bool xaxis, bool yaxis, bool zaxis) {
			invoker::invoke<void>(0x239a3351ac1da385, entity, xpos, ypos, zpos, xaxis, yaxis, zaxis);
		}
		static void set_entity_dynamic(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x1718de8e3f2823ca, entity, toggle);
		}
		static void set_entity_heading(type::entity entity, float heading) {
			invoker::invoke<void>(0x8e2530aa8ada980e, entity, heading);
		}
		static void set_entity_health(type::entity entity, int health) {
			invoker::invoke<void>(0x6b76dc1f3ae6e6a3, entity, health);
		}
		static void set_entity_invincible(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x3882114bde571ad4, entity, toggle);
		}
		static void set_entity_is_target_priority(type::entity entity, bool p1, float p2) {
			invoker::invoke<void>(0xea02e132f5c68722, entity, p1, p2);
		}
		static void set_entity_lights(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x7cfba6a80bdf3874, entity, toggle);
		}
		static void set_entity_load_collision_flag(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x0dc7cabab1e9b67e, entity, toggle);
		}
		static bool has_collision_loaded_around_entity(type::entity entity) {
			return invoker::invoke<bool>(0xe9676f61bc0b3321, entity);
		}
		static void set_entity_max_speed(type::entity entity, float speed) {
			invoker::invoke<void>(0x0e46a3fcbde2a1b1, entity, speed);
		}
		static void set_entity_only_damaged_by_player(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x79f020ff9edc0748, entity, toggle);
		}
		static void set_entity_only_damaged_by_relationship_group(type::entity entity, bool p1, type::hash relationshiphash) {
			invoker::invoke<void>(0x7022bd828fa0b082, entity, p1, relationshiphash);
		}
		static void set_entity_proofs(type::entity entity, bool bulletproof, bool fireproof, bool explosionproof, bool collisionproof, bool meleeproof, bool steamproof, bool smokeproof, bool drownproof) {
			invoker::invoke<void>(0xfaee099c6f890bb8, entity, bulletproof, fireproof, explosionproof, collisionproof, meleeproof, steamproof, smokeproof, drownproof);
		}
		static void set_entity_quaternion(type::entity entity, float x, float y, float z, float w) {
			invoker::invoke<void>(0x77b21be7ac540f07, entity, x, y, z, w);
		}
		static void set_entity_records_collisions(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x0a50a1eedad01e65, entity, toggle);
		}
		static void set_entity_rotation(type::entity entity, float pitch, float roll, float yaw, int rotationorder, bool p5) {
			invoker::invoke<void>(0x8524a8b0171d5e07, entity, pitch, roll, yaw, rotationorder, p5);
		}
		static void set_entity_visible(type::entity entity, bool toggle, bool unk) {
			invoker::invoke<void>(0xea1c610a04db6bbb, entity, toggle, unk);
		}
		static void set_entity_velocity(type::entity entity, float x, float y, float z) {
			invoker::invoke<void>(0x1c99bb7b6e96d16f, entity, x, y, z);
		}
		static void set_entity_has_gravity(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x4a4722448f18eef5, entity, toggle);
		}
		static void set_entity_lod_dist(type::entity entity, int value) {
			invoker::invoke<void>(0x5927f96a78577363, entity, value);
		}
		static int get_entity_lod_dist(type::entity entity) {
			return invoker::invoke<int>(0x4159c2762b5791d6, entity);
		}
		static void set_entity_alpha(type::entity entity, int alphalevel, bool unk) {
			invoker::invoke<void>(0x44a0870b7e92d7c0, entity, alphalevel, unk);
		}
		static int get_entity_alpha(type::entity entity) {
			return invoker::invoke<int>(0x5a47b3b5e63e94c6, entity);
		}
		static void reset_entity_alpha(type::entity entity) {
			invoker::invoke<void>(0x9b1e824ffbb7027a, entity);
		}
		static void _0x5c3b791d580e0bc2(type::entity entity, float p1) {
			invoker::invoke<void>(0x5c3b791d580e0bc2, entity, p1);
		}
		static void set_entity_always_prerender(type::entity entity, bool toggle) {
			invoker::invoke<void>(0xacad101e1fb66689, entity, toggle);
		}
		static void set_entity_render_scorched(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x730f5f8d3f0f2050, entity, toggle);
		}
		static void set_entity_trafficlight_override(type::entity entity, int state) {
			invoker::invoke<void>(0x57c5db656185eac4, entity, state);
		}
		static void _0x78e8e3a640178255(type::entity entity) {
			invoker::invoke<void>(0x78e8e3a640178255, entity);
		}
		static void create_model_swap(float x, float y, float z, float radius, type::hash originalmodel, type::hash newmodel, bool p6) {
			invoker::invoke<void>(0x92c47782fda8b2a3, x, y, z, radius, originalmodel, newmodel, p6);
		}
		static void remove_model_swap(float x, float y, float z, float radius, type::hash originalmodel, type::hash newmodel, bool p6) {
			invoker::invoke<void>(0x033c0f9a64e229ae, x, y, z, radius, originalmodel, newmodel, p6);
		}
		static void create_model_hide(float x, float y, float z, float radius, type::hash model, bool p5) {
			invoker::invoke<void>(0x8a97bca30a0ce478, x, y, z, radius, model, p5);
		}
		static void create_model_hide_excluding_script_objects(float x, float y, float z, float radius, type::hash model, bool p5) {
			invoker::invoke<void>(0x3a52ae588830bf7f, x, y, z, radius, model, p5);
		}
		static void remove_model_hide(float x, float y, float z, float radius, type::hash model, bool p5) {
			invoker::invoke<void>(0xd9e3006fb3cbd765, x, y, z, radius, model, p5);
		}
		static void create_forced_object(float x, float y, float z, type::any p3, type::hash modelhash, bool p5) {
			invoker::invoke<void>(0x150e808b375a385a, x, y, z, p3, modelhash, p5);
		}
		static void remove_forced_object(float posx, float posy, float posz, float unk, type::hash modelhash) {
			invoker::invoke<void>(0x61b6775e83c0db6f, posx, posy, posz, unk, modelhash);
		}
		static void set_entity_no_collision_entity(type::entity entity1, type::entity entity2, bool unknown) {
			invoker::invoke<void>(0xa53ed5520c07654a, entity1, entity2, unknown);
		}
		static void set_entity_motion_blur(type::entity entity, bool toggle) {
			invoker::invoke<void>(0x295d82a8559f9150, entity, toggle);
		}
		static void _0xe12abe5e3a389a6c(type::entity entity, bool p1) {
			invoker::invoke<void>(0xe12abe5e3a389a6c, entity, p1);
		}
		static void _0xa80ae305e0a3044f(type::entity entity, bool p1) {
			invoker::invoke<void>(0xa80ae305e0a3044f, entity, p1);
		}
		static void _0xdc6f8601faf2e893(type::entity entity, bool p1) {
			invoker::invoke<void>(0xdc6f8601faf2e893, entity, p1);
		}
		static void _0x2c2e3dc128f44309(type::entity entity, bool p1) {
			invoker::invoke<void>(0x2c2e3dc128f44309, entity, p1);
		}
		static void _0x1a092bb0c3808b96(type::entity entity, bool p1) {
			invoker::invoke<void>(0x1a092bb0c3808b96, entity, p1);
		}
	}

	namespace ped {
		static type::ped create_ped(int pedtype, type::hash modelhash, float x, float y, float z, float heading, bool isnetwork, bool thisscriptcheck) {
			return invoker::invoke<type::ped>(0xd49f9b0955c367de, pedtype, modelhash, x, y, z, heading, isnetwork, thisscriptcheck);
		}
		static void delete_ped(type::ped* ped) {
			invoker::invoke<void>(0x9614299dcb53e54b, ped);
		}
		static type::ped clone_ped(type::ped ped, float heading, bool isnetwork, bool thisscriptcheck) {
			return invoker::invoke<type::ped>(0xef29a16337facadb, ped, heading, isnetwork, thisscriptcheck);
		}
		static void clone_ped_to_target(type::ped ped, type::ped targetped) {
			invoker::invoke<void>(0xe952d6431689ad9a, ped, targetped);
		}
		static bool is_ped_in_vehicle(type::ped ped, type::vehicle vehicle, bool atgetin) {
			return invoker::invoke<bool>(0xa3ee4a07279bb9db, ped, vehicle, atgetin);
		}
		static bool is_ped_in_model(type::ped ped, type::hash modelhash) {
			return invoker::invoke<bool>(0x796d90efb19aa332, ped, modelhash);
		}
		static bool is_ped_in_any_vehicle(type::ped ped, bool atgetin) {
			return invoker::invoke<bool>(0x997abd671d25ca0b, ped, atgetin);
		}
		static bool is_cop_ped_in_area_3d(float x1, float y1, float z1, float x2, float y2, float z2) {
			return invoker::invoke<bool>(0x16ec4839969f9f5e, x1, y1, z1, x2, y2, z2);
		}
		static bool is_ped_injured(type::ped ped) {
			return invoker::invoke<bool>(0x84a2dd9ac37c35c1, ped);
		}
		static bool is_ped_hurt(type::ped ped) {
			return invoker::invoke<bool>(0x5983bb449d7fdb12, ped);
		}
		static bool is_ped_fatally_injured(type::ped ped) {
			return invoker::invoke<bool>(0xd839450756ed5a80, ped);
		}
		static bool is_ped_dead_or_dying(type::ped ped, bool p1) {
			return invoker::invoke<bool>(0x3317dedb88c95038, ped, p1);
		}
		static bool is_conversation_ped_dead(type::ped ped) {
			return invoker::invoke<bool>(0xe0a0aec214b1faba, ped);
		}
		static bool is_ped_aiming_from_cover(type::ped ped) {
			return invoker::invoke<bool>(0x3998b1276a3300e5, ped);
		}
		static bool is_ped_reloading(type::ped ped) {
			return invoker::invoke<bool>(0x24b100c68c645951, ped);
		}
		static bool is_ped_a_player(type::ped ped) {
			return invoker::invoke<bool>(0x12534c348c6cb68b, ped);
		}
		static type::ped create_ped_inside_vehicle(type::vehicle vehicle, int pedtype, type::hash modelhash, int seat, bool isnetwork, bool thisscriptcheck) {
			return invoker::invoke<type::ped>(0x7dd959874c1fd534, vehicle, pedtype, modelhash, seat, isnetwork, thisscriptcheck);
		}
		static void set_ped_desired_heading(type::ped ped, float heading) {
			invoker::invoke<void>(0xaa5a7ece2aa8fe70, ped, heading);
		}
		static void _freeze_ped_camera_rotation(type::ped ped) {
			invoker::invoke<void>(0xff287323b0e2c69a, ped);
		}
		static bool is_ped_facing_ped(type::ped ped, type::ped otherped, float angle) {
			return invoker::invoke<bool>(0xd71649db0a545aa3, ped, otherped, angle);
		}
		static bool is_ped_in_melee_combat(type::ped ped) {
			return invoker::invoke<bool>(0x4e209b2c1ead5159, ped);
		}
		static bool is_ped_stopped(type::ped ped) {
			return invoker::invoke<bool>(0x530944f6f4b8a214, ped);
		}
		static bool is_ped_shooting_in_area(type::ped ped, float x1, float y1, float z1, float x2, float y2, float z2, bool p7, bool p8) {
			return invoker::invoke<bool>(0x7e9dfe24ac1e58ef, ped, x1, y1, z1, x2, y2, z2, p7, p8);
		}
		static bool is_any_ped_shooting_in_area(float x1, float y1, float z1, float x2, float y2, float z2, bool p6, bool p7) {
			return invoker::invoke<bool>(0xa0d3d71ea1086c55, x1, y1, z1, x2, y2, z2, p6, p7);
		}
		static bool is_ped_shooting(type::ped ped) {
			return invoker::invoke<bool>(0x34616828cd07f1a1, ped);
		}
		static void set_ped_accuracy(type::ped ped, int accuracy) {
			invoker::invoke<void>(0x7aefb85c1d49deb6, ped, accuracy);
		}
		static int get_ped_accuracy(type::ped ped) {
			return invoker::invoke<int>(0x37f4ad56ecbc0cd6, ped);
		}
		static bool is_ped_model(type::ped ped, type::hash modelhash) {
			return invoker::invoke<bool>(0xc9d55b1a358a5bf7, ped, modelhash);
		}
		static void explode_ped_head(type::ped ped, type::hash weaponhash) {
			invoker::invoke<void>(0x2d05ced3a38d0f3a, ped, weaponhash);
		}
		static void remove_ped_elegantly(type::ped* ped) {
			invoker::invoke<void>(0xac6d445b994df95e, ped);
		}
		static void add_armour_to_ped(type::ped ped, int amount) {
			invoker::invoke<void>(0x5ba652a0cd14df2f, ped, amount);
		}
		static void set_ped_armour(type::ped ped, int amount) {
			invoker::invoke<void>(0xcea04d83135264cc, ped, amount);
		}
		static void set_ped_into_vehicle(type::ped ped, type::vehicle vehicle, int seatindex) {
			invoker::invoke<void>(0xf75b0d629e1c063d, ped, vehicle, seatindex);
		}
		static void set_ped_allow_vehicles_override(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x3c028c636a414ed9, ped, toggle);
		}
		static bool can_create_random_ped(bool unk) {
			return invoker::invoke<bool>(0x3e8349c08e4b82e4, unk);
		}
		static type::ped create_random_ped(float posx, float posy, float posz) {
			return invoker::invoke<type::ped>(0xb4ac7d0cf06bfe8f, posx, posy, posz);
		}
		static type::ped create_random_ped_as_driver(type::vehicle banshee, bool returnhandle) {
			return invoker::invoke<type::ped>(0x9b62392b474f44a0, banshee, returnhandle);
		}
		static bool can_create_random_driver() {
			return invoker::invoke<bool>(0xb8eb95e5b4e56978);
		}
		static bool can_create_random_bike_rider() {
			return invoker::invoke<bool>(0xeaceeda81751915c);
		}
		static void set_ped_move_anims_blend_out(type::ped ped) {
			invoker::invoke<void>(0x9e8c908f41584ecd, ped);
		}
		static void set_ped_can_be_dragged_out(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xc1670e958eee24e5, ped, toggle);
		}
		static void _0xf2bebcdfafdaa19e(bool toggle) {
			invoker::invoke<void>(0xf2bebcdfafdaa19e, toggle);
		}
		static bool is_ped_male(type::ped ped) {
			return invoker::invoke<bool>(0x6d9f5faa7488ba46, ped);
		}
		static bool is_ped_human(type::ped ped) {
			return invoker::invoke<bool>(0xb980061da992779d, ped);
		}
		static type::vehicle get_vehicle_ped_is_in(type::ped ped, bool lastvehicle) {
			return invoker::invoke<type::vehicle>(0x9a9112a0fe9a4713, ped, lastvehicle);
		}
		static void reset_ped_last_vehicle(type::ped ped) {
			invoker::invoke<void>(0xbb8de8cf6a8dd8bb, ped);
		}
		static void set_ped_density_multiplier_this_frame(float multiplier) {
			invoker::invoke<void>(0x95e3d6257b166cf2, multiplier);
		}
		static void set_scenario_ped_density_multiplier_this_frame(float p0, float p1) {
			invoker::invoke<void>(0x7a556143a1c03898, p0, p1);
		}
		static void _0x5a7f62fda59759bd() {
			invoker::invoke<void>(0x5a7f62fda59759bd);
		}
		static void set_scripted_conversion_coord_this_frame(float x, float y, float z) {
			invoker::invoke<void>(0x5086c7843552cf85, x, y, z);
		}
		static void set_ped_non_creation_area(float x1, float y1, float z1, float x2, float y2, float z2) {
			invoker::invoke<void>(0xee01041d559983ea, x1, y1, z1, x2, y2, z2);
		}
		static void clear_ped_non_creation_area() {
			invoker::invoke<void>(0x2e05208086ba0651);
		}
		static void _0x4759cc730f947c81() {
			invoker::invoke<void>(0x4759cc730f947c81);
		}
		static bool is_ped_on_mount(type::ped ped) {
			return invoker::invoke<bool>(0x460bc76a0e10655e, ped);
		}
		static type::ped get_mount(type::ped ped) {
			return invoker::invoke<type::ped>(0xe7e11b8dcbed1058, ped);
		}
		static bool is_ped_on_vehicle(type::ped ped) {
			return invoker::invoke<bool>(0x67722aeb798e5fab, ped);
		}
		static bool is_ped_on_specific_vehicle(type::ped ped, type::vehicle vehicle) {
			return invoker::invoke<bool>(0xec5f66e459af3bb2, ped, vehicle);
		}
		static void set_ped_money(type::ped ped, int amount) {
			invoker::invoke<void>(0xa9c8960e8684c1b5, ped, amount);
		}
		static int get_ped_money(type::ped ped) {
			return invoker::invoke<int>(0x3f69145bba87bae7, ped);
		}
		static void _0xff4803bc019852d9(float p0, type::any p1) {
			invoker::invoke<void>(0xff4803bc019852d9, p0, p1);
		}
		static void _0x6b0e6172c9a4d902(bool p0) {
			invoker::invoke<void>(0x6b0e6172c9a4d902, p0);
		}
		static void _0x9911f4a24485f653(bool p0) {
			invoker::invoke<void>(0x9911f4a24485f653, p0);
		}
		static void set_ped_suffers_critical_hits(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xebd76f2359f190ac, ped, toggle);
		}
		static void _0xafc976fd0580c7b3(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xafc976fd0580c7b3, ped, toggle);
		}
		static bool is_ped_sitting_in_vehicle(type::ped ped, type::vehicle vehicle) {
			return invoker::invoke<bool>(0xa808aa1d79230fc2, ped, vehicle);
		}
		static bool is_ped_sitting_in_any_vehicle(type::ped ped) {
			return invoker::invoke<bool>(0x826aa586edb9fef8, ped);
		}
		static bool is_ped_on_foot(type::ped ped) {
			return invoker::invoke<bool>(0x01fee67db37f59b2, ped);
		}
		static bool is_ped_on_any_bike(type::ped ped) {
			return invoker::invoke<bool>(0x94495889e22c6479, ped);
		}
		static bool is_ped_planting_bomb(type::ped ped) {
			return invoker::invoke<bool>(0xc70b5fae151982d8, ped);
		}
		static PVector3 get_dead_ped_pickup_coords(type::ped ped, float p1, float p2) {
			return invoker::invoke<PVector3>(0xcd5003b097200f36, ped, p1, p2);
		}
		static bool is_ped_in_any_boat(type::ped ped) {
			return invoker::invoke<bool>(0x2e0e1c2b4f6cb339, ped);
		}
		static bool is_ped_in_any_sub(type::ped ped) {
			return invoker::invoke<bool>(0xfbfc01ccfb35d99e, ped);
		}
		static bool is_ped_in_any_heli(type::ped ped) {
			return invoker::invoke<bool>(0x298b91ae825e5705, ped);
		}
		static bool is_ped_in_any_plane(type::ped ped) {
			return invoker::invoke<bool>(0x5fff4cfc74d8fb80, ped);
		}
		static bool is_ped_in_flying_vehicle(type::ped ped) {
			return invoker::invoke<bool>(0x9134873537fa419c, ped);
		}
		static void set_ped_dies_in_water(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x56cef0ac79073bde, ped, toggle);
		}
		static void set_ped_dies_in_sinking_vehicle(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xd718a22995e2b4bc, ped, toggle);
		}
		static int get_ped_armour(type::ped ped) {
			return invoker::invoke<int>(0x9483af821605b1d8, ped);
		}
		static void set_ped_stay_in_vehicle_when_jacked(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xedf4079f9d54c9a1, ped, toggle);
		}
		static void set_ped_can_be_shot_in_vehicle(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xc7ef1ba83230ba07, ped, toggle);
		}
		static bool get_ped_last_damage_bone(type::ped ped, int* outbone) {
			return invoker::invoke<bool>(0xd75960f6bd9ea49c, ped, outbone);
		}
		static void clear_ped_last_damage_bone(type::ped ped) {
			invoker::invoke<void>(0x8ef6b7ac68e2f01b, ped);
		}
		static void set_ai_weapon_damage_modifier(float value) {
			invoker::invoke<void>(0x1b1e2a40a65b8521, value);
		}
		static void reset_ai_weapon_damage_modifier() {
			invoker::invoke<void>(0xea16670e7ba4743c);
		}
		static void set_ai_melee_weapon_damage_modifier(float modifier) {
			invoker::invoke<void>(0x66460deddd417254, modifier);
		}
		static void reset_ai_melee_weapon_damage_modifier() {
			invoker::invoke<void>(0x46e56a7cd1d63c3f);
		}
		static void _0x2f3c3d9f50681de4(type::any p0, bool p1) {
			invoker::invoke<void>(0x2f3c3d9f50681de4, p0, p1);
		}
		static void set_ped_can_be_targetted(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x63f58f7c80513aad, ped, toggle);
		}
		static void set_ped_can_be_targetted_by_team(type::ped ped, int team, bool toggle) {
			invoker::invoke<void>(0xbf1ca77833e58f2c, ped, team, toggle);
		}
		static void set_ped_can_be_targetted_by_player(type::ped ped, type::player player, bool toggle) {
			invoker::invoke<void>(0x66b57b72e0836a76, ped, player, toggle);
		}
		static void _0x061cb768363d6424(type::any p0, bool p1) {
			invoker::invoke<void>(0x061cb768363d6424, p0, p1);
		}
		static void set_time_exclusive_display_texture(type::any p0, bool p1) {
			invoker::invoke<void>(0xfd325494792302d7, p0, p1);
		}
		static bool is_ped_in_any_police_vehicle(type::ped ped) {
			return invoker::invoke<bool>(0x0bd04e29640c9c12, ped);
		}
		static void force_ped_to_open_parachute(type::ped ped) {
			invoker::invoke<void>(0x16e42e800b472221, ped);
		}
		static bool is_ped_in_parachute_free_fall(type::ped ped) {
			return invoker::invoke<bool>(0x7dce8bda0f1c1200, ped);
		}
		static bool is_ped_falling(type::ped ped) {
			return invoker::invoke<bool>(0xfb92a102f1c4dfa3, ped);
		}
		static bool is_ped_jumping(type::ped ped) {
			return invoker::invoke<bool>(0xcedabc5900a0bf97, ped);
		}
		static bool is_ped_climbing(type::ped ped) {
			return invoker::invoke<bool>(0x53e8cb4f48bfe623, ped);
		}
		static bool is_ped_vaulting(type::ped ped) {
			return invoker::invoke<bool>(0x117c70d1f5730b5e, ped);
		}
		static bool is_ped_diving(type::ped ped) {
			return invoker::invoke<bool>(0x5527b8246fef9b11, ped);
		}
		static bool is_ped_jumping_out_of_vehicle(type::ped ped) {
			return invoker::invoke<bool>(0x433ddffe2044b636, ped);
		}
		static bool _0x26af0e8e30bd2a2c(type::ped ped) {
			return invoker::invoke<bool>(0x26af0e8e30bd2a2c, ped);
		}
		static int get_ped_parachute_state(type::ped ped) {
			return invoker::invoke<int>(0x79cfd9827cc979b6, ped);
		}
		static int get_ped_parachute_landing_type(type::ped ped) {
			return invoker::invoke<int>(0x8b9f1fc6ae8166c0, ped);
		}
		static void set_ped_parachute_tint_index(type::ped ped, int tintindex) {
			invoker::invoke<void>(0x333fc8db079b7186, ped, tintindex);
		}
		static void get_ped_parachute_tint_index(type::ped ped, int* outtintindex) {
			invoker::invoke<void>(0xeaf5f7e5ae7c6c9d, ped, outtintindex);
		}
		static void set_ped_reserve_parachute_tint_index(type::ped ped, type::any p1) {
			invoker::invoke<void>(0xe88da0751c22a2ad, ped, p1);
		}
		static type::entity _0x8c4f3bf23b6237db(type::ped ped, bool p1, bool p2) {
			return invoker::invoke<type::entity>(0x8c4f3bf23b6237db, ped, p1, p2);
		}
		static void set_ped_ducking(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x030983ca930b692d, ped, toggle);
		}
		static bool is_ped_ducking(type::ped ped) {
			return invoker::invoke<bool>(0xd125ae748725c6bc, ped);
		}
		static bool is_ped_in_any_taxi(type::ped ped) {
			return invoker::invoke<bool>(0x6e575d6a898ab852, ped);
		}
		static void set_ped_id_range(type::ped ped, float value) {
			invoker::invoke<void>(0xf107e836a70dce05, ped, value);
		}
		static void _0x52d59ab61ddc05dd(type::ped ped, bool p1) {
			invoker::invoke<void>(0x52d59ab61ddc05dd, ped, p1);
		}
		static void _0xec4b4b3b9908052a(type::ped ped, float unk) {
			invoker::invoke<void>(0xec4b4b3b9908052a, ped, unk);
		}
		static void _0x733c87d4ce22bea2(type::any p0) {
			invoker::invoke<void>(0x733c87d4ce22bea2, p0);
		}
		static void set_ped_seeing_range(type::ped ped, float value) {
			invoker::invoke<void>(0xf29cf591c4bf6cee, ped, value);
		}
		static void set_ped_hearing_range(type::ped ped, float value) {
			invoker::invoke<void>(0x33a8f7f7d5f7f33c, ped, value);
		}
		static void set_ped_visual_field_min_angle(type::ped ped, float value) {
			invoker::invoke<void>(0x2db492222fb21e26, ped, value);
		}
		static void set_ped_visual_field_max_angle(type::ped ped, float value) {
			invoker::invoke<void>(0x70793bdca1e854d4, ped, value);
		}
		static void set_ped_visual_field_min_elevation_angle(type::ped ped, float angle) {
			invoker::invoke<void>(0x7a276eb2c224d70f, ped, angle);
		}
		static void set_ped_visual_field_max_elevation_angle(type::ped ped, float angle) {
			invoker::invoke<void>(0x78d0b67629d75856, ped, angle);
		}
		static void set_ped_visual_field_peripheral_range(type::ped ped, float range) {
			invoker::invoke<void>(0x9c74b0bc831b753a, ped, range);
		}
		static void set_ped_visual_field_center_angle(type::ped ped, float angle) {
			invoker::invoke<void>(0x3b6405e8ab34a907, ped, angle);
		}
		static void set_ped_stealth_movement(type::ped ped, bool p1, const char* action) {
			invoker::invoke<void>(0x88cbb5ceb96b7bd2, ped, p1, action);
		}
		static bool get_ped_stealth_movement(type::ped ped) {
			return invoker::invoke<bool>(0x7c2ac9ca66575fbf, ped);
		}
		static int create_group(int unused) {
			return invoker::invoke<int>(0x90370ebe0fee1a3d, unused);
		}
		static void set_ped_as_group_leader(type::ped ped, int groupid) {
			invoker::invoke<void>(0x2a7819605465fbce, ped, groupid);
		}
		static void set_ped_as_group_member(type::ped ped, int groupid) {
			invoker::invoke<void>(0x9f3480fe65db31b5, ped, groupid);
		}
		static void set_ped_can_teleport_to_group_leader(type::ped pedhandle, int grouphandle, bool toggle) {
			invoker::invoke<void>(0x2e2f4240b3f24647, pedhandle, grouphandle, toggle);
		}
		static void remove_group(int groupid) {
			invoker::invoke<void>(0x8eb2f69076af7053, groupid);
		}
		static void remove_ped_from_group(type::ped ped) {
			invoker::invoke<void>(0xed74007ffb146bc2, ped);
		}
		static bool is_ped_group_member(type::ped ped, int groupid) {
			return invoker::invoke<bool>(0x9bb01e3834671191, ped, groupid);
		}
		static bool is_ped_hanging_on_to_vehicle(type::ped ped) {
			return invoker::invoke<bool>(0x1c86d8aef8254b78, ped);
		}
		static void set_group_separation_range(int grouphandle, float separationrange) {
			invoker::invoke<void>(0x4102c7858cfee4e4, grouphandle, separationrange);
		}
		static void set_ped_min_ground_time_for_stungun(type::ped ped, int ms1000000) {
			invoker::invoke<void>(0xfa0675ab151073fa, ped, ms1000000);
		}
		static bool is_ped_prone(type::ped ped) {
			return invoker::invoke<bool>(0xd6a86331a537a7b9, ped);
		}
		static bool is_ped_in_combat(type::ped ped, type::ped target) {
			return invoker::invoke<bool>(0x4859f1fc66a6278e, ped, target);
		}
		static bool can_ped_in_combat_see_target(type::ped ped, type::ped target) {
			return invoker::invoke<bool>(0xead42de3610d0721, ped, target);
		}
		static bool is_ped_doing_driveby(type::ped ped) {
			return invoker::invoke<bool>(0xb2c086cc1bf8f2bf, ped);
		}
		static bool is_ped_jacking(type::ped ped) {
			return invoker::invoke<bool>(0x4ae4ff911dfb61da, ped);
		}
		static bool is_ped_being_jacked(type::ped ped) {
			return invoker::invoke<bool>(0x9a497fe2df198913, ped);
		}
		static bool is_ped_being_stunned(type::ped ped, int p1) {
			return invoker::invoke<bool>(0x4fbacce3b4138ee8, ped, p1);
		}
		static type::ped get_peds_jacker(type::ped ped) {
			return invoker::invoke<type::ped>(0x9b128dc36c1e04cf, ped);
		}
		static type::ped get_jack_target(type::ped ped) {
			return invoker::invoke<type::ped>(0x5486a79d9fbd342d, ped);
		}
		static bool is_ped_fleeing(type::ped ped) {
			return invoker::invoke<bool>(0xbbcce00b381f8482, ped);
		}
		static bool is_ped_in_cover(type::ped ped, bool exceptuseweapon) {
			return invoker::invoke<bool>(0x60dfd0691a170b88, ped, exceptuseweapon);
		}
		static bool is_ped_in_cover_facing_left(type::ped ped) {
			return invoker::invoke<bool>(0x845333b3150583ab, ped);
		}
		static bool _is_ped_standing_in_cover(type::ped ped) {
			return invoker::invoke<bool>(0x6a03bf943d767c93, ped);
		}
		static bool is_ped_going_into_cover(type::ped ped) {
			return invoker::invoke<bool>(0x9f65dbc537e59ad5, ped);
		}
		static type::any set_ped_pinned_down(type::ped player, bool pinned, int p2) {
			return invoker::invoke<type::any>(0xaad6d1acf08f4612, player, pinned, p2);
		}
		static int get_seat_ped_is_trying_to_enter(type::ped ped) {
			return invoker::invoke<int>(0x6f4c85acd641bcd2, ped);
		}
		static type::vehicle get_vehicle_ped_is_trying_to_enter(type::ped ped) {
			return invoker::invoke<type::vehicle>(0x814fa8be5449445d, ped);
		}
		static type::entity get_ped_source_of_death(type::ped ped) {
			return invoker::invoke<type::entity>(0x93c8b64deb84728c, ped);
		}
		static type::hash get_ped_cause_of_death(type::ped ped) {
			return invoker::invoke<type::hash>(0x16ffe42ab2d2dc59, ped);
		}
		static int _0x1e98817b311ae98a(type::ped ped) {
			return invoker::invoke<int>(0x1e98817b311ae98a, ped);
		}
		static int _0x5407b7288d0478b7(type::any p0) {
			return invoker::invoke<int>(0x5407b7288d0478b7, p0);
		}
		static bool _is_enemy_in_range(type::ped ped, float x, float y, float z, float range) {
			return invoker::invoke<bool>(0x336b3d200ab007cb, ped, x, y, z, range);
		}
		static void set_ped_relationship_group_default_hash(type::ped ped, type::hash hash) {
			invoker::invoke<void>(0xadb3f206518799e8, ped, hash);
		}
		static void set_ped_relationship_group_hash(type::ped ped, type::hash hash) {
			invoker::invoke<void>(0xc80a74ac829ddd92, ped, hash);
		}
		static void set_relationship_between_groups(int relationship, type::hash group1, type::hash group2) {
			invoker::invoke<void>(0xbf25eb89375a37ad, relationship, group1, group2);
		}
		static void clear_relationship_between_groups(int relationship, type::hash group1, type::hash group2) {
			invoker::invoke<void>(0x5e29243fb56fc6d4, relationship, group1, group2);
		}
		static type::any add_relationship_group(const char* name, type::hash* grouphash) {
			return invoker::invoke<type::any>(0xf372bc22fcb88606, name, grouphash);
		}
		static void remove_relationship_group(type::hash grouphash) {
			invoker::invoke<void>(0xb6ba2444ab393da2, grouphash);
		}
		static int get_relationship_between_peds(type::ped ped1, type::ped ped2) {
			return invoker::invoke<int>(0xeba5ad3a0eaf7121, ped1, ped2);
		}
		static type::hash get_ped_relationship_group_default_hash(type::ped ped) {
			return invoker::invoke<type::hash>(0x42fdd0f017b1e38e, ped);
		}
		static type::hash get_ped_relationship_group_hash(type::ped ped) {
			return invoker::invoke<type::hash>(0x7dbdd04862d95f04, ped);
		}
		static int get_relationship_between_groups(type::hash group1, type::hash group2) {
			return invoker::invoke<int>(0x9e6b70061662ae5c, group1, group2);
		}
		static void set_ped_can_be_targeted_without_los(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x4328652ae5769c71, ped, toggle);
		}
		static void set_ped_to_inform_respected_friends(type::ped ped, float radius, int maxfriends) {
			invoker::invoke<void>(0x112942c6e708f70b, ped, radius, maxfriends);
		}
		static bool is_ped_responding_to_event(type::ped ped, type::any event) {
			return invoker::invoke<bool>(0x625b774d75c87068, ped, event);
		}
		static void set_ped_firing_pattern(type::ped ped, type::hash patternhash) {
			invoker::invoke<void>(0x9ac577f5a12ad8a9, ped, patternhash);
		}
		static void set_ped_shoot_rate(type::ped ped, int shootrate) {
			invoker::invoke<void>(0x614da022990752dc, ped, shootrate);
		}
		static void set_combat_float(type::ped ped, int combattype, float p2) {
			invoker::invoke<void>(0xff41b4b141ed981c, ped, combattype, p2);
		}
		static float get_combat_float(type::ped ped, int p1) {
			return invoker::invoke<float>(0x52dff8a10508090a, ped, p1);
		}
		static void get_group_size(int groupid, type::any* unknown, int* sizeinmembers) {
			invoker::invoke<void>(0x8de69fe35ca09a45, groupid, unknown, sizeinmembers);
		}
		static bool does_group_exist(int groupid) {
			return invoker::invoke<bool>(0x7c6b0c22f9f40bbe, groupid);
		}
		static int get_ped_group_index(type::ped ped) {
			return invoker::invoke<int>(0xf162e133b4e7a675, ped);
		}
		static bool is_ped_in_group(type::ped ped) {
			return invoker::invoke<bool>(0x5891cac5d4acff74, ped);
		}
		static type::player get_player_ped_is_following(type::ped ped) {
			return invoker::invoke<type::player>(0x6a3975dea89f9a17, ped);
		}
		static void set_group_formation(int groupid, int formationtype) {
			invoker::invoke<void>(0xce2f5fc3af7e8c1e, groupid, formationtype);
		}
		static void set_group_formation_spacing(int groupid, float p1, float p2, float p3) {
			invoker::invoke<void>(0x1d9d45004c28c916, groupid, p1, p2, p3);
		}
		static void reset_group_formation_default_spacing(int groupid) {
			invoker::invoke<void>(0x63dab4ccb3273205, groupid);
		}
		static type::vehicle get_vehicle_ped_is_using(type::ped ped) {
			return invoker::invoke<type::vehicle>(0x6094ad011a2ea87d, ped);
		}
		static type::vehicle set_exclusive_phone_relationships(type::ped ped) {
			return invoker::invoke<type::vehicle>(0xf92691aed837a5fc, ped);
		}
		static void set_ped_gravity(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x9ff447b6b6ad960a, ped, toggle);
		}
		static void apply_damage_to_ped(type::ped ped, int damageamount, bool armorfirst) {
			invoker::invoke<void>(0x697157ced63f18d4, ped, damageamount, armorfirst);
		}
		static type::any _0x36b77bb84687c318(type::ped ped, type::any p1) {
			return invoker::invoke<type::any>(0x36b77bb84687c318, ped, p1);
		}
		static void set_ped_allowed_to_duck(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xda1f1b7be1a8766f, ped, toggle);
		}
		static void set_ped_never_leaves_group(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x3dbfc55d5c9bb447, ped, toggle);
		}
		static int get_ped_type(type::ped ped) {
			return invoker::invoke<int>(0xff059e1e4c01e63c, ped);
		}
		static void set_ped_as_cop(type::ped player, bool toggle) {
			invoker::invoke<void>(0xbb03c38dd3fb7ffd, player, toggle);
		}
		static void set_ped_max_health(type::ped ped, int value) {
			invoker::invoke<void>(0xf5f6378c4f3419d3, ped, value);
		}
		static int get_ped_max_health(type::ped ped) {
			return invoker::invoke<int>(0x4700a416e8324ef3, ped);
		}
		static void set_ped_max_time_in_water(type::ped ped, float value) {
			invoker::invoke<void>(0x43c851690662113d, ped, value);
		}
		static void set_ped_max_time_underwater(type::ped ped, float value) {
			invoker::invoke<void>(0x6ba428c528d9e522, ped, value);
		}
		static void _0x2735233a786b1bef(type::ped ped, float p1) {
			invoker::invoke<void>(0x2735233a786b1bef, ped, p1);
		}
		static void _0x952f06beecd775cc(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x952f06beecd775cc, p0, p1, p2, p3);
		}
		static void _0xe6ca85e7259ce16b(type::any p0) {
			invoker::invoke<void>(0xe6ca85e7259ce16b, p0);
		}
		static void set_ped_can_be_knocked_off_vehicle(type::ped ped, int state) {
			invoker::invoke<void>(0x7a6535691b477c48, ped, state);
		}
		static bool can_knock_ped_off_vehicle(type::ped ped) {
			return invoker::invoke<bool>(0x51ac07a44d4f5b8a, ped);
		}
		static void knock_ped_off_vehicle(type::ped ped) {
			invoker::invoke<void>(0x45bbcba77c29a841, ped);
		}
		static void set_ped_coords_no_gang(type::ped ped, float posx, float posy, float posz) {
			invoker::invoke<void>(0x87052fe446e07247, ped, posx, posy, posz);
		}
		static type::ped get_ped_as_group_member(int groupid, int membernumber) {
			return invoker::invoke<type::ped>(0x51455483cf23ed97, groupid, membernumber);
		}
		static type::ped _get_ped_as_group_leader(int groupid) {
			return invoker::invoke<type::ped>(0x5cce68dbd5fe93ec, groupid);
		}
		static void set_ped_keep_task(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x971d38760fbc02ef, ped, toggle);
		}
		static void _0x49e50bdb8ba4dab2(type::ped ped, bool p1) {
			invoker::invoke<void>(0x49e50bdb8ba4dab2, ped, p1);
		}
		static bool is_ped_swimming(type::ped ped) {
			return invoker::invoke<bool>(0x9de327631295b4c2, ped);
		}
		static bool is_ped_swimming_under_water(type::ped ped) {
			return invoker::invoke<bool>(0xc024869a53992f34, ped);
		}
		static void set_ped_coords_keep_vehicle(type::ped ped, float posx, float posy, float posz) {
			invoker::invoke<void>(0x9afeff481a85ab2e, ped, posx, posy, posz);
		}
		static void set_ped_dies_in_vehicle(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x2a30922c90c9b42c, ped, toggle);
		}
		static void set_create_random_cops(bool toggle) {
			invoker::invoke<void>(0x102e68b2024d536d, toggle);
		}
		static void set_create_random_cops_not_on_scenarios(bool toggle) {
			invoker::invoke<void>(0x8a4986851c4ef6e7, toggle);
		}
		static void set_create_random_cops_on_scenarios(bool toggle) {
			invoker::invoke<void>(0x444cb7d7dbe6973d, toggle);
		}
		static bool can_create_random_cops() {
			return invoker::invoke<bool>(0x5ee2caff7f17770d);
		}
		static void set_ped_as_enemy(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x02a0c9720b854bfa, ped, toggle);
		}
		static void set_ped_can_smash_glass(type::ped ped, bool p1, bool p2) {
			invoker::invoke<void>(0x1cce141467ff42a2, ped, p1, p2);
		}
		static bool is_ped_in_any_train(type::ped ped) {
			return invoker::invoke<bool>(0x6f972c1ab75a1ed0, ped);
		}
		static bool is_ped_getting_into_a_vehicle(type::ped ped) {
			return invoker::invoke<bool>(0xbb062b2b5722478e, ped);
		}
		static bool is_ped_trying_to_enter_a_locked_vehicle(type::ped ped) {
			return invoker::invoke<bool>(0x44d28d5ddfe5f68c, ped);
		}
		static void set_enable_handcuffs(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xdf1af8b5d56542fa, ped, toggle);
		}
		static void set_enable_bound_ankles(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xc52e0f855c58fc2e, ped, toggle);
		}
		static void set_enable_scuba(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xf99f62004024d506, ped, toggle);
		}
		static void set_can_attack_friendly(type::ped ped, bool toggle, bool p2) {
			invoker::invoke<void>(0xb3b1cb349ff9c75d, ped, toggle, p2);
		}
		static int get_ped_alertness(type::ped ped) {
			return invoker::invoke<int>(0xf6aa118530443fd2, ped);
		}
		static void set_ped_alertness(type::ped ped, int value) {
			invoker::invoke<void>(0xdba71115ed9941a6, ped, value);
		}
		static void set_ped_get_out_upside_down_vehicle(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xbc0ed94165a48bc2, ped, toggle);
		}
		static void set_ped_movement_clipset(bool player, bool clipset, float blendtime) {
			invoker::invoke<void>(0xaf8a94ede7712bef, player, clipset, blendtime);
		}
		static void reset_ped_movement_clipset(type::ped ped, float p1) {
			invoker::invoke<void>(0xaa74ec0cb0aaea2c, ped, p1);
		}
		static void set_ped_strafe_clipset(type::ped ped, const char* clipset) {
			invoker::invoke<void>(0x29a28f3f8cf6d854, ped, clipset);
		}
		static void reset_ped_strafe_clipset(type::ped ped) {
			invoker::invoke<void>(0x20510814175ea477, ped);
		}
		static void set_ped_weapon_movement_clipset(type::ped ped, const char* clipset) {
			invoker::invoke<void>(0x2622e35b77d3aca2, ped, clipset);
		}
		static void reset_ped_weapon_movement_clipset(type::ped ped) {
			invoker::invoke<void>(0x97b0db5b4aa74e77, ped);
		}
		static void set_ped_drive_by_clipset_override(type::ped ped, const char* clipset) {
			invoker::invoke<void>(0xed34ab6c5cb36520, ped, clipset);
		}
		static void clear_ped_drive_by_clipset_override(type::ped ped) {
			invoker::invoke<void>(0x4afe3690d7e0b5ac, ped);
		}
		static void _set_ped_cover_clipset_override(type::ped ped, const char* clipset) {
			invoker::invoke<void>(0x9dba107b4937f809, ped, clipset);
		}
		static void _clear_ped_cover_clipset_override(type::ped ped) {
			invoker::invoke<void>(0xc79196dcb36f6121, ped);
		}
		static void _0x80054d7fcc70eec6(type::any p0) {
			invoker::invoke<void>(0x80054d7fcc70eec6, p0);
		}
		static void set_ped_in_vehicle_context(type::ped ped, type::hash context) {
			invoker::invoke<void>(0x530071295899a8c6, ped, context);
		}
		static void reset_ped_in_vehicle_context(type::ped ped) {
			invoker::invoke<void>(0x22ef8ff8778030eb, ped);
		}
		static bool is_scripted_scenario_ped_using_conditional_anim(type::ped ped, const char* animdict, const char* anim) {
			return invoker::invoke<bool>(0x6ec47a344923e1ed, ped, animdict, anim);
		}
		static void set_ped_alternate_walk_anim(type::ped ped, const char* animdict, const char* animname, float p3, bool p4) {
			invoker::invoke<void>(0x6c60394cb4f75e9a, ped, animdict, animname, p3, p4);
		}
		static void clear_ped_alternate_walk_anim(type::ped ped, float p1) {
			invoker::invoke<void>(0x8844bbfce30aa9e9, ped, p1);
		}
		static void set_ped_alternate_movement_anim(type::ped ped, int stance, const char* animdictionary, const char* animationname, float p4, bool p5) {
			invoker::invoke<void>(0x90a43cc281ffab46, ped, stance, animdictionary, animationname, p4, p5);
		}
		static void clear_ped_alternate_movement_anim(type::ped ped, int stance, float p2) {
			invoker::invoke<void>(0xd8d19675ed5fbdce, ped, stance, p2);
		}
		static void set_ped_gesture_group(type::ped ped, const char* animgroupgesture) {
			invoker::invoke<void>(0xddf803377f94aaa8, ped, animgroupgesture);
		}
		static PVector3 get_anim_initial_offset_position(const char* animdict, const char* animname, float x, float y, float z, float xrot, float yrot, float zrot, float p8, int p9) {
			return invoker::invoke<PVector3>(0xbe22b26dd764c040, animdict, animname, x, y, z, xrot, yrot, zrot, p8, p9);
		}
		static PVector3 get_anim_initial_offset_rotation(const char* animdict, const char* animname, float x, float y, float z, float xrot, float yrot, float zrot, float p8, int p9) {
			return invoker::invoke<PVector3>(0x4b805e6046ee9e47, animdict, animname, x, y, z, xrot, yrot, zrot, p8, p9);
		}
		static int get_ped_drawable_variation(type::ped ped, int componentid) {
			return invoker::invoke<int>(0x67f3780dd425d4fc, ped, componentid);
		}
		static int get_number_of_ped_drawable_variations(type::ped ped, int sc) {
			return invoker::invoke<int>(0x27561561732a7842, ped, sc);
		}
		static int get_ped_texture_variation(type::ped ped, int componentid) {
			return invoker::invoke<int>(0x04a355e041e004e6, ped, componentid);
		}
		static int get_number_of_ped_texture_variations(type::ped mp_f_freemode_01, int te, int drawableid) {
			return invoker::invoke<int>(0x8f7156a3142a6bad, mp_f_freemode_01, te, drawableid);
		}
		static int get_number_of_ped_prop_drawable_variations(type::ped ped, int propid) {
			return invoker::invoke<int>(0x5faf9754e789fb47, ped, propid);
		}
		static int get_number_of_ped_prop_texture_variations(type::ped ped, int propid, int drawableid) {
			return invoker::invoke<int>(0xa6e7f1ceb523e171, ped, propid, drawableid);
		}
		static int get_ped_palette_variation(type::ped ped, int componentid) {
			return invoker::invoke<int>(0xe3dd5f2a84b42281, ped, componentid);
		}
		static bool _0x9e30e91fb03a2caf(type::any* p0, type::any* p1) {
			return invoker::invoke<bool>(0x9e30e91fb03a2caf, p0, p1);
		}
		static type::any _0x1e77fa7a62ee6c4c(type::any p0) {
			return invoker::invoke<type::any>(0x1e77fa7a62ee6c4c, p0);
		}
		static type::any _0xf033419d1b81fae8(type::any p0) {
			return invoker::invoke<type::any>(0xf033419d1b81fae8, p0);
		}
		static bool is_ped_component_variation_valid(type::ped ped, int componentid, int drawableid, int textureid) {
			return invoker::invoke<bool>(0xe825f6b6cea7671d, ped, componentid, drawableid, textureid);
		}
		static void set_ped_component_variation(type::ped ped, int componentid, int drawableid, int textureid, int paletteid) {
			invoker::invoke<void>(0x262b14f48d29de80, ped, componentid, drawableid, textureid, paletteid);
		}
		static void set_ped_random_component_variation(type::ped ped, bool p1) {
			invoker::invoke<void>(0xc8a9481a01e63c28, ped, p1);
		}
		static void set_ped_random_props(type::ped ped) {
			invoker::invoke<void>(0xc44aa05345c992c6, ped);
		}
		static void set_ped_default_component_variation(type::ped ped) {
			invoker::invoke<void>(0x45eee61580806d63, ped);
		}
		static void set_ped_blend_from_parents(type::ped ped, type::ped father, type::ped mother, float fathersside, float mothersside) {
			invoker::invoke<void>(0x137bbd05230db22d, ped, father, mother, fathersside, mothersside);
		}
		static void set_ped_head_blend_data(type::ped ped, int shapefirstid, int shapesecondid, int shapethirdid, int skinfirstid, int skinsecondid, int skinthirdid, float shapemix, float skinmix, float thirdmix, bool isparent) {
			invoker::invoke<void>(0x9414e18b9434c2fe, ped, shapefirstid, shapesecondid, shapethirdid, skinfirstid, skinsecondid, skinthirdid, shapemix, skinmix, thirdmix, isparent);
		}
		static bool _get_ped_head_blend_data(type::ped ped, type::any* headblenddata) {
			return invoker::invoke<bool>(0x2746bd9d88c5c5d0, ped, headblenddata);
		}
		static void update_ped_head_blend_data(type::ped ped, float shapemix, float skinmix, float thirdmix) {
			invoker::invoke<void>(0x723538f61c647c5a, ped, shapemix, skinmix, thirdmix);
		}
		static void _set_ped_eye_color(type::ped ped, int index) {
			invoker::invoke<void>(0x50b56988b170afdf, ped, index);
		}
		static void set_ped_head_overlay(type::ped ped, int overlayid, int index, float opacity) {
			invoker::invoke<void>(0x48f44967fa05cc1e, ped, overlayid, index, opacity);
		}
		static int _get_ped_head_overlay_value(type::ped ped, int overlayid) {
			return invoker::invoke<int>(0xa60ef3b6461a4d43, ped, overlayid);
		}
		static int _get_num_head_overlay_values(int overlayid) {
			return invoker::invoke<int>(0xcf1ce768bb43480e, overlayid);
		}
		static void _set_ped_head_overlay_color(type::ped ped, int overlayid, int colortype, int colorid, int secondcolorid) {
			invoker::invoke<void>(0x497bf74a7b9cb952, ped, overlayid, colortype, colorid, secondcolorid);
		}
		static void _set_ped_hair_color(type::ped ped, int colorid, int highlightcolorid) {
			invoker::invoke<void>(0x4cffc65454c93a49, ped, colorid, highlightcolorid);
		}
		static int _get_num_hair_colors() {
			return invoker::invoke<int>(0xe5c0cf872c2ad150);
		}
		static int _get_num_makeup_colors() {
			return invoker::invoke<int>(0xd1f7ca1535d22818);
		}
		static void _get_hair_color(int colorid, int* r, int* g, int* b) {
			invoker::invoke<void>(0x4852fc386e2e1bb5, colorid, r, g, b);
		}
		static void _get_lipstick_color(int colorid, int* r, int* g, type::any* b) {
			invoker::invoke<void>(0x013e5cfc38cd5387, colorid, r, g, b);
		}
		static bool _0xed6d8e27a43b8cde(int colorid) {
			return invoker::invoke<bool>(0xed6d8e27a43b8cde, colorid);
		}
		static int _0xea9960d07dadcf10(type::any p0) {
			return invoker::invoke<int>(0xea9960d07dadcf10, p0);
		}
		static bool _0x3e802f11fbe27674(type::any p0) {
			return invoker::invoke<bool>(0x3e802f11fbe27674, p0);
		}
		static bool _0xf41b5d290c99a3d6(type::any p0) {
			return invoker::invoke<bool>(0xf41b5d290c99a3d6, p0);
		}
		static bool _is_ped_hair_color_valid(int colorid) {
			return invoker::invoke<bool>(0xe0d36e5d9e99cc21, colorid);
		}
		static type::any _0xaaa6a3698a69e048(type::any p0) {
			return invoker::invoke<type::any>(0xaaa6a3698a69e048, p0);
		}
		static bool _is_ped_lipstick_color_valid(int colorid) {
			return invoker::invoke<bool>(0x0525a2c2562f3cd4, colorid);
		}
		static bool _is_ped_blush_color_valid(int colorid) {
			return invoker::invoke<bool>(0x604e810189ee3a59, colorid);
		}
		static type::any _0xc56fbf2f228e1dac(type::hash modelhash, type::any p1, type::any p2) {
			return invoker::invoke<type::any>(0xc56fbf2f228e1dac, modelhash, p1, p2);
		}
		static void _set_ped_face_feature(type::ped ped, int index, float scale) {
			invoker::invoke<void>(0x71a5c1dba060049e, ped, index, scale);
		}
		static bool has_ped_head_blend_finished(type::ped ped) {
			return invoker::invoke<bool>(0x654cd0a825161131, ped);
		}
		static void _0x4668d80430d6c299(type::ped ped) {
			invoker::invoke<void>(0x4668d80430d6c299, ped);
		}
		static void _0xcc9682b8951c5229(type::ped ped, int r, int g, int b, int p4) {
			invoker::invoke<void>(0xcc9682b8951c5229, ped, r, g, b, p4);
		}
		static void _0xa21c118553bbdf02(type::ped ped) {
			invoker::invoke<void>(0xa21c118553bbdf02, ped);
		}
		static int _get_first_parent_id_for_ped_type(int type) {
			return invoker::invoke<int>(0x68d353ab88b97e0c, type);
		}
		static int _get_num_parent_peds_of_type(int type) {
			return invoker::invoke<int>(0x5ef37013a6539c9d, type);
		}
		static type::any _0x39d55a620fcb6a3a(type::ped ped, int slot, int drawableid, int textureid) {
			return invoker::invoke<type::any>(0x39d55a620fcb6a3a, ped, slot, drawableid, textureid);
		}
		static bool _0x66680a92700f43df(type::ped p0) {
			return invoker::invoke<bool>(0x66680a92700f43df, p0);
		}
		static void _0x5aab586ffec0fd96(type::any p0) {
			invoker::invoke<void>(0x5aab586ffec0fd96, p0);
		}
		static bool _is_ped_prop_valid(type::ped ped, int componentid, int drawableid, int textureid) {
			return invoker::invoke<bool>(0x2b16a3bff1fbce49, ped, componentid, drawableid, textureid);
		}
		static bool _0x784002a632822099(type::ped ped) {
			return invoker::invoke<bool>(0x784002a632822099, ped);
		}
		static void _0xf79f9def0aade61a(type::ped ped) {
			invoker::invoke<void>(0xf79f9def0aade61a, ped);
		}
		static int get_ped_prop_index(type::ped ped, int componentid) {
			return invoker::invoke<int>(0x898cc20ea75bacd8, ped, componentid);
		}
		static void set_ped_prop_index(type::ped ped, int componentid, int drawableid, int textureid, bool isnetworkgame) {
			invoker::invoke<void>(0x93376b65a266eb5f, ped, componentid, drawableid, textureid, isnetworkgame);
		}
		static void knock_off_ped_prop(type::ped ped, bool p1, bool knockoff, bool p2, bool p3) {
			invoker::invoke<void>(0x6fd7816a36615f48, ped, p1, knockoff, p2, p3);
		}
		static void clear_ped_prop(type::ped ped, int propid) {
			invoker::invoke<void>(0x0943e5b8e078e76e, ped, propid);
		}
		static void clear_all_ped_props(type::ped ped) {
			invoker::invoke<void>(0xcd8a7537a9b52f06, ped);
		}
		static void _0xaff4710e2a0a6c12(type::ped ped) {
			invoker::invoke<void>(0xaff4710e2a0a6c12, ped);
		}
		static int get_ped_prop_texture_index(type::ped ped, int componentid) {
			return invoker::invoke<int>(0xe131a28626f81ab2, ped, componentid);
		}
		static void _0x1280804f7cfd2d6c(type::any p0) {
			invoker::invoke<void>(0x1280804f7cfd2d6c, p0);
		}
		static void _0x36c6984c3ed0c911(type::any p0) {
			invoker::invoke<void>(0x36c6984c3ed0c911, p0);
		}
		static void _0xb50eb4ccb29704ac(type::any p0) {
			invoker::invoke<void>(0xb50eb4ccb29704ac, p0);
		}
		static bool _0xfec9a3b1820f3331(type::any p0) {
			return invoker::invoke<bool>(0xfec9a3b1820f3331, p0);
		}
		static void set_blocking_of_non_temporary_events(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x9f8aa94d6d97dbf4, ped, toggle);
		}
		static void set_ped_bounds_orientation(type::ped ped, float p1, float p2, float p3, float p4, float p5) {
			invoker::invoke<void>(0x4f5f651accc9c4cf, ped, p1, p2, p3, p4, p5);
		}
		static void register_target(type::ped ped, type::ped target) {
			invoker::invoke<void>(0x2f25d9aefa34fba2, ped, target);
		}
		static void register_hated_targets_around_ped(type::ped ped, float radius) {
			invoker::invoke<void>(0x9222f300bf8354fe, ped, radius);
		}
		static type::ped get_random_ped_at_coord(float x, float y, float z, float xradius, float yradius, float zradius, int pedtype) {
			return invoker::invoke<type::ped>(0x876046a8e3a4b71c, x, y, z, xradius, yradius, zradius, pedtype);
		}
		static bool get_closest_ped(float x, float y, float z, float radius, bool p4, bool p5, type::ped* outped, bool p7, bool p8, int pedtype) {
			return invoker::invoke<bool>(0xc33ab876a77f8164, x, y, z, radius, p4, p5, outped, p7, p8, pedtype);
		}
		static void set_scenario_peds_to_be_returned_by_next_command(bool value) {
			invoker::invoke<void>(0x14f19a8782c8071e, value);
		}
		static bool _0x03ea03af85a85cb7(type::ped ped, bool p1, bool p2, bool p3, bool p4, bool p5, bool p6, bool p7, type::any p8) {
			return invoker::invoke<bool>(0x03ea03af85a85cb7, ped, p1, p2, p3, p4, p5, p6, p7, p8);
		}
		static void set_driver_racing_modifier(type::ped driver, float racingmodifier) {
			invoker::invoke<void>(0xded5af5a0ea4b297, driver, racingmodifier);
		}
		static void set_driver_ability(type::ped driver, float ability) {
			invoker::invoke<void>(0xb195ffa8042fc5c3, driver, ability);
		}
		static void set_driver_aggressiveness(type::ped driver, float aggressiveness) {
			invoker::invoke<void>(0xa731f608ca104e3c, driver, aggressiveness);
		}
		static bool can_ped_ragdoll(type::ped ped) {
			return invoker::invoke<bool>(0x128f79edcece4fd5, ped);
		}
		static bool set_ped_to_ragdoll(type::ped ped, int time1, int time2, int ragdolltype, bool p4, bool p5, bool p6) {
			return invoker::invoke<bool>(0xae99fb955581844a, ped, time1, time2, ragdolltype, p4, p5, p6);
		}
		static bool set_ped_to_ragdoll_with_fall(type::ped ped, int time, int p2, int ragdolltype, float x, float y, float z, float p7, float p8, float p9, float p10, float p11, float p12, float p13) {
			return invoker::invoke<bool>(0xd76632d99e4966c8, ped, time, p2, ragdolltype, x, y, z, p7, p8, p9, p10, p11, p12, p13);
		}
		static void set_ped_ragdoll_on_collision(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xf0a4f1bbf4fa7497, ped, toggle);
		}
		static bool is_ped_ragdoll(type::ped ped) {
			return invoker::invoke<bool>(0x47e4e977581c5b55, ped);
		}
		static bool is_ped_running_ragdoll_task(type::ped ped) {
			return invoker::invoke<bool>(0xe3b6097cc25aa69e, ped);
		}
		static void set_ped_ragdoll_force_fall(type::ped ped) {
			invoker::invoke<void>(0x01f6594b923b9251, ped);
		}
		static void reset_ped_ragdoll_timer(type::ped ped) {
			invoker::invoke<void>(0x9fa4664cf62e47e8, ped);
		}
		static void set_ped_can_ragdoll(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xb128377056a54e2a, ped, toggle);
		}
		static bool _0xd1871251f3b5acd7(type::ped ped) {
			return invoker::invoke<bool>(0xd1871251f3b5acd7, ped);
		}
		static bool is_ped_running_mobile_phone_task(type::ped ped) {
			return invoker::invoke<bool>(0x2afe52f782f25775, ped);
		}
		static bool _0xa3f3564a5b3646c0(type::ped ped) {
			return invoker::invoke<bool>(0xa3f3564a5b3646c0, ped);
		}
		static void _set_ped_ragdoll_blocking_flags(type::ped ped, int flags) {
			invoker::invoke<void>(0x26695ec767728d84, ped, flags);
		}
		static void _reset_ped_ragdoll_blocking_flags(type::ped ped, int flags) {
			invoker::invoke<void>(0xd86d101fcfd00a4b, ped, flags);
		}
		static void set_ped_angled_defensive_area(type::ped ped, float p1, float p2, float p3, float p4, float p5, float p6, float p7, bool p8, bool p9) {
			invoker::invoke<void>(0xc7f76df27a5045a1, ped, p1, p2, p3, p4, p5, p6, p7, p8, p9);
		}
		static void set_ped_sphere_defensive_area(type::ped ped, float x, float y, float z, float radius, bool p5, bool p6) {
			invoker::invoke<void>(0x9d3151a373974804, ped, x, y, z, radius, p5, p6);
		}
		static void set_ped_defensive_sphere_attached_to_ped(type::ped ped, type::ped target, float xoffset, float yoffset, float zoffset, float radius, bool p6) {
			invoker::invoke<void>(0xf9b8f91aad3b953e, ped, target, xoffset, yoffset, zoffset, radius, p6);
		}
		static void _0xe4723db6e736ccff(type::ped ped, type::any p1, float p2, float p3, float p4, float p5, bool p6) {
			invoker::invoke<void>(0xe4723db6e736ccff, ped, p1, p2, p3, p4, p5, p6);
		}
		static void set_ped_defensive_area_attached_to_ped(type::ped ped, type::ped attachped, float p2, float p3, float p4, float p5, float p6, float p7, float p8, bool p9, bool p10) {
			invoker::invoke<void>(0x4ef47fe21698a8b6, ped, attachped, p2, p3, p4, p5, p6, p7, p8, p9, p10);
		}
		static void set_ped_defensive_area_direction(type::ped ped, float p1, float p2, float p3, bool p4) {
			invoker::invoke<void>(0x413c6c763a4affad, ped, p1, p2, p3, p4);
		}
		static void remove_ped_defensive_area(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x74d4e028107450a9, ped, toggle);
		}
		static PVector3 get_ped_defensive_area_position(type::ped ped, bool p1) {
			return invoker::invoke<PVector3>(0x3c06b8786dd94cd1, ped, p1);
		}
		static bool _0xba63d9fe45412247(type::ped ped, bool p1) {
			return invoker::invoke<bool>(0xba63d9fe45412247, ped, p1);
		}
		static void set_ped_preferred_cover_set(type::ped ped, type::any itemset) {
			invoker::invoke<void>(0x8421eb4da7e391b9, ped, itemset);
		}
		static void remove_ped_preferred_cover_set(type::ped ped) {
			invoker::invoke<void>(0xfddb234cf74073d9, ped);
		}
		static void revive_injured_ped(type::ped ped) {
			invoker::invoke<void>(0x8d8acd8388cd99ce, ped);
		}
		static void resurrect_ped(type::ped ped) {
			invoker::invoke<void>(0x71bc8e838b9c6035, ped);
		}
		static void set_ped_name_debug(type::ped ped, const char* name) {
			invoker::invoke<void>(0x98efa132a4117be1, ped, name);
		}
		static PVector3 get_ped_extracted_displacement(type::ped ped, bool worldspace) {
			return invoker::invoke<PVector3>(0xe0af41401adf87e3, ped, worldspace);
		}
		static void set_ped_dies_when_injured(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x5ba7919bed300023, ped, toggle);
		}
		static void set_ped_enable_weapon_blocking(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x97a790315d3831fd, ped, toggle);
		}
		static void _0xf9acf4a08098ea25(type::ped ped, bool p1) {
			invoker::invoke<void>(0xf9acf4a08098ea25, ped, p1);
		}
		static void reset_ped_visible_damage(type::ped ped) {
			invoker::invoke<void>(0x3ac1f7b898f30c05, ped);
		}
		static void apply_ped_blood_damage_by_zone(type::ped ped, type::any p1, float p2, float p3, type::any p4) {
			invoker::invoke<void>(0x816f6981c60bf53b, ped, p1, p2, p3, p4);
		}
		static void apply_ped_blood(type::ped ped, int boneindex, float xrot, float yrot, float zrot, const char* woundtype) {
			invoker::invoke<void>(0x83f7e01c7b769a26, ped, boneindex, xrot, yrot, zrot, woundtype);
		}
		static void apply_ped_blood_by_zone(type::ped ped, type::any p1, float p2, float p3, type::any* p4) {
			invoker::invoke<void>(0x3311e47b91edcbbc, ped, p1, p2, p3, p4);
		}
		static void apply_ped_blood_specific(type::ped ped, type::any p1, float xoffset, float yoffset, float zoffset, float scale, type::any p6, float p7, const char* bloodtype) {
			invoker::invoke<void>(0xef0d582cbf2d9b0f, ped, p1, xoffset, yoffset, zoffset, scale, p6, p7, bloodtype);
		}
		static void apply_ped_damage_decal(type::ped ped, int damagezone, float xoffset, float yoffset, float zoffset, float scale, float alpha, int variation, bool fadein, const char* decalname) {
			invoker::invoke<void>(0x397c38aa7b4a5f83, ped, damagezone, xoffset, yoffset, zoffset, scale, alpha, variation, fadein, decalname);
		}
		static void apply_ped_damage_pack(type::ped ped, const char* damagepack, float damage, float mult) {
			invoker::invoke<void>(0x46df918788cb093f, ped, damagepack, damage, mult);
		}
		static void clear_ped_blood_damage(type::ped ped) {
			invoker::invoke<void>(0x8fe22675a5a45817, ped);
		}
		static void clear_ped_blood_damage_by_zone(type::ped ped, int p1) {
			invoker::invoke<void>(0x56e3b78c5408d9f4, ped, p1);
		}
		static void hide_ped_blood_damage_by_zone(type::ped ped, type::any p1, bool p2) {
			invoker::invoke<void>(0x62ab793144de75dc, ped, p1, p2);
		}
		static void clear_ped_damage_decal_by_zone(type::ped ped, int p1, const char* p2) {
			invoker::invoke<void>(0x523c79aeefcc4a2a, ped, p1, p2);
		}
		static bool get_ped_decorations_state(type::ped ped) {
			return invoker::invoke<bool>(0x71eab450d86954a1, ped);
		}
		static void _0x2b694afcf64e6994(type::ped ped, bool p1) {
			invoker::invoke<void>(0x2b694afcf64e6994, ped, p1);
		}
		static void clear_ped_wetness(type::ped ped) {
			invoker::invoke<void>(0x9c720776daa43e7e, ped);
		}
		static void set_ped_wetness_height(type::ped ped, float height) {
			invoker::invoke<void>(0x44cb6447d2571aa0, ped, height);
		}
		static void set_ped_wetness_enabled_this_frame(type::ped ped) {
			invoker::invoke<void>(0xb5485e4907b53019, ped);
		}
		static void _0x6585d955a68452a5(type::ped ped) {
			invoker::invoke<void>(0x6585d955a68452a5, ped);
		}
		static void set_ped_sweat(type::ped ped, float sweat) {
			invoker::invoke<void>(0x27b0405f59637d1f, ped, sweat);
		}
		static void _set_ped_decoration(type::ped ped, type::hash collection, type::hash overlay) {
			invoker::invoke<void>(0x5f5d1665e352a839, ped, collection, overlay);
		}
		static void _set_ped_facial_decoration(type::ped ped, type::hash collection, type::hash overlay) {
			invoker::invoke<void>(0x5619bfa07cfd7833, ped, collection, overlay);
		}
		static int _get_tattoo_zone(type::hash collection, type::hash overlay) {
			return invoker::invoke<int>(0x9fd452bfbe7a7a8b, collection, overlay);
		}
		static void clear_ped_decorations(type::ped ped) {
			invoker::invoke<void>(0x0e5173c163976e38, ped);
		}
		static void _clear_ped_facial_decorations(type::ped ped) {
			invoker::invoke<void>(0xe3b27e70ceab9f0c, ped);
		}
		static bool was_ped_skeleton_updated(type::ped ped) {
			return invoker::invoke<bool>(0x11b499c1e0ff8559, ped);
		}
		static PVector3 get_ped_bone_coords(type::ped ped, int boneid, float offsetx, float offsety, float offsetz) {
			return invoker::invoke<PVector3>(0x17c07fc640e86b4e, ped, boneid, offsetx, offsety, offsetz);
		}
		static void create_nm_message(bool startimmediately, int messageid) {
			invoker::invoke<void>(0x418ef2a1bce56685, startimmediately, messageid);
		}
		static void give_ped_nm_message(type::ped ped) {
			invoker::invoke<void>(0xb158dfccc56e5c5b, ped);
		}
		static int add_scenario_blocking_area(float x1, float y1, float z1, float x2, float y2, float z2, bool p6, bool p7, bool p8, bool p9) {
			return invoker::invoke<int>(0x1b5c85c612e5256e, x1, y1, z1, x2, y2, z2, p6, p7, p8, p9);
		}
		static void remove_scenario_blocking_areas() {
			invoker::invoke<void>(0xd37401d78a929a49);
		}
		static void remove_scenario_blocking_area(int areahandle, bool p1) {
			invoker::invoke<void>(0x31d16b74c6e29d66, areahandle, p1);
		}
		static void set_scenario_peds_spawn_in_sphere_area(float x, float y, float z, float range, int p4) {
			invoker::invoke<void>(0x28157d43cf600981, x, y, z, range, p4);
		}
		static bool is_ped_using_scenario(type::ped ped, const char* scenario) {
			return invoker::invoke<bool>(0x1bf094736dd62c2e, ped, scenario);
		}
		static bool is_ped_using_any_scenario(type::ped ped) {
			return invoker::invoke<bool>(0x57ab4a3080f85143, ped);
		}
		static type::any _0xfe07ff6495d52e2a(type::any p0, type::any p1, type::any p2, type::any p3) {
			return invoker::invoke<type::any>(0xfe07ff6495d52e2a, p0, p1, p2, p3);
		}
		static void _0x9a77dfd295e29b09(type::any p0, bool p1) {
			invoker::invoke<void>(0x9a77dfd295e29b09, p0, p1);
		}
		static type::any _0x25361a96e0f7e419(type::any p0, type::any p1, type::any p2, type::any p3) {
			return invoker::invoke<type::any>(0x25361a96e0f7e419, p0, p1, p2, p3);
		}
		static type::any _0xec6935ebe0847b90(type::any p0, type::any p1, type::any p2, type::any p3) {
			return invoker::invoke<type::any>(0xec6935ebe0847b90, p0, p1, p2, p3);
		}
		static void _0xa3a9299c4f2adb98(type::any p0) {
			invoker::invoke<void>(0xa3a9299c4f2adb98, p0);
		}
		static void _0xf1c03a5352243a30(type::any p0) {
			invoker::invoke<void>(0xf1c03a5352243a30, p0);
		}
		static type::any _0xeeed8fafec331a70(type::any p0, type::any p1, type::any p2, type::any p3) {
			return invoker::invoke<type::any>(0xeeed8fafec331a70, p0, p1, p2, p3);
		}
		static void _0x425aecf167663f48(type::ped ped, bool p1) {
			invoker::invoke<void>(0x425aecf167663f48, ped, p1);
		}
		static void _0x5b6010b3cbc29095(type::any p0, bool p1) {
			invoker::invoke<void>(0x5b6010b3cbc29095, p0, p1);
		}
		static void _0xceda60a74219d064(type::any p0, bool p1) {
			invoker::invoke<void>(0xceda60a74219d064, p0, p1);
		}
		static void play_facial_anim(type::ped ped, const char* animname, const char* animdict) {
			invoker::invoke<void>(0xe1e65ca8ac9c00ed, ped, animname, animdict);
		}
		static void set_facial_idle_anim_override(type::ped ped, const char* animname, const char* animdict) {
			invoker::invoke<void>(0xffc24b988b938b38, ped, animname, animdict);
		}
		static void clear_facial_idle_anim_override(type::ped ped) {
			invoker::invoke<void>(0x726256cc1eeb182f, ped);
		}
		static void set_ped_can_play_gesture_anims(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xbaf20c5432058024, ped, toggle);
		}
		static void set_ped_can_play_viseme_anims(type::ped ped, bool toggle, bool p2) {
			invoker::invoke<void>(0xf833ddba3b104d43, ped, toggle, p2);
		}
		static void _set_ped_can_play_injured_anims(type::ped ped, bool p1) {
			invoker::invoke<void>(0x33a60d8bdd6e508c, ped, p1);
		}
		static void set_ped_can_play_ambient_anims(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x6373d1349925a70e, ped, toggle);
		}
		static void set_ped_can_play_ambient_base_anims(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x0eb0585d15254740, ped, toggle);
		}
		static void _0xc2ee020f5fb4db53(type::ped ped) {
			invoker::invoke<void>(0xc2ee020f5fb4db53, ped);
		}
		static void set_ped_can_arm_ik(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x6c3b4d6d13b4c841, ped, toggle);
		}
		static void set_ped_can_head_ik(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xc11c18092c5530dc, ped, toggle);
		}
		static void set_ped_can_leg_ik(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x73518ece2485412b, ped, toggle);
		}
		static void set_ped_can_torso_ik(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xf2b7106d37947ce0, ped, toggle);
		}
		static void _0xf5846edb26a98a24(type::ped ped, bool p1) {
			invoker::invoke<void>(0xf5846edb26a98a24, ped, p1);
		}
		static void _0x6647c5f6f5792496(type::ped ped, bool p1) {
			invoker::invoke<void>(0x6647c5f6f5792496, ped, p1);
		}
		static void set_ped_can_use_auto_conversation_lookat(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xec4686ec06434678, ped, toggle);
		}
		static bool is_ped_headtracking_ped(type::ped ped1, type::ped ped2) {
			return invoker::invoke<bool>(0x5cd3cb88a7f8850d, ped1, ped2);
		}
		static bool is_ped_headtracking_entity(type::ped ped, type::entity entity) {
			return invoker::invoke<bool>(0x813a0a7c9d2e831f, ped, entity);
		}
		static void set_ped_primary_lookat(type::ped ped, type::ped lookat) {
			invoker::invoke<void>(0xcd17b554996a8d9e, ped, lookat);
		}
		static void _0x78c4e9961db3eb5b(type::any p0, type::any p1) {
			invoker::invoke<void>(0x78c4e9961db3eb5b, p0, p1);
		}
		static void set_ped_cloth_prone(type::any p0, type::any p1) {
			invoker::invoke<void>(0x82a3d6d9cc2cb8e3, p0, p1);
		}
		static void _0xa660faf550eb37e5(type::any p0, bool p1) {
			invoker::invoke<void>(0xa660faf550eb37e5, p0, p1);
		}
		static void set_ped_config_flag(type::ped ped, int flagid, bool value) {
			invoker::invoke<void>(0x1913fe4cbf41c463, ped, flagid, value);
		}
		static void set_ped_reset_flag(type::ped ped, int flagid, bool value) {
			invoker::invoke<void>(0xc1e8a365bf3b29f2, ped, flagid, value);
		}
		static bool get_ped_config_flag(type::ped ped, int flagid, bool p2) {
			return invoker::invoke<bool>(0x7ee53118c892b513, ped, flagid, p2);
		}
		static bool get_ped_reset_flag(type::ped ped, int flagid) {
			return invoker::invoke<bool>(0xaf9e59b1b1fbf2a0, ped, flagid);
		}
		static void set_ped_group_member_passenger_index(type::ped ped, int index) {
			invoker::invoke<void>(0x0bddb8d9ec6bcf3c, ped, index);
		}
		static void set_ped_can_evasive_dive(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x6b7a646c242a7059, ped, toggle);
		}
		static bool is_ped_evasive_diving(type::ped ped, type::entity* evadingentity) {
			return invoker::invoke<bool>(0x414641c26e105898, ped, evadingentity);
		}
		static void set_ped_shoots_at_coord(type::ped ped, float x, float y, float z, bool toggle) {
			invoker::invoke<void>(0x96a05e4fb321b1ba, ped, x, y, z, toggle);
		}
		static void set_ped_model_is_suppressed(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xe163a4bce4de6f11, ped, toggle);
		}
		static void stop_any_ped_model_being_suppressed() {
			invoker::invoke<void>(0xb47bd05fa66b40cf);
		}
		static void set_ped_can_be_targeted_when_injured(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x638c03b0f9878f57, ped, toggle);
		}
		static void set_ped_generates_dead_body_events(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x7fb17ba2e7deca5b, ped, toggle);
		}
		static void _0xe43a13c9e4cccbcf(type::ped ped, bool p1) {
			invoker::invoke<void>(0xe43a13c9e4cccbcf, ped, p1);
		}
		static void set_ped_can_ragdoll_from_player_impact(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xdf993ee5e90aba25, ped, toggle);
		}
		static void give_ped_helmet(type::ped ped, bool cannotremove, int helmetflag, int textureindex) {
			invoker::invoke<void>(0x54c7c4a94367717e, ped, cannotremove, helmetflag, textureindex);
		}
		static void remove_ped_helmet(type::ped ped, bool instantly) {
			invoker::invoke<void>(0xa7b2458d0ad6ded8, ped, instantly);
		}
		static bool _0x14590ddbedb1ec85(type::ped ped) {
			return invoker::invoke<bool>(0x14590ddbedb1ec85, ped);
		}
		static void set_ped_helmet(type::ped ped, bool canwearhelmet) {
			invoker::invoke<void>(0x560a43136eb58105, ped, canwearhelmet);
		}
		static void set_ped_helmet_flag(type::ped ped, int helmetflag) {
			invoker::invoke<void>(0xc0e78d5c2ce3eb25, ped, helmetflag);
		}
		static void set_ped_helmet_prop_index(type::ped ped, int propindex) {
			invoker::invoke<void>(0x26d83693ed99291c, ped, propindex);
		}
		static void set_ped_helmet_texture_index(type::ped ped, int textureindex) {
			invoker::invoke<void>(0xf1550c4bd22582e2, ped, textureindex);
		}
		static bool is_ped_wearing_helmet(type::ped ped) {
			return invoker::invoke<bool>(0xf33bdfe19b309b19, ped);
		}
		static void _0x687c0b594907d2e8(type::ped ped) {
			invoker::invoke<void>(0x687c0b594907d2e8, ped);
		}
		static bool _0x451294e859ecc018(int p0) {
			return invoker::invoke<bool>(0x451294e859ecc018, p0);
		}
		static type::any _0x9d728c1e12bf5518(type::any p0) {
			return invoker::invoke<type::any>(0x9d728c1e12bf5518, p0);
		}
		static bool _0xf2385935bffd4d92(type::any p0) {
			return invoker::invoke<bool>(0xf2385935bffd4d92, p0);
		}
		static void set_ped_to_load_cover(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x332b562eeda62399, ped, toggle);
		}
		static void set_ped_can_cower_in_cover(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xcb7553cdcef4a735, ped, toggle);
		}
		static void set_ped_can_peek_in_cover(type::ped player, bool aim) {
			invoker::invoke<void>(0xc514825c507e3736, player, aim);
		}
		static void set_ped_plays_head_on_horn_anim_when_dies_in_vehicle(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x94d94bf1a75aed3d, ped, toggle);
		}
		static void set_ped_leg_ik_mode(type::ped ped, int mode) {
			invoker::invoke<void>(0xc396f5b86ff9febd, ped, mode);
		}
		static void set_ped_motion_blur(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x0a986918b102b448, ped, toggle);
		}
		static void set_ped_can_switch_weapon(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xed7f7efe9fabf340, ped, toggle);
		}
		static void set_ped_dies_instantly_in_water(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xeeb64139ba29a7cf, ped, toggle);
		}
		static void _0x1a330d297aac6bc1(type::ped ped, int p1) {
			invoker::invoke<void>(0x1a330d297aac6bc1, ped, p1);
		}
		static void stop_ped_weapon_firing_when_dropped(type::ped ped) {
			invoker::invoke<void>(0xc158d28142a34608, ped);
		}
		static void set_scripted_anim_seat_offset(type::ped ped, float p1) {
			invoker::invoke<void>(0x5917bba32d06c230, ped, p1);
		}
		static void set_ped_combat_movement(type::ped ped, int combatmovement) {
			invoker::invoke<void>(0x4d9ca1009afbd057, ped, combatmovement);
		}
		static int get_ped_combat_movement(type::ped ped) {
			return invoker::invoke<int>(0xdea92412fcaeb3f5, ped);
		}
		static void set_ped_combat_ability(type::ped ped, int p1) {
			invoker::invoke<void>(0xc7622c0d36b2fda8, ped, p1);
		}
		static void set_ped_combat_range(type::ped ped, int p1) {
			invoker::invoke<void>(0x3c606747b23e497b, ped, p1);
		}
		static type::any get_ped_combat_range(type::ped ped) {
			return invoker::invoke<type::any>(0xf9d9f7f2db8e2fa0, ped);
		}
		static void set_ped_combat_attributes(type::ped ped, int attributeindex, bool enabled) {
			invoker::invoke<void>(0x9f7794730795e019, ped, attributeindex, enabled);
		}
		static void set_ped_target_loss_response(type::ped ped, int responsetype) {
			invoker::invoke<void>(0x0703b9079823da4a, ped, responsetype);
		}
		static bool _0xdcca191df9980fd7(type::ped ped) {
			return invoker::invoke<bool>(0xdcca191df9980fd7, ped);
		}
		static bool is_ped_performing_stealth_kill(type::ped ped) {
			return invoker::invoke<bool>(0xfd4ccdbcc59941b7, ped);
		}
		static bool _0xebd0edba5be957cf(type::ped ped) {
			return invoker::invoke<bool>(0xebd0edba5be957cf, ped);
		}
		static bool is_ped_being_stealth_killed(type::ped ped) {
			return invoker::invoke<bool>(0x863b23efde9c5df2, ped);
		}
		static type::ped get_melee_target_for_ped(type::ped ped) {
			return invoker::invoke<type::ped>(0x18a3e9ee1297fd39, ped);
		}
		static bool was_ped_killed_by_stealth(type::ped ped) {
			return invoker::invoke<bool>(0xf9800aa1a771b000, ped);
		}
		static bool was_ped_killed_by_takedown(type::ped ped) {
			return invoker::invoke<bool>(0x7f08e26039c7347c, ped);
		}
		static bool _0x61767f73eaceed21(type::ped ped) {
			return invoker::invoke<bool>(0x61767f73eaceed21, ped);
		}
		static void set_ped_flee_attributes(type::ped ped, int attributes, bool p2) {
			invoker::invoke<void>(0x70a2d1137c8ed7c9, ped, attributes, p2);
		}
		static void set_ped_cower_hash(type::ped ped, const char* p1) {
			invoker::invoke<void>(0xa549131166868ed3, ped, p1);
		}
		static void _0x2016c603d6b8987c(type::any p0, bool p1) {
			invoker::invoke<void>(0x2016c603d6b8987c, p0, p1);
		}
		static void set_ped_steers_around_peds(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x46f2193b3ad1d891, ped, toggle);
		}
		static void set_ped_steers_around_objects(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x1509c089adc208bf, ped, toggle);
		}
		static void set_ped_steers_around_vehicles(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xeb6fb9d48dde23ec, ped, toggle);
		}
		static void _0xa9b61a329bfdcbea(type::any p0, bool p1) {
			invoker::invoke<void>(0xa9b61a329bfdcbea, p0, p1);
		}
		static void _0x570389d1c3de3c6b(type::any p0) {
			invoker::invoke<void>(0x570389d1c3de3c6b, p0);
		}
		static void _0x576594e8d64375e2(type::any p0, bool p1) {
			invoker::invoke<void>(0x576594e8d64375e2, p0, p1);
		}
		static void _0xa52d5247a4227e14(type::any p0) {
			invoker::invoke<void>(0xa52d5247a4227e14, p0);
		}
		static bool is_any_ped_near_point(float x, float y, float z, float radius) {
			return invoker::invoke<bool>(0x083961498679dc9f, x, y, z, radius);
		}
		static void _0x2208438012482a1a(type::ped ped, bool p1, bool p2) {
			invoker::invoke<void>(0x2208438012482a1a, ped, p1, p2);
		}
		static bool _0xfcf37a457cb96dc0(type::any p0, float p1, float p2, float p3, float p4) {
			return invoker::invoke<bool>(0xfcf37a457cb96dc0, p0, p1, p2, p3, p4);
		}
		static void _track_ped_visibility(type::ped ped) {
			invoker::invoke<void>(0x7d7a2e43e74e2eb8, ped);
		}
		static void get_ped_flood_invincibility(type::ped ped, bool p1) {
			invoker::invoke<void>(0x2bc338a7b21f4608, ped, p1);
		}
		static void _0xcd018c591f94cb43(type::any p0, bool p1) {
			invoker::invoke<void>(0xcd018c591f94cb43, p0, p1);
		}
		static void _0x75ba1cb3b7d40caf(type::ped ped, bool p1) {
			invoker::invoke<void>(0x75ba1cb3b7d40caf, ped, p1);
		}
		static bool is_tracked_ped_visible(type::ped ped) {
			return invoker::invoke<bool>(0x91c8e617f64188ac, ped);
		}
		static type::any _0x511f1a683387c7e2(type::any p0) {
			return invoker::invoke<type::any>(0x511f1a683387c7e2, p0);
		}
		static bool is_ped_tracked(type::ped ped) {
			return invoker::invoke<bool>(0x4c5e1f087cd10bb7, ped);
		}
		static bool has_ped_received_event(type::ped ped, type::any p1) {
			return invoker::invoke<bool>(0x8507bcb710fa6dc0, ped, p1);
		}
		static bool _can_ped_see_ped(type::ped ped1, type::ped ped2) {
			return invoker::invoke<bool>(0x6cd5a433374d4cfb, ped1, ped2);
		}
		static bool _0x9c6a6c19b6c0c496(type::ped p0, type::any* p1) {
			return invoker::invoke<bool>(0x9c6a6c19b6c0c496, p0, p1);
		}
		static int get_ped_bone_index(type::ped ped, int boneid) {
			return invoker::invoke<int>(0x3f428d08be5aae31, ped, boneid);
		}
		static int get_ped_ragdoll_bone_index(type::ped ped, int bone) {
			return invoker::invoke<int>(0x2057ef813397a772, ped, bone);
		}
		static void set_ped_enveff_scale(type::ped ped, float value) {
			invoker::invoke<void>(0xbf29516833893561, ped, value);
		}
		static float get_ped_enveff_scale(type::ped ped) {
			return invoker::invoke<float>(0x9c14d30395a51a3c, ped);
		}
		static void set_enable_ped_enveff_scale(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xd2c5aa0c0e8d0f1e, ped, toggle);
		}
		static void _0x110f526ab784111f(type::ped ped, float p1) {
			invoker::invoke<void>(0x110f526ab784111f, ped, p1);
		}
		static void _0xd69411aa0cebf9e9(type::ped ped, int p1, int p2, int p3) {
			invoker::invoke<void>(0xd69411aa0cebf9e9, ped, p1, p2, p3);
		}
		static void _0x1216e0bfa72cc703(type::any p0, type::any p1) {
			invoker::invoke<void>(0x1216e0bfa72cc703, p0, p1);
		}
		static void _0x2b5aa717a181fb4c(type::ped ped, bool p1) {
			invoker::invoke<void>(0x2b5aa717a181fb4c, ped, p1);
		}
		static bool _0xb8b52e498014f5b0(type::ped ped) {
			return invoker::invoke<bool>(0xb8b52e498014f5b0, ped);
		}
		static int create_synchronized_scene(float posx, float posy, float posz, float roll, float pitch, float yaw, int p6) {
			return invoker::invoke<int>(0x8c18e0f9080add73, posx, posy, posz, roll, pitch, yaw, p6);
		}
		static int _create_synchronized_scene_2(float x, float y, float z, float radius, type::hash object) {
			return invoker::invoke<int>(0x62ec273d00187dca, x, y, z, radius, object);
		}
		static bool is_synchronized_scene_running(int sceneid) {
			return invoker::invoke<bool>(0x25d39b935a038a26, sceneid);
		}
		static void set_synchronized_scene_origin(int sceneid, float x, float y, float z, float roll, float pitch, float yaw, int unk) {
			invoker::invoke<void>(0x6acf6b7225801cd7, sceneid, x, y, z, roll, pitch, yaw, unk);
		}
		static void set_synchronized_scene_phase(int sceneid, float phase) {
			invoker::invoke<void>(0x734292f4f0abf6d0, sceneid, phase);
		}
		static float get_synchronized_scene_phase(int sceneid) {
			return invoker::invoke<float>(0xe4a310b1d7fa73cc, sceneid);
		}
		static void set_synchronized_scene_rate(int sceneid, float rate) {
			invoker::invoke<void>(0xb6c49f8a5e295a5d, sceneid, rate);
		}
		static float get_synchronized_scene_rate(int sceneid) {
			return invoker::invoke<float>(0xd80932d577274d40, sceneid);
		}
		static void set_synchronized_scene_looped(int sceneid, bool toggle) {
			invoker::invoke<void>(0xd9a897a4c6c2974f, sceneid, toggle);
		}
		static bool is_synchronized_scene_looped(int sceneid) {
			return invoker::invoke<bool>(0x62522002e0c391ba, sceneid);
		}
		static void _set_synchronized_scene_occlusion_portal(int sceneid, bool p1) {
			invoker::invoke<void>(0x394b9cd12435c981, sceneid, p1);
		}
		static bool _0x7f2f4f13ac5257ef(int sceneid) {
			return invoker::invoke<bool>(0x7f2f4f13ac5257ef, sceneid);
		}
		static void attach_synchronized_scene_to_entity(int sceneid, type::entity entity, int boneindex) {
			invoker::invoke<void>(0x272e4723b56a3b96, sceneid, entity, boneindex);
		}
		static void detach_synchronized_scene(int sceneid) {
			invoker::invoke<void>(0x6d38f1f04cbb37ea, sceneid);
		}
		static void _dispose_synchronized_scene(int scene) {
			invoker::invoke<void>(0xcd9cc7e200a52a6f, scene);
		}
		static bool force_ped_motion_state(type::ped ped, type::hash motionstatehash, bool p2, bool p3, bool p4) {
			return invoker::invoke<bool>(0xf28965d04f570dca, ped, motionstatehash, p2, p3, p4);
		}
		static bool _0xf60165e1d2c5370b(type::ped ped, type::any* p1, type::any* p2) {
			return invoker::invoke<bool>(0xf60165e1d2c5370b, ped, p1, p2);
		}
		static void set_ped_max_move_blend_ratio(type::ped ped, float value) {
			invoker::invoke<void>(0x433083750c5e064a, ped, value);
		}
		static void set_ped_min_move_blend_ratio(type::ped ped, float value) {
			invoker::invoke<void>(0x01a898d26e2333dd, ped, value);
		}
		static void set_ped_move_rate_override(type::ped ped, float value) {
			invoker::invoke<void>(0x085bf80fa50a39d1, ped, value);
		}
		static bool _0x46b05bcae43856b0(type::ped ped, int flag) {
			return invoker::invoke<bool>(0x46b05bcae43856b0, ped, flag);
		}
		static int get_ped_nearby_vehicles(type::ped ped, int* sizeandvehs) {
			return invoker::invoke<int>(0xcff869cbfa210d82, ped, sizeandvehs);
		}
		static int get_ped_nearby_peds(type::ped ped, int* sizeandpeds, int ignore) {
			return invoker::invoke<int>(0x23f8f5fc7e8c4a6b, ped, sizeandpeds, ignore);
		}
		static bool _0x7350823473013c02(type::ped ped) {
			return invoker::invoke<bool>(0x7350823473013c02, ped);
		}
		static bool is_ped_using_action_mode(type::ped ped) {
			return invoker::invoke<bool>(0x00e73468d085f745, ped);
		}
		static void set_ped_using_action_mode(type::ped ped, bool p1, type::any p2, const char* action) {
			invoker::invoke<void>(0xd75accf5e0fb5367, ped, p1, p2, action);
		}
		static void _0x781de8fa214e87d2(type::ped ped, const char* p1) {
			invoker::invoke<void>(0x781de8fa214e87d2, ped, p1);
		}
		static void set_ped_capsule(type::ped ped, float value) {
			invoker::invoke<void>(0x364df566ec833de2, ped, value);
		}
		static int register_pedheadshot(type::ped ped) {
			return invoker::invoke<int>(0x4462658788425076, ped);
		}
		static type::any get_quadbike_display_variations(type::any p0) {
			return invoker::invoke<type::any>(0x953563ce563143af, p0);
		}
		static void unregister_pedheadshot(int handle) {
			invoker::invoke<void>(0x96b1361d9b24c2ff, handle);
		}
		static bool is_pedheadshot_valid(int handle) {
			return invoker::invoke<bool>(0xa0a9668f158129a2, handle);
		}
		static bool is_pedheadshot_ready(int handle) {
			return invoker::invoke<bool>(0x7085228842b13a67, handle);
		}
		static const char* get_pedheadshot_txd_string(int handle) {
			return invoker::invoke<const char*>(0xdb4eacd4ad0a5d6b, handle);
		}
		static bool _0xf0daef2f545bee25(int headshothandle) {
			return invoker::invoke<bool>(0xf0daef2f545bee25, headshothandle);
		}
		static void _0x5d517b27cf6ecd04(type::any p0) {
			invoker::invoke<void>(0x5d517b27cf6ecd04, p0);
		}
		static type::any _0xebb376779a760aa8() {
			return invoker::invoke<type::any>(0xebb376779a760aa8);
		}
		static type::any _0x876928dddfccc9cd() {
			return invoker::invoke<type::any>(0x876928dddfccc9cd);
		}
		static type::any _0xe8a169e666cbc541() {
			return invoker::invoke<type::any>(0xe8a169e666cbc541);
		}
		static void _0xc1f6ebf9a3d55538(type::any p0, type::any p1) {
			invoker::invoke<void>(0xc1f6ebf9a3d55538, p0, p1);
		}
		static void _0x600048c60d5c2c51(type::any p0) {
			invoker::invoke<void>(0x600048c60d5c2c51, p0);
		}
		static void _0x2df9038c90ad5264(float p0, float p1, float p2, float p3, float p4, int interiorflags, float scale, int duration) {
			invoker::invoke<void>(0x2df9038c90ad5264, p0, p1, p2, p3, p4, interiorflags, scale, duration);
		}
		static void _0xb2aff10216defa2f(float x, float y, float z, float p3, float p4, float p5, float p6, int interiorflags, float scale, int duration) {
			invoker::invoke<void>(0xb2aff10216defa2f, x, y, z, p3, p4, p5, p6, interiorflags, scale, duration);
		}
		static void _0xfee4a5459472a9f8() {
			invoker::invoke<void>(0xfee4a5459472a9f8);
		}
		static type::any _0x3c67506996001f5e() {
			return invoker::invoke<type::any>(0x3c67506996001f5e);
		}
		static type::any _0xa586fbeb32a53dbb() {
			return invoker::invoke<type::any>(0xa586fbeb32a53dbb);
		}
		static type::any _0xf445de8da80a1792() {
			return invoker::invoke<type::any>(0xf445de8da80a1792);
		}
		static type::any _0xa635c11b8c44afc2() {
			return invoker::invoke<type::any>(0xa635c11b8c44afc2);
		}
		static void _0x280c7e3ac7f56e90(type::any p0, type::any* p1, type::any* p2, type::any* p3) {
			invoker::invoke<void>(0x280c7e3ac7f56e90, p0, p1, p2, p3);
		}
		static void _0xb782f8238512bad5(type::any p0, type::any* p1) {
			invoker::invoke<void>(0xb782f8238512bad5, p0, p1);
		}
		static void set_ik_target(type::ped ped, int ikindex, type::entity entitylookat, int bonelookat, float offsetx, float offsety, float offsetz, type::any p7, int blendinduration, int blendoutduration) {
			invoker::invoke<void>(0xc32779c16fceecd9, ped, ikindex, entitylookat, bonelookat, offsetx, offsety, offsetz, p7, blendinduration, blendoutduration);
		}
		static void _0xed3c76adfa6d07c4(type::ped ped) {
			invoker::invoke<void>(0xed3c76adfa6d07c4, ped);
		}
		static void request_action_mode_asset(const char* asset) {
			invoker::invoke<void>(0x290e2780bb7aa598, asset);
		}
		static bool has_action_mode_asset_loaded(const char* asset) {
			return invoker::invoke<bool>(0xe4b5f4bf2cb24e65, asset);
		}
		static void remove_action_mode_asset(const char* asset) {
			invoker::invoke<void>(0x13e940f88470fa51, asset);
		}
		static void request_stealth_mode_asset(const char* asset) {
			invoker::invoke<void>(0x2a0a62fcdee16d4f, asset);
		}
		static bool has_stealth_mode_asset_loaded(const char* asset) {
			return invoker::invoke<bool>(0xe977fc5b08af3441, asset);
		}
		static void remove_stealth_mode_asset(const char* asset) {
			invoker::invoke<void>(0x9219857d21f0e842, asset);
		}
		static void set_ped_lod_multiplier(type::ped ped, float multiplier) {
			invoker::invoke<void>(0xdc2c5c242aac342b, ped, multiplier);
		}
		static void _0xe861d0b05c7662b8(type::pickup p0, type::any p1, type::blip* p2) {
			invoker::invoke<void>(0xe861d0b05c7662b8, p0, p1, p2);
		}
		static void _0x129466ed55140f8d(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x129466ed55140f8d, ped, toggle);
		}
		static void _0xcb968b53fc7f916d(type::any p0, bool p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0xcb968b53fc7f916d, p0, p1, p2, p3);
		}
		static bool _0x68772db2b2526f9f(type::ped ped, float x, float y, float z, float range) {
			return invoker::invoke<bool>(0x68772db2b2526f9f, ped, x, y, z, range);
		}
		static bool _0x06087579e7aa85a9(type::any p0, type::any p1, float p2, float p3, float p4, float p5) {
			return invoker::invoke<bool>(0x06087579e7aa85a9, p0, p1, p2, p3, p4, p5);
		}
		static void _0xd8c3be3ee94caf2d(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4) {
			invoker::invoke<void>(0xd8c3be3ee94caf2d, p0, p1, p2, p3, p4);
		}
		static void _0xd33daa36272177c4(type::ped ped) {
			invoker::invoke<void>(0xd33daa36272177c4, ped);
		}
		static void _0x83a169eabcdb10a2(type::any p0, type::any p1) {
			invoker::invoke<void>(0x83a169eabcdb10a2, p0, p1);
		}
		static void _0x288df530c92dad6f(type::any p0, float p1) {
			invoker::invoke<void>(0x288df530c92dad6f, p0, p1);
		}
	}

	namespace vehicle {
		static type::vehicle create_vehicle(type::hash modelhash, float x, float y, float z, float heading, bool isnetwork, bool thisscriptcheck) {
			return invoker::invoke<type::vehicle>(0xaf35d0d2583051b0, modelhash, x, y, z, heading, isnetwork, thisscriptcheck);
		}
		static void delete_vehicle(type::vehicle* vehicle) {
			invoker::invoke<void>(0xea386986e786a54f, vehicle);
		}
		static void _0x7d6f9a3ef26136a0(type::vehicle vehicle, bool p1, bool p2) {
			invoker::invoke<void>(0x7d6f9a3ef26136a0, vehicle, p1, p2);
		}
		static void set_vehicle_allow_no_passengers_lockon(type::vehicle veh, bool toggle) {
			invoker::invoke<void>(0x5d14d4154bfe7b2c, veh, toggle);
		}
		static int _0xe6b0e8cfc3633bf0(type::vehicle vehicle) {
			return invoker::invoke<int>(0xe6b0e8cfc3633bf0, vehicle);
		}
		static bool is_vehicle_model(type::vehicle vehicle, type::hash model) {
			return invoker::invoke<bool>(0x423e8de37d934d89, vehicle, model);
		}
		static bool does_script_vehicle_generator_exist(int v) {
			return invoker::invoke<bool>(0xf6086bc836400876, v);
		}
		static int create_script_vehicle_generator(float x, float y, float z, float heading, float p4, float p5, type::hash modelhash, int p7, int p8, int p9, int p10, bool p11, bool p12, bool p13, bool p14, bool p15, int p16) {
			return invoker::invoke<int>(0x9def883114668116, x, y, z, heading, p4, p5, modelhash, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16);
		}
		static void delete_script_vehicle_generator(int vehiclegenerator) {
			invoker::invoke<void>(0x22102c9abfcf125d, vehiclegenerator);
		}
		static void set_script_vehicle_generator(int vehiclegenerator, bool enabled) {
			invoker::invoke<void>(0xd9d620e0ac6dc4b0, vehiclegenerator, enabled);
		}
		static void set_all_vehicle_generators_active_in_area(float minx, float miny, float minz, float maxx, float maxy, float maxz, bool p6, bool p7) {
			invoker::invoke<void>(0xc12321827687fe4d, minx, miny, minz, maxx, maxy, maxz, p6, p7);
		}
		static void set_all_vehicle_generators_active() {
			invoker::invoke<void>(0x34ad89078831a4bc);
		}
		static void set_all_low_priority_vehicle_generators_active(bool active) {
			invoker::invoke<void>(0x608207e7a8fb787c, active);
		}
		static void _0x9a75585fb2e54fad(float p0, float p1, float p2, float p3) {
			invoker::invoke<void>(0x9a75585fb2e54fad, p0, p1, p2, p3);
		}
		static void _0x0a436b8643716d14() {
			invoker::invoke<void>(0x0a436b8643716d14);
		}
		static bool set_vehicle_on_ground_properly(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x49733e92263139d1, vehicle);
		}
		static type::any set_all_vehicles_spawn(type::vehicle vehicle, bool p1, bool p2, bool p3) {
			return invoker::invoke<type::any>(0xe023e8ac4ef7c117, vehicle, p1, p2, p3);
		}
		static bool is_vehicle_stuck_on_roof(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xb497f06b288dcfdf, vehicle);
		}
		static void add_vehicle_upsidedown_check(type::vehicle vehicle) {
			invoker::invoke<void>(0xb72e26d81006005b, vehicle);
		}
		static void remove_vehicle_upsidedown_check(type::vehicle vehicle) {
			invoker::invoke<void>(0xc53eb42a499a7e90, vehicle);
		}
		static bool is_vehicle_stopped(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x5721b434ad84d57a, vehicle);
		}
		static int get_vehicle_number_of_passengers(type::vehicle vehicle) {
			return invoker::invoke<int>(0x24cb2137731ffe89, vehicle);
		}
		static int get_vehicle_max_number_of_passengers(type::vehicle vehicle) {
			return invoker::invoke<int>(0xa7c4f2c6e744a550, vehicle);
		}
		static int get_vehicle_model_number_of_seats(type::hash modelhash) {
			return invoker::invoke<int>(0x2ad93716f184eda4, modelhash);
		}
		static bool _0xf7f203e31f96f6a1(type::vehicle vehicle, bool flag) {
			return invoker::invoke<bool>(0xf7f203e31f96f6a1, vehicle, flag);
		}
		static bool _0xe33ffa906ce74880(type::vehicle vehicle, type::any p1) {
			return invoker::invoke<bool>(0xe33ffa906ce74880, vehicle, p1);
		}
		static void set_vehicle_density_multiplier_this_frame(float multiplier) {
			invoker::invoke<void>(0x245a6883d966d537, multiplier);
		}
		static void set_random_vehicle_density_multiplier_this_frame(float multiplier) {
			invoker::invoke<void>(0xb3b3359379fe77d3, multiplier);
		}
		static void set_parked_vehicle_density_multiplier_this_frame(float multiplier) {
			invoker::invoke<void>(0xeae6dcc7eee3db1d, multiplier);
		}
		static void _0xd4b8e3d1917bc86b(bool toggle) {
			invoker::invoke<void>(0xd4b8e3d1917bc86b, toggle);
		}
		static void _set_some_vehicle_density_multiplier_this_frame(float value) {
			invoker::invoke<void>(0x90b6da738a9a25da, value);
		}
		static void set_far_draw_vehicles(bool toggle) {
			invoker::invoke<void>(0x26324f33423f3cc3, toggle);
		}
		static void set_number_of_parked_vehicles(int value) {
			invoker::invoke<void>(0xcaa15f13ebd417ff, value);
		}
		static void set_vehicle_doors_locked(int* vehicle, float* doorlockstatus) {
			invoker::invoke<void>(0xb664292eaecf7fa6, vehicle, doorlockstatus);
		}
		static void set_ped_targettable_vehicle_destroy(type::vehicle vehicle, int doorindex, int destroytype) {
			invoker::invoke<void>(0xbe70724027f85bcd, vehicle, doorindex, destroytype);
		}
		static void disable_vehicle_impact_explosion_activation(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xd8050e0eb60cf274, vehicle, toggle);
		}
		static void set_vehicle_doors_locked_for_player(type::vehicle vehicle, type::player player, bool toggle) {
			invoker::invoke<void>(0x517aaf684bb50cd1, vehicle, player, toggle);
		}
		static bool get_vehicle_doors_locked_for_player(type::vehicle vehicle, type::player player) {
			return invoker::invoke<bool>(0xf6af6cb341349015, vehicle, player);
		}
		static void set_vehicle_doors_locked_for_all_players(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xa2f80b8d040727cc, vehicle, toggle);
		}
		static void _0x9737a37136f07e75(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x9737a37136f07e75, vehicle, toggle);
		}
		static void set_vehicle_doors_locked_for_team(type::vehicle vehicle, int team, bool toggle) {
			invoker::invoke<void>(0xb81f6d4a8f5eeba8, vehicle, team, toggle);
		}
		static void explode_vehicle(type::vehicle vehicle, bool isaudible, bool isinvisible) {
			invoker::invoke<void>(0xba71116adf5b514c, vehicle, isaudible, isinvisible);
		}
		static void set_vehicle_out_of_control(type::vehicle vehicle, bool killdriver, bool explodeonimpact) {
			invoker::invoke<void>(0xf19d095e42d430cc, vehicle, killdriver, explodeonimpact);
		}
		static void set_vehicle_timed_explosion(type::vehicle vehicle, type::ped ped, bool toggle) {
			invoker::invoke<void>(0x2e0a74e1002380b1, vehicle, ped, toggle);
		}
		static void add_vehicle_phone_explosive_device(type::vehicle vehicle) {
			invoker::invoke<void>(0x99ad4cccb128cbc9, vehicle);
		}
		static bool has_vehicle_phone_explosive_device() {
			return invoker::invoke<bool>(0x6adaabd3068c5235);
		}
		static void detonate_vehicle_phone_explosive_device() {
			invoker::invoke<void>(0xef49cf0270307cbe);
		}
		static bool _0xae3fee8709b39dcb(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xae3fee8709b39dcb, vehicle);
		}
		static void set_taxi_lights(type::vehicle vehicle, bool state) {
			invoker::invoke<void>(0x598803e85e8448d9, vehicle, state);
		}
		static bool is_taxi_light_on(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x7504c0f113ab50fc, vehicle);
		}
		static bool is_vehicle_in_garage_area(const char* garagename, type::vehicle vehicle) {
			return invoker::invoke<bool>(0xcee4490cd57bb3c2, garagename, vehicle);
		}
		static void set_vehicle_colours(type::vehicle vehicle, int colorprimary, int colorsecondary) {
			invoker::invoke<void>(0x4f1d4be3a7f24601, vehicle, colorprimary, colorsecondary);
		}
		static void set_vehicle_fullbeam(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x8b7fd87f0ddb421e, vehicle, toggle);
		}
		static void steer_unlock_bias(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x07116e24e9d1929d, vehicle, toggle);
		}
		static void set_vehicle_custom_primary_colour(type::vehicle vehicle, int r, int g, int b) {
			invoker::invoke<void>(0x7141766f91d15bea, vehicle, r, g, b);
		}
		static void get_vehicle_custom_primary_colour(type::vehicle vehicle, int* r, int* g, int* b) {
			invoker::invoke<void>(0xb64cf2cca9d95f52, vehicle, r, g, b);
		}
		static void clear_vehicle_custom_primary_colour(type::vehicle vehicle) {
			invoker::invoke<void>(0x55e1d2758f34e437, vehicle);
		}
		static bool get_is_vehicle_primary_colour_custom(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xf095c0405307b21b, vehicle);
		}
		static void set_vehicle_custom_secondary_colour(type::vehicle vehicle, int r, int g, int b) {
			invoker::invoke<void>(0x36ced73bfed89754, vehicle, r, g, b);
		}
		static void get_vehicle_custom_secondary_colour(type::vehicle vehicle, int* r, int* g, int* b) {
			invoker::invoke<void>(0x8389cd56ca8072dc, vehicle, r, g, b);
		}
		static void clear_vehicle_custom_secondary_colour(type::vehicle vehicle) {
			invoker::invoke<void>(0x5ffbdeec3e8e2009, vehicle);
		}
		static bool get_is_vehicle_secondary_colour_custom(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x910a32e7aad2656c, vehicle);
		}
		static void set_vehicle_enveff_scale(type::vehicle vehicle, float fade) {
			invoker::invoke<void>(0x3afdc536c3d01674, vehicle, fade);
		}
		static float get_vehicle_enveff_scale(type::vehicle vehicle) {
			return invoker::invoke<float>(0xa82819cac9c4c403, vehicle);
		}
		static void set_can_respray_vehicle(type::vehicle vehicle, bool state) {
			invoker::invoke<void>(0x52bba29d5ec69356, vehicle, state);
		}
		static void _0x33506883545ac0df(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x33506883545ac0df, vehicle, p1);
		}
		static void _jitter_vehicle(type::vehicle vehicle, bool p1, float yaw, float pitch, float roll) {
			invoker::invoke<void>(0xc59872a5134879c7, vehicle, p1, yaw, pitch, roll);
		}
		static void set_boat_anchor(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x75dbec174aeead10, vehicle, toggle);
		}
		static bool _get_boat_anchor(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x26c10ecbda5d043b, vehicle);
		}
		static void _0xe3ebaae484798530(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xe3ebaae484798530, vehicle, p1);
		}
		static void _0xb28b1fe5bfadd7f5(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xb28b1fe5bfadd7f5, vehicle, p1);
		}
		static void _0xe842a9398079bd82(type::vehicle vehicle, float p1) {
			invoker::invoke<void>(0xe842a9398079bd82, vehicle, p1);
		}
		static void _0x8f719973e1445ba2(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x8f719973e1445ba2, vehicle, toggle);
		}
		static void set_vehicle_siren(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xf4924635a19eb37d, vehicle, toggle);
		}
		static bool is_vehicle_siren_on(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x4c9bf537be2634b2, vehicle);
		}
		static bool _is_vehicle_siren_sound_on(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xb5cc40fbcb586380, vehicle);
		}
		static void set_vehicle_strong(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x3e8c8727991a8a0b, vehicle, toggle);
		}
		static void remove_vehicle_stuck_check(type::vehicle vehicle) {
			invoker::invoke<void>(0x8386bfb614d06749, vehicle);
		}
		static void get_vehicle_colours(type::vehicle vehicle, int* colorprimary, int* colorsecondary) {
			invoker::invoke<void>(0xa19435f193e081ac, vehicle, colorprimary, colorsecondary);
		}
		static bool is_vehicle_seat_free(type::vehicle vehicle, int seatindex) {
			return invoker::invoke<bool>(0x22ac59a870e6a669, vehicle, seatindex);
		}
		static type::ped get_ped_in_vehicle_seat(type::vehicle vehicle, int index) {
			return invoker::invoke<type::ped>(0xbb40dd2270b65366, vehicle, index);
		}
		static type::ped get_last_ped_in_vehicle_seat(type::vehicle vehicle, int seatindex) {
			return invoker::invoke<type::ped>(0x83f969aa1ee2a664, vehicle, seatindex);
		}
		static bool get_vehicle_lights_state(type::vehicle vehicle, bool* lightson, bool* highbeamson) {
			return invoker::invoke<bool>(0xb91b4c20085bd12f, vehicle, lightson, highbeamson);
		}
		static bool is_vehicle_tyre_burst(type::vehicle vehicle, int wheelid, bool completely) {
			return invoker::invoke<bool>(0xba291848a0815ca9, vehicle, wheelid, completely);
		}
		static void set_vehicle_forward_speed(type::vehicle vehicle, float speed) {
			invoker::invoke<void>(0xab54a438726d25d5, vehicle, speed);
		}
		static void bring_vehicle_to_halt(type::vehicle vehicle, float distance, int type, bool unknown) {
			invoker::invoke<void>(0x260be8f09e326a20, vehicle, distance, type, unknown);
		}
		static void _set_vehicle_forklift_height(type::vehicle vehicle, float height) {
			invoker::invoke<void>(0x37ebbf3117bd6a25, vehicle, height);
		}
		static bool set_ped_enabled_bike_ringtone(type::vehicle vehicle, type::entity entity) {
			return invoker::invoke<bool>(0x57715966069157ad, vehicle, entity);
		}
		static bool _0x62ca17b74c435651(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x62ca17b74c435651, vehicle);
		}
		static type::vehicle _get_vehicle_attached_to_entity(type::vehicle object) {
			return invoker::invoke<type::vehicle>(0x375e7fc44f21c8ab, object);
		}
		static bool _0x89d630cf5ea96d23(type::vehicle vehicle, type::entity entity) {
			return invoker::invoke<bool>(0x89d630cf5ea96d23, vehicle, entity);
		}
		static void _0x6a98c2ecf57fa5d4(type::vehicle vehicle, type::entity entity) {
			invoker::invoke<void>(0x6a98c2ecf57fa5d4, vehicle, entity);
		}
		static void _0x7c0043fdff6436bc(type::vehicle vehicle) {
			invoker::invoke<void>(0x7c0043fdff6436bc, vehicle);
		}
		static void _0x8aa9180de2fedd45(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x8aa9180de2fedd45, vehicle, p1);
		}
		static void _0x0a6a279f3aa4fd70(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x0a6a279f3aa4fd70, vehicle, p1);
		}
		static bool _0x634148744f385576(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x634148744f385576, vehicle);
		}
		static void _0xe6f13851780394da(type::vehicle vehicle, float p1) {
			invoker::invoke<void>(0xe6f13851780394da, vehicle, p1);
		}
		static void set_vehicle_tyre_burst(type::vehicle vehicle, int index, bool onrim, float p3) {
			invoker::invoke<void>(0xec6a202ee4960385, vehicle, index, onrim, p3);
		}
		static void set_vehicle_doors_shut(type::vehicle vehicle, bool closeinstantly) {
			invoker::invoke<void>(0x781b3d62bb013ef5, vehicle, closeinstantly);
		}
		static void set_vehicle_tyres_can_burst(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xeb9dc3c7d8596c46, vehicle, toggle);
		}
		static bool get_vehicle_tyres_can_burst(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x678b9bb8c3f58feb, vehicle);
		}
		static void set_vehicle_wheels_can_break(type::vehicle vehicle, bool enabled) {
			invoker::invoke<void>(0x29b18b4fd460ca8f, vehicle, enabled);
		}
		static void set_vehicle_door_open(type::vehicle vehicle, int doorindex, bool loose, bool openinstantly) {
			invoker::invoke<void>(0x7c65dac73c35c862, vehicle, doorindex, loose, openinstantly);
		}
		static void remove_vehicle_window(type::vehicle vehicle, int windowindex) {
			invoker::invoke<void>(0xa711568eedb43069, vehicle, windowindex);
		}
		static void roll_down_windows(type::vehicle vehicle) {
			invoker::invoke<void>(0x85796b0549dde156, vehicle);
		}
		static void roll_down_window(type::vehicle vehicle, int windowindex) {
			invoker::invoke<void>(0x7ad9e6ce657d69e3, vehicle, windowindex);
		}
		static void roll_up_window(type::vehicle vehicle, int windowindex) {
			invoker::invoke<void>(0x602e548f46e24d59, vehicle, windowindex);
		}
		static void smash_vehicle_window(type::vehicle vehicle, int index) {
			invoker::invoke<void>(0x9e5b5e4d2ccd2259, vehicle, index);
		}
		static void fix_vehicle_window(type::vehicle vehicle, int index) {
			invoker::invoke<void>(0x772282ebeb95e682, vehicle, index);
		}
		static void _detach_vehicle_windscreen(type::vehicle vehicle) {
			invoker::invoke<void>(0x6d645d59fb5f5ad3, vehicle);
		}
		static void _eject_jb700_roof(type::vehicle vehicle, float x, float y, float z) {
			invoker::invoke<void>(0xe38cb9d7d39fdbcc, vehicle, x, y, z);
		}
		static void set_vehicle_lights(type::vehicle vehicle, int state) {
			invoker::invoke<void>(0x34e710ff01247c5a, vehicle, state);
		}
		static void _0xc45c27ef50f36adc(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xc45c27ef50f36adc, vehicle, p1);
		}
		static void _set_vehicle_lights_mode(type::vehicle vehicle, int p1) {
			invoker::invoke<void>(0x1fd09e7390a74d54, vehicle, p1);
		}
		static void set_vehicle_alarm(type::vehicle vehicle, bool state) {
			invoker::invoke<void>(0xcde5e70c1ddb954c, vehicle, state);
		}
		static void start_vehicle_alarm(type::vehicle vehicle) {
			invoker::invoke<void>(0xb8ff7ab45305c345, vehicle);
		}
		static bool is_vehicle_alarm_activated(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x4319e335b71fff34, vehicle);
		}
		static void set_vehicle_interiorlight(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xbc2042f090af6ad3, vehicle, toggle);
		}
		static void set_vehicle_light_multiplier(type::vehicle vehicle, float multiplier) {
			invoker::invoke<void>(0xb385454f8791f57c, vehicle, multiplier);
		}
		static void attach_vehicle_to_trailer(type::vehicle vehicle, type::vehicle trailer, float radius) {
			invoker::invoke<void>(0x3c7d42d58f770b54, vehicle, trailer, radius);
		}
		static void _0x16b5e274bde402f8(type::vehicle vehicle, type::vehicle trailer, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11) {
			invoker::invoke<void>(0x16b5e274bde402f8, vehicle, trailer, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
		}
		static void _0x374706271354cb18(type::vehicle vehicle, type::entity p1, float p2) {
			invoker::invoke<void>(0x374706271354cb18, vehicle, p1, p2);
		}
		static void detach_vehicle_from_trailer(type::vehicle vehicle) {
			invoker::invoke<void>(0x90532edf0d2bdd86, vehicle);
		}
		static bool is_vehicle_attached_to_trailer(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xe7cf3c4f9f489f0c, vehicle);
		}
		static void _0x2a8f319b392e7b3f(type::vehicle vehicle, float p1) {
			invoker::invoke<void>(0x2a8f319b392e7b3f, vehicle, p1);
		}
		static void _0x95cf53b3d687f9fa(type::vehicle vehicle) {
			invoker::invoke<void>(0x95cf53b3d687f9fa, vehicle);
		}
		static void set_vehicle_tyre_fixed(type::vehicle vehicle, int tyreindex) {
			invoker::invoke<void>(0x6e13fc662b882d1d, vehicle, tyreindex);
		}
		static void set_vehicle_number_plate_text(type::vehicle vehicle, const char* platetext) {
			invoker::invoke<void>(0x95a88f0b409cda47, vehicle, platetext);
		}
		static const char* get_vehicle_number_plate_text(type::vehicle vehicle) {
			return invoker::invoke<const char*>(0x7ce1ccb9b293020e, vehicle);
		}
		static int get_number_of_vehicle_number_plates() {
			return invoker::invoke<int>(0x4c4d6b2644f458cb);
		}
		static void set_vehicle_number_plate_text_index(type::vehicle vehicle, int plateindex) {
			invoker::invoke<void>(0x9088eb5a43ffb0a1, vehicle, plateindex);
		}
		static int get_vehicle_number_plate_text_index(type::vehicle elegy) {
			return invoker::invoke<int>(0xf11bc2dd9a3e7195, elegy);
		}
		static void set_random_trains(bool toggle) {
			invoker::invoke<void>(0x80d9f74197ea47d9, toggle);
		}
		static type::vehicle create_mission_train(int variation, float x, float y, float z, bool direction) {
			return invoker::invoke<type::vehicle>(0x63c6cca8e68ae8c8, variation, x, y, z, direction);
		}
		static void switch_train_track(int intersectionid, bool state) {
			invoker::invoke<void>(0xfd813bb7db977f20, intersectionid, state);
		}
		static void _0x21973bbf8d17edfa(type::any p0, type::any p1) {
			invoker::invoke<void>(0x21973bbf8d17edfa, p0, p1);
		}
		static void delete_all_trains() {
			invoker::invoke<void>(0x736a718577f39c7d);
		}
		static void set_train_speed(type::vehicle train, float speed) {
			invoker::invoke<void>(0xaa0bc91be0b796e3, train, speed);
		}
		static void set_train_cruise_speed(type::vehicle train, float speed) {
			invoker::invoke<void>(0x16469284db8c62b5, train, speed);
		}
		static void set_random_boats(bool toggle) {
			invoker::invoke<void>(0x84436ec293b1415f, toggle);
		}
		static void set_garbage_trucks(bool toggle) {
			invoker::invoke<void>(0x2afd795eeac8d30d, toggle);
		}
		static bool does_vehicle_have_stuck_vehicle_check(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x57e4c39de5ee8470, vehicle);
		}
		static int get_vehicle_recording_id(int p0, const char* p1) {
			return invoker::invoke<int>(0x21543c612379db3c, p0, p1);
		}
		static void request_vehicle_recording(int i, const char* name) {
			invoker::invoke<void>(0xaf514cabe74cbf15, i, name);
		}
		static bool has_vehicle_recording_been_loaded(type::any p0, type::any* p1) {
			return invoker::invoke<bool>(0x300d614a4c785fc4, p0, p1);
		}
		static void remove_vehicle_recording(type::any p0, type::any* p1) {
			invoker::invoke<void>(0xf1160accf98a3fc8, p0, p1);
		}
		static PVector3 _0x92523b76657a517d(type::any p0, float p1) {
			return invoker::invoke<PVector3>(0x92523b76657a517d, p0, p1);
		}
		static PVector3 get_position_of_vehicle_recording_at_time(int p0, float p1, const char* p2) {
			return invoker::invoke<PVector3>(0xd242728aa6f0fba2, p0, p1, p2);
		}
		static PVector3 _0xf0f2103efaf8cba7(type::any p0, float p1) {
			return invoker::invoke<PVector3>(0xf0f2103efaf8cba7, p0, p1);
		}
		static PVector3 get_rotation_of_vehicle_recording_at_time(type::any p0, float p1, type::any* p2) {
			return invoker::invoke<PVector3>(0x2058206fbe79a8ad, p0, p1, p2);
		}
		static float get_total_duration_of_vehicle_recording_id(int recordingid) {
			return invoker::invoke<float>(0x102d125411a7b6e6, recordingid);
		}
		static type::any get_total_duration_of_vehicle_recording(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x0e48d1c262390950, p0, p1);
		}
		static float get_position_in_recording(type::any p0) {
			return invoker::invoke<float>(0x2dacd605fc681475, p0);
		}
		static float get_time_position_in_recording(type::any p0) {
			return invoker::invoke<float>(0x5746f3a7ab7fe544, p0);
		}
		static void start_playback_recorded_vehicle(type::vehicle vehicle, int p1, const char* playback, bool p3) {
			invoker::invoke<void>(0x3f878f92b3a7a071, vehicle, p1, playback, p3);
		}
		static void start_playback_recorded_vehicle_with_flags(type::vehicle vehicle, type::any p1, const char* playback, type::any p3, type::any p4, type::any p5) {
			invoker::invoke<void>(0x7d80fd645d4da346, vehicle, p1, playback, p3, p4, p5);
		}
		static void _0x1f2e4e06dea8992b(type::vehicle p0, bool p1) {
			invoker::invoke<void>(0x1f2e4e06dea8992b, p0, p1);
		}
		static void stop_playback_recorded_vehicle(type::vehicle vehicle) {
			invoker::invoke<void>(0x54833611c17abdea, vehicle);
		}
		static void pause_playback_recorded_vehicle(type::any p0) {
			invoker::invoke<void>(0x632a689bf42301b1, p0);
		}
		static void unpause_playback_recorded_vehicle(type::any p0) {
			invoker::invoke<void>(0x8879ee09268305d5, p0);
		}
		static bool is_playback_going_on_for_vehicle(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x1c8a4c2c19e68eec, vehicle);
		}
		static bool is_playback_using_ai_going_on_for_vehicle(type::any p0) {
			return invoker::invoke<bool>(0xaea8fd591fad4106, p0);
		}
		static int get_current_playback_for_vehicle(type::vehicle vehicle) {
			return invoker::invoke<int>(0x42bc05c27a946054, vehicle);
		}
		static void skip_to_end_and_stop_playback_recorded_vehicle(type::any p0) {
			invoker::invoke<void>(0xab8e2eda0c0a5883, p0);
		}
		static void set_playback_speed(type::vehicle vehicle, float speed) {
			invoker::invoke<void>(0x6683ab880e427778, vehicle, speed);
		}
		static void start_playback_recorded_vehicle_using_ai(type::any p0, type::any p1, type::any* p2, float p3, type::any p4) {
			invoker::invoke<void>(0x29de5fa52d00428c, p0, p1, p2, p3, p4);
		}
		static void skip_time_in_playback_recorded_vehicle(type::any p0, float p1) {
			invoker::invoke<void>(0x9438f7ad68771a20, p0, p1);
		}
		static void set_playback_to_use_ai(type::vehicle vehicle, int flag) {
			invoker::invoke<void>(0xa549c3b37ea28131, vehicle, flag);
		}
		static void set_playback_to_use_ai_try_to_revert_back_later(type::any p0, type::any p1, type::any p2, bool p3) {
			invoker::invoke<void>(0x6e63860bbb190730, p0, p1, p2, p3);
		}
		static void _0x5845066d8a1ea7f7(type::vehicle vehicle, float x, float y, float z, type::any p4) {
			invoker::invoke<void>(0x5845066d8a1ea7f7, vehicle, x, y, z, p4);
		}
		static void _0x796a877e459b99ea(type::any p0, float p1, float p2, float p3) {
			invoker::invoke<void>(0x796a877e459b99ea, p0, p1, p2, p3);
		}
		static void _0xfaf2a78061fd9ef4(type::any p0, float p1, float p2, float p3) {
			invoker::invoke<void>(0xfaf2a78061fd9ef4, p0, p1, p2, p3);
		}
		static void _0x063ae2b2cc273588(type::any p0, bool p1) {
			invoker::invoke<void>(0x063ae2b2cc273588, p0, p1);
		}
		static void explode_vehicle_in_cutscene(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x786a4eb67b01bf0b, vehicle, p1);
		}
		static void add_vehicle_stuck_check_with_warp(type::any p0, float p1, type::any p2, bool p3, bool p4, bool p5, type::any p6) {
			invoker::invoke<void>(0x2fa9923062dd396c, p0, p1, p2, p3, p4, p5, p6);
		}
		static void set_vehicle_model_is_suppressed(type::hash model, bool suppressed) {
			invoker::invoke<void>(0x0fc2d89ac25a5814, model, suppressed);
		}
		static type::vehicle get_random_vehicle_in_sphere(float x, float y, float z, float radius, type::hash modelhash, int flags) {
			return invoker::invoke<type::vehicle>(0x386f6ce5baf6091c, x, y, z, radius, modelhash, flags);
		}
		static type::vehicle get_random_vehicle_front_bumper_in_sphere(float p0, float p1, float p2, float p3, int p4, int p5, int p6) {
			return invoker::invoke<type::vehicle>(0xc5574e0aeb86ba68, p0, p1, p2, p3, p4, p5, p6);
		}
		static type::vehicle get_random_vehicle_back_bumper_in_sphere(float p0, float p1, float p2, float p3, int p4, int p5, int p6) {
			return invoker::invoke<type::vehicle>(0xb50807eabe20a8dc, p0, p1, p2, p3, p4, p5, p6);
		}
		static type::vehicle get_closest_vehicle(float x, float y, float z, float radius, type::hash modelhash, int flags) {
			return invoker::invoke<type::vehicle>(0xf73eb622c4f1689b, x, y, z, radius, modelhash, flags);
		}
		static type::entity get_train_carriage(type::vehicle train, int trailernumber) {
			return invoker::invoke<type::entity>(0x08aafd0814722bc3, train, trailernumber);
		}
		static void delete_mission_train(type::vehicle* train) {
			invoker::invoke<void>(0x5b76b14ae875c795, train);
		}
		static void set_mission_train_as_no_longer_needed(type::vehicle* train, bool p1) {
			invoker::invoke<void>(0xbbe7648349b49be8, train, p1);
		}
		static void set_mission_train_coords(type::vehicle train, float x, float y, float z) {
			invoker::invoke<void>(0x591ca673aa6ab736, train, x, y, z);
		}
		static bool is_this_model_a_boat(type::hash model) {
			return invoker::invoke<bool>(0x45a9187928f4b9e3, model);
		}
		static bool _is_this_model_a_jetski(type::hash model) {
			return invoker::invoke<bool>(0x9537097412cf75fe, model);
		}
		static bool is_this_model_a_plane(type::hash model) {
			return invoker::invoke<bool>(0xa0948ab42d7ba0de, model);
		}
		static bool is_this_model_a_heli(type::hash model) {
			return invoker::invoke<bool>(0xdce4334788af94ea, model);
		}
		static bool is_this_model_a_car(type::hash model) {
			return invoker::invoke<bool>(0x7f6db52eefc96df8, model);
		}
		static bool is_this_model_a_train(type::hash model) {
			return invoker::invoke<bool>(0xab935175b22e822b, model);
		}
		static bool is_this_model_a_bike(type::hash model) {
			return invoker::invoke<bool>(0xb50c0b0cedc6ce84, model);
		}
		static bool is_this_model_a_bicycle(type::hash model) {
			return invoker::invoke<bool>(0xbf94dd42f63bded2, model);
		}
		static bool is_this_model_a_quadbike(type::hash model) {
			return invoker::invoke<bool>(0x39dac362ee65fa28, model);
		}
		static void set_heli_blades_full_speed(type::vehicle vehicle) {
			invoker::invoke<void>(0xa178472ebb8ae60d, vehicle);
		}
		static void set_heli_blades_speed(type::vehicle vehicle, float speed) {
			invoker::invoke<void>(0xfd280b4d7f3abc4d, vehicle, speed);
		}
		static void _0x99cad8e7afdb60fa(type::vehicle vehicle, float p1, float p2) {
			invoker::invoke<void>(0x99cad8e7afdb60fa, vehicle, p1, p2);
		}
		static void set_vehicle_can_be_targetted(type::vehicle vehicle, bool state) {
			invoker::invoke<void>(0x3750146a28097a82, vehicle, state);
		}
		static void _0xdbc631f109350b8c(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xdbc631f109350b8c, vehicle, p1);
		}
		static void set_vehicle_can_be_visibly_damaged(type::vehicle vehicle, bool state) {
			invoker::invoke<void>(0x4c7028f78ffd3681, vehicle, state);
		}
		static void _0x1aa8a837d2169d94(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x1aa8a837d2169d94, vehicle, p1);
		}
		static void _0x2311dd7159f00582(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x2311dd7159f00582, vehicle, p1);
		}
		static float get_vehicle_dirt_level(type::vehicle vehicle) {
			return invoker::invoke<float>(0x8f17bc8ba08da62b, vehicle);
		}
		static void set_vehicle_dirt_level(type::vehicle vehicle, float dirtlevel) {
			invoker::invoke<void>(0x79d3b596fe44ee8b, vehicle, dirtlevel);
		}
		static bool _is_vehicle_damaged(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xbcdc5017d3ce1e9e, vehicle);
		}
		static bool is_vehicle_door_fully_open(type::vehicle v, int doorindex) {
			return invoker::invoke<bool>(0x3e933cff7b111c22, v, doorindex);
		}
		static void set_vehicle_engine_on(type::vehicle vehicle, bool value, bool instantly, bool noautoturnon) {
			invoker::invoke<void>(0x2497c4717c8b881e, vehicle, value, instantly, noautoturnon);
		}
		static void set_vehicle_undriveable(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x8aba6af54b942b95, vehicle, toggle);
		}
		static void set_vehicle_provides_cover(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x5afeedd9bb2899d7, vehicle, toggle);
		}
		static void set_vehicle_door_control(type::vehicle vehicle, int doorindex, int speed, float angle) {
			invoker::invoke<void>(0xf2bfa0430f0a0fcb, vehicle, doorindex, speed, angle);
		}
		static void set_vehicle_door_latched(type::vehicle vehicle, int doorindex, bool forceclose, bool lock, bool p4) {
			invoker::invoke<void>(0xa5a9653a8d2caf48, vehicle, doorindex, forceclose, lock, p4);
		}
		static float get_vehicle_door_angle_ratio(type::vehicle vehicle, int door) {
			return invoker::invoke<float>(0xfe3f9c29f7b32bd5, vehicle, door);
		}
		static type::ped _get_ped_using_vehicle_door(type::vehicle vehicle, int doorindex) {
			return invoker::invoke<type::ped>(0x218297bf0cfd853b, vehicle, doorindex);
		}
		static void set_vehicle_door_shut(type::vehicle vehicle, int doorindex, bool closeinstantly) {
			invoker::invoke<void>(0x93d9bd300d7789e5, vehicle, doorindex, closeinstantly);
		}
		static void set_vehicle_door_broken(type::vehicle vehicle, int doorindex, bool deletedoor) {
			invoker::invoke<void>(0xd4d4f6a4ab575a33, vehicle, doorindex, deletedoor);
		}
		static void set_vehicle_can_break(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x59bf8c3d52c92f66, vehicle, toggle);
		}
		static bool does_vehicle_have_roof(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x8ac862b0b32c5b80, vehicle);
		}
		static bool is_big_vehicle(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x9f243d3919f442fe, vehicle);
		}
		static int get_number_of_vehicle_colours(type::vehicle vehicle) {
			return invoker::invoke<int>(0x3b963160cd65d41e, vehicle);
		}
		static void set_vehicle_colour_combination(type::vehicle vehicle, int colorcombination) {
			invoker::invoke<void>(0x33e8cd3322e2fe31, vehicle, colorcombination);
		}
		static int get_vehicle_colour_combination(type::vehicle vehicle) {
			return invoker::invoke<int>(0x6a842d197f845d56, vehicle);
		}
		static void set_vehicle_is_considered_by_player(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x31b927bbc44156cd, vehicle, toggle);
		}
		static void _0xbe5c1255a1830ff5(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xbe5c1255a1830ff5, vehicle, toggle);
		}
		static void _0x9becd4b9fef3f8a6(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x9becd4b9fef3f8a6, vehicle, p1);
		}
		static void _0x88bc673ca9e0ae99(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x88bc673ca9e0ae99, vehicle, p1);
		}
		static void _0xe851e480b814d4ba(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xe851e480b814d4ba, vehicle, p1);
		}
		static void get_random_vehicle_model_in_memory(bool p0, native::type::hash* modelhash, int* p2) {
			invoker::invoke<void>(0x055bf0ac0c34f4fd, p0, modelhash, p2);
		}
		static int get_vehicle_door_lock_status(type::vehicle vehicle) {
			return invoker::invoke<int>(0x25bc98a59c2ea962, vehicle);
		}
		static bool is_vehicle_door_damaged(type::vehicle veh, int doorid) {
			return invoker::invoke<bool>(0xb8e181e559464527, veh, doorid);
		}
		static void _set_vehicle_door_can_break(type::vehicle vehicle, int doorindex, bool isbreakable) {
			invoker::invoke<void>(0x2fa133a4a9d37ed8, vehicle, doorindex, isbreakable);
		}
		static bool is_vehicle_bumper_bouncing(type::vehicle vehicle, bool frontbumper) {
			return invoker::invoke<bool>(0x27b926779deb502d, vehicle, frontbumper);
		}
		static bool is_vehicle_bumper_broken_off(type::vehicle vehicle, bool front) {
			return invoker::invoke<bool>(0x468056a6bb6f3846, vehicle, front);
		}
		static bool is_cop_vehicle_in_area_3d(float x1, float x2, float y1, float y2, float z1, float z2) {
			return invoker::invoke<bool>(0x7eef65d5f153e26a, x1, x2, y1, y2, z1, z2);
		}
		static bool is_vehicle_on_all_wheels(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xb104cd1babf302e2, vehicle);
		}
		static type::hash get_vehicle_layout_hash(type::vehicle vehicle) {
			return invoker::invoke<type::hash>(0x28d37d4f71ac5c58, vehicle);
		}
		static type::any _0xa01bc64dd4bfbbac(type::vehicle vehicle, int p1) {
			return invoker::invoke<type::any>(0xa01bc64dd4bfbbac, vehicle, p1);
		}
		static void set_render_train_as_derailed(type::vehicle train, bool toggle) {
			invoker::invoke<void>(0x317b11a312df5534, train, toggle);
		}
		static void set_vehicle_extra_colours(type::vehicle vehicle, int pearlescentcolor, int wheelcolor) {
			invoker::invoke<void>(0x2036f561add12e33, vehicle, pearlescentcolor, wheelcolor);
		}
		static void get_vehicle_extra_colours(type::vehicle vehicle, int* pearlescentcolor, int* wheelcolor) {
			invoker::invoke<void>(0x3bc4245933a166f7, vehicle, pearlescentcolor, wheelcolor);
		}
		static void stop_all_garage_activity() {
			invoker::invoke<void>(0x0f87e938bdf29d66);
		}
		static void set_vehicle_fixed(type::vehicle vehicle) {
			invoker::invoke<void>(0x115722b1b9c14c1c, vehicle);
		}
		static void set_vehicle_deformation_fixed(type::vehicle vehicle) {
			invoker::invoke<void>(0x953da1e1b12c0491, vehicle);
		}
		static void _0x206bc5dc9d1ac70a(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x206bc5dc9d1ac70a, vehicle, p1);
		}
		static void _0x51bb2d88d31a914b(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x51bb2d88d31a914b, vehicle, p1);
		}
		static void _0x192547247864dfdd(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x192547247864dfdd, vehicle, p1);
		}
		static void set_disable_vehicle_petrol_tank_fires(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x465bf26ab9684352, vehicle, toggle);
		}
		static void set_disable_vehicle_petrol_tank_damage(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x37c8252a7c92d017, vehicle, toggle);
		}
		static void _0x91a0bd635321f145(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x91a0bd635321f145, vehicle, p1);
		}
		static void _0xc50ce861b55eab8b(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xc50ce861b55eab8b, vehicle, p1);
		}
		static void _0x6ebfb22d646ffc18(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x6ebfb22d646ffc18, vehicle, p1);
		}
		static void _0x25367de49d64cf16(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x25367de49d64cf16, vehicle, p1);
		}
		static void remove_vehicles_from_generators_in_area(float x1, float y1, float z1, float x2, float y2, float z2, type::any unk) {
			invoker::invoke<void>(0x46a1e1a299ec4bba, x1, y1, z1, x2, y2, z2, unk);
		}
		static void set_vehicle_steer_bias(type::vehicle vehicle, float value) {
			invoker::invoke<void>(0x42a8ec77d5150cbe, vehicle, value);
		}
		static bool is_vehicle_extra_turned_on(type::vehicle vehicle, int extraid) {
			return invoker::invoke<bool>(0xd2e6822dbfd6c8bd, vehicle, extraid);
		}
		static void set_vehicle_extra(type::vehicle vehicle, int extraid, bool disable) {
			invoker::invoke<void>(0x7ee3a3c5e4a40cc9, vehicle, extraid, disable);
		}
		static bool does_extra_exist(type::vehicle vehicle, int extraid) {
			return invoker::invoke<bool>(0x1262d55792428154, vehicle, extraid);
		}
		static void set_convertible_roof(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xf39c4f538b5124c2, vehicle, p1);
		}
		static void lower_convertible_roof(type::vehicle vehicle, bool instantlylower) {
			invoker::invoke<void>(0xded51f703d0fa83d, vehicle, instantlylower);
		}
		static void raise_convertible_roof(type::vehicle vehicle, bool instantlyraise) {
			invoker::invoke<void>(0x8f5fb35d7e88fc70, vehicle, instantlyraise);
		}
		static int get_convertible_roof_state(type::vehicle vehicle) {
			return invoker::invoke<int>(0xf8c397922fc03f41, vehicle);
		}
		static bool is_vehicle_a_convertible(type::vehicle vehicle, bool p1) {
			return invoker::invoke<bool>(0x52f357a30698bcce, vehicle, p1);
		}
		static bool is_vehicle_stopped_at_traffic_lights(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x2959f696ae390a99, vehicle);
		}
		static void set_vehicle_damage(type::vehicle vehicle, float xoffset, float yoffset, float zoffset, float damage, float radius, bool p6) {
			invoker::invoke<void>(0xa1dd317ea8fd4f29, vehicle, xoffset, yoffset, zoffset, damage, radius, p6);
		}
		static float get_vehicle_engine_health(type::vehicle vehicle) {
			return invoker::invoke<float>(0xc45d23baf168aab8, vehicle);
		}
		static void set_vehicle_engine_health(type::vehicle vehicle, float health) {
			invoker::invoke<void>(0x45f6d8eef34abef1, vehicle, health);
		}
		static float get_vehicle_petrol_tank_health(type::vehicle vehicle) {
			return invoker::invoke<float>(0x7d5dabe888d2d074, vehicle);
		}
		static void set_vehicle_petrol_tank_health(type::vehicle vehicle, float health) {
			invoker::invoke<void>(0x70db57649fa8d0d8, vehicle, health);
		}
		static bool is_vehicle_stuck_timer_up(type::vehicle vehicle, int p1, int p2) {
			return invoker::invoke<bool>(0x679be1daf71da874, vehicle, p1, p2);
		}
		static void reset_vehicle_stuck_timer(type::vehicle vehicle, int nullattributes) {
			invoker::invoke<void>(0xd7591b0065afaa7a, vehicle, nullattributes);
		}
		static bool is_vehicle_driveable(type::vehicle vehicle, bool isonfirecheck) {
			return invoker::invoke<bool>(0x4c241e39b23df959, vehicle, isonfirecheck);
		}
		static void set_vehicle_has_been_owned_by_player(type::vehicle vehicle, bool owned) {
			invoker::invoke<void>(0x2b5f9d2af1f1722d, vehicle, owned);
		}
		static void set_vehicle_needs_to_be_hotwired(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xfba550ea44404ee6, vehicle, toggle);
		}
		static void _0x9f3f689b814f2599(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x9f3f689b814f2599, vehicle, p1);
		}
		static void _0x4e74e62e0a97e901(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x4e74e62e0a97e901, vehicle, p1);
		}
		static void start_vehicle_horn(type::vehicle vehicle, int duration, type::hash mode, bool forever) {
			invoker::invoke<void>(0x9c8c6504b5b63d2c, vehicle, duration, mode, forever);
		}
		static void _set_vehicle_silent(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x9d44fcce98450843, vehicle, toggle);
		}
		static void set_vehicle_has_strong_axles(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x92f0cf722bc4202f, vehicle, toggle);
		}
		static const char* get_display_name_from_vehicle_model(type::hash modelhash) {
			return invoker::invoke<const char*>(0xb215aac32d25d019, modelhash);
		}
		static PVector3 get_vehicle_deformation_at_pos(type::vehicle vehicle, float offsetx, float offsety, float offsetz) {
			return invoker::invoke<PVector3>(0x4ec6cfbc7b2e9536, vehicle, offsetx, offsety, offsetz);
		}
		static void set_vehicle_livery(type::vehicle vehicle, int liveryindex) {
			invoker::invoke<void>(0x60bf608f1b8cd1b6, vehicle, liveryindex);
		}
		static int get_vehicle_livery(type::vehicle trailers2) {
			return invoker::invoke<int>(0x2bb9230590da5e8a, trailers2);
		}
		static int get_vehicle_livery_count(type::vehicle vehicle) {
			return invoker::invoke<int>(0x87b63e25a529d526, vehicle);
		}
		static bool is_vehicle_window_intact(type::vehicle vehicle, int windowindex) {
			return invoker::invoke<bool>(0x46e571a0e20d01f1, vehicle, windowindex);
		}
		static bool are_all_vehicle_windows_intact(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x11d862a3e977a9ef, vehicle);
		}
		static bool are_any_vehicle_seats_free(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x2d34fc3bc4adb780, vehicle);
		}
		static void reset_vehicle_wheels(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x21d2e5662c1f6fed, vehicle, toggle);
		}
		static bool is_heli_part_broken(type::vehicle vehicle, bool p1, bool p2, bool p3) {
			return invoker::invoke<bool>(0xbc74b4be25eb6c8a, vehicle, p1, p2, p3);
		}
		static float _get_heli_main_rotor_health(type::vehicle vehicle) {
			return invoker::invoke<float>(0xe4cb7541f413d2c5, vehicle);
		}
		static float _get_heli_tail_rotor_health(type::vehicle vehicle) {
			return invoker::invoke<float>(0xae8ce82a4219ac8c, vehicle);
		}
		static float _get_heli_engine_health(type::vehicle vehicle) {
			return invoker::invoke<float>(0xac51915d27e4a5f7, vehicle);
		}
		static bool was_counter_activated(type::vehicle vehicle, type::any p1) {
			return invoker::invoke<bool>(0x3ec8bf18aa453fe9, vehicle, p1);
		}
		static void set_vehicle_name_debug(type::vehicle vehicle, const char* name) {
			invoker::invoke<void>(0xbfdf984e2c22b94f, vehicle, name);
		}
		static void set_vehicle_explodes_on_high_explosion_damage(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x71b0892ec081d60a, vehicle, toggle);
		}
		static void _0x3441cad2f2231923(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x3441cad2f2231923, vehicle, p1);
		}
		static void _0x2b6747faa9db9d6b(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x2b6747faa9db9d6b, vehicle, p1);
		}
		static void control_landing_gear(type::vehicle vehicle, int state) {
			invoker::invoke<void>(0xcfc8be9a5e1fe575, vehicle, state);
		}
		static int get_landing_gear_state(type::vehicle vehicle) {
			return invoker::invoke<int>(0x9b0f3dca3db0f4cd, vehicle);
		}
		static bool is_any_vehicle_near_point(float x, float y, float z, float radius) {
			return invoker::invoke<bool>(0x61e1dd6125a3eee6, x, y, z, radius);
		}
		static void request_vehicle_high_detail_model(type::vehicle vehicle) {
			invoker::invoke<void>(0xa6e9fdcb2c76785e, vehicle);
		}
		static void remove_vehicle_high_detail_model(type::vehicle vehicle) {
			invoker::invoke<void>(0x00689cde5f7c6787, vehicle);
		}
		static bool is_vehicle_high_detail(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x1f25887f3c104278, vehicle);
		}
		static void request_vehicle_asset(type::hash vehiclehash, int vehicleasset) {
			invoker::invoke<void>(0x81a15811460fab3a, vehiclehash, vehicleasset);
		}
		static bool has_vehicle_asset_loaded(int vehicleasset) {
			return invoker::invoke<bool>(0x1bbe0523b8db9a21, vehicleasset);
		}
		static void remove_vehicle_asset(int vehicleasset) {
			invoker::invoke<void>(0xace699c71ab9deb5, vehicleasset);
		}
		static void _set_tow_truck_crane_height(type::vehicle towtruck, float height) {
			invoker::invoke<void>(0xfe54b92a344583ca, towtruck, height);
		}
		static void attach_vehicle_to_tow_truck(type::vehicle towtruck, type::vehicle vehicle, int index, float hookoffsetx, float hookoffsety, float hookoffsetz) {
			invoker::invoke<void>(0x29a16f8d621c4508, towtruck, vehicle, index, hookoffsetx, hookoffsety, hookoffsetz);
		}
		static void detach_vehicle_from_tow_truck(type::vehicle towtruck, type::vehicle vehicle) {
			invoker::invoke<void>(0xc2db6b6708350ed8, towtruck, vehicle);
		}
		static bool detach_vehicle_from_any_tow_truck(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xd0e9ce05a1e68cd8, vehicle);
		}
		static bool is_vehicle_attached_to_tow_truck(type::vehicle towtruck, type::vehicle vehicle) {
			return invoker::invoke<bool>(0x146df9ec4c4b9fd4, towtruck, vehicle);
		}
		static type::entity get_entity_attached_to_tow_truck(type::vehicle towtruck) {
			return invoker::invoke<type::entity>(0xefea18dcf10f8f75, towtruck);
		}
		static type::any set_vehicle_automatically_attaches(type::vehicle vehicle, bool p1, type::any p2) {
			return invoker::invoke<type::any>(0x8ba6f76bc53a1493, vehicle, p1, p2);
		}
		static void set_vehicle_bulldozer_arm_position(type::vehicle vehicle, float position, bool p2) {
			invoker::invoke<void>(0xf8ebccc96adb9fb7, vehicle, position, p2);
		}
		static void _0x56b94c6d7127dfba(type::any p0, float p1, bool p2) {
			invoker::invoke<void>(0x56b94c6d7127dfba, p0, p1, p2);
		}
		static void _0x1093408b4b9d1146(type::any p0, float p1) {
			invoker::invoke<void>(0x1093408b4b9d1146, p0, p1);
		}
		static void _set_desired_vertical_flight_phase(type::vehicle vehicle, float angleratio) {
			invoker::invoke<void>(0x30d779de7c4f6dd3, vehicle, angleratio);
		}
		static void _set_vertical_flight_phase(type::vehicle vehicle, float angle) {
			invoker::invoke<void>(0x9aa47fff660cb932, vehicle, angle);
		}
		static bool _0xa4822f1cf23f4810(PVector3* outvec, type::any p1, PVector3* outvec1, type::any p3, type::any p4, type::any p5, type::any p6, type::any p7, type::any p8) {
			return invoker::invoke<bool>(0xa4822f1cf23f4810, outvec, p1, outvec1, p3, p4, p5, p6, p7, p8);
		}
		static void set_vehicle_burnout(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xfb8794444a7d60fb, vehicle, toggle);
		}
		static bool is_vehicle_in_burnout(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x1297a88e081430eb, vehicle);
		}
		static void set_vehicle_reduce_grip(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x222ff6a823d122e2, vehicle, toggle);
		}
		static void set_vehicle_indicator_lights(type::vehicle vehicle, int turnsignal, bool toggle) {
			invoker::invoke<void>(0xb5d45264751b7df0, vehicle, turnsignal, toggle);
		}
		static void set_vehicle_brake_lights(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x92b35082e0b42f66, vehicle, p1);
		}
		static void set_vehicle_handbrake(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x684785568ef26a22, vehicle, toggle);
		}
		static void _0x48adc8a773564670() {
			invoker::invoke<void>(0x48adc8a773564670);
		}
		static bool _0x91d6dd290888cbab() {
			return invoker::invoke<bool>(0x91d6dd290888cbab);
		}
		static void _0x51db102f4a3ba5e0(bool p0) {
			invoker::invoke<void>(0x51db102f4a3ba5e0, p0);
		}
		static bool get_vehicle_trailer_vehicle(type::vehicle vehicle, type::vehicle* trailer) {
			return invoker::invoke<bool>(0x1cdd6badc297830d, vehicle, trailer);
		}
		static void _0xcac66558b944da67(const char* vehicle, bool p1) {
			invoker::invoke<void>(0xcac66558b944da67, vehicle, p1);
		}
		static void set_vehicle_rudder_broken(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x09606148b6c71def, vehicle, toggle);
		}
		static void _0x1a78ad3d8240536f(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x1a78ad3d8240536f, vehicle, p1);
		}
		static float _get_vehicle_max_speed(type::vehicle vehicle) {
			return invoker::invoke<float>(0x53af99baa671ca47, vehicle);
		}
		static float get_vehicle_max_braking(type::vehicle vehicle) {
			return invoker::invoke<float>(0xad7e85fc227197c4, vehicle);
		}
		static float get_vehicle_max_traction(type::vehicle vehicle) {
			return invoker::invoke<float>(0xa132fb5370554db0, vehicle);
		}
		static float get_vehicle_acceleration(type::vehicle vehicle) {
			return invoker::invoke<float>(0x5dd35c8d074e57ae, vehicle);
		}
		static float _get_vehicle_model_max_speed(type::hash modelhash) {
			return invoker::invoke<float>(0xf417c2502fffed43, modelhash);
		}
		static float get_vehicle_model_max_braking(type::hash modelhash) {
			return invoker::invoke<float>(0xdc53fd41b4ed944c, modelhash);
		}
		static float _get_vehicle_model_hand_brake(type::hash modelhash) {
			return invoker::invoke<float>(0xbfba3ba79cff7ebf, modelhash);
		}
		static float get_vehicle_model_max_traction(type::hash modelhash) {
			return invoker::invoke<float>(0x539de94d44fdfd0d, modelhash);
		}
		static float get_vehicle_model_acceleration(type::hash modelhash) {
			return invoker::invoke<float>(0x8c044c5c84505b6a, modelhash);
		}
		static float _get_vehicle_model_down_force(type::hash modelhash) {
			return invoker::invoke<float>(0x53409b5163d5b846, modelhash);
		}
		static float _get_vehicle_model_max_knots(type::hash modelhash) {
			return invoker::invoke<float>(0xc6ad107ddc9054cc, modelhash);
		}
		static float _get_vehicle_model_move_resistance(type::hash modelhash) {
			return invoker::invoke<float>(0x5aa3f878a178c4fc, modelhash);
		}
		static float _get_vehicle_class_max_speed(int vehicleclass) {
			return invoker::invoke<float>(0x00c09f246abedd82, vehicleclass);
		}
		static float get_vehicle_class_max_traction(int vehicleclass) {
			return invoker::invoke<float>(0xdbc86d85c5059461, vehicleclass);
		}
		static float get_vehicle_class_max_agility(int vehicleclass) {
			return invoker::invoke<float>(0x4f930ad022d6de3b, vehicleclass);
		}
		static float get_vehicle_class_max_acceleration(int vehicleclass) {
			return invoker::invoke<float>(0x2f83e7e45d9ea7ae, vehicleclass);
		}
		static float get_vehicle_class_max_braking(int vehicleclass) {
			return invoker::invoke<float>(0x4bf54c16ec8fec03, vehicleclass);
		}
		static int _add_speed_zone_for_coord(float x, float y, float z, float radius, float speed, bool p5) {
			return invoker::invoke<int>(0x2ce544c68fb812a0, x, y, z, radius, speed, p5);
		}
		static bool _remove_speed_zone(int speedzone) {
			return invoker::invoke<bool>(0x1033371fc8e842a7, speedzone);
		}
		static void open_bomb_bay_doors(type::vehicle vehicle) {
			invoker::invoke<void>(0x87e7f24270732cb1, vehicle);
		}
		static void close_bomb_bay_doors(type::vehicle vehicle) {
			invoker::invoke<void>(0x3556041742a0dc74, vehicle);
		}
		static bool is_vehicle_searchlight_on(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xc0f97fce55094987, vehicle);
		}
		static void set_vehicle_searchlight(type::vehicle heli, bool toggle, bool canbeusedbyai) {
			invoker::invoke<void>(0x14e85c5ee7a4d542, heli, toggle, canbeusedbyai);
		}
		static bool _0x639431e895b9aa57(type::ped ped, type::vehicle vehicle, bool p2, bool p3, bool p4) {
			return invoker::invoke<bool>(0x639431e895b9aa57, ped, vehicle, p2, p3, p4);
		}
		static bool can_shuffle_seat(type::vehicle vehicle, type::ped ped) {
			return invoker::invoke<bool>(0x30785d90c956bf35, vehicle, ped);
		}
		static int get_num_mod_kits(type::vehicle vehicle) {
			return invoker::invoke<int>(0x33f2e3fe70eaae1d, vehicle);
		}
		static void set_vehicle_mod_kit(type::vehicle vehicle, int modkit) {
			invoker::invoke<void>(0x1f2aa07f00b3217a, vehicle, modkit);
		}
		static int get_vehicle_mod_kit(type::vehicle vehicle) {
			return invoker::invoke<int>(0x6325d1a044ae510d, vehicle);
		}
		static int get_vehicle_mod_kit_type(type::vehicle vehicle) {
			return invoker::invoke<int>(0xfc058f5121e54c32, vehicle);
		}
		static int get_vehicle_wheel_type(type::vehicle vehicle) {
			return invoker::invoke<int>(0xb3ed1bfb4be636dc, vehicle);
		}
		static void set_vehicle_wheel_type(type::vehicle vehicle, int wheeltype) {
			invoker::invoke<void>(0x487eb21cc7295ba1, vehicle, wheeltype);
		}
		static int get_num_mod_colors(int p0, bool p1) {
			return invoker::invoke<int>(0xa551be18c11a476d, p0, p1);
		}
		static void set_vehicle_mod_color_1(type::vehicle vehicle, int painttype, int color, int p3) {
			invoker::invoke<void>(0x43feb945ee7f85b8, vehicle, painttype, color, p3);
		}
		static void set_vehicle_mod_color_2(type::vehicle vehicle, int painttype, int color) {
			invoker::invoke<void>(0x816562badfdec83e, vehicle, painttype, color);
		}
		static void get_vehicle_mod_color_1(type::vehicle vehicle, int* painttype, int* color, int* pearlescentcolor) {
			invoker::invoke<void>(0xe8d65ca700c9a693, vehicle, painttype, color, pearlescentcolor);
		}
		static void get_vehicle_mod_color_2(type::vehicle vehicle, int* painttype, int* color) {
			invoker::invoke<void>(0x81592be4e3878728, vehicle, painttype, color);
		}
		static const char* get_vehicle_mod_color_1_name(type::vehicle vehicle, bool p1) {
			return invoker::invoke<const char*>(0xb45085b721efd38c, vehicle, p1);
		}
		static const char* get_vehicle_mod_color_2_name(type::vehicle vehicle) {
			return invoker::invoke<const char*>(0x4967a516ed23a5a1, vehicle);
		}
		static bool _is_vehicle_mod_load_done(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x9a83f5f9963775ef, vehicle);
		}
		static void set_vehicle_mod(type::vehicle vehicle, int modtype, int modindex, bool customtires) {
			invoker::invoke<void>(0x6af0636ddedcb6dd, vehicle, modtype, modindex, customtires);
		}
		static int get_vehicle_mod(type::vehicle vehicle, int modtype) {
			return invoker::invoke<int>(0x772960298da26fdb, vehicle, modtype);
		}
		static bool get_vehicle_mod_variation(type::vehicle vehicle, int modtype) {
			return invoker::invoke<bool>(0xb3924ecd70e095dc, vehicle, modtype);
		}
		static int get_num_vehicle_mods(type::vehicle vehicle, int modtype) {
			return invoker::invoke<int>(0xe38e9162a2500646, vehicle, modtype);
		}
		static void remove_vehicle_mod(type::vehicle vehicle, int modtype) {
			invoker::invoke<void>(0x92d619e420858204, vehicle, modtype);
		}
		static void toggle_vehicle_mod(type::vehicle vehicle, int modtype, bool toggle) {
			invoker::invoke<void>(0x2a1f4f37f95bad08, vehicle, modtype, toggle);
		}
		static bool is_toggle_mod_on(type::vehicle vehicle, int modtype) {
			return invoker::invoke<bool>(0x84b233a8c8fc8ae7, vehicle, modtype);
		}
		static const char* get_mod_text_label(type::vehicle vehicle, int modtype, int modvalue) {
			return invoker::invoke<const char*>(0x8935624f8c5592cc, vehicle, modtype, modvalue);
		}
		static const char* get_mod_slot_name(type::vehicle vehicle, int modtype) {
			return invoker::invoke<const char*>(0x51f0feb9f6ae98c0, vehicle, modtype);
		}
		static const char* get_livery_name(type::vehicle vehicle, int liveryindex) {
			return invoker::invoke<const char*>(0xb4c7a93837c91a1f, vehicle, liveryindex);
		}
		static float get_vehicle_mod_modifier_value(type::vehicle vehicle, int modtype, int modindex) {
			return invoker::invoke<float>(0x90a38e9838e0a8c1, vehicle, modtype, modindex);
		}
		static type::any _get_vehicle_mod_data(type::vehicle vehicle, int modtype, int modindex) {
			return invoker::invoke<type::any>(0x4593cf82aa179706, vehicle, modtype, modindex);
		}
		static void preload_vehicle_mod(type::any p0, int modtype, type::any p2) {
			invoker::invoke<void>(0x758f49c24925568a, p0, modtype, p2);
		}
		static bool has_preload_mods_finished(type::any p0) {
			return invoker::invoke<bool>(0x06f43e5175eb6d96, p0);
		}
		static void release_preload_mods(type::vehicle vehicle) {
			invoker::invoke<void>(0x445d79f995508307, vehicle);
		}
		static void set_vehicle_tyre_smoke_color(type::vehicle vehicle, int r, int g, int b) {
			invoker::invoke<void>(0xb5ba80f839791c0f, vehicle, r, g, b);
		}
		static void get_vehicle_tyre_smoke_color(type::vehicle vehicle, int* r, int* g, int* b) {
			invoker::invoke<void>(0xb635392a4938b3c3, vehicle, r, g, b);
		}
		static void set_vehicle_window_tint(type::vehicle vehicle, int tint) {
			invoker::invoke<void>(0x57c51e6bad752696, vehicle, tint);
		}
		static int get_vehicle_window_tint(type::vehicle vehicle) {
			return invoker::invoke<int>(0x0ee21293dad47c95, vehicle);
		}
		static int get_num_vehicle_window_tints() {
			return invoker::invoke<int>(0x9d1224004b3a6707);
		}
		static void get_vehicle_color(type::vehicle vehicle, int* r, int* g, int* b) {
			invoker::invoke<void>(0xf3cc740d36221548, vehicle, r, g, b);
		}
		static int _0xeebfc7a7efdc35b4(type::vehicle vehicle) {
			return invoker::invoke<int>(0xeebfc7a7efdc35b4, vehicle);
		}
		static type::hash get_vehicle_cause_of_destruction(type::vehicle vehicle) {
			return invoker::invoke<type::hash>(0xe495d1ef4c91fd20, vehicle);
		}
		static void set_vehicle_xenon_lights_colour(type::vehicle vehicle, int color) {
			return invoker::invoke<void>(0xE41033B25D003A07, vehicle, color);
		}
		static int get_vehicle_xenon_lights_colour(type::vehicle vehicle) {
			return invoker::invoke<int>(0x3DFF319A831E0CDB, vehicle);
		}
		static bool get_is_left_vehicle_headlight_damaged(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x5ef77c9add3b11a3, vehicle);
		}
		static bool get_is_right_vehicle_headlight_damaged(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xa7ecb73355eb2f20, vehicle);
		}
		static void _set_vehicle_engine_power_multiplier(type::vehicle vehicle, float value) {
			invoker::invoke<void>(0x93a3996368c94158, vehicle, value);
		}
		static void _set_vehicle_st(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x1cf38d529d7441d9, vehicle, toggle);
		}
		static void _0x1f9fb66f3a3842d2(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x1f9fb66f3a3842d2, vehicle, p1);
		}
		static type::any _0x54b0f614960f4a5f(float p0, float p1, float p2, float p3, float p4, float p5, float p6) {
			return invoker::invoke<type::any>(0x54b0f614960f4a5f, p0, p1, p2, p3, p4, p5, p6);
		}
		static void _0xe30524e1871f481d(type::any p0) {
			invoker::invoke<void>(0xe30524e1871f481d, p0);
		}
		static bool _is_any_passenger_rappelling(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x291e373d483e7ee7, vehicle);
		}
		static void _set_vehicle_engine_torque_multiplier(type::vehicle vehicle, float value) {
			invoker::invoke<void>(0xb59e4bd37ae292db, vehicle, value);
		}
		static void _0x0ad9e8f87ff7c16f(type::any p0, bool p1) {
			invoker::invoke<void>(0x0ad9e8f87ff7c16f, p0, p1);
		}
		static void set_vehicle_is_wanted(type::vehicle vehicle, bool state) {
			invoker::invoke<void>(0xf7ec25a3ebeec726, vehicle, state);
		}
		static void _0xf488c566413b4232(type::any p0, float p1) {
			invoker::invoke<void>(0xf488c566413b4232, p0, p1);
		}
		static void _0xc1f981a6f74f0c23(type::any p0, bool p1) {
			invoker::invoke<void>(0xc1f981a6f74f0c23, p0, p1);
		}
		static void _0x0f3b4d4e43177236(type::any p0, bool p1) {
			invoker::invoke<void>(0x0f3b4d4e43177236, p0, p1);
		}
		static float _0x6636c535f6cc2725(type::vehicle vehicle) {
			return invoker::invoke<float>(0x6636c535f6cc2725, vehicle);
		}
		static void disable_plane_aileron(type::vehicle vehicle, bool p1, bool p2) {
			invoker::invoke<void>(0x23428fc53c60919c, vehicle, p1, p2);
		}
		static bool get_is_vehicle_engine_running(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xae31e7df9b5b132e, vehicle);
		}
		static void _0x1d97d1e3a70a649f(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x1d97d1e3a70a649f, vehicle, p1);
		}
		static void _set_bike_lean_angle(type::vehicle vehicle, float x, float y) {
			invoker::invoke<void>(0x9cfa4896c3a53cbb, vehicle, x, y);
		}
		static void _0xab04325045427aae(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xab04325045427aae, vehicle, p1);
		}
		static void _0xcfd778e7904c255e(type::vehicle vehicle) {
			invoker::invoke<void>(0xcfd778e7904c255e, vehicle);
		}
		static void set_last_driven_vehicle(type::vehicle vehicle) {
			invoker::invoke<void>(0xacfb2463cc22bed2, vehicle);
		}
		static type::vehicle get_last_driven_vehicle() {
			return invoker::invoke<type::vehicle>(0xb2d06faede65b577);
		}
		static void _0xe01903c47c7ac89e() {
			invoker::invoke<void>(0xe01903c47c7ac89e);
		}
		static void _0x02398b627547189c(type::vehicle p0, bool p1) {
			invoker::invoke<void>(0x02398b627547189c, p0, p1);
		}
		static void _set_plane_min_height_above_terrain(type::vehicle plane, int height) {
			invoker::invoke<void>(0xb893215d8d4c015b, plane, height);
		}
		static void set_vehicle_lod_multiplier(type::vehicle vehicle, float multiplier) {
			invoker::invoke<void>(0x93ae6a61be015bf1, vehicle, multiplier);
		}
		static void _0x428baccdf5e26ead(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x428baccdf5e26ead, vehicle, p1);
		}
		static type::any _0x42a4beb35d372407(type::any p0) {
			return invoker::invoke<type::any>(0x42a4beb35d372407, p0);
		}
		static type::any _0x2c8cbfe1ea5fc631(type::any p0) {
			return invoker::invoke<type::any>(0x2c8cbfe1ea5fc631, p0);
		}
		static void _0x4d9d109f63fee1d4(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x4d9d109f63fee1d4, vehicle, toggle);
		}
		static void _0x279d50de5652d935(type::any p0, bool p1) {
			invoker::invoke<void>(0x279d50de5652d935, p0, p1);
		}
		static void _0xe44a982368a4af23(type::vehicle vehicle, type::vehicle vehicle2) {
			invoker::invoke<void>(0xe44a982368a4af23, vehicle, vehicle2);
		}
		static void _0xf25e02cb9c5818f8() {
			invoker::invoke<void>(0xf25e02cb9c5818f8);
		}
		static void _0xbc3cca5844452b06(float p0) {
			invoker::invoke<void>(0xbc3cca5844452b06, p0);
		}
		static void set_vehicle_shoot_at_target(type::ped driver, type::entity entity, float xtarget, float ytarget, float ztarget) {
			invoker::invoke<void>(0x74cd9a9327a282ea, driver, entity, xtarget, ytarget, ztarget);
		}
		static bool _get_vehicle_owner(type::vehicle vehicle, type::entity* entity) {
			return invoker::invoke<bool>(0x8f5ebab1f260cfce, vehicle, entity);
		}
		static void set_force_hd_vehicle(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x97ce68cb032583f0, vehicle, toggle);
		}
		static void _0x182f266c2d9e2beb(type::vehicle vehicle, float p1) {
			invoker::invoke<void>(0x182f266c2d9e2beb, vehicle, p1);
		}
		static int get_vehicle_plate_type(type::vehicle vehicle) {
			return invoker::invoke<int>(0x9ccc9525bf2408e0, vehicle);
		}
		static void track_vehicle_visibility(type::vehicle vehicle) {
			invoker::invoke<void>(0x64473aefdcf47dca, vehicle);
		}
		static bool is_vehicle_visible(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xaa0a52d24fb98293, vehicle);
		}
		static void set_vehicle_gravity(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x89f149b6131e57da, vehicle, toggle);
		}
		static void _0xe6c0c80b8c867537(bool p0) {
			invoker::invoke<void>(0xe6c0c80b8c867537, p0);
		}
		static type::any _0x36492c2f0d134c56(type::any p0) {
			return invoker::invoke<type::any>(0x36492c2f0d134c56, p0);
		}
		static void _0x06582aff74894c75(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x06582aff74894c75, vehicle, p1);
		}
		static void _0xdffcef48e511db48(type::any p0, bool p1) {
			invoker::invoke<void>(0xdffcef48e511db48, p0, p1);
		}
		static bool _is_vehicle_shop_respray_allowed(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x8d474c8faeff6cde, vehicle);
		}
		static void set_vehicle_engine_can_degrade(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x983765856f2564f9, vehicle, toggle);
		}
		static void _0xf0e4ba16d1db546c(type::vehicle vehicle, int p1, int p2) {
			invoker::invoke<void>(0xf0e4ba16d1db546c, vehicle, p1, p2);
		}
		static void _0xf87d9f2301f7d206(type::any p0) {
			invoker::invoke<void>(0xf87d9f2301f7d206, p0);
		}
		static bool _vehicle_has_landing_gear(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x4198ab0022b15f87, vehicle);
		}
		static bool _are_propellers_undamaged(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x755d6d5267cbbd7e, vehicle);
		}
		static void _0x0cdda42f9e360ca6(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x0cdda42f9e360ca6, vehicle, toggle);
		}
		static bool is_vehicle_stolen(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x4af9bd80eebeb453, vehicle);
		}
		static void set_vehicle_is_stolen(type::vehicle vehicle, bool isstolen) {
			invoker::invoke<void>(0x67b2c79aa7ff5738, vehicle, isstolen);
		}
		static void set_plane_turbulence_multiplier(type::vehicle plane, float multiplier) {
			invoker::invoke<void>(0xad2d28a1afdff131, plane, multiplier);
		}
		static bool _are_vehicle_wings_intact(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x5991a01434ce9677, vehicle);
		}
		static void _0xb264c4d2f2b0a78b(type::vehicle vehicle) {
			invoker::invoke<void>(0xb264c4d2f2b0a78b, vehicle);
		}
		static void detach_vehicle_from_cargobob(type::vehicle vehicle, type::vehicle cargobob) {
			invoker::invoke<void>(0x0e21d3df1051399d, vehicle, cargobob);
		}
		static bool detach_vehicle_from_any_cargobob(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xadf7be450512c12f, vehicle);
		}
		static bool is_vehicle_attached_to_cargobob(type::vehicle cargobob, type::vehicle vehicleattached) {
			return invoker::invoke<bool>(0xd40148f22e81a1d9, cargobob, vehicleattached);
		}
		static type::vehicle get_vehicle_attached_to_cargobob(type::vehicle cargobob) {
			return invoker::invoke<type::vehicle>(0x873b82d42ac2b9e5, cargobob);
		}
		static void attach_vehicle_to_cargobob(type::vehicle vehicle, type::vehicle cargobob, int p2, float x, float y, float z) {
			invoker::invoke<void>(0x4127f1d84e347769, vehicle, cargobob, p2, x, y, z);
		}
		static void _0x571feb383f629926(type::vehicle cargobob, bool p1) {
			invoker::invoke<void>(0x571feb383f629926, cargobob, p1);
		}
		static PVector3 _get_cargobob_hook_position(type::vehicle cargobob) {
			return invoker::invoke<PVector3>(0xcbdb9b923cacc92d, cargobob);
		}
		static bool does_cargobob_have_pick_up_rope(type::vehicle cargobob) {
			return invoker::invoke<bool>(0x1821d91ad4b56108, cargobob);
		}
		static void create_pick_up_rope_for_cargobob(type::vehicle cargobob, int state) {
			invoker::invoke<void>(0x7beb0c7a235f6f3b, cargobob, state);
		}
		static void remove_pick_up_rope_for_cargobob(type::vehicle cargobob) {
			invoker::invoke<void>(0x9768cf648f54c804, cargobob);
		}
		static void _set_cargobob_hook_position(type::vehicle cargobob, float xoffset, float yoffset, int state) {
			invoker::invoke<void>(0x877c1eaeac531023, cargobob, xoffset, yoffset, state);
		}
		static void _0xcf1182f682f65307(type::any p0, type::player p1) {
			invoker::invoke<void>(0xcf1182f682f65307, p0, p1);
		}
		static bool _does_cargobob_have_pickup_magnet(type::vehicle cargobob) {
			return invoker::invoke<bool>(0x6e08bf5b3722bac9, cargobob);
		}
		static void _set_cargobob_pickup_magnet_active(type::vehicle cargobob, bool isactive) {
			invoker::invoke<void>(0x9a665550f8da349b, cargobob, isactive);
		}
		static void _set_cargobob_pickup_magnet_strength(type::vehicle cargobob, float strength) {
			invoker::invoke<void>(0xbcbfcd9d1dac19e2, cargobob, strength);
		}
		static void _0xa17bad153b51547e(type::vehicle cargobob, float p1) {
			invoker::invoke<void>(0xa17bad153b51547e, cargobob, p1);
		}
		static void _0x66979acf5102fd2f(type::vehicle cargobob, float p1) {
			invoker::invoke<void>(0x66979acf5102fd2f, cargobob, p1);
		}
		static void _0x6d8eac07506291fb(type::vehicle cargobob, float p1) {
			invoker::invoke<void>(0x6d8eac07506291fb, cargobob, p1);
		}
		static void _0xed8286f71a819baa(type::vehicle cargobob, float p1) {
			invoker::invoke<void>(0xed8286f71a819baa, cargobob, p1);
		}
		static void _0x685d5561680d088b(type::vehicle vehicle, float p1) {
			invoker::invoke<void>(0x685d5561680d088b, vehicle, p1);
		}
		static void _0xe301bd63e9e13cf0(type::vehicle vehicle, type::vehicle cargobob) {
			invoker::invoke<void>(0xe301bd63e9e13cf0, vehicle, cargobob);
		}
		static void _0x9bddc73cc6a115d4(type::vehicle vehicle, bool p1, bool p2) {
			invoker::invoke<void>(0x9bddc73cc6a115d4, vehicle, p1, p2);
		}
		static void _0x56eb5e94318d3fb6(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x56eb5e94318d3fb6, vehicle, p1);
		}
		static bool does_vehicle_have_weapons(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x25ecb9f8017d98e0, vehicle);
		}
		static void _0x2c4a1590abf43e8b(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x2c4a1590abf43e8b, vehicle, p1);
		}
		static void disable_vehicle_weapon(bool disabled, type::hash weaponhash, type::vehicle vehicle, type::ped owner) {
			invoker::invoke<void>(0xf4fc6a6f67d8d856, disabled, weaponhash, vehicle, owner);
		}
		static void _0xe05dd0e9707003a3(type::any p0, bool p1) {
			invoker::invoke<void>(0xe05dd0e9707003a3, p0, p1);
		}
		static void _0x21115bcd6e44656a(type::any p0, bool p1) {
			invoker::invoke<void>(0x21115bcd6e44656a, p0, p1);
		}
		static int get_vehicle_class(type::vehicle vehicle) {
			return invoker::invoke<int>(0x29439776aaa00a62, vehicle);
		}
		static int get_vehicle_class_from_name(type::hash modelhash) {
			return invoker::invoke<int>(0xdedf1c8bd47c2200, modelhash);
		}
		static void set_players_last_vehicle(type::vehicle vehicle) {
			invoker::invoke<void>(0xbcdf8baf56c87b6a, vehicle);
		}
		static void set_vehicle_can_be_used_by_fleeing_peds(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x300504b23bd3b711, vehicle, toggle);
		}
		static void _0xe5810ac70602f2f5(type::vehicle vehicle, float p1) {
			invoker::invoke<void>(0xe5810ac70602f2f5, vehicle, p1);
		}
		static void _set_vehicle_creates_money_pickups_when_exploded(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x068f64f2470f9656, vehicle, toggle);
		}
		static void _set_vehicle_jet_engine_on(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xb8fbc8b1330ca9b4, vehicle, toggle);
		}
		static void _0x10655fab9915623d(type::any p0, type::any p1) {
			invoker::invoke<void>(0x10655fab9915623d, p0, p1);
		}
		static void _0x79df7e806202ce01(type::any p0, type::any p1) {
			invoker::invoke<void>(0x79df7e806202ce01, p0, p1);
		}
		static void _0x9007a2f21dc108d4(type::any p0, float p1) {
			invoker::invoke<void>(0x9007a2f21dc108d4, p0, p1);
		}
		static void _set_helicopter_roll_pitch_yaw_mult_health(type::vehicle helicopter, float multiplier) {
			invoker::invoke<void>(0x6e0859b530a365cc, helicopter, multiplier);
		}
		static void set_vehicle_friction_override(type::vehicle vehicle, float friction) {
			invoker::invoke<void>(0x1837af7c627009ba, vehicle, friction);
		}
		static void set_vehicle_wheels_can_break_off_when_blow_up(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xa37b9a517b133349, vehicle, toggle);
		}
		static bool _0xf78f94d60248c737(type::any p0, bool p1) {
			return invoker::invoke<bool>(0xf78f94d60248c737, p0, p1);
		}
		static void set_vehicle_ceiling_height(type::vehicle vehicle, float p1) {
			invoker::invoke<void>(0xa46413066687a328, vehicle, p1);
		}
		static void _0x5e569ec46ec21cae(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x5e569ec46ec21cae, vehicle, toggle);
		}
		static void _0x6d6af961b72728ae(type::vehicle vehicle) {
			invoker::invoke<void>(0x6d6af961b72728ae, vehicle);
		}
		static bool does_vehicle_exist_with_decorator(const char* decorator) {
			return invoker::invoke<bool>(0x956b409b984d9bf7, decorator);
		}
		static void set_vehicle_exclusive_driver(type::vehicle vehicle, type::ped ped) {
			invoker::invoke<void>(0x41062318f23ed854, vehicle, ped);
		}
		static void _set_vehicle_exclusive_driver_2(type::vehicle vehicle, type::ped ped, int p2) {
			invoker::invoke<void>(0xb5c51b5502e85e83, vehicle, ped, p2);
		}
		static void _0x500873a45724c863(type::vehicle vehicle, type::any p1) {
			invoker::invoke<void>(0x500873a45724c863, vehicle, p1);
		}
		static void _0xb055a34527cb8fd7(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xb055a34527cb8fd7, vehicle, p1);
		}
		static void set_distant_cars_enabled(bool toggle) {
			invoker::invoke<void>(0xf796359a959df65d, toggle);
		}
		static void _set_vehicle_neon_lights_colour(type::vehicle vehicle, int r, int g, int b) {
			invoker::invoke<void>(0x8e0a582209a62695, vehicle, r, g, b);
		}
		static void _get_vehicle_neon_lights_colour(type::vehicle vehicle, int* r, int* g, int* b) {
			invoker::invoke<void>(0x7619eee8c886757f, vehicle, r, g, b);
		}
		static void _set_vehicle_neon_light_enabled(type::vehicle vehicle, int index, bool toggle) {
			invoker::invoke<void>(0x2aa720e4287bf269, vehicle, index, toggle);
		}
		static bool _is_vehicle_neon_light_enabled(type::vehicle vehicle, int index) {
			return invoker::invoke<bool>(0x8c4b92553e4766a5, vehicle, index);
		}
		static void _0x35e0654f4bad7971(bool p0) {
			invoker::invoke<void>(0x35e0654f4bad7971, p0);
		}
		static void _0xb088e9a47ae6edd5(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xb088e9a47ae6edd5, vehicle, p1);
		}
		static void _request_vehicle_scaleform_movie(type::vehicle vehicle) {
			invoker::invoke<void>(0xdba3c090e3d74690, vehicle);
		}
		static float get_vehicle_body_health(type::vehicle vehicle) {
			return invoker::invoke<float>(0xf271147eb7b40f12, vehicle);
		}
		static void set_vehicle_body_health(type::vehicle vehicle, float value) {
			invoker::invoke<void>(0xb77d05ac8c78aadb, vehicle, value);
		}
		static void _0xdf7e3eeb29642c38(type::vehicle vehicle, PVector3* out1, PVector3* out2) {
			invoker::invoke<void>(0xdf7e3eeb29642c38, vehicle, out1, out2);
		}
		static float _get_vehicle_suspension_height(type::vehicle vehicle) {
			return invoker::invoke<float>(0x53952fd2baa19f17, vehicle);
		}
		static void _set_car_high_speed_bump_severity_multiplier(float multiplier) {
			invoker::invoke<void>(0x84fd40f56075e816, multiplier);
		}
		static void _0xa7dcdf4ded40a8f4(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xa7dcdf4ded40a8f4, vehicle, p1);
		}
		static float _get_vehicle_body_health_2(type::vehicle vehicle) {
			return invoker::invoke<float>(0xb8ef61207c2393a9, vehicle);
		}
		static bool _0xd4c4642cb7f50b5d(type::vehicle vehicle) {
			return invoker::invoke<bool>(0xd4c4642cb7f50b5d, vehicle);
		}
		static void _0xc361aa040d6637a8(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xc361aa040d6637a8, vehicle, p1);
		}
		static void _set_vehicle_hud_special_ability_bar_active(type::vehicle vehicle, bool active) {
			invoker::invoke<void>(0x99c82f8a139f3e4e, vehicle, active);
		}
		static void _0xe16142b94664defd(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0xe16142b94664defd, vehicle, p1);
		}
	}

	namespace object {
		static type::object create_object(type::object modelhash, float x, float y, float z, bool isnetwork, bool thisscriptcheck, bool dynamic) {
			return invoker::invoke<type::object>(0x509d5878eb39e842, modelhash, x, y, z, isnetwork, thisscriptcheck, dynamic);
		}
		static float create_object_no_offset(int* modelfwgahash, float x, float y, float z, bool isnetwork, bool thisscriptcheck, bool dynamic) {
			return invoker::invoke<float>(0x9a294b2138abb884, modelfwgahash, x, y, z, isnetwork, thisscriptcheck, dynamic);
		}
		static void delete_object(type::object* object) {
			invoker::invoke<void>(0x539e0ae3e6634b9f, object);
		}
		static bool place_object_on_ground_properly(type::object object) {
			return invoker::invoke<bool>(0x58a850eaee20faa3, object);
		}
		static bool slide_object(type::object object, float tox, float toy, float toz, float speedx, float speedy, float speedz, bool collision) {
			return invoker::invoke<bool>(0x2fdff4107b8c1147, object, tox, toy, toz, speedx, speedy, speedz, collision);
		}
		static void set_object_targettable(type::object object, bool targettable) {
			invoker::invoke<void>(0x8a7391690f5afd81, object, targettable);
		}
		static void _set_object_lod(type::object object, bool toggle) {
			invoker::invoke<void>(0x77f33f2ccf64b3aa, object, toggle);
		}
		static type::object get_closest_object_of_type(float x, float y, float z, float radius, type::hash modelhash, bool ismission, bool p6, bool p7) {
			return invoker::invoke<type::object>(0xe143fa2249364369, x, y, z, radius, modelhash, ismission, p6, p7);
		}
		static bool has_object_been_broken(type::object object) {
			return invoker::invoke<bool>(0x8abfb70c49cc43e2, object);
		}
		static bool has_closest_object_of_type_been_broken(float p0, float p1, float p2, float p3, type::hash modelhash, type::any p5) {
			return invoker::invoke<bool>(0x761b0e69ac4d007e, p0, p1, p2, p3, modelhash, p5);
		}
		static bool _0x46494a2475701343(float p0, float p1, float p2, float p3, type::hash modelhash, bool p5) {
			return invoker::invoke<bool>(0x46494a2475701343, p0, p1, p2, p3, modelhash, p5);
		}
		static PVector3 _get_object_offset_from_coords(float xpos, float ypos, float zpos, float heading, float xoffset, float yoffset, float zoffset) {
			return invoker::invoke<PVector3>(0x163e252de035a133, xpos, ypos, zpos, heading, xoffset, yoffset, zoffset);
		}
		static type::any _0x163f8b586bc95f2a(type::any coords, float radius, type::hash modelhash, float x, float y, float z, PVector3* p6, int p7) {
			return invoker::invoke<type::any>(0x163f8b586bc95f2a, coords, radius, modelhash, x, y, z, p6, p7);
		}
		static void set_state_of_closest_door_of_type(type::hash type, float x, float y, float z, bool locked, float heading, bool p6) {
			invoker::invoke<void>(0xf82d8f1926a02c3d, type, x, y, z, locked, heading, p6);
		}
		static void get_state_of_closest_door_of_type(type::hash type, float x, float y, float z, bool* locked, float* heading) {
			invoker::invoke<void>(0xedc1a5b84aef33ff, type, x, y, z, locked, heading);
		}
		static void _door_control(type::hash doorhash, float x, float y, float z, bool locked, float xrotmult, float yrotmult, float zrotmult) {
			invoker::invoke<void>(0x9b12f9a24fabedb0, doorhash, x, y, z, locked, xrotmult, yrotmult, zrotmult);
		}
		static void add_door_to_system(type::hash doorhash, type::hash modelhash, float x, float y, float z, bool p5, bool p6, bool p7) {
			invoker::invoke<void>(0x6f8838d03d1dc226, doorhash, modelhash, x, y, z, p5, p6, p7);
		}
		static void remove_door_from_system(type::hash doorhash) {
			invoker::invoke<void>(0x464d8e1427156fe4, doorhash);
		}
		static void _set_door_acceleration_limit(type::hash doorhash, int limit, bool p2, bool p3) {
			invoker::invoke<void>(0x6bab9442830c7f53, doorhash, limit, p2, p3);
		}
		static int _0x160aa1b32f6139b8(type::hash doorhash) {
			return invoker::invoke<int>(0x160aa1b32f6139b8, doorhash);
		}
		static int _0x4bc2854478f3a749(type::hash doorhash) {
			return invoker::invoke<int>(0x4bc2854478f3a749, doorhash);
		}
		static void _0x03c27e13b42a0e82(type::hash doorhash, float p1, bool p2, bool p3) {
			invoker::invoke<void>(0x03c27e13b42a0e82, doorhash, p1, p2, p3);
		}
		static void _0x9ba001cb45cbf627(type::hash doorhash, float heading, bool p2, bool p3) {
			invoker::invoke<void>(0x9ba001cb45cbf627, doorhash, heading, p2, p3);
		}
		static void _set_door_ajar_angle(type::hash doorhash, float ajar, bool p2, bool p3) {
			invoker::invoke<void>(0xb6e6fba95c7324ac, doorhash, ajar, p2, p3);
		}
		static float _0x65499865fca6e5ec(type::hash doorhash) {
			return invoker::invoke<float>(0x65499865fca6e5ec, doorhash);
		}
		static void _0xc485e07e4f0b7958(type::hash doorhash, bool p1, bool p2, bool p3) {
			invoker::invoke<void>(0xc485e07e4f0b7958, doorhash, p1, p2, p3);
		}
		static void _0xd9b71952f78a2640(type::hash doorhash, bool p1) {
			invoker::invoke<void>(0xd9b71952f78a2640, doorhash, p1);
		}
		static void _0xa85a21582451e951(type::hash doorhash, bool p1) {
			invoker::invoke<void>(0xa85a21582451e951, doorhash, p1);
		}
		static bool _does_door_exist(type::hash doorhash) {
			return invoker::invoke<bool>(0xc153c43ea202c8c1, doorhash);
		}
		static bool is_door_closed(type::hash door) {
			return invoker::invoke<bool>(0xc531ee8a1145a149, door);
		}
		static void _0xc7f29ca00f46350e(bool p0) {
			invoker::invoke<void>(0xc7f29ca00f46350e, p0);
		}
		static void _0x701fda1e82076ba4() {
			invoker::invoke<void>(0x701fda1e82076ba4);
		}
		static bool _0xdf97cdd4fc08fd34(type::any p0) {
			return invoker::invoke<bool>(0xdf97cdd4fc08fd34, p0);
		}
		static bool _0x589f80b325cc82c5(float p0, float p1, float p2, type::any p3, type::any* p4) {
			return invoker::invoke<bool>(0x589f80b325cc82c5, p0, p1, p2, p3, p4);
		}
		static bool is_garage_empty(type::any garage, bool p1, int p2) {
			return invoker::invoke<bool>(0x90e47239ea1980b8, garage, p1, p2);
		}
		static bool _0x024a60deb0ea69f0(type::any p0, type::player player, float p2, int p3) {
			return invoker::invoke<bool>(0x024a60deb0ea69f0, p0, player, p2, p3);
		}
		static bool _0x1761dc5d8471cbaa(type::any p0, type::player player, int p2) {
			return invoker::invoke<bool>(0x1761dc5d8471cbaa, p0, player, p2);
		}
		static bool _0x85b6c850546fdde2(type::any p0, bool p1, bool p2, bool p3, type::any p4) {
			return invoker::invoke<bool>(0x85b6c850546fdde2, p0, p1, p2, p3, p4);
		}
		static bool _0x673ed815d6e323b7(type::any p0, bool p1, bool p2, bool p3, type::any p4) {
			return invoker::invoke<bool>(0x673ed815d6e323b7, p0, p1, p2, p3, p4);
		}
		static bool _0x372ef6699146a1e4(type::any p0, type::entity entity, float p2, int p3) {
			return invoker::invoke<bool>(0x372ef6699146a1e4, p0, entity, p2, p3);
		}
		static bool _0xf0eed5a6bc7b237a(type::any p0, type::entity entity, int p2) {
			return invoker::invoke<bool>(0xf0eed5a6bc7b237a, p0, entity, p2);
		}
		static void _0x190428512b240692(type::any p0, bool p1, bool p2, bool p3, bool p4) {
			invoker::invoke<void>(0x190428512b240692, p0, p1, p2, p3, p4);
		}
		static void _0xf2e1a7133dd356a6(type::hash hash, bool toggle) {
			invoker::invoke<void>(0xf2e1a7133dd356a6, hash, toggle);
		}
		static void _0x66a49d021870fe88() {
			invoker::invoke<void>(0x66a49d021870fe88);
		}
		static bool does_object_of_type_exist_at_coords(float x, float y, float z, float radius, type::hash hash, bool p5) {
			return invoker::invoke<bool>(0xbfa48e2ff417213f, x, y, z, radius, hash, p5);
		}
		static bool is_point_in_angled_area(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, bool p10, bool p11) {
			return invoker::invoke<bool>(0x2a70bae8883e4c81, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
		}
		static void _0x4d89d607cb3dd1d2(type::object object, bool toggle) {
			invoker::invoke<void>(0x4d89d607cb3dd1d2, object, toggle);
		}
		static void set_object_physics_params(type::object object, float mass, float gravityfactor, float dampinglinearc, float dampinglinearv, float dampinglinearv2, float dampingangularc, float dampingangularv, float dampingangularv2, float margin, float default2pi, float buoyancyfactor) {
			invoker::invoke<void>(0xf6df6e90de7df90f, object, mass, gravityfactor, dampinglinearc, dampinglinearv, dampinglinearv2, dampingangularc, dampingangularv, dampingangularv2, margin, default2pi, buoyancyfactor);
		}
		static float get_object_fragment_damage_health(type::any p0, bool p1) {
			return invoker::invoke<float>(0xb6fbfd079b8d0596, p0, p1);
		}
		static void set_activate_object_physics_as_soon_as_it_is_unfrozen(type::object object, bool toggle) {
			invoker::invoke<void>(0x406137f8ef90eaf5, object, toggle);
		}
		static bool is_any_object_near_point(float x, float y, float z, float range, bool p4) {
			return invoker::invoke<bool>(0x397dc58ff00298d1, x, y, z, range, p4);
		}
		static bool is_object_near_point(type::hash objecthash, float x, float y, float z, float range) {
			return invoker::invoke<bool>(0x8c90fe4b381ba60a, objecthash, x, y, z, range);
		}
		static void _0x4a39db43e47cf3aa(type::any p0) {
			invoker::invoke<void>(0x4a39db43e47cf3aa, p0);
		}
		static void _0xe7e4c198b0185900(type::object p0, type::any p1, bool p2) {
			invoker::invoke<void>(0xe7e4c198b0185900, p0, p1, p2);
		}
		static void _0xf9c1681347c8bd15(type::object object) {
			invoker::invoke<void>(0xf9c1681347c8bd15, object);
		}
		static void track_object_visibility(type::any p0) {
			invoker::invoke<void>(0xb252bc036b525623, p0);
		}
		static bool is_object_visible(type::object object) {
			return invoker::invoke<bool>(0x8b32ace6326a7546, object);
		}
		static void _0xc6033d32241f6fb5(type::any p0, bool p1) {
			invoker::invoke<void>(0xc6033d32241f6fb5, p0, p1);
		}
		static void _0xeb6f1a9b5510a5d2(type::any p0, bool p1) {
			invoker::invoke<void>(0xeb6f1a9b5510a5d2, p0, p1);
		}
		static void _0xbce595371a5fbaaf(type::any p0, bool p1) {
			invoker::invoke<void>(0xbce595371a5fbaaf, p0, p1);
		}
		static int _get_des_object(float x, float y, float z, float rotation, const char* name) {
			return invoker::invoke<int>(0xb48fced898292e52, x, y, z, rotation, name);
		}
		static void _set_des_object_state(int handle, int state) {
			invoker::invoke<void>(0x5c29f698d404c5e1, handle, state);
		}
		static type::any _get_des_object_state(int handle) {
			return invoker::invoke<type::any>(0x899ba936634a322e, handle);
		}
		static bool _does_des_object_exist(int handle) {
			return invoker::invoke<bool>(0x52af537a0c5b8aad, handle);
		}
		static float _0x260ee4fdbdf4db01(type::any p0) {
			return invoker::invoke<float>(0x260ee4fdbdf4db01, p0);
		}
		static type::pickup create_pickup(type::hash pickuphash, float posx, float posy, float posz, int p4, int value, bool p6, type::hash modelhash) {
			return invoker::invoke<type::pickup>(0xfba08c503dd5fa58, pickuphash, posx, posy, posz, p4, value, p6, modelhash);
		}
		static type::pickup create_pickup_rotate(type::hash pickuphash, float posx, float posy, float posz, float rotx, float roty, float rotz, int flag, int amount, type::any p9, bool p10, type::hash modelhash) {
			return invoker::invoke<type::pickup>(0x891804727e0a98b7, pickuphash, posx, posy, posz, rotx, roty, rotz, flag, amount, p9, p10, modelhash);
		}
		static type::pickup create_ambient_pickup(type::hash pickuphash, float posx, float posy, float posz, int flag, int value, type::hash modelhash, bool returnhandle, bool p8) {
			return invoker::invoke<type::pickup>(0x673966a0c0fd7171, pickuphash, posx, posy, posz, flag, value, modelhash, returnhandle, p8);
		}
		static type::pickup create_portable_pickup(type::hash pickuphash, float x, float y, float z, bool placeonground, type::hash modelhash) {
			return invoker::invoke<type::pickup>(0x2eaf1fdb2fb55698, pickuphash, x, y, z, placeonground, modelhash);
		}
		static type::pickup _create_portable_pickup_2(type::hash pickuphash, float x, float y, float z, bool placeonground, type::hash modelhash) {
			return invoker::invoke<type::pickup>(0x125494b98a21aaf7, pickuphash, x, y, z, placeonground, modelhash);
		}
		static void attach_portable_pickup_to_ped(type::ped ped, type::any p1) {
			invoker::invoke<void>(0x8dc39368bdd57755, ped, p1);
		}
		static void detach_portable_pickup_from_ped(type::ped ped) {
			invoker::invoke<void>(0xcf463d1e9a0aecb1, ped);
		}
		static void _0x0bf3b3bd47d79c08(type::hash hash, int p1) {
			invoker::invoke<void>(0x0bf3b3bd47d79c08, hash, p1);
		}
		static void _0x78857fc65cadb909(bool p0) {
			invoker::invoke<void>(0x78857fc65cadb909, p0);
		}
		static PVector3 get_safe_pickup_coords(float x, float y, float z, type::any p3, type::any p4) {
			return invoker::invoke<PVector3>(0x6e16bc2503ff1ff0, x, y, z, p3, p4);
		}
		static PVector3 get_pickup_coords(type::pickup pickup) {
			return invoker::invoke<PVector3>(0x225b8b35c88029b3, pickup);
		}
		static void remove_all_pickups_of_type(type::hash pickuphash) {
			invoker::invoke<void>(0x27f9d613092159cf, pickuphash);
		}
		static bool has_pickup_been_collected(type::pickup pickup) {
			return invoker::invoke<bool>(0x80ec48e6679313f9, pickup);
		}
		static void remove_pickup(type::pickup pickup) {
			invoker::invoke<void>(0x3288d8acaecd2ab2, pickup);
		}
		static void create_money_pickups(float x, float y, float z, int value, int amount, type::hash model) {
			invoker::invoke<void>(0x0589b5e791ce9b2b, x, y, z, value, amount, model);
		}
		static bool does_pickup_exist(type::pickup pickup) {
			return invoker::invoke<bool>(0xafc1ca75ad4074d1, pickup);
		}
		static bool does_pickup_object_exist(type::object pickupobject) {
			return invoker::invoke<bool>(0xd9efb6dbf7daaea3, pickupobject);
		}
		static type::object get_pickup_object(type::pickup pickup) {
			return invoker::invoke<type::object>(0x5099bc55630b25ae, pickup);
		}
		static bool _0x0378c08504160d0d(type::any p0) {
			return invoker::invoke<bool>(0x0378c08504160d0d, p0);
		}
		static bool _is_pickup_within_radius(type::hash pickuphash, float x, float y, float z, float radius) {
			return invoker::invoke<bool>(0xf9c36251f6e48e33, pickuphash, x, y, z, radius);
		}
		static void set_pickup_regeneration_time(type::pickup pickup, int duration) {
			invoker::invoke<void>(0x78015c9b4b3ecc9d, pickup, duration);
		}
		static void _0x616093ec6b139dd9(type::player player, type::hash pickuphash, bool p2) {
			invoker::invoke<void>(0x616093ec6b139dd9, player, pickuphash, p2);
		}
		static void _0x88eaec617cd26926(type::hash p0, bool p1) {
			invoker::invoke<void>(0x88eaec617cd26926, p0, p1);
		}
		static void set_team_pickup_object(type::object object, type::any p1, bool p2) {
			invoker::invoke<void>(0x53e0df1a2a3cf0ca, object, p1, p2);
		}
		static void _0x92aefb5f6e294023(type::object object, bool p1, bool p2) {
			invoker::invoke<void>(0x92aefb5f6e294023, object, p1, p2);
		}
		static void _0xa08fe5e49bdc39dd(type::any p0, float p1, bool p2) {
			invoker::invoke<void>(0xa08fe5e49bdc39dd, p0, p1, p2);
		}
		static int* _0xdb41d07a45a6d4b7(type::hash pickuphash) {
			return invoker::invoke<int*>(0xdb41d07a45a6d4b7, pickuphash);
		}
		static void _0x318516e02de3ece2(float p0) {
			invoker::invoke<void>(0x318516e02de3ece2, p0);
		}
		static void _0x31f924b53eaddf65(bool p0) {
			invoker::invoke<void>(0x31f924b53eaddf65, p0);
		}
		static void _0xf92099527db8e2a7(type::any p0, type::any p1) {
			invoker::invoke<void>(0xf92099527db8e2a7, p0, p1);
		}
		static void _0xa2c1f5e92afe49ed() {
			invoker::invoke<void>(0xa2c1f5e92afe49ed);
		}
		static void _0x762db2d380b48d04(type::any p0) {
			invoker::invoke<void>(0x762db2d380b48d04, p0);
		}
		static void _highlight_placement_coords(float x, float y, float z, int colorindex) {
			invoker::invoke<void>(0x3430676b11cdf21d, x, y, z, colorindex);
		}
		static void _0xb2d0bde54f0e8e5a(type::object object, bool toggle) {
			invoker::invoke<void>(0xb2d0bde54f0e8e5a, object, toggle);
		}
		static type::hash _get_weapon_hash_from_pickup(type::pickup pickuphash) {
			return invoker::invoke<type::hash>(0x08f96ca6c551ad51, pickuphash);
		}
		static bool _0x11d1e53a726891fe(type::object object) {
			return invoker::invoke<bool>(0x11d1e53a726891fe, object);
		}
		static void _set_object_texture_variant(type::object object, int paintindex) {
			invoker::invoke<void>(0x971da0055324d033, object, paintindex);
		}
		static type::hash _get_pickup_hash(type::pickup pickuphash) {
			return invoker::invoke<type::hash>(0x5eaad83f8cfb4575, pickuphash);
		}
		static void set_force_object_this_frame(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0xf538081986e49e9d, p0, p1, p2, p3);
		}
		static void _mark_object_for_deletion(type::object object) {
			invoker::invoke<void>(0xadbe4809f19f927a, object);
		}
	}

	namespace brain {
		static void task_pause(type::ped ped, int time) {
			invoker::invoke<void>(0xe73a266db0ca9042, ped, time);
		}
		static void task_stand_still(type::ped ped, int time) {
			invoker::invoke<void>(0x919be13eed931959, ped, time);
		}
		static void task_jump(type::ped ped, bool unused) {
			invoker::invoke<void>(0x0ae4086104e067b1, ped, unused);
		}
		static void task_cower(type::ped ped, int duration) {
			invoker::invoke<void>(0x3eb1fe9e8e908e15, ped, duration);
		}
		static void task_hands_up(type::ped ped, int duration, type::ped facingped, int p3, bool p4) {
			invoker::invoke<void>(0xf2eab31979a7f910, ped, duration, facingped, p3, p4);
		}
		static void update_task_hands_up_duration(type::ped ped, int duration) {
			invoker::invoke<void>(0xa98fcafd7893c834, ped, duration);
		}
		static void task_open_vehicle_door(type::ped ped, type::vehicle vehicle, int timeout, int doorindex, float speed) {
			invoker::invoke<void>(0x965791a9a488a062, ped, vehicle, timeout, doorindex, speed);
		}
		static void task_enter_vehicle(type::ped ped, type::vehicle vehicle, int timeout, int seat, float speed, int flag, type::any p6) {
			invoker::invoke<void>(0xc20e50aa46d09ca8, ped, vehicle, timeout, seat, speed, flag, p6);
		}
		static void task_leave_vehicle(type::ped ped, type::vehicle vehicle, int flags) {
			invoker::invoke<void>(0xd3dbce61a490be02, ped, vehicle, flags);
		}
		static void _task_get_off_boat(type::ped ped, type::vehicle boat) {
			invoker::invoke<void>(0x9c00e77af14b2dff, ped, boat);
		}
		static void task_sky_dive(type::ped ped) {
			invoker::invoke<void>(0x601736cfe536b0a0, ped);
		}
		static void task_parachute(type::ped ped, bool p1) {
			invoker::invoke<void>(0xd2f1c53c97ee81ab, ped, p1);
		}
		static void task_parachute_to_target(type::ped ped, float x, float y, float z) {
			invoker::invoke<void>(0xb33e291afa6bd03a, ped, x, y, z);
		}
		static void set_parachute_task_target(type::ped ped, float x, float y, float z) {
			invoker::invoke<void>(0xc313379af0fceda7, ped, x, y, z);
		}
		static void set_parachute_task_thrust(type::ped ped, float thrust) {
			invoker::invoke<void>(0x0729bac1b8c64317, ped, thrust);
		}
		static void task_rappel_from_heli(type::ped ped, int unused) {
			invoker::invoke<void>(0x09693b0312f91649, ped, unused);
		}
		static void task_vehicle_drive_to_coord(type::ped ped, type::vehicle vehicle, float x, float y, float z, float speed, type::any p6, type::hash vehiclemodel, int drivingmode, float stoprange, float p10) {
			invoker::invoke<void>(0xe2a2aa2f659d77a7, ped, vehicle, x, y, z, speed, p6, vehiclemodel, drivingmode, stoprange, p10);
		}
		static void task_vehicle_drive_to_coord_longrange(type::ped ped, type::vehicle vehicle, float x, float y, float z, float speed, int drivemode, float stoprange) {
			invoker::invoke<void>(0x158bb33f920d360c, ped, vehicle, x, y, z, speed, drivemode, stoprange);
		}
		static void task_vehicle_drive_wander(type::ped ped, type::vehicle vehicle, float speed, int drivingstyle) {
			invoker::invoke<void>(0x480142959d337d00, ped, vehicle, speed, drivingstyle);
		}
		static void task_follow_to_offset_of_entity(type::ped ped, type::entity entity, float offsetx, float offsety, float offsetz, float movementspeed, int timeout, float stoppingrange, bool persistfollowing) {
			invoker::invoke<void>(0x304ae42e357b8c7e, ped, entity, offsetx, offsety, offsetz, movementspeed, timeout, stoppingrange, persistfollowing);
		}
		static void task_go_straight_to_coord(type::ped ped, float x, float y, float z, float speed, int timeout, float targetheading, float distancetoslide) {
			invoker::invoke<void>(0xd76b57b44f1e6f8b, ped, x, y, z, speed, timeout, targetheading, distancetoslide);
		}
		static void task_go_straight_to_coord_relative_to_entity(type::entity entity1, type::entity entity2, float p2, float p3, float p4, float p5, type::any p6) {
			invoker::invoke<void>(0x61e360b7e040d12e, entity1, entity2, p2, p3, p4, p5, p6);
		}
		static void task_achieve_heading(type::ped ped, float heading, int timeout) {
			invoker::invoke<void>(0x93b93a37987f1f3d, ped, heading, timeout);
		}
		static void task_flush_route() {
			invoker::invoke<void>(0x841142a1376e9006);
		}
		static void task_extend_route(float x, float y, float z) {
			invoker::invoke<void>(0x1e7889778264843a, x, y, z);
		}
		static void task_follow_point_route(type::ped ped, float speed, int unknown) {
			invoker::invoke<void>(0x595583281858626e, ped, speed, unknown);
		}
		static void task_go_to_entity(type::entity entity, type::entity target, int duration, float distance, float speed, float p5, int p6) {
			invoker::invoke<void>(0x6a071245eb0d1882, entity, target, duration, distance, speed, p5, p6);
		}
		static void task_smart_flee_coord(type::ped ped, float x, float y, float z, float distance, int time, bool p6, bool p7) {
			invoker::invoke<void>(0x94587f17e9c365d5, ped, x, y, z, distance, time, p6, p7);
		}
		static void task_smart_flee_ped(type::ped ped, type::ped fleetarget, float distance, type::any fleetime, bool p4, bool p5) {
			invoker::invoke<void>(0x22b0d0e37ccb840d, ped, fleetarget, distance, fleetime, p4, p5);
		}
		static void task_react_and_flee_ped(type::ped ped, type::ped fleetarget) {
			invoker::invoke<void>(0x72c896464915d1b1, ped, fleetarget);
		}
		static void task_shocking_event_react(type::ped ped, int eventhandle) {
			invoker::invoke<void>(0x452419cbd838065b, ped, eventhandle);
		}
		static void task_wander_in_area(type::ped ped, float x, float y, float z, float radius, float minimallength, float timebetweenwalks) {
			invoker::invoke<void>(0xe054346ca3a0f315, ped, x, y, z, radius, minimallength, timebetweenwalks);
		}
		static void task_wander_standard(type::ped ped, float p1, int p2) {
			invoker::invoke<void>(0xbb9ce077274f6a1b, ped, p1, p2);
		}
		static void task_vehicle_park(type::ped ped, type::vehicle vehicle, float x, float y, float z, float heading, int mode, float radius, bool keepengineon) {
			invoker::invoke<void>(0x0f3e34e968ea374e, ped, vehicle, x, y, z, heading, mode, radius, keepengineon);
		}
		static void task_stealth_kill(type::ped killer, type::ped target, type::hash actiontype, float p3, type::any p4) {
			invoker::invoke<void>(0xaa5dc05579d60bd9, killer, target, actiontype, p3, p4);
		}
		static void task_plant_bomb(type::ped ped, float x, float y, float z, float heading) {
			invoker::invoke<void>(0x965fec691d55e9bf, ped, x, y, z, heading);
		}
		static void task_follow_nav_mesh_to_coord(type::ped ped, float x, float y, float z, float speed, int timeout, float stoppingrange, bool persistfollowing, float unk) {
			invoker::invoke<void>(0x15d3a79d4e44b913, ped, x, y, z, speed, timeout, stoppingrange, persistfollowing, unk);
		}
		static void task_follow_nav_mesh_to_coord_advanced(type::ped ped, float x, float y, float z, float speed, int timeout, float unkfloat, int unkint, float unkx, float unky, float unkz, float unk_40000f) {
			invoker::invoke<void>(0x17f58b88d085dbac, ped, x, y, z, speed, timeout, unkfloat, unkint, unkx, unky, unkz, unk_40000f);
		}
		static void set_ped_path_can_use_climbovers(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x8e06a6fe76c9eff4, ped, toggle);
		}
		static void set_ped_path_can_use_ladders(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x77a5b103c87f476e, ped, toggle);
		}
		static void set_ped_path_can_drop_from_height(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xe361c5c71c431a4f, ped, toggle);
		}
		static void _0x88e32db8c1a4aa4b(type::ped ped, float p1) {
			invoker::invoke<void>(0x88e32db8c1a4aa4b, ped, p1);
		}
		static void set_ped_paths_width_plant(type::ped ped, bool mayenterwater) {
			invoker::invoke<void>(0xf35425a4204367ec, ped, mayenterwater);
		}
		static void set_ped_path_prefer_to_avoid_water(type::ped ped, bool avoidwater) {
			invoker::invoke<void>(0x38fe1ec73743793c, ped, avoidwater);
		}
		static void set_ped_path_avoid_fire(type::ped ped, bool avoidfire) {
			invoker::invoke<void>(0x4455517b28441e60, ped, avoidfire);
		}
		static void set_global_min_bird_flight_height(float height) {
			invoker::invoke<void>(0x6c6b148586f934f7, height);
		}
		static int get_navmesh_route_distance_remaining(type::ped ped, float* distremaining, bool* ispathready) {
			return invoker::invoke<int>(0xc6f5c0bcdc74d62d, ped, distremaining, ispathready);
		}
		static int get_navmesh_route_result(type::ped ped) {
			return invoker::invoke<int>(0x632e831f382a0fa8, ped);
		}
		static bool _0x3e38e28a1d80ddf6(type::ped ped) {
			return invoker::invoke<bool>(0x3e38e28a1d80ddf6, ped);
		}
		static void task_go_to_coord_any_means(type::ped ped, float x, float y, float z, float speed, type::any p5, bool p6, int walkingstyle, float p8) {
			invoker::invoke<void>(0x5bc448cb78fa3e88, ped, x, y, z, speed, p5, p6, walkingstyle, p8);
		}
		static void task_go_to_coord_any_means_extra_params(type::ped ped, float x, float y, float z, float speed, type::any p5, bool p6, int walkingstyle, float p8, type::any p9, type::any p10, type::any p11) {
			invoker::invoke<void>(0x1dd45f9ecfdb1bc9, ped, x, y, z, speed, p5, p6, walkingstyle, p8, p9, p10, p11);
		}
		static void task_go_to_coord_any_means_extra_params_with_cruise_speed(type::ped ped, float x, float y, float z, float speed, type::any p5, bool p6, int walkingstyle, float p8, type::any p9, type::any p10, type::any p11, type::any p12) {
			invoker::invoke<void>(0xb8ecd61f531a7b02, ped, x, y, z, speed, p5, p6, walkingstyle, p8, p9, p10, p11, p12);
		}
		static void task_play_anim(type::ped ped, const char* animdictionary, const char* animationname, float speed, float speedmultiplier, int duration, int flag, float playbackrate, bool lockx, bool locky, bool lockz) {
			invoker::invoke<void>(0xea47fe3719165b94, ped, animdictionary, animationname, speed, speedmultiplier, duration, flag, playbackrate, lockx, locky, lockz);
		}
		static void task_play_anim_advanced(type::ped ped, const char* animdict, const char* animname, float posx, float posy, float posz, float rotx, float roty, float rotz, float speed, float speedmultiplier, int duration, type::any flag, float animtime, int p14, int p15) {
			invoker::invoke<void>(0x83cdb10ea29b370b, ped, animdict, animname, posx, posy, posz, rotx, roty, rotz, speed, speedmultiplier, duration, flag, animtime, p14, p15);
		}
		static void stop_anim_task(type::ped ped, const char* animdictionary, const char* animationname, float p3) {
			invoker::invoke<void>(0x97ff36a1d40ea00a, ped, animdictionary, animationname, p3);
		}
		static void task_scripted_animation(type::ped ped, type::any* p1, type::any* p2, type::any* p3, float p4, float p5) {
			invoker::invoke<void>(0x126ef75f1e17abe5, ped, p1, p2, p3, p4, p5);
		}
		static void play_entity_scripted_anim(type::any p0, type::any* p1, type::any* p2, type::any* p3, float p4, float p5) {
			invoker::invoke<void>(0x77a1eec547e7fcf1, p0, p1, p2, p3, p4, p5);
		}
		static void stop_anim_playback(type::ped ped, int p1, bool p2) {
			invoker::invoke<void>(0xee08c992d238c5d1, ped, p1, p2);
		}
		static void set_anim_weight(type::any p0, float p1, type::any p2, type::any p3, bool p4) {
			invoker::invoke<void>(0x207f1a47c0342f48, p0, p1, p2, p3, p4);
		}
		static void set_anim_rate(type::any p0, float p1, type::any p2, bool p3) {
			invoker::invoke<void>(0x032d49c5e359c847, p0, p1, p2, p3);
		}
		static void set_anim_looped(type::any p0, bool p1, type::any p2, bool p3) {
			invoker::invoke<void>(0x70033c3cc29a1ff4, p0, p1, p2, p3);
		}
		static void task_play_phone_gesture_animation(type::ped ped, const char* animdict, const char* animation, const char* bonemasktype, float p4, float p5, bool p6, bool p7) {
			invoker::invoke<void>(0x8fbb6758b3b3e9ec, ped, animdict, animation, bonemasktype, p4, p5, p6, p7);
		}
		static void _task_stop_phone_gesture_animation(type::ped ped) {
			invoker::invoke<void>(0x3fa00d4f4641bfae, ped);
		}
		static bool is_playing_phone_gesture_anim(type::ped ped) {
			return invoker::invoke<bool>(0xb8ebb1e9d3588c10, ped);
		}
		static float get_phone_gesture_anim_current_time(type::ped ped) {
			return invoker::invoke<float>(0x47619abe8b268c60, ped);
		}
		static float get_phone_gesture_anim_total_time(type::ped ped) {
			return invoker::invoke<float>(0x1ee0f68a7c25dec6, ped);
		}
		static void task_vehicle_play_anim(type::vehicle vehicle, const char* animation_set, const char* animation_name) {
			invoker::invoke<void>(0x69f5c3bd0f3ebd89, vehicle, animation_set, animation_name);
		}
		static void task_look_at_coord(type::entity entity, float x, float y, float z, float duration, type::any p5, type::any p6) {
			invoker::invoke<void>(0x6fa46612594f7973, entity, x, y, z, duration, p5, p6);
		}
		static void task_look_at_entity(type::ped ped, type::entity lookat, int duration, int unknown1, int unknown2) {
			invoker::invoke<void>(0x69f4be8c8cc4796c, ped, lookat, duration, unknown1, unknown2);
		}
		static void task_clear_look_at(type::ped ped) {
			invoker::invoke<void>(0x0f804f1db19b9689, ped);
		}
		static void open_sequence_task(type::object* tasksequence) {
			invoker::invoke<void>(0xe8854a4326b9e12b, tasksequence);
		}
		static void close_sequence_task(type::object tasksequence) {
			invoker::invoke<void>(0x39e72bc99e6360cb, tasksequence);
		}
		static void task_perform_sequence(type::ped ped, type::object tasksequence) {
			invoker::invoke<void>(0x5aba3986d90d8a3b, ped, tasksequence);
		}
		static void clear_sequence_task(type::object* tasksequence) {
			invoker::invoke<void>(0x3841422e9c488d8c, tasksequence);
		}
		static void set_sequence_to_repeat(type::object tasksequence, bool repeat) {
			invoker::invoke<void>(0x58c70cf3a41e4ae7, tasksequence, repeat);
		}
		static int get_sequence_progress(type::ped ped) {
			return invoker::invoke<int>(0x00a9010cfe1e3533, ped);
		}
		static bool get_is_task_active(type::ped ped, int tasknumber) {
			return invoker::invoke<bool>(0xb0760331c7aa4155, ped, tasknumber);
		}
		static int get_script_task_status(type::ped targetped, type::hash taskhash) {
			return invoker::invoke<int>(0x77f1beb8863288d5, targetped, taskhash);
		}
		static int get_active_vehicle_mission_type(type::vehicle veh) {
			return invoker::invoke<int>(0x534aeba6e5ed4cab, veh);
		}
		static void task_leave_any_vehicle(type::ped ped, int p1, int p2) {
			invoker::invoke<void>(0x504d54df3f6f2247, ped, p1, p2);
		}
		static void task_aim_gun_scripted(type::ped ped, type::hash scripttask, bool p2, bool p3) {
			invoker::invoke<void>(0x7a192be16d373d00, ped, scripttask, p2, p3);
		}
		static void task_aim_gun_scripted_with_target(type::any p0, type::any p1, float p2, float p3, float p4, type::any p5, bool p6, bool p7) {
			invoker::invoke<void>(0x8605af0de8b3a5ac, p0, p1, p2, p3, p4, p5, p6, p7);
		}
		static void update_task_aim_gun_scripted_target(type::ped p0, type::ped p1, float p2, float p3, float p4, bool p5) {
			invoker::invoke<void>(0x9724fb59a3e72ad0, p0, p1, p2, p3, p4, p5);
		}
		static const char* get_clip_set_for_scripted_gun_task(int p0) {
			return invoker::invoke<const char*>(0x3a8cadc7d37aacc5, p0);
		}
		static void task_aim_gun_at_entity(type::ped ped, type::entity entity, int duration, bool unk) {
			invoker::invoke<void>(0x9b53bb6e8943af53, ped, entity, duration, unk);
		}
		static void task_turn_ped_to_face_entity(type::ped ped, type::entity entity, int duration) {
			invoker::invoke<void>(0x5ad23d40115353ac, ped, entity, duration);
		}
		static void task_aim_gun_at_coord(type::ped ped, float x, float y, float z, int time, bool p5, bool p6) {
			invoker::invoke<void>(0x6671f3eec681bda1, ped, x, y, z, time, p5, p6);
		}
		static void task_shoot_at_coord(type::ped ped, float x, float y, float z, int duration, type::hash firingpattern) {
			invoker::invoke<void>(0x46a6cc01e0826106, ped, x, y, z, duration, firingpattern);
		}
		static void task_shuffle_to_next_vehicle_seat(type::ped ped, type::vehicle vehicle) {
			invoker::invoke<void>(0x7aa80209bda643eb, ped, vehicle);
		}
		static void clear_ped_tasks(type::ped ped) {
			invoker::invoke<void>(0xe1ef3c1216aff2cd, ped);
		}
		static void clear_ped_secondary_task(type::ped ped) {
			invoker::invoke<void>(0x176cecf6f920d707, ped);
		}
		static void task_everyone_leave_vehicle(type::vehicle vehicle) {
			invoker::invoke<void>(0x7f93691ab4b92272, vehicle);
		}
		static void task_goto_entity_offset(type::ped ped, type::any p1, type::any p2, float x, float y, float z, int duration) {
			invoker::invoke<void>(0xe39b4ff4fdebde27, ped, p1, p2, x, y, z, duration);
		}
		static void task_goto_entity_offset_xy(type::ped ped, type::entity entity, int duration, float xoffset, float yoffset, float zoffset, float moveblendratio, bool usenavmesh) {
			invoker::invoke<void>(0x338e7ef52b6095a9, ped, entity, duration, xoffset, yoffset, zoffset, moveblendratio, usenavmesh);
		}
		static void task_turn_ped_to_face_coord(type::ped ped, float x, float y, float z, int duration) {
			invoker::invoke<void>(0x1dda930a0ac38571, ped, x, y, z, duration);
		}
		static void task_vehicle_temp_action(type::ped driver, type::vehicle vehicle, int action, int time) {
			invoker::invoke<void>(0xc429dceeb339e129, driver, vehicle, action, time);
		}
		static void task_vehicle_mission(int p0, int p1, type::vehicle veh, type::any p3, float p4, type::any p5, float p6, float p7, bool p8) {
			invoker::invoke<void>(0x659427e0ef36bcde, p0, p1, veh, p3, p4, p5, p6, p7, p8);
		}
		static void task_vehicle_mission_ped_target(type::ped ped, type::vehicle vehicle, type::ped pedtarget, int mode, float maxspeed, int drivingstyle, float mindistance, float p7, bool p8) {
			invoker::invoke<void>(0x9454528df15d657a, ped, vehicle, pedtarget, mode, maxspeed, drivingstyle, mindistance, p7, p8);
		}
		static void task_vehicle_mission_coors_target(type::ped ped, type::vehicle vehicle, float x, float y, float z, int p5, int p6, int p7, float p8, float p9, bool p10) {
			invoker::invoke<void>(0xf0af20aa7731f8c3, ped, vehicle, x, y, z, p5, p6, p7, p8, p9, p10);
		}
		static void task_vehicle_escort(type::ped ped, type::vehicle vehicle, type::vehicle targetvehicle, int mode, float speed, int drivingstyle, float mindistance, int p7, float noroadsdistance) {
			invoker::invoke<void>(0x0fa6e4b75f302400, ped, vehicle, targetvehicle, mode, speed, drivingstyle, mindistance, p7, noroadsdistance);
		}
		static void _task_vehicle_follow(type::ped driver, type::vehicle vehicle, type::entity targetentity, float speed, int drivingstyle, int mindistance) {
			invoker::invoke<void>(0xfc545a9f0626e3b6, driver, vehicle, targetentity, speed, drivingstyle, mindistance);
		}
		static void task_vehicle_chase(type::ped driver, type::entity targetent) {
			invoker::invoke<void>(0x3c08a8e30363b353, driver, targetent);
		}
		static void task_vehicle_heli_protect(type::ped pilot, type::vehicle vehicle, type::entity entitytofollow, float targetspeed, int p4, float radius, int altitude, int p7) {
			invoker::invoke<void>(0x1e09c32048fefd1c, pilot, vehicle, entitytofollow, targetspeed, p4, radius, altitude, p7);
		}
		static void set_task_vehicle_chase_behavior_flag(type::ped ped, int flag, bool set) {
			invoker::invoke<void>(0xcc665aac360d31e7, ped, flag, set);
		}
		static void set_task_vehicle_chase_ideal_pursuit_distance(type::ped ped, float distance) {
			invoker::invoke<void>(0x639b642facbe4edd, ped, distance);
		}
		static void task_heli_chase(type::ped pilot, type::entity entitytofollow, float x, float y, float z) {
			invoker::invoke<void>(0xac83b1db38d0ada0, pilot, entitytofollow, x, y, z);
		}
		static void task_plane_chase(type::ped pilot, type::entity entitytofollow, float x, float y, float z) {
			invoker::invoke<void>(0x2d2386f273ff7a25, pilot, entitytofollow, x, y, z);
		}
		static void task_plane_land(type::ped pilot, type::vehicle plane, float runwaystartx, float runwaystarty, float runwaystartz, float runwayendx, float runwayendy, float runwayendz) {
			invoker::invoke<void>(0xbf19721fa34d32c0, pilot, plane, runwaystartx, runwaystarty, runwaystartz, runwayendx, runwayendy, runwayendz);
		}
		static void task_heli_mission(type::ped pilot, type::vehicle aircraft, type::vehicle targetvehicle, type::ped targetped, float destinationx, float destinationy, float destinationz, int missionflag, float maxspeed, float landingradius, float targetheading, int unk1, int unk2, type::hash unk3, int landingflags) {
			invoker::invoke<void>(0xdad029e187a2beb4, pilot, aircraft, targetvehicle, targetped, destinationx, destinationy, destinationz, missionflag, maxspeed, landingradius, targetheading, unk1, unk2, unk3, landingflags);
		}
		static void task_plane_mission(type::ped pilot, type::vehicle aircraft, type::vehicle targetvehicle, type::ped targetped, float destinationx, float destinationy, float destinationz, int missionflag, float angulardrag, float unk, float targetheading, float maxz, float minz) {
			invoker::invoke<void>(0x23703cd154e83b88, pilot, aircraft, targetvehicle, targetped, destinationx, destinationy, destinationz, missionflag, angulardrag, unk, targetheading, maxz, minz);
		}
		static void task_boat_mission(type::ped peddriver, type::vehicle boat, type::any p2, type::any p3, float x, float y, float z, type::any p7, float maxspeed, int drivingstyle, float p10, type::any p11) {
			invoker::invoke<void>(0x15c86013127ce63f, peddriver, boat, p2, p3, x, y, z, p7, maxspeed, drivingstyle, p10, p11);
		}
		static void task_drive_by(type::ped driverped, type::ped targetped, type::vehicle targetvehicle, float targetx, float targety, float targetz, float distancetoshoot, int pedaccuracy, bool p8, type::hash firingpattern) {
			invoker::invoke<void>(0x2f8af0e82773a171, driverped, targetped, targetvehicle, targetx, targety, targetz, distancetoshoot, pedaccuracy, p8, firingpattern);
		}
		static void set_driveby_task_target(type::ped shootingped, type::ped targetped, type::vehicle targetvehicle, float x, float y, float z) {
			invoker::invoke<void>(0xe5b302114d8162ee, shootingped, targetped, targetvehicle, x, y, z);
		}
		static void clear_driveby_task_underneath_driving_task(type::ped ped) {
			invoker::invoke<void>(0xc35b5cdb2824cf69, ped);
		}
		static bool is_driveby_task_underneath_driving_task(type::ped ped) {
			return invoker::invoke<bool>(0x8785e6e40c7a8818, ped);
		}
		static bool control_mounted_weapon(type::ped ped) {
			return invoker::invoke<bool>(0xdcfe42068fe0135a, ped);
		}
		static void set_mounted_weapon_target(type::ped shootingped, type::ped targetped, type::vehicle targetvehicle, float x, float y, float z) {
			invoker::invoke<void>(0xccd892192c6d2bb9, shootingped, targetped, targetvehicle, x, y, z);
		}
		static bool is_mounted_weapon_task_underneath_driving_task(type::ped ped) {
			return invoker::invoke<bool>(0xa320ef046186fa3b, ped);
		}
		static void task_use_mobile_phone(type::ped ped, int p1) {
			invoker::invoke<void>(0xbd2a8ec3af4de7db, ped, p1);
		}
		static void task_use_mobile_phone_timed(type::ped ped, int duration) {
			invoker::invoke<void>(0x5ee02954a14c69db, ped, duration);
		}
		static void task_chat_to_ped(type::ped ped, type::ped target, int p2, float p3, float p4, float p5, float p6, float p7) {
			invoker::invoke<void>(0x8c338e0263e4fd19, ped, target, p2, p3, p4, p5, p6, p7);
		}
		static void task_warp_ped_into_vehicle(type::ped ped, type::vehicle vehicle, int seat) {
			invoker::invoke<void>(0x9a7d091411c5f684, ped, vehicle, seat);
		}
		static void task_shoot_at_entity(type::entity entity, type::entity target, int duration, type::hash firingpattern) {
			invoker::invoke<void>(0x08da95e8298ae772, entity, target, duration, firingpattern);
		}
		static void task_climb(type::ped ped, bool unused) {
			invoker::invoke<void>(0x89d9fcc2435112f1, ped, unused);
		}
		static void task_climb_ladder(type::ped ped, int p1) {
			invoker::invoke<void>(0xb6c987f9285a3814, ped, p1);
		}
		static void clear_ped_tasks_immediately(type::ped ped) {
			invoker::invoke<void>(0xaaa34f8a7cb32098, ped);
		}
		static void soul_clear_ped_tasks_immediately(type::ped ped) {
			invoker::invoke<void>(0xaaa34f8a7cb32098, ped, 1);
		}
		static void task_perform_sequence_from_progress(type::ped ped, type::object tasksequence, int currentprogress, int progresstosetto) {
			invoker::invoke<void>(0x89221b16730234f0, ped, tasksequence, currentprogress, progresstosetto);
		}
		static void set_next_desired_move_state(float p0) {
			invoker::invoke<void>(0xf1b9f16e89e2c93a, p0);
		}
		static void set_ped_desired_move_blend_ratio(type::ped ped, float p1) {
			invoker::invoke<void>(0x1e982ac8716912c5, ped, p1);
		}
		static float get_ped_desired_move_blend_ratio(type::ped ped) {
			return invoker::invoke<float>(0x8517d4a6ca8513ed, ped);
		}
		static void task_goto_entity_aiming(type::ped ped, type::entity target, float distancetostopat, float startaimingdist) {
			invoker::invoke<void>(0xa9da48fab8a76c12, ped, target, distancetostopat, startaimingdist);
		}
		static void task_set_decision_maker(type::ped p0, type::hash p1) {
			invoker::invoke<void>(0xeb8517dda73720da, p0, p1);
		}
		static void task_set_sphere_defensive_area(type::any p0, float p1, float p2, float p3, float p4) {
			invoker::invoke<void>(0x933c06518b52a9a4, p0, p1, p2, p3, p4);
		}
		static void task_clear_defensive_area(type::any p0) {
			invoker::invoke<void>(0x95a6c46a31d1917d, p0);
		}
		static void task_ped_slide_to_coord(type::ped ped, float x, float y, float z, float heading, float duration) {
			invoker::invoke<void>(0xd04fe6765d990a06, ped, x, y, z, heading, duration);
		}
		static void task_ped_slide_to_coord_hdg_rate(type::ped ped, float x, float y, float z, float heading, float p5, float p6) {
			invoker::invoke<void>(0x5a4a6a6d3dc64f52, ped, x, y, z, heading, p5, p6);
		}
		static type::any add_cover_point(float posx, float posy, float posz, float heading, bool p4, int p5, bool p6, bool p7) {
			return invoker::invoke<type::any>(0xd5c12a75c7b9497f, posx, posy, posz, heading, p4, p5, p6, p7);
		}
		static void remove_cover_point(type::scrhandle coverpoint) {
			invoker::invoke<void>(0xae287c923d891715, coverpoint);
		}
		static bool does_scripted_cover_point_exist_at_coords(float x, float y, float z) {
			return invoker::invoke<bool>(0xa98b8e3c088e5a31, x, y, z);
		}
		static PVector3 get_scripted_cover_point_coords(type::scrhandle coverpoint) {
			return invoker::invoke<PVector3>(0x594a1028fc2a3e85, coverpoint);
		}
		static void task_combat_ped(type::ped ped, type::ped targetped, int p2, int p3) {
			invoker::invoke<void>(0xf166e48407bac484, ped, targetped, p2, p3);
		}
		static void task_combat_ped_timed(type::any p0, type::ped ped, int duration, type::any p3) {
			invoker::invoke<void>(0x944f30dcb7096bde, p0, ped, duration, p3);
		}
		static void task_seek_cover_from_pos(type::ped ped, float x, float y, float z, int duration, bool p5) {
			invoker::invoke<void>(0x75ac2b60386d89f2, ped, x, y, z, duration, p5);
		}
		static void task_seek_cover_from_ped(type::ped ped, type::ped target, int duration, bool p3) {
			invoker::invoke<void>(0x84d32b3bec531324, ped, target, duration, p3);
		}
		static void task_seek_cover_to_cover_point(type::any p0, type::any p1, float p2, float p3, float p4, type::any p5, bool p6) {
			invoker::invoke<void>(0xd43d95c7a869447f, p0, p1, p2, p3, p4, p5, p6);
		}
		static void task_seek_cover_to_coords(type::ped ped, float x1, float y1, float z1, float x2, float y2, float z2, type::any p7, bool p8) {
			invoker::invoke<void>(0x39246a6958ef072c, ped, x1, y1, z1, x2, y2, z2, p7, p8);
		}
		static void task_put_ped_directly_into_cover(type::ped ped, float x, float y, float z, type::any timeout, bool p5, float p6, bool p7, bool p8, type::any p9, bool p10) {
			invoker::invoke<void>(0x4172393e6be1fece, ped, x, y, z, timeout, p5, p6, p7, p8, p9, p10);
		}
		static void task_exit_cover(type::ped ped, int p1, float posx, float posy, float posz) {
			invoker::invoke<void>(0x79b258e397854d29, ped, p1, posx, posy, posz);
		}
		static void task_put_ped_directly_into_melee(type::ped ped, type::ped target, float p2, float p3, float p4, float flag) {
			invoker::invoke<void>(0x1c6cd14a876ffe39, ped, target, p2, p3, p4, flag);
		}
		static void task_toggle_duck(bool p0, bool p1) {
			invoker::invoke<void>(0xac96609b9995edf8, p0, p1);
		}
		static void task_guard_current_position(type::ped p0, float p1, float p2, bool p3) {
			invoker::invoke<void>(0x4a58a47a72e3fcb4, p0, p1, p2, p3);
		}
		static void task_guard_assigned_defensive_area(type::any p0, float p1, float p2, float p3, float p4, float p5, type::any p6) {
			invoker::invoke<void>(0xd2a207eebdf9889b, p0, p1, p2, p3, p4, p5, p6);
		}
		static void task_guard_sphere_defensive_area(type::ped p0, float p1, float p2, float p3, float p4, float p5, type::any p6, float p7, float p8, float p9, float p10) {
			invoker::invoke<void>(0xc946fe14be0eb5e2, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
		}
		static void task_stand_guard(type::ped ped, float x, float y, float z, float heading, const char* scenarioname) {
			invoker::invoke<void>(0xae032f8bba959e90, ped, x, y, z, heading, scenarioname);
		}
		static void set_drive_task_cruise_speed(type::ped driver, float cruisespeed) {
			invoker::invoke<void>(0x5c9b84bd7d31d908, driver, cruisespeed);
		}
		static void set_drive_task_max_cruise_speed(type::any p0, float p1) {
			invoker::invoke<void>(0x404a5aa9b9f0b746, p0, p1);
		}
		static void set_drive_task_driving_style(type::ped ped, int drivingstyle) {
			invoker::invoke<void>(0xdace1be37d88af67, ped, drivingstyle);
		}
		static void add_cover_blocking_area(float playerx, float playery, float playerz, float radiusx, float radiusy, float radiusz, bool p6, bool p7, bool p8, bool p9) {
			invoker::invoke<void>(0x45c597097dd7cb81, playerx, playery, playerz, radiusx, radiusy, radiusz, p6, p7, p8, p9);
		}
		static void remove_all_cover_blocking_areas() {
			invoker::invoke<void>(0xdb6708c0b46f56d8);
		}
		static void task_start_scenario_in_place(type::ped ped, const char* scenarioname, int unkdelay, bool playenteranim) {
			invoker::invoke<void>(0x142a02425ff02bd9, ped, scenarioname, unkdelay, playenteranim);
		}
		static void task_start_scenario_at_position(type::ped ped, const char* scenarioname, float x, float y, float z, float heading, int duration, bool sittingscenario, bool teleport) {
			invoker::invoke<void>(0xfa4efc79f69d4f07, ped, scenarioname, x, y, z, heading, duration, sittingscenario, teleport);
		}
		static void task_use_nearest_scenario_to_coord(type::ped ped, float x, float y, float z, float distance, int duration) {
			invoker::invoke<void>(0x277f471ba9db000b, ped, x, y, z, distance, duration);
		}
		static void task_use_nearest_scenario_to_coord_warp(type::ped ped, float x, float y, float z, float radius, type::any p5) {
			invoker::invoke<void>(0x58e2e0f23f6b76c3, ped, x, y, z, radius, p5);
		}
		static void task_use_nearest_scenario_chain_to_coord(type::any p0, float p1, float p2, float p3, float p4, type::any p5) {
			invoker::invoke<void>(0x9fda1b3d7e7028b3, p0, p1, p2, p3, p4, p5);
		}
		static void task_use_nearest_scenario_chain_to_coord_warp(type::any p0, float p1, float p2, float p3, float p4, type::any p5) {
			invoker::invoke<void>(0x97a28e63f0ba5631, p0, p1, p2, p3, p4, p5);
		}
		static bool does_scenario_exist_in_area(float x, float y, float z, float radius, bool b) {
			return invoker::invoke<bool>(0x5a59271ffadd33c1, x, y, z, radius, b);
		}
		static bool does_scenario_of_type_exist_in_area(float p0, float p1, float p2, type::any* p3, float p4, bool p5) {
			return invoker::invoke<bool>(0x0a9d0c2a3bbc86c1, p0, p1, p2, p3, p4, p5);
		}
		static bool is_scenario_occupied(float x, float y, float z, float radius, bool p4) {
			return invoker::invoke<bool>(0x788756d73ac2e07c, x, y, z, radius, p4);
		}
		static bool ped_has_use_scenario_task(type::ped ped) {
			return invoker::invoke<bool>(0x295e3ccec879ccd7, ped);
		}
		static void play_anim_on_running_scenario(type::ped ped, const char* animdict, const char* animname) {
			invoker::invoke<void>(0x748040460f8df5dc, ped, animdict, animname);
		}
		static bool does_scenario_group_exist(const char* scenariogroup) {
			return invoker::invoke<bool>(0xf9034c136c9e00d3, scenariogroup);
		}
		static bool is_scenario_group_enabled(const char* scenariogroup) {
			return invoker::invoke<bool>(0x367a09ded4e05b99, scenariogroup);
		}
		static void set_scenario_group_enabled(const char* scenariogroup, bool p1) {
			invoker::invoke<void>(0x02c8e5b49848664e, scenariogroup, p1);
		}
		static void reset_scenario_groups_enabled() {
			invoker::invoke<void>(0xdd902d0349afad3a);
		}
		static void set_exclusive_scenario_group(const char* scenariogroup) {
			invoker::invoke<void>(0x535e97e1f7fc0c6a, scenariogroup);
		}
		static void reset_exclusive_scenario_group() {
			invoker::invoke<void>(0x4202bbcb8684563d);
		}
		static bool is_scenario_type_enabled(const char* scenariotype) {
			return invoker::invoke<bool>(0x3a815db3ea088722, scenariotype);
		}
		static void set_scenario_type_enabled(const char* scenariotype, bool toggle) {
			invoker::invoke<void>(0xeb47ec4e34fb7ee1, scenariotype, toggle);
		}
		static void reset_scenario_types_enabled() {
			invoker::invoke<void>(0x0d40ee2a7f2b2d6d);
		}
		static bool is_ped_active_in_scenario(type::ped ped) {
			return invoker::invoke<bool>(0xaa135f9482c82cc3, ped);
		}
		static bool _0x621c6e4729388e41(type::ped ped) {
			return invoker::invoke<bool>(0x621c6e4729388e41, ped);
		}
		static void _0x8fd89a6240813fd0(type::ped ped, bool p1, bool p2) {
			invoker::invoke<void>(0x8fd89a6240813fd0, ped, p1, p2);
		}
		static void task_combat_hated_targets_in_area(type::ped ped, float x, float y, float z, float radius, type::any p5) {
			invoker::invoke<void>(0x4cf5f55dac3280a0, ped, x, y, z, radius, p5);
		}
		static void task_combat_hated_targets_around_ped(type::ped ped, float radius, int p2) {
			invoker::invoke<void>(0x7bf835bb9e2698c8, ped, radius, p2);
		}
		static void task_combat_hated_targets_around_ped_timed(type::ped ped, float radius, int duration, bool unk) {
			invoker::invoke<void>(0x2bba30b854534a0c, ped, radius, duration, unk);
		}
		static void task_throw_projectile(int ped, float x, float y, float z) {
			invoker::invoke<void>(0x7285951dbf6b5a51, ped, x, y, z);
		}
		static void task_swap_weapon(type::ped ped, bool p1) {
			invoker::invoke<void>(0xa21c51255b205245, ped, p1);
		}
		static void task_reload_weapon(type::ped ped, bool unused) {
			invoker::invoke<void>(0x62d2916f56b9cd2d, ped, unused);
		}
		static bool is_ped_getting_up(type::ped ped) {
			return invoker::invoke<bool>(0x2a74e1d5f2f00eec, ped);
		}
		static void task_writhe(type::ped ped, type::ped target, int time, int p3) {
			invoker::invoke<void>(0xcddc2b77ce54ac6e, ped, target, time, p3);
		}
		static bool is_ped_in_writhe(type::ped ped) {
			return invoker::invoke<bool>(0xdeb6d52126e7d640, ped);
		}
		static void open_patrol_route(const char* patrolroute) {
			invoker::invoke<void>(0xa36bfb5ee89f3d82, patrolroute);
		}
		static void close_patrol_route() {
			invoker::invoke<void>(0xb043eca801b8cbc1);
		}
		static void add_patrol_route_node(int p0, const char* p1, float x1, float y1, float z1, float x2, float y2, float z2, int p8) {
			invoker::invoke<void>(0x8edf950167586b7c, p0, p1, x1, y1, z1, x2, y2, z2, p8);
		}
		static void add_patrol_route_link(type::any p0, type::any p1) {
			invoker::invoke<void>(0x23083260dec3a551, p0, p1);
		}
		static void create_patrol_route() {
			invoker::invoke<void>(0xaf8a443ccc8018dc);
		}
		static void delete_patrol_route(const char* patrolroute) {
			invoker::invoke<void>(0x7767dd9d65e91319, patrolroute);
		}
		static void task_patrol(type::ped ped, const char* p1, type::any p2, bool p3, bool p4) {
			invoker::invoke<void>(0xbda5df49d080fe4e, ped, p1, p2, p3, p4);
		}
		static void task_stay_in_cover(type::ped ped) {
			invoker::invoke<void>(0xe5da8615a6180789, ped);
		}
		static void add_vehicle_subtask_attack_coord(type::ped ped, float x, float y, float z) {
			invoker::invoke<void>(0x5cf0d8f9bba0dd75, ped, x, y, z);
		}
		static void add_vehicle_subtask_attack_ped(type::ped ped, type::ped ped2) {
			invoker::invoke<void>(0x85f462badc7da47f, ped, ped2);
		}
		static void task_vehicle_shoot_at_ped(type::ped ped, type::ped target, float flag) {
			invoker::invoke<void>(0x10ab107b887214d8, ped, target, flag);
		}
		static void task_vehicle_aim_at_ped(type::ped ped, type::ped target) {
			invoker::invoke<void>(0xe41885592b08b097, ped, target);
		}
		static void task_vehicle_shoot_at_coord(type::ped ped, float x, float y, float z, float p4) {
			invoker::invoke<void>(0x5190796ed39c9b6d, ped, x, y, z, p4);
		}
		static void task_vehicle_aim_at_coord(type::ped ped, float x, float y, float z) {
			invoker::invoke<void>(0x447c1e9ef844bc0f, ped, x, y, z);
		}
		static void task_vehicle_goto_navmesh(type::ped ped, type::vehicle vehicle, float x, float y, float z, float speed, int behaviorflag, float stoppingrange) {
			invoker::invoke<void>(0x195aeeb13cefe2ee, ped, vehicle, x, y, z, speed, behaviorflag, stoppingrange);
		}
		static void task_go_to_coord_while_aiming_at_coord(type::ped ped, float x, float y, float z, float aimatx, float aimaty, float aimatz, float movespeed, bool p8, float p9, float p10, bool p11, type::any flags, bool p13, type::hash firingpattern) {
			invoker::invoke<void>(0x11315ab3385b8ac0, ped, x, y, z, aimatx, aimaty, aimatz, movespeed, p8, p9, p10, p11, flags, p13, firingpattern);
		}
		static void task_go_to_coord_while_aiming_at_entity(type::any p0, float p1, float p2, float p3, type::any p4, float p5, bool p6, float p7, float p8, bool p9, type::any p10, bool p11, type::any p12, type::any p13) {
			invoker::invoke<void>(0xb2a16444ead9ae47, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);
		}
		static void task_go_to_coord_and_aim_at_hated_entities_near_coord(type::ped pedhandle, float gotolocationx, float gotolocationy, float gotolocationz, float focuslocationx, float focuslocationy, float focuslocationz, float speed, bool shootatenemies, float distancetostopat, float noroadsdistance, bool unktrue, int unkflag, int aimingflag, type::hash firingpattern) {
			invoker::invoke<void>(0xa55547801eb331fc, pedhandle, gotolocationx, gotolocationy, gotolocationz, focuslocationx, focuslocationy, focuslocationz, speed, shootatenemies, distancetostopat, noroadsdistance, unktrue, unkflag, aimingflag, firingpattern);
		}
		static void task_go_to_entity_while_aiming_at_coord(type::any p0, type::any p1, float p2, float p3, float p4, float p5, bool p6, float p7, float p8, bool p9, bool p10, type::any p11) {
			invoker::invoke<void>(0x04701832b739dce5, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
		}
		static void task_go_to_entity_while_aiming_at_entity(type::ped ped, type::entity entitytowalkto, type::entity entitytoaimat, float speed, bool shootatentity, float p5, float p6, bool p7, bool p8, type::hash firingpattern) {
			invoker::invoke<void>(0x97465886d35210e9, ped, entitytowalkto, entitytoaimat, speed, shootatentity, p5, p6, p7, p8, firingpattern);
		}
		static void set_high_fall_task(type::ped ped, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x8c825bdc7741d37c, ped, p1, p2, p3);
		}
		static void request_waypoint_recording(const char* name) {
			invoker::invoke<void>(0x9eefb62eb27b5792, name);
		}
		static bool get_is_waypoint_recording_loaded(const char* name) {
			return invoker::invoke<bool>(0xcb4e8be8a0063c5d, name);
		}
		static void remove_waypoint_recording(const char* name) {
			invoker::invoke<void>(0xff1b8b4aa1c25dc8, name);
		}
		static bool waypoint_recording_get_num_points(const char* name, int* points) {
			return invoker::invoke<bool>(0x5343532c01a07234, name, points);
		}
		static bool waypoint_recording_get_coord(const char* name, int point, PVector3* coord) {
			return invoker::invoke<bool>(0x2fb897405c90b361, name, point, coord);
		}
		static float waypoint_recording_get_speed_at_point(const char* name, int point) {
			return invoker::invoke<float>(0x005622aebc33aca9, name, point);
		}
		static bool waypoint_recording_get_closest_waypoint(const char* name, float x, float y, float z, int* point) {
			return invoker::invoke<bool>(0xb629a298081f876f, name, x, y, z, point);
		}
		static void task_follow_waypoint_recording(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4) {
			invoker::invoke<void>(0x0759591819534f7b, p0, p1, p2, p3, p4);
		}
		static bool is_waypoint_playback_going_on_for_ped(type::any p0) {
			return invoker::invoke<bool>(0xe03b3f2d3dc59b64, p0);
		}
		static int get_ped_waypoint_progress(type::ped ped) {
			return invoker::invoke<int>(0x2720aaa75001e094, ped);
		}
		static float get_ped_waypoint_distance(type::ped ped) {
			return invoker::invoke<float>(0xe6a877c64caf1bc5, ped);
		}
		static type::any set_ped_waypoint_route_offset(type::ped ped, float offsetx, float offsety, float offsetz) {
			return invoker::invoke<type::any>(0xed98e10b0afce4b4, ped, offsetx, offsety, offsetz);
		}
		static float get_waypoint_distance_along_route(const char* p0, int p1) {
			return invoker::invoke<float>(0xa5b769058763e497, p0, p1);
		}
		static bool waypoint_playback_get_is_paused(type::any p0) {
			return invoker::invoke<bool>(0x701375a7d43f01cb, p0);
		}
		static void waypoint_playback_pause(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x0f342546aa06fed5, p0, p1, p2);
		}
		static void waypoint_playback_resume(type::any p0, bool p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x244f70c84c547d2d, p0, p1, p2, p3);
		}
		static void waypoint_playback_override_speed(type::any p0, float p1, bool p2) {
			invoker::invoke<void>(0x7d7d2b47fa788e85, p0, p1, p2);
		}
		static void waypoint_playback_use_default_speed(type::any p0) {
			invoker::invoke<void>(0x6599d834b12d0800, p0);
		}
		static void use_waypoint_recording_as_assisted_movement_route(type::any* p0, bool p1, float p2, float p3) {
			invoker::invoke<void>(0x5a353b8e6b1095b5, p0, p1, p2, p3);
		}
		static void waypoint_playback_start_aiming_at_ped(type::any p0, type::any p1, bool p2) {
			invoker::invoke<void>(0x20e330937c399d29, p0, p1, p2);
		}
		static void waypoint_playback_start_aiming_at_coord(type::any p0, float p1, float p2, float p3, bool p4) {
			invoker::invoke<void>(0x8968400d900ed8b3, p0, p1, p2, p3, p4);
		}
		static void _waypoint_playback_start_shooting_at_ped(type::any p0, type::any p1, bool p2, type::any p3) {
			invoker::invoke<void>(0xe70ba7b90f8390dc, p0, p1, p2, p3);
		}
		static void waypoint_playback_start_shooting_at_coord(type::any p0, float p1, float p2, float p3, bool p4, type::any p5) {
			invoker::invoke<void>(0x057a25cfcc9db671, p0, p1, p2, p3, p4, p5);
		}
		static void waypoint_playback_stop_aiming_or_shooting(type::any p0) {
			invoker::invoke<void>(0x47efa040ebb8e2ea, p0);
		}
		static void assisted_movement_request_route(const char* route) {
			invoker::invoke<void>(0x817268968605947a, route);
		}
		static void assisted_movement_remove_route(const char* route) {
			invoker::invoke<void>(0x3548536485dd792b, route);
		}
		static bool assisted_movement_is_route_loaded(const char* route) {
			return invoker::invoke<bool>(0x60f9a4393a21f741, route);
		}
		static void assisted_movement_set_route_properties(const char* route, int props) {
			invoker::invoke<void>(0xd5002d78b7162e1b, route, props);
		}
		static void assisted_movement_override_load_distance_this_frame(float dist) {
			invoker::invoke<void>(0x13945951e16ef912, dist);
		}
		static void task_vehicle_follow_waypoint_recording(type::ped ped, type::vehicle vehicle, const char* wprecording, int p3, int p4, int p5, int p6, float p7, bool p8, float p9) {
			invoker::invoke<void>(0x3123faa6db1cf7ed, ped, vehicle, wprecording, p3, p4, p5, p6, p7, p8, p9);
		}
		static bool is_waypoint_playback_going_on_for_vehicle(type::any p0) {
			return invoker::invoke<bool>(0xf5134943ea29868c, p0);
		}
		static int get_vehicle_waypoint_progress(type::vehicle vehicle) {
			return invoker::invoke<int>(0x9824cff8fc66e159, vehicle);
		}
		static type::any get_vehicle_waypoint_target_point(type::ped ped) {
			return invoker::invoke<type::any>(0x416b62ac8b9e5bbd, ped);
		}
		static void vehicle_waypoint_playback_pause(type::any p0) {
			invoker::invoke<void>(0x8a4e6ac373666bc5, p0);
		}
		static void vehicle_waypoint_playback_resume(type::any p0) {
			invoker::invoke<void>(0xdc04fcaa7839d492, p0);
		}
		static void vehicle_waypoint_playback_use_default_speed(type::any p0) {
			invoker::invoke<void>(0x5ceb25a7d2848963, p0);
		}
		static void vehicle_waypoint_playback_override_speed(type::any p0, float p1) {
			invoker::invoke<void>(0x121f0593e0a431d7, p0, p1);
		}
		static void task_set_blocking_of_non_temporary_events(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x90d2156198831d69, ped, toggle);
		}
		static void task_force_motion_state(type::ped ped, type::hash state, bool p2) {
			invoker::invoke<void>(0x4f056e1affef17ab, ped, state, p2);
		}
		static void _task_move_network(type::ped ped, const char* task, float multiplier, bool p3, const char* animdict, int flags) {
			invoker::invoke<void>(0x2d537ba194896636, ped, task, multiplier, p3, animdict, flags);
		}
		static void _task_move_network_advanced(type::ped ped, const char* p1, float p2, float p3, float p4, float p5, float p6, float p7, type::any p8, float p9, bool p10, const char* animdict, int flags) {
			invoker::invoke<void>(0xd5b35bea41919acb, ped, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, animdict, flags);
		}
		static bool _is_task_move_scripted_active(type::ped handle) {
			return invoker::invoke<bool>(0x921ce12c489c4c41, handle);
		}
		static bool _0x30ed88d5e0c56a37(type::any p0) {
			return invoker::invoke<bool>(0x30ed88d5e0c56a37, p0);
		}
		static type::any _set_network_task_action(type::ped ped, const char* p1) {
			return invoker::invoke<type::any>(0xd01015c7316ae176, ped, p1);
		}
		static type::any _0xab13a5565480b6d9(type::ped ped, const char* unk) {
			return invoker::invoke<type::any>(0xab13a5565480b6d9, ped, unk);
		}
		static const char* _get_ped_network_task_phase(type::ped ped) {
			return invoker::invoke<const char*>(0x717e4d1f2048376d, ped);
		}
		static void _set_network_task_param_float(type::ped ped, const char* p1, float p2) {
			invoker::invoke<void>(0xd5bb4025ae449a4e, ped, p1, p2);
		}
		static void _set_network_task_param_bool(type::ped ped, const char* p1, bool p2) {
			invoker::invoke<void>(0xb0a6cfd2c69c1088, ped, p1, p2);
		}
		static bool _0xa7ffba498e4aaf67(type::ped ped, const char* p1) {
			return invoker::invoke<bool>(0xa7ffba498e4aaf67, ped, p1);
		}
		static bool _0xb4f47213df45a64c(type::ped ped, const char* p1) {
			return invoker::invoke<bool>(0xb4f47213df45a64c, ped, p1);
		}
		static bool is_move_blend_ratio_still(type::ped ped) {
			return invoker::invoke<bool>(0x349ce7b56dafd95c, ped);
		}
		static bool is_move_blend_ratio_walking(type::ped ped) {
			return invoker::invoke<bool>(0xf133bbbe91e1691f, ped);
		}
		static bool is_move_blend_ratio_running(type::ped ped) {
			return invoker::invoke<bool>(0xd4d8636c0199a939, ped);
		}
		static bool is_move_blend_ratio_sprinting(type::ped ped) {
			return invoker::invoke<bool>(0x24a2ad74fa9814e2, ped);
		}
		static bool is_ped_still(type::ped ped) {
			return invoker::invoke<bool>(0xac29253eef8f0180, ped);
		}
		static bool is_ped_walking(type::ped ped) {
			return invoker::invoke<bool>(0xde4c184b2b9b071a, ped);
		}
		static bool is_ped_running(type::ped ped) {
			return invoker::invoke<bool>(0xc5286ffc176f28a2, ped);
		}
		static bool is_ped_sprinting(type::ped ped) {
			return invoker::invoke<bool>(0x57e457cd2c0fc168, ped);
		}
		static bool is_ped_strafing(type::ped ped) {
			return invoker::invoke<bool>(0xe45b7f222de47e09, ped);
		}
		static void task_synchronized_scene(type::ped ped, int scene, const char* animdictionary, const char* animationname, float speed, float speedmultiplier, int headingflag, int flag, float playbackrate, type::any p9) {
			invoker::invoke<void>(0xeea929141f699854, ped, scene, animdictionary, animationname, speed, speedmultiplier, headingflag, flag, playbackrate, p9);
		}
		static void task_sweep_aim_entity(type::ped ped, const char* animdict, const char* animname1, const char* animname2, const char* animname3, int duration, type::entity entity, float p7, float p8) {
			invoker::invoke<void>(0x2047c02158d6405a, ped, animdict, animname1, animname2, animname3, duration, entity, p7, p8);
		}
		static void update_task_sweep_aim_entity(type::ped ped, type::entity entity) {
			invoker::invoke<void>(0xe4973dbdbe6e44b3, ped, entity);
		}
		static void task_sweep_aim_position(type::ped ped, const char* animdict, const char* animname1, const char* animname2, const char* animname3, int timeout, float x, float y, float z, float unk, float flag) {
			invoker::invoke<void>(0x7afe8fdc10bc07d2, ped, animdict, animname1, animname2, animname3, timeout, x, y, z, unk, flag);
		}
		static void update_task_sweep_aim_position(type::ped ped, float x, float y, float z) {
			invoker::invoke<void>(0xbb106883f5201fc4, ped, x, y, z);
		}
		static void task_arrest_ped(type::ped ped, type::ped target) {
			invoker::invoke<void>(0xf3b9a78a178572b1, ped, target);
		}
		static bool is_ped_running_arrest_task(type::ped ped) {
			return invoker::invoke<bool>(0x3dc52677769b4ae0, ped);
		}
		static bool is_ped_being_arrested(type::ped ped) {
			return invoker::invoke<bool>(0x90a09f3a45fed688, ped);
		}
		static void uncuff_ped(type::ped ped) {
			invoker::invoke<void>(0x67406f2c8f87fc4f, ped);
		}
		static bool is_ped_cuffed(type::ped ped) {
			return invoker::invoke<bool>(0x74e559b3bc910685, ped);
		}
	}

	namespace gameplay {
		static int get_allocated_stack_size() {
			return invoker::invoke<int>(0x8b3ca62b1ef19b62);
		}
		static int _get_free_stack_slots_count(int threadid) {
			return invoker::invoke<int>(0xfead16fc8f9dfc0f, threadid);
		}
		static void set_random_seed(int time) {
			invoker::invoke<void>(0x444d98f98c11f3ec, time);
		}
		static void set_time_scale(float time) {
			invoker::invoke<void>(0x1d408577d440e81e, time);
		}
		static void set_mission_flag(bool toggle) {
			invoker::invoke<void>(0xc4301e5121a0ed73, toggle);
		}
		static bool get_mission_flag() {
			return invoker::invoke<bool>(0xa33cdccda663159e);
		}
		static void set_random_event_flag(bool p0) {
			invoker::invoke<void>(0x971927086cfd2158, p0);
		}
		static bool get_random_event_flag() {
			return invoker::invoke<bool>(0xd2d57f1d764117b1);
		}
		static const char* _get_global_char_buffer() {
			return invoker::invoke<const char*>(0x24da7d7667fd7b09);
		}
		static void _0x4dcdf92bf64236cd(const char* p0, const char* p1) {
			invoker::invoke<void>(0x4dcdf92bf64236cd, p0, p1);
		}
		static void _0x31125fd509d9043f(type::any* p0) {
			invoker::invoke<void>(0x31125fd509d9043f, p0);
		}
		static void _0xebd3205a207939ed(type::any* p0) {
			invoker::invoke<void>(0xebd3205a207939ed, p0);
		}
		static void _0x97e7e2c04245115b(type::any p0) {
			invoker::invoke<void>(0x97e7e2c04245115b, p0);
		}
		static void _0xeb078ca2b5e82add(type::any p0, type::any p1) {
			invoker::invoke<void>(0xeb078ca2b5e82add, p0, p1);
		}
		static void _0x703cc7f60cbb2b57(type::any p0) {
			invoker::invoke<void>(0x703cc7f60cbb2b57, p0);
		}
		static void _0x8951eb9c6906d3c8() {
			invoker::invoke<void>(0x8951eb9c6906d3c8);
		}
		static void _0xba4b8d83bdc75551(type::any p0) {
			invoker::invoke<void>(0xba4b8d83bdc75551, p0);
		}
		static bool _0xe8b9c0ec9e183f35() {
			return invoker::invoke<bool>(0xe8b9c0ec9e183f35);
		}
		static void _0x65d2ebb47e1cec21(bool p0) {
			invoker::invoke<void>(0x65d2ebb47e1cec21, p0);
		}
		static void _0x6f2135b6129620c1(bool p0) {
			invoker::invoke<void>(0x6f2135b6129620c1, p0);
		}
		static void _0x8d74e26f54b4e5c3(const char* p0) {
			invoker::invoke<void>(0x8d74e26f54b4e5c3, p0);
		}
		static bool _0xb335f761606db47c(type::any* p1, type::any* p2, type::any p3, bool p4) {
			return invoker::invoke<bool>(0xb335f761606db47c, p1, p2, p3, p4);
		}
		static type::hash get_prev_weather_type_hash_name() {
			return invoker::invoke<type::hash>(0x564b884a05ec45a3);
		}
		static type::hash get_next_weather_type_hash_name() {
			return invoker::invoke<type::hash>(0x711327cd09c8f162);
		}
		static bool is_prev_weather_type(const char* weathertype) {
			return invoker::invoke<bool>(0x44f28f86433b10a9, weathertype);
		}
		static bool is_next_weather_type(const char* weathertype) {
			return invoker::invoke<bool>(0x2faa3a30bec0f25d, weathertype);
		}
		static void set_weather_type_persist(const char* weathertype) {
			invoker::invoke<void>(0x704983df373b198f, weathertype);
		}
		static void set_weather_type_now_persist(const char* weathertype) {
			invoker::invoke<void>(0xed712ca327900c8a, weathertype);
		}
		static void set_weather_type_now(const char* weathertype) {
			invoker::invoke<void>(0x29b487c359e19889, weathertype);
		}
		static void _set_weather_type_over_time(const char* weathertype, float time) {
			invoker::invoke<void>(0xfb5045b7c42b75bf, weathertype, time);
		}
		static void set_random_weather_type() {
			invoker::invoke<void>(0x8b05f884cf7e8020);
		}
		static void clear_weather_type_persist() {
			invoker::invoke<void>(0xccc39339bef76cf5);
		}
		static void _get_weather_type_transition(type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0xf3bbe884a14bb413, p1, p2, p3);
		}
		static void _set_weather_type_transition(type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x578c752848ecfa0c, p1, p2, p3);
		}
		static void set_override_weather(const char* weathertype) {
			invoker::invoke<void>(0xa43d5c6fe51adbef, weathertype);
		}
		static void clear_override_weather() {
			invoker::invoke<void>(0x338d2e3477711050);
		}
		static void _0xb8f87ead7533b176(float p0) {
			invoker::invoke<void>(0xb8f87ead7533b176, p0);
		}
		static void _0xc3ead29ab273ece8(float p0) {
			invoker::invoke<void>(0xc3ead29ab273ece8, p0);
		}
		static void _0xa7a1127490312c36(float p0) {
			invoker::invoke<void>(0xa7a1127490312c36, p0);
		}
		static void _0x31727907b2c43c55(float p0) {
			invoker::invoke<void>(0x31727907b2c43c55, p0);
		}
		static void _0x405591ec8fd9096d(float p0) {
			invoker::invoke<void>(0x405591ec8fd9096d, p0);
		}
		static void _0xf751b16fb32abc1d(float p0) {
			invoker::invoke<void>(0xf751b16fb32abc1d, p0);
		}
		static void _0xb3e6360dde733e82(float p0) {
			invoker::invoke<void>(0xb3e6360dde733e82, p0);
		}
		static void _0x7c9c0b1eeb1f9072(float p0) {
			invoker::invoke<void>(0x7c9c0b1eeb1f9072, p0);
		}
		static void _0x6216b116083a7cb4(float p0) {
			invoker::invoke<void>(0x6216b116083a7cb4, p0);
		}
		static void _0x9f5e6bb6b34540da(float p0) {
			invoker::invoke<void>(0x9f5e6bb6b34540da, p0);
		}
		static void _0xb9854dfde0d833d6(float p0) {
			invoker::invoke<void>(0xb9854dfde0d833d6, p0);
		}
		static void _0xc54a08c85ae4d410(float p0) {
			invoker::invoke<void>(0xc54a08c85ae4d410, p0);
		}
		static void _0xa8434f1dff41d6e7(float p0) {
			invoker::invoke<void>(0xa8434f1dff41d6e7, p0);
		}
		static void _0xc3c221addde31a11(float p0) {
			invoker::invoke<void>(0xc3c221addde31a11, p0);
		}
		static void set_wind(float speed) {
			invoker::invoke<void>(0xac3a74e8384a9919, speed);
		}
		static void set_wind_speed(float speed) {
			invoker::invoke<void>(0xee09ecedbabe47fc, speed);
		}
		static float get_wind_speed() {
			return invoker::invoke<float>(0xa8cf1cc0afcd3f12);
		}
		static void set_wind_direction(float direction) {
			invoker::invoke<void>(0xeb0f4468467b4528, direction);
		}
		static PVector3 get_wind_direction() {
			return invoker::invoke<PVector3>(0x1f400fef721170da);
		}
		static void _set_rain_fx_intensity(float intensity) {
			invoker::invoke<void>(0x643e26ea6e024d92, intensity);
		}
		static float get_rain_level() {
			return invoker::invoke<float>(0x96695e368ad855f3);
		}
		static float get_snow_level() {
			return invoker::invoke<float>(0xc5868a966e5be3ae);
		}
		static void force_lightning_flash() {
			invoker::invoke<void>(0xf6062e089251c898);
		}
		static void _0x02deaac8f8ea7fe7(const char* p0) {
			invoker::invoke<void>(0x02deaac8f8ea7fe7, p0);
		}
		static void preload_cloud_hat(const char* name) {
			invoker::invoke<void>(0x11b56fbbf7224868, name);
		}
		static void load_cloud_hat(const char* name, float transitiontime) {
			invoker::invoke<void>(0xfc4842a34657bfcb, name, transitiontime);
		}
		static void unload_cloud_hat(const char* name, float p1) {
			invoker::invoke<void>(0xa74802fb8d0b7814, name, p1);
		}
		static void _clear_cloud_hat() {
			invoker::invoke<void>(0x957e790ea1727b64);
		}
		static void _set_cloud_hat_opacity(float opacity) {
			invoker::invoke<void>(0xf36199225d6d8c86, opacity);
		}
		static float _get_cloud_hat_opacity() {
			return invoker::invoke<float>(0x20ac25e781ae4a84);
		}
		static int get_game_timer() {
			return invoker::invoke<int>(0x9cd27b0045628463);
		}
		static float get_frame_time() {
			return invoker::invoke<float>(0x15c40837039ffaf7);
		}
		static float _get_benchmark_time() {
			return invoker::invoke<float>(0xe599a503b3837e1b);
		}
		static int get_frame_count() {
			return invoker::invoke<int>(0xfc8202efc642e6f2);
		}
		static float get_random_float_in_range(float startrange, float endrange) {
			return invoker::invoke<float>(0x313ce5879ceb6fcd, startrange, endrange);
		}
		static int get_random_int_in_range(int startrange, int endrange) {
			return invoker::invoke<int>(0xd53343aa4fb7dd28, startrange, endrange);
		}
		static bool get_ground_z_for_3d_coord(float x, float y, float z, float* groundz, bool unk) {
			return invoker::invoke<bool>(0xc906a7dab05c8d2b, x, y, z, groundz, unk);
		}
		static bool _get_ground_z_coord_with_offsets(float x, float y, float z, float* groundz, PVector3* offsets) {
			return invoker::invoke<bool>(0x8bdc7bfc57a81e76, x, y, z, groundz, offsets);
		}
		static float asin(float p0) {
			return invoker::invoke<float>(0xc843060b5765dce7, p0);
		}
		static float acos(float p0) {
			return invoker::invoke<float>(0x1d08b970013c34b6, p0);
		}
		static float tan(float p0) {
			return invoker::invoke<float>(0x632106cc96e82e91, p0);
		}
		static float atan(float p0) {
			return invoker::invoke<float>(0xa9d1795cd5043663, p0);
		}
		static float atan2(float p0, float p1) {
			return invoker::invoke<float>(0x8927cbf9d22261a4, p0, p1);
		}
		static float get_distance_between_coords(float x1, float y1, float z1, float x2, float y2, float z2, bool usez) {
			return invoker::invoke<float>(0xf1b760881820c952, x1, y1, z1, x2, y2, z2, usez);
		}
		static float get_angle_between_2d_vectors(float x1, float y1, float x2, float y2) {
			return invoker::invoke<float>(0x186fc4be848e1c92, x1, y1, x2, y2);
		}
		static float get_heading_from_vector_2d(float dx, float dy) {
			return invoker::invoke<float>(0x2ffb6b224f4b2926, dx, dy);
		}
		static float _0x7f8f6405f4777af6(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, bool p9) {
			return invoker::invoke<float>(0x7f8f6405f4777af6, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
		}
		static PVector3 _0x21c235bc64831e5a(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, bool p9) {
			return invoker::invoke<PVector3>(0x21c235bc64831e5a, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
		}
		static bool _0xf56dfb7b61be7276(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11, type::any* p12) {
			return invoker::invoke<bool>(0xf56dfb7b61be7276, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
		}
		static void set_bit(int* address, int offset) {
			invoker::invoke<void>(0x933d6a9eec1bacd0, address, offset);
		}
		static void clear_bit(int* address, int offset) {
			invoker::invoke<void>(0xe80492a9ac099a93, address, offset);
		}
		static type::hash get_hash_key(const char* string) {
			return invoker::invoke<type::hash>(0xd24d37cc275948cc, string);
		}
		static void _0xf2f6a2fa49278625(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, type::any* p9, type::any* p10, type::any* p11, type::any* p12) {
			invoker::invoke<void>(0xf2f6a2fa49278625, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
		}
		static bool is_area_occupied(float p0, float p1, float p2, float p3, float p4, float p5, bool p6, bool p7, bool p8, bool p9, bool p10, type::any p11, bool p12) {
			return invoker::invoke<bool>(0xa61b4df533dcb56e, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
		}
		static bool is_position_occupied(float x, float y, float z, float range, bool p4, bool p5, bool p6, bool p7, bool p8, type::any p9, bool p10) {
			return invoker::invoke<bool>(0xadcde75e1c60f32d, x, y, z, range, p4, p5, p6, p7, p8, p9, p10);
		}
		static bool is_point_obscured_by_a_mission_entity(float p0, float p1, float p2, float p3, float p4, float p5, type::any p6) {
			return invoker::invoke<bool>(0xe54e209c35ffa18d, p0, p1, p2, p3, p4, p5, p6);
		}
		static void clear_area(float x, float y, float z, float radius, bool p4, bool ignorecopcars, bool ignoreobjects, bool p7) {
			invoker::invoke<void>(0xa56f01f3765b93a0, x, y, z, radius, p4, ignorecopcars, ignoreobjects, p7);
		}
		static void _clear_area_of_everything(float x, float y, float z, float radius, bool p4, bool p5, bool p6, bool p7) {
			invoker::invoke<void>(0x957838aaf91bd12d, x, y, z, radius, p4, p5, p6, p7);
		}
		static void clear_area_of_vehicles(float x, float y, float z, float radius, bool p4, bool p5, bool p6, bool p7, bool p8) {
			invoker::invoke<void>(0x01c7b9b38428aeb6, x, y, z, radius, p4, p5, p6, p7, p8);
		}
		static void clear_angled_area_of_vehicles(float p0, float p1, float p2, float p3, float p4, float p5, float p6, bool p7, bool p8, bool p9, bool p10, bool p11) {
			invoker::invoke<void>(0x11db3500f042a8aa, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
		}
		static void clear_area_of_objects(float x, float y, float z, float radius, int flags) {
			invoker::invoke<void>(0xdd9b9b385aac7f5b, x, y, z, radius, flags);
		}
		static void clear_area_of_peds(float x, float y, float z, float radius, int flags) {
			invoker::invoke<void>(0xbe31fd6ce464ac59, x, y, z, radius, flags);
		}
		static void clear_area_of_cops(float x, float y, float z, float radius, int flags) {
			invoker::invoke<void>(0x04f8fc8fcf58f88d, x, y, z, radius, flags);
		}
		static void clear_area_of_projectiles(float x, float y, float z, float radius, bool isnetworkgame) {
			invoker::invoke<void>(0x0a1cb9094635d1a6, x, y, z, radius, isnetworkgame);
		}
		static void _0x7ec6f9a478a6a512() {
			invoker::invoke<void>(0x7ec6f9a478a6a512);
		}
		static void set_save_menu_active(bool unk) {
			invoker::invoke<void>(0xc9bf75d28165ff77, unk);
		}
		static int _0x397baa01068baa96() {
			return invoker::invoke<int>(0x397baa01068baa96);
		}
		static void set_credits_active(bool toggle) {
			invoker::invoke<void>(0xb938b7e6d3c0620c, toggle);
		}
		static void _0xb51b9ab9ef81868c(bool toggle) {
			invoker::invoke<void>(0xb51b9ab9ef81868c, toggle);
		}
		static bool have_credits_reached_end() {
			return invoker::invoke<bool>(0x075f1d57402c93ba);
		}
		static void terminate_all_scripts_with_this_name(const char* scriptname) {
			invoker::invoke<void>(0x9dc711bc69c548df, scriptname);
		}
		static void network_set_script_is_safe_for_network_game() {
			invoker::invoke<void>(0x9243bac96d64c050);
		}
		static int add_hospital_restart(float x, float y, float z, float p3, type::any p4) {
			return invoker::invoke<int>(0x1f464ef988465a81, x, y, z, p3, p4);
		}
		static void disable_hospital_restart(int hospitalindex, bool toggle) {
			invoker::invoke<void>(0xc8535819c450eba8, hospitalindex, toggle);
		}
		static type::any add_police_restart(float p0, float p1, float p2, float p3, type::any p4) {
			return invoker::invoke<type::any>(0x452736765b31fc4b, p0, p1, p2, p3, p4);
		}
		static void disable_police_restart(int policeindex, bool toggle) {
			invoker::invoke<void>(0x23285ded6ebd7ea3, policeindex, toggle);
		}
		static void _set_custom_respawn_position(float x, float y, float z, float heading) {
			invoker::invoke<void>(0x706b5edcaa7fa663, x, y, z, heading);
		}
		static void _set_next_respawn_to_custom() {
			invoker::invoke<void>(0xa2716d40842eaf79);
		}
		static void _disable_automatic_respawn(bool disablerespawn) {
			invoker::invoke<void>(0x2c2b3493fbf51c71, disablerespawn);
		}
		static void ignore_next_restart(bool toggle) {
			invoker::invoke<void>(0x21ffb63d8c615361, toggle);
		}
		static void set_fade_out_after_death(bool toggle) {
			invoker::invoke<void>(0x4a18e01df2c87b86, toggle);
		}
		static void set_fade_out_after_arrest(bool toggle) {
			invoker::invoke<void>(0x1e0b4dc0d990a4e7, toggle);
		}
		static void set_fade_in_after_death_arrest(bool toggle) {
			invoker::invoke<void>(0xda66d2796ba33f12, toggle);
		}
		static void set_fade_in_after_load(bool toggle) {
			invoker::invoke<void>(0xf3d78f59dfe18d79, toggle);
		}
		static type::any register_save_house(float p0, float p1, float p2, float p3, type::any* p4, type::any p5, type::any p6) {
			return invoker::invoke<type::any>(0xc0714d0a7eeeca54, p0, p1, p2, p3, p4, p5, p6);
		}
		static void set_save_house(int index, bool p1, bool p2) {
			invoker::invoke<void>(0x4f548cabeae553bc, index, p1, p2);
		}
		static bool override_save_house(bool p0, float p1, float p2, float p3, float p4, bool p5, float p6, float p7) {
			return invoker::invoke<bool>(0x1162ea8ae9d24eea, p0, p1, p2, p3, p4, p5, p6, p7);
		}
		static type::any _0xa4a0065e39c9f25c(type::any p0, type::any p1, type::any p2, type::any p3) {
			return invoker::invoke<type::any>(0xa4a0065e39c9f25c, p0, p1, p2, p3);
		}
		static void do_auto_save() {
			invoker::invoke<void>(0x50eeaad86232ee55);
		}
		static type::any _0x6e04f06094c87047() {
			return invoker::invoke<type::any>(0x6e04f06094c87047);
		}
		static bool is_auto_save_in_progress() {
			return invoker::invoke<bool>(0x69240733738c19a0);
		}
		static type::any _0x2107a3773771186d() {
			return invoker::invoke<type::any>(0x2107a3773771186d);
		}
		static void _0x06462a961e94b67c() {
			invoker::invoke<void>(0x06462a961e94b67c);
		}
		static void begin_replay_stats(type::any p0, type::any p1) {
			invoker::invoke<void>(0xe0e500246ff73d66, p0, p1);
		}
		static void add_replay_stat_value(type::any value) {
			invoker::invoke<void>(0x69fe6dc87bd2a5e9, value);
		}
		static void end_replay_stats() {
			invoker::invoke<void>(0xa23e821fbdf8a5f2);
		}
		static type::any _0xd642319c54aadeb6() {
			return invoker::invoke<type::any>(0xd642319c54aadeb6);
		}
		static type::any _0x5b1f2e327b6b6fe1() {
			return invoker::invoke<type::any>(0x5b1f2e327b6b6fe1);
		}
		static type::any _0x2b626a0150e4d449() {
			return invoker::invoke<type::any>(0x2b626a0150e4d449);
		}
		static type::any _0xdc9274a7ef6b2867() {
			return invoker::invoke<type::any>(0xdc9274a7ef6b2867);
		}
		static type::any _0x8098c8d6597aae18(type::any p0) {
			return invoker::invoke<type::any>(0x8098c8d6597aae18, p0);
		}
		static void clear_replay_stats() {
			invoker::invoke<void>(0x1b1ab132a16fda55);
		}
		static type::any _0x72de52178c291cb5() {
			return invoker::invoke<type::any>(0x72de52178c291cb5);
		}
		static type::any _0x44a0bdc559b35f6e() {
			return invoker::invoke<type::any>(0x44a0bdc559b35f6e);
		}
		static type::any _0xeb2104e905c6f2e9() {
			return invoker::invoke<type::any>(0xeb2104e905c6f2e9);
		}
		static type::any _0x2b5e102e4a42f2bf() {
			return invoker::invoke<type::any>(0x2b5e102e4a42f2bf);
		}
		static bool is_memory_card_in_use() {
			return invoker::invoke<bool>(0x8a75ce2956274add);
		}
		static void shoot_single_bullet_between_coords(float x1, float y1, float z1, float x2, float y2, float z2, int damage, bool p7, type::hash weaponhash, type::ped ownerped, bool isaudible, bool isinvisible, float speed) {
			invoker::invoke<void>(0x867654cbc7606f2c, x1, y1, z1, x2, y2, z2, damage, p7, weaponhash, ownerped, isaudible, isinvisible, speed);
		}
		static void _shoot_single_bullet_between_coords_preset_params(float x1, float y1, float z1, float x2, float y2, float z2, int damage, bool p7, type::hash weaponhash, type::ped ownerped, bool isaudible, bool isinvisible, float speed, type::entity entity) {
			invoker::invoke<void>(0xe3a7742e0b7a2f8b, x1, y1, z1, x2, y2, z2, damage, p7, weaponhash, ownerped, isaudible, isinvisible, speed, entity);
		}
		static void _shoot_single_bullet_between_coords_with_extra_params(float x1, float y1, float z1, float x2, float y2, float z2, int damage, bool p7, type::hash weaponhash, type::ped ownerped, bool isaudible, bool isinvisible, float speed, type::entity entity, bool p14, bool p15, bool p16, bool p17) {
			invoker::invoke<void>(0xbfe5756e7407064a, x1, y1, z1, x2, y2, z2, damage, p7, weaponhash, ownerped, isaudible, isinvisible, speed, entity, p14, p15, p16, p17);
		}
		static void get_model_dimensions(type::hash modelhash, PVector3* minimum, PVector3* maximum) {
			invoker::invoke<void>(0x03e8d3d5f549087a, modelhash, minimum, maximum);
		}
		static void set_fake_wanted_level(int fakewantedlevel) {
			invoker::invoke<void>(0x1454f2448de30163, fakewantedlevel);
		}
		static int get_fake_wanted_level() {
			return invoker::invoke<int>(0x4c9296cbcd1b971e);
		}
		static bool is_bit_set(int* address, int offset) {
			return invoker::invoke<bool>(0xa921aa820c25702f, address, offset);
		}
		static void using_mission_creator(bool toggle) {
			invoker::invoke<void>(0xf14878fc50bec6ee, toggle);
		}
		static void _0xdea36202fc3382df(bool p0) {
			invoker::invoke<void>(0xdea36202fc3382df, p0);
		}
		static void set_minigame_in_progress(bool toggle) {
			invoker::invoke<void>(0x19e00d7322c6f85b, toggle);
		}
		static bool is_minigame_in_progress() {
			return invoker::invoke<bool>(0x2b4a15e44de0f478);
		}
		static bool is_this_a_minigame_script() {
			return invoker::invoke<bool>(0x7b30f65d7b710098);
		}
		static bool is_sniper_inverted() {
			return invoker::invoke<bool>(0x61a23b7eda9bda24);
		}
		static bool _0xd3d15555431ab793() {
			return invoker::invoke<bool>(0xd3d15555431ab793);
		}
		static int get_profile_setting(int profilesetting) {
			return invoker::invoke<int>(0xc488ff2356ea7791, profilesetting);
		}
		static bool are_strings_equal(const char* string1, const char* string2) {
			return invoker::invoke<bool>(0x0c515fab3ff9ea92, string1, string2);
		}
		static int compare_strings(const char* str1, const char* str2, bool matchcase, int maxlength) {
			return invoker::invoke<int>(0x1e34710ecd4ab0eb, str1, str2, matchcase, maxlength);
		}
		static int absi(int value) {
			return invoker::invoke<int>(0xf0d31ad191a74f87, value);
		}
		static float absf(float value) {
			return invoker::invoke<float>(0x73d57cffdd12c355, value);
		}
		static bool is_sniper_bullet_in_area(float x1, float y1, float z1, float x2, float y2, float z2) {
			return invoker::invoke<bool>(0xfefcf11b01287125, x1, y1, z1, x2, y2, z2);
		}
		static bool is_projectile_in_area(float x1, float y1, float z1, float x2, float y2, float z2, bool ownedbyplayer) {
			return invoker::invoke<bool>(0x5270a8fbc098c3f8, x1, y1, z1, x2, y2, z2, ownedbyplayer);
		}
		static bool is_projectile_type_in_area(float x1, float y1, float z1, float x2, float y2, float z2, int type, bool p7) {
			return invoker::invoke<bool>(0x2e0dc353342c4a6d, x1, y1, z1, x2, y2, z2, type, p7);
		}
		static bool is_projectile_type_in_angled_area(float p0, float p1, float p2, float p3, float p4, float p5, float p6, type::any p7, bool p8) {
			return invoker::invoke<bool>(0xf0bc12401061dea0, p0, p1, p2, p3, p4, p5, p6, p7, p8);
		}
		static bool is_projectile_type_within_distance(float x, float y, float z, type::hash projhash, float radius, bool ownedbyplayer) {
			return invoker::invoke<bool>(0x34318593248c8fb2, x, y, z, projhash, radius, ownedbyplayer);
		}
		static bool _get_is_projectile_type_in_area(float x1, float y1, float z1, float x2, float y2, float z2, type::hash projhash, PVector3* projpos, bool ownedbyplayer) {
			return invoker::invoke<bool>(0x8d7a43ec6a5fea45, x1, y1, z1, x2, y2, z2, projhash, projpos, ownedbyplayer);
		}
		static bool _get_projectile_near_ped_coords(type::ped ped, type::hash projhash, float radius, PVector3* projpos, bool ownedbyplayer) {
			return invoker::invoke<bool>(0xdfb4138eefed7b81, ped, projhash, radius, projpos, ownedbyplayer);
		}
		static bool _get_projectile_near_ped(type::ped ped, type::hash projhash, float radius, PVector3* projpos, type::entity* projent, bool ownedbyplayer) {
			return invoker::invoke<bool>(0x82fde6a57ee4ee44, ped, projhash, radius, projpos, projent, ownedbyplayer);
		}
		static bool is_bullet_in_angled_area(float p0, float p1, float p2, float p3, float p4, float p5, float p6, bool p7) {
			return invoker::invoke<bool>(0x1a8b5f3c01e2b477, p0, p1, p2, p3, p4, p5, p6, p7);
		}
		static bool is_bullet_in_area(float p0, float p1, float p2, float p3, bool p4) {
			return invoker::invoke<bool>(0x3f2023999ad51c1f, p0, p1, p2, p3, p4);
		}
		static bool is_bullet_in_box(float p0, float p1, float p2, float p3, float p4, float p5, bool p6) {
			return invoker::invoke<bool>(0xde0f6d7450d37351, p0, p1, p2, p3, p4, p5, p6);
		}
		static bool has_bullet_impacted_in_area(float x, float y, float z, float p3, bool p4, bool p5) {
			return invoker::invoke<bool>(0x9870acfb89a90995, x, y, z, p3, p4, p5);
		}
		static bool has_bullet_impacted_in_box(float p0, float p1, float p2, float p3, float p4, float p5, bool p6, bool p7) {
			return invoker::invoke<bool>(0xdc8c5d7cfeab8394, p0, p1, p2, p3, p4, p5, p6, p7);
		}
		static bool is_orbis_version() {
			return invoker::invoke<bool>(0xa72bc0b675b1519e);
		}
		static bool is_durango_version() {
			return invoker::invoke<bool>(0x4d982adb1978442d);
		}
		static bool is_xbox360_version() {
			return invoker::invoke<bool>(0xf6201b4daf662a9d);
		}
		static bool is_ps3_version() {
			return invoker::invoke<bool>(0xcca1072c29d096c2);
		}
		static bool is_pc_version() {
			return invoker::invoke<bool>(0x48af36444b965238);
		}
		static bool is_aussie_version() {
			return invoker::invoke<bool>(0x9f1935ca1f724008);
		}
		static bool is_string_null(const char* string) {
			return invoker::invoke<bool>(0xf22b6c47c6eab066, string);
		}
		static bool is_string_null_or_empty(const char* string) {
			return invoker::invoke<bool>(0xca042b6957743895, string);
		}
		static bool string_to_int(const char* string, int* outinteger) {
			return invoker::invoke<bool>(0x5a5f40fe637eb584, string, outinteger);
		}
		static void set_bits_in_range(int* var, int rangestart, int rangeend, int p3) {
			invoker::invoke<void>(0x8ef07e15701d61ed, var, rangestart, rangeend, p3);
		}
		static int get_bits_in_range(int var, int rangestart, int rangeend) {
			return invoker::invoke<int>(0x53158863fcc0893a, var, rangestart, rangeend);
		}
		static int add_stunt_jump(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11, float p12, float p13, float p14, type::any p15, type::any p16) {
			return invoker::invoke<int>(0x1a992da297a4630c, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16);
		}
		static int add_stunt_jump_angled(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11, float p12, float p13, float p14, float p15, float p16, type::any p17, type::any p18) {
			return invoker::invoke<int>(0xbbe5d803a5360cbf, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18);
		}
		static void delete_stunt_jump(int player) {
			invoker::invoke<void>(0xdc518000e39dae1f, player);
		}
		static void enable_stunt_jump_set(int p0) {
			invoker::invoke<void>(0xe369a5783b866016, p0);
		}
		static void disable_stunt_jump_set(int p0) {
			invoker::invoke<void>(0xa5272ebedd4747f6, p0);
		}
		static void _0xd79185689f8fd5df(bool p0) {
			invoker::invoke<void>(0xd79185689f8fd5df, p0);
		}
		static bool is_stunt_jump_in_progress() {
			return invoker::invoke<bool>(0x7a3f19700a4d0525);
		}
		static bool is_stunt_jump_message_showing() {
			return invoker::invoke<bool>(0x2272b0a1343129f4);
		}
		static int _0x996dd1e1e02f1008() {
			return invoker::invoke<int>(0x996dd1e1e02f1008);
		}
		static int _0x6856ec3d35c81ea4() {
			return invoker::invoke<int>(0x6856ec3d35c81ea4);
		}
		static void cancel_stunt_jump() {
			invoker::invoke<void>(0xe6b7b0acd4e4b75e);
		}
		static void set_game_paused(bool toggle) {
			invoker::invoke<void>(0x577d1284d6873711, toggle);
		}
		static void set_this_script_can_be_paused(bool toggle) {
			invoker::invoke<void>(0xaa391c728106f7af, toggle);
		}
		static void set_this_script_can_remove_blips_created_by_any_script(bool toggle) {
			invoker::invoke<void>(0xb98236caaecef897, toggle);
		}
		static bool _has_button_combination_just_been_entered(type::hash hash, int amount) {
			return invoker::invoke<bool>(0x071e2a839de82d90, hash, amount);
		}
		static bool _has_cheat_string_just_been_entered(type::hash hash) {
			return invoker::invoke<bool>(0x557e43c447e700a8, hash);
		}
		static void _use_freemode_map_behavior(bool toggle) {
			invoker::invoke<void>(0x9bae5ad2508df078, toggle);
		}
		static void _set_unk_map_flag(int flag) {
			invoker::invoke<void>(0xc5f0a8ebd3f361ce, flag);
		}
		static bool is_frontend_fading() {
			return invoker::invoke<bool>(0x7ea2b6af97eca6ed);
		}
		static void populate_now() {
			invoker::invoke<void>(0x7472bb270d7b4f3e);
		}
		static int get_index_of_current_level() {
			return invoker::invoke<int>(0xcbad6729f7b1f4fc);
		}
		static void set_gravity_level(int level) {
			invoker::invoke<void>(0x740e14fad5842351, level);
		}
		static void start_save_data(type::any* p0, type::any p1, bool p2) {
			invoker::invoke<void>(0xa9575f812c6a7997, p0, p1, p2);
		}
		static void stop_save_data() {
			invoker::invoke<void>(0x74e20c9145fb66fd);
		}
		static int _0xa09f896ce912481f(bool p0) {
			return invoker::invoke<int>(0xa09f896ce912481f, p0);
		}
		static void register_int_to_save(type::any* p0, const char* name) {
			invoker::invoke<void>(0x34c9ee5986258415, p0, name);
		}
		static void _0xa735353c77334ea0(type::any* p0, type::any* p1) {
			invoker::invoke<void>(0xa735353c77334ea0, p0, p1);
		}
		static void register_enum_to_save(type::any* p0, const char* name) {
			invoker::invoke<void>(0x10c2fa78d0e128a1, p0, name);
		}
		static void register_float_to_save(type::any* p0, const char* name) {
			invoker::invoke<void>(0x7caec29ecb5dfebb, p0, name);
		}
		static void register_bool_to_save(type::any* p0, const char* name) {
			invoker::invoke<void>(0xc8f4131414c835a1, p0, name);
		}
		static void register_text_label_to_save(type::any* p0, const char* name) {
			invoker::invoke<void>(0xedb1232c5beae62f, p0, name);
		}
		static void _0x6f7794f28c6b2535(type::any* p0, const char* name) {
			invoker::invoke<void>(0x6f7794f28c6b2535, p0, name);
		}
		static void _0x48f069265a0e4bec(type::any* p0, const char* name) {
			invoker::invoke<void>(0x48f069265a0e4bec, p0, name);
		}
		static void _0x8269816f6cfd40f8(type::any* p0, const char* name) {
			invoker::invoke<void>(0x8269816f6cfd40f8, p0, name);
		}
		static void _0xfaa457ef263e8763(type::any* p0, const char* name) {
			invoker::invoke<void>(0xfaa457ef263e8763, p0, name);
		}
		static void _start_save_struct(type::any* p0, int p1, const char* structname) {
			invoker::invoke<void>(0xbf737600cddbeadd, p0, p1, structname);
		}
		static void stop_save_struct() {
			invoker::invoke<void>(0xeb1774df12bb9f12);
		}
		static void _start_save_array(type::any* p0, int p1, const char* arrayname) {
			invoker::invoke<void>(0x60fe567df1b1af9d, p0, p1, arrayname);
		}
		static void stop_save_array() {
			invoker::invoke<void>(0x04456f95153c6be4);
		}
		static void enable_dispatch_service(int dispatchtype, bool toggle) {
			invoker::invoke<void>(0xdc0f817884cdd856, dispatchtype, toggle);
		}
		static void block_dispatch_service_resource_creation(int dispatchtype, bool toggle) {
			invoker::invoke<void>(0x9b2bd3773123ea2f, dispatchtype, toggle);
		}
		static int _get_number_of_dispatched_units_for_player(int dispatchservice) {
			return invoker::invoke<int>(0xeb4a0c2d56441717, dispatchservice);
		}
		static bool create_incident(int incidenttype, float x, float y, float z, int p5, float radius, int* outincidentid) {
			return invoker::invoke<bool>(0x3f892caf67444ae7, incidenttype, x, y, z, p5, radius, outincidentid);
		}
		static bool create_incident_with_entity(int incidenttype, type::ped ped, int amountofpeople, float radius, int* outincidentid) {
			return invoker::invoke<bool>(0x05983472f0494e60, incidenttype, ped, amountofpeople, radius, outincidentid);
		}
		static void delete_incident(int incidentid) {
			invoker::invoke<void>(0x556c1aa270d5a207, incidentid);
		}
		static bool is_incident_valid(int incidentid) {
			return invoker::invoke<bool>(0xc8bc6461e629beaa, incidentid);
		}
		static void _0xb08b85d860e7ba3c(type::any p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0xb08b85d860e7ba3c, p0, p1, p2);
		}
		static void _0xd261ba3e7e998072(type::any p0, float p1) {
			invoker::invoke<void>(0xd261ba3e7e998072, p0, p1);
		}
		static bool find_spawn_point_in_direction(float x1, float y1, float z1, float x2, float y2, float z2, float distance, PVector3* spawnpoint) {
			return invoker::invoke<bool>(0x6874e2190b0c1972, x1, y1, z1, x2, y2, z2, distance, spawnpoint);
		}
		static type::any _0x67f6413d3220e18d(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4, type::any p5, type::any p6, type::any p7, type::any p8) {
			return invoker::invoke<type::any>(0x67f6413d3220e18d, p0, p1, p2, p3, p4, p5, p6, p7, p8);
		}
		static bool _0x1327e2fe9746baee(type::any p0) {
			return invoker::invoke<bool>(0x1327e2fe9746baee, p0);
		}
		static void _0xb129e447a2eda4bf(type::any p0, bool p1) {
			invoker::invoke<void>(0xb129e447a2eda4bf, p0, p1);
		}
		static type::any _0x32c7a7e8c43a1f80(float p0, float p1, float p2, float p3, float p4, float p5, bool p6, bool p7) {
			return invoker::invoke<type::any>(0x32c7a7e8c43a1f80, p0, p1, p2, p3, p4, p5, p6, p7);
		}
		static void _0xe6869becdd8f2403(type::any p0, bool p1) {
			invoker::invoke<void>(0xe6869becdd8f2403, p0, p1);
		}
		static void enable_tennis_mode(type::ped ped, bool toggle, bool p2) {
			invoker::invoke<void>(0x28a04b411933f8a6, ped, toggle, p2);
		}
		static bool is_tennis_mode(type::ped ped) {
			return invoker::invoke<bool>(0x5d5479d115290c3f, ped);
		}
		static void _0xe266ed23311f24d4(type::any p0, type::any* p1, type::any* p2, float p3, float p4, bool p5) {
			invoker::invoke<void>(0xe266ed23311f24d4, p0, p1, p2, p3, p4, p5);
		}
		static bool _0x17df68d720aa77f8(type::any p0) {
			return invoker::invoke<bool>(0x17df68d720aa77f8, p0);
		}
		static bool _0x19bfed045c647c49(type::any p0) {
			return invoker::invoke<bool>(0x19bfed045c647c49, p0);
		}
		static bool _0xe95b0c7d5ba3b96b(type::any p0) {
			return invoker::invoke<bool>(0xe95b0c7d5ba3b96b, p0);
		}
		static void _0x8fa9c42fc5d7c64b(type::any p0, type::any p1, float p2, float p3, float p4, bool p5) {
			invoker::invoke<void>(0x8fa9c42fc5d7c64b, p0, p1, p2, p3, p4, p5);
		}
		static void _0x54f157e0336a3822(type::any p0, const char* p1, float p2) {
			invoker::invoke<void>(0x54f157e0336a3822, p0, p1, p2);
		}
		static void _0xd10f442036302d50(type::any p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0xd10f442036302d50, p0, p1, p2);
		}
		static void reset_dispatch_ideal_spawn_distance() {
			invoker::invoke<void>(0x77a84429dd9f0a15);
		}
		static void set_dispatch_ideal_spawn_distance(float p0) {
			invoker::invoke<void>(0x6fe601a64180d423, p0);
		}
		static void set_dispatch_time_between_spawn_attempts(type::any p0, float p1) {
			invoker::invoke<void>(0x44f7cbc1beb3327d, p0, p1);
		}
		static void set_dispatch_time_between_spawn_attempts_multiplier(type::any p0, float p1) {
			invoker::invoke<void>(0x48838ed9937a15d1, p0, p1);
		}
		static type::any _0x918c7b2d2ff3928b(float p0, float p1, float p2, float p3, float p4, float p5, float p6) {
			return invoker::invoke<type::any>(0x918c7b2d2ff3928b, p0, p1, p2, p3, p4, p5, p6);
		}
		static type::any _0x2d4259f1feb81da9(float p0, float p1, float p2, float p3) {
			return invoker::invoke<type::any>(0x2d4259f1feb81da9, p0, p1, p2, p3);
		}
		static void remove_dispatch_spawn_blocking_area(type::any p0) {
			invoker::invoke<void>(0x264ac28b01b353a5, p0);
		}
		static void reset_dispatch_spawn_blocking_areas() {
			invoker::invoke<void>(0xac7bfd5c1d83ea75);
		}
		static void _0xd9f692d349249528() {
			invoker::invoke<void>(0xd9f692d349249528);
		}
		static void _0xe532ec1a63231b4f(type::any p0, type::any p1) {
			invoker::invoke<void>(0xe532ec1a63231b4f, p0, p1);
		}
		static void _0xb8721407ee9c3ff6(type::any p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0xb8721407ee9c3ff6, p0, p1, p2);
		}
		static void _0xb3cd58cca6cda852() {
			invoker::invoke<void>(0xb3cd58cca6cda852);
		}
		static void _0x2587a48bc88dfadf(bool p0) {
			invoker::invoke<void>(0x2587a48bc88dfadf, p0);
		}
		static void _display_onscreen_keyboard_2(int p0, const char* windowtitle, type::any* p2, type::player* defaulttext, const char* defaultconcat1, const char* defaultconcat2, const char* defaultconcat3, const char* defaultconcat4, const char* defaultconcat5, const char* defaultconcat6, const char* defaultconcat7, int maxinputlength) {
			invoker::invoke<void>(0xca78cfa0366592fe, p0, windowtitle, p2, defaulttext, defaultconcat1, defaultconcat2, defaultconcat3, defaultconcat4, defaultconcat5, defaultconcat6, defaultconcat7, maxinputlength);
		}
		static void display_onscreen_keyboard(int p0, const char* windowtitle, const char* p2, const char* defaulttext, const char* defaultconcat1, const char* defaultconcat2, const char* defaultconcat3, int maxinputlength) {
			invoker::invoke<void>(0x00dc833f2568dbf6, p0, windowtitle, p2, defaulttext, defaultconcat1, defaultconcat2, defaultconcat3, maxinputlength);
		}
		static int update_onscreen_keyboard() {
			return invoker::invoke<int>(0x0cf2b696bbf945ae);
		}
		static const char* get_onscreen_keyboard_result() {
			return invoker::invoke<const char*>(0x8362b09b91893647);
		}
		static void _0x3ed1438c1f5c6612(int p0) {
			invoker::invoke<void>(0x3ed1438c1f5c6612, p0);
		}
		static void _remove_stealth_kill(type::hash hash, bool p1) {
			invoker::invoke<void>(0xa6a12939f16d85be, hash, p1);
		}
		static void _0x1eae0a6e978894a2(int p0, bool p1) {
			invoker::invoke<void>(0x1eae0a6e978894a2, p0, p1);
		}
		static void set_explosive_ammo_this_frame(type::player player) {
			invoker::invoke<void>(0xa66c71c98d5f2cfb, player);
		}
		static void set_fire_ammo_this_frame(type::player player) {
			invoker::invoke<void>(0x11879cdd803d30f4, player);
		}
		static void set_explosive_melee_this_frame(type::player player) {
			invoker::invoke<void>(0xff1bed81bfdc0fe0, player);
		}
		static void set_super_jump_this_frame(type::player player) {
			invoker::invoke<void>(0x57fff03e423a4c0b, player);
		}
		static bool _0x6fddf453c0c756ec() {
			return invoker::invoke<bool>(0x6fddf453c0c756ec);
		}
		static void _0xfb00ca71da386228() {
			invoker::invoke<void>(0xfb00ca71da386228);
		}
		static type::any _0x5aa3befa29f03ad4() {
			return invoker::invoke<type::any>(0x5aa3befa29f03ad4);
		}
		static void _0xe3d969d2785ffb5e() {
			invoker::invoke<void>(0xe3d969d2785ffb5e);
		}
		static void _reset_localplayer_state() {
			invoker::invoke<void>(0xc0aa53f866b3134d);
		}
		static void _0x0a60017f841a54f2(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x0a60017f841a54f2, p0, p1, p2, p3);
		}
		static void _0x1ff6bf9a63e5757f() {
			invoker::invoke<void>(0x1ff6bf9a63e5757f);
		}
		static void _0x1bb299305c3e8c13(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x1bb299305c3e8c13, p0, p1, p2, p3);
		}
		static bool _0x8ef5573a1f801a5c(type::any p0, type::any* p1, type::any* p2) {
			return invoker::invoke<bool>(0x8ef5573a1f801a5c, p0, p1, p2);
		}
		static void _0x92790862e36c2ada() {
			invoker::invoke<void>(0x92790862e36c2ada);
		}
		static void _0xc7db36c24634f52b() {
			invoker::invoke<void>(0xc7db36c24634f52b);
		}
		static void _0x437138b6a830166a() {
			invoker::invoke<void>(0x437138b6a830166a);
		}
		static void _0x37deb0aa183fb6d8() {
			invoker::invoke<void>(0x37deb0aa183fb6d8);
		}
		static type::any _0xea2f2061875eed90() {
			return invoker::invoke<type::any>(0xea2f2061875eed90);
		}
		static type::any _0x3bbbd13e5041a79e() {
			return invoker::invoke<type::any>(0x3bbbd13e5041a79e);
		}
		static bool _0xa049a5be0f04f2f8() {
			return invoker::invoke<bool>(0xa049a5be0f04f2f8);
		}
		static type::any _0x4750fc27570311ec() {
			return invoker::invoke<type::any>(0x4750fc27570311ec);
		}
		static type::any _0x1b2366c3f2a5c8df() {
			return invoker::invoke<type::any>(0x1b2366c3f2a5c8df);
		}
		static void _force_social_club_update() {
			invoker::invoke<void>(0xeb6891f03362fb12);
		}
		static bool _has_all_chunks_on_hdd() {
			return invoker::invoke<bool>(0x14832bf2aba53fc5);
		}
		static void _cleanup_async_install() {
			invoker::invoke<void>(0xc79ae21974b01fb2);
		}
		static bool _0x684a41975f077262() {
			return invoker::invoke<bool>(0x684a41975f077262);
		}
		static type::any _0xabb2fa71c83a1b72() {
			return invoker::invoke<type::any>(0xabb2fa71c83a1b72);
		}
		static void _set_show_ped_in_pause_menu(bool toggle) {
			invoker::invoke<void>(0x4ebb7e87aa0dbed4, toggle);
		}
		static bool _get_show_ped_in_pause_menu() {
			return invoker::invoke<bool>(0x9689123e3f213aa5);
		}
		static void _0x9d8d44adbba61ef2(bool p0) {
			invoker::invoke<void>(0x9d8d44adbba61ef2, p0);
		}
		static void _0x23227df0b2115469() {
			invoker::invoke<void>(0x23227df0b2115469);
		}
		static type::any _0xd10282b6e3751ba0() {
			return invoker::invoke<type::any>(0xd10282b6e3751ba0);
		}
	}

	namespace audio {
		static void play_ped_ringtone(const char* ringtonename, type::ped ped, bool p2) {
			invoker::invoke<void>(0xf9e56683ca8e11a5, ringtonename, ped, p2);
		}
		static bool is_ped_ringtone_playing(type::ped ped) {
			return invoker::invoke<bool>(0x1e8e5e20937e3137, ped);
		}
		static void stop_ped_ringtone(type::ped ped) {
			invoker::invoke<void>(0x6c5ae23efa885092, ped);
		}
		static bool is_mobile_phone_call_ongoing() {
			return invoker::invoke<bool>(0x7497d2ce2c30d24c);
		}
		static bool _0xc8b1b2425604cdd0() {
			return invoker::invoke<bool>(0xc8b1b2425604cdd0);
		}
		static void create_new_scripted_conversation() {
			invoker::invoke<void>(0xd2c91a0b572aae56);
		}
		static void add_line_to_conversation(int p0, const char* p1, const char* p2, int p3, int p4, bool p5, bool p6, bool p7, bool p8, int p9, bool p10, bool p11, bool p12) {
			invoker::invoke<void>(0xc5ef963405593646, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
		}
		static void add_ped_to_conversation(int pedindex, type::ped ped, const char* name) {
			invoker::invoke<void>(0x95d9f4bc443956e7, pedindex, ped, name);
		}
		static void _0x33e3c6c6f2f0b506(type::any p0, float p1, float p2, float p3) {
			invoker::invoke<void>(0x33e3c6c6f2f0b506, p0, p1, p2, p3);
		}
		static void _0x892b6ab8f33606f5(type::any p0, type::any p1) {
			invoker::invoke<void>(0x892b6ab8f33606f5, p0, p1);
		}
		static void set_microphone_position(bool p0, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3) {
			invoker::invoke<void>(0xb6ae90edde95c762, p0, x1, y1, z1, x2, y2, z2, x3, y3, z3);
		}
		static void _0x0b568201dd99f0eb(bool p0) {
			invoker::invoke<void>(0x0b568201dd99f0eb, p0);
		}
		static void _0x61631f5df50d1c34(bool p0) {
			invoker::invoke<void>(0x61631f5df50d1c34, p0);
		}
		static void start_script_phone_conversation(bool p0, bool p1) {
			invoker::invoke<void>(0x252e5f915eabb675, p0, p1);
		}
		static void preload_script_phone_conversation(bool p0, bool p1) {
			invoker::invoke<void>(0x6004bcb0e226aaea, p0, p1);
		}
		static void start_script_conversation(bool p0, bool p1, bool p2, bool p3) {
			invoker::invoke<void>(0x6b17c62c9635d2dc, p0, p1, p2, p3);
		}
		static void preload_script_conversation(bool p0, bool p1, bool p2, bool p3) {
			invoker::invoke<void>(0x3b3cad6166916d87, p0, p1, p2, p3);
		}
		static void start_preloaded_conversation() {
			invoker::invoke<void>(0x23641afe870af385);
		}
		static bool _0xe73364db90778ffa() {
			return invoker::invoke<bool>(0xe73364db90778ffa);
		}
		static bool is_scripted_conversation_ongoing() {
			return invoker::invoke<bool>(0x16754c556d2ede3d);
		}
		static bool is_scripted_conversation_loaded() {
			return invoker::invoke<bool>(0xdf0d54be7a776737);
		}
		static type::any get_current_scripted_conversation_line() {
			return invoker::invoke<type::any>(0x480357ee890c295a);
		}
		static void pause_scripted_conversation(bool finishcurline) {
			invoker::invoke<void>(0x8530ad776cd72b12, finishcurline);
		}
		static void restart_scripted_conversation() {
			invoker::invoke<void>(0x9aeb285d1818c9ac);
		}
		static type::any stop_scripted_conversation(bool p0) {
			return invoker::invoke<type::any>(0xd79deefb53455eba, p0);
		}
		static void skip_to_next_scripted_conversation_line() {
			invoker::invoke<void>(0x9663fe6b7a61eb00);
		}
		static void interrupt_conversation(type::ped targ, const char* gxtline, const char* charname) {
			invoker::invoke<void>(0xa018a12e5c5c2fa6, targ, gxtline, charname);
		}
		static void _0x8a694d7a68f8dc38(type::ped p0, const char* p1, const char* p2) {
			invoker::invoke<void>(0x8a694d7a68f8dc38, p0, p1, p2);
		}
		static type::any _0xaa19f5572c38b564(type::any* p0) {
			return invoker::invoke<type::any>(0xaa19f5572c38b564, p0);
		}
		static void _0xb542de8c3d1cb210(bool p0) {
			invoker::invoke<void>(0xb542de8c3d1cb210, p0);
		}
		static void register_script_with_audio(int p0) {
			invoker::invoke<void>(0xc6ed9d5092438d91, p0);
		}
		static void unregister_script_with_audio() {
			invoker::invoke<void>(0xa8638be228d4751a);
		}
		static bool request_mission_audio_bank(const char* p0, bool p1) {
			return invoker::invoke<bool>(0x7345bdd95e62e0f2, p0, p1);
		}
		static bool request_ambient_audio_bank(const char* p0, bool p1) {
			return invoker::invoke<bool>(0xfe02ffbed8ca9d99, p0, p1);
		}
		static bool request_script_audio_bank(const char* p0, bool p1) {
			return invoker::invoke<bool>(0x2f844a8b08d76685, p0, p1);
		}
		static type::any hint_ambient_audio_bank(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x8f8c0e370ae62f5c, p0, p1);
		}
		static type::any hint_script_audio_bank(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0xfb380a29641ec31a, p0, p1);
		}
		static void release_mission_audio_bank() {
			invoker::invoke<void>(0x0ec92a1bf0857187);
		}
		static void release_ambient_audio_bank() {
			invoker::invoke<void>(0x65475a218ffaa93d);
		}
		static void release_named_script_audio_bank(const char* audiobank) {
			invoker::invoke<void>(0x77ed170667f50170, audiobank);
		}
		static void release_script_audio_bank() {
			invoker::invoke<void>(0x7a2d8ad0a9eb9c3f);
		}
		static void _0x19af7ed9b9d23058() {
			invoker::invoke<void>(0x19af7ed9b9d23058);
		}
		static void _0x9ac92eed5e4793ab() {
			invoker::invoke<void>(0x9ac92eed5e4793ab);
		}
		static int get_sound_id() {
			return invoker::invoke<int>(0x430386fe9bf80b45);
		}
		static void release_sound_id(int soundid) {
			invoker::invoke<void>(0x353fc880830b88fa, soundid);
		}
		static void play_sound(type::player soundid, const char* audioname, const char* audioref, bool p3, type::any p4, bool p5) {
			invoker::invoke<void>(0x7ff4944cc209192d, soundid, audioname, audioref, p3, p4, p5);
		}
		static void play_sound_frontend(int soundid, const char* audioname, const char* audioref, bool p3) {
			invoker::invoke<void>(0x67c540aa08e4a6f5, soundid, audioname, audioref, p3);
		}
		static void _0xcada5a0d0702381e(const char* p0, const char* soundset) {
			invoker::invoke<void>(0xcada5a0d0702381e, p0, soundset);
		}
		static void play_sound_from_entity(int soundid, const char* audioname, type::entity entity, const char* audioref, bool p4, type::any p5) {
			invoker::invoke<void>(0xe65f427eb70ab1ed, soundid, audioname, entity, audioref, p4, p5);
		}
		static void play_sound_from_coord(int soundid, const char* audioname, float x, float y, float z, const char* audioref, bool p6, int range, bool p8) {
			invoker::invoke<void>(0x8d8686b622b88120, soundid, audioname, x, y, z, audioref, p6, range, p8);
		}
		static void stop_sound(int soundid) {
			invoker::invoke<void>(0xa3b0c41ba5cc0bb5, soundid);
		}
		static int get_network_id_from_sound_id(int soundid) {
			return invoker::invoke<int>(0x2de3f0a134ffbc0d, soundid);
		}
		static int get_sound_id_from_network_id(int netid) {
			return invoker::invoke<int>(0x75262fd12d0a1c84, netid);
		}
		static void set_variable_on_sound(int soundid, const char* variablename, float value) {
			invoker::invoke<void>(0xad6b3148a78ae9b6, soundid, variablename, value);
		}
		static void set_variable_on_stream(const char* p0, float p1) {
			invoker::invoke<void>(0x2f9d3834aeb9ef79, p0, p1);
		}
		static void override_underwater_stream(type::any* p0, bool p1) {
			invoker::invoke<void>(0xf2a9cdabcea04bd6, p0, p1);
		}
		static void _0x733adf241531e5c2(const char* name, float p1) {
			invoker::invoke<void>(0x733adf241531e5c2, name, p1);
		}
		static bool has_sound_finished(int soundid) {
			return invoker::invoke<bool>(0xfcbdce714a7c88e5, soundid);
		}
		static void _play_ambient_speech1(type::ped ped, const char* speechname, const char* speechparam) {
			invoker::invoke<void>(0x8e04fedd28d42462, ped, speechname, speechparam);
		}
		static void _play_ambient_speech2(type::ped ped, const char* speechname, const char* speechparam) {
			invoker::invoke<void>(0xc6941b4a3a8fbbb9, ped, speechname, speechparam);
		}
		static void _play_ambient_speech_with_voice(type::ped p0, const char* speechname, const char* voicename, const char* speechparam, bool p4) {
			invoker::invoke<void>(0x3523634255fc3318, p0, speechname, voicename, speechparam, p4);
		}
		static void _play_ambient_speech_at_coords(const char* speechname, const char* voicename, float x, float y, float z, const char* speechparam) {
			invoker::invoke<void>(0xed640017ed337e45, speechname, voicename, x, y, z, speechparam);
		}
		static void override_trevor_rage(const char* p0) {
			invoker::invoke<void>(0x13ad665062541a7e, p0);
		}
		static void reset_trevor_rage() {
			invoker::invoke<void>(0xe78503b10c4314e0);
		}
		static void set_player_angry(type::ped playerped, bool value) {
			invoker::invoke<void>(0xea241bb04110f091, playerped, value);
		}
		static void play_pain(type::ped ped, int painid, float p1) {
			invoker::invoke<void>(0xbc9ae166038a5cec, ped, painid, p1);
		}
		static void _0xd01005d2ba2eb778(const char* p0) {
			invoker::invoke<void>(0xd01005d2ba2eb778, p0);
		}
		static void _0xddc635d5b3262c56(const char* p0) {
			invoker::invoke<void>(0xddc635d5b3262c56, p0);
		}
		static void set_ambient_voice_name(type::ped ped, const char* name) {
			invoker::invoke<void>(0x6c8065a3b780185b, ped, name);
		}
		static void _reset_ambient_voice(type::ped ped) {
			invoker::invoke<void>(0x40cf0d12d142a9e8, ped);
		}
		static void _0x7cdc8c3b89f661b3(type::ped playerped, type::hash p1) {
			invoker::invoke<void>(0x7cdc8c3b89f661b3, playerped, p1);
		}
		static void _0xa5342d390cda41d6(type::any p0, bool p1) {
			invoker::invoke<void>(0xa5342d390cda41d6, p0, p1);
		}
		static void _set_ped_mute(type::ped ped) {
			invoker::invoke<void>(0x7a73d05a607734c7, ped);
		}
		static void stop_current_playing_ambient_speech(type::ped ped) {
			invoker::invoke<void>(0xb8bec0ca6f0edb0f, ped);
		}
		static bool is_ambient_speech_playing(type::ped p0) {
			return invoker::invoke<bool>(0x9072c8b49907bfad, p0);
		}
		static bool is_scripted_speech_playing(type::any p0) {
			return invoker::invoke<bool>(0xcc9aa18dcc7084f4, p0);
		}
		static bool is_any_speech_playing(type::ped ped) {
			return invoker::invoke<bool>(0x729072355fa39ec9, ped);
		}
		static bool _can_ped_speak(type::ped ped, const char* speechname, bool unk) {
			return invoker::invoke<bool>(0x49b99bf3fda89a7a, ped, speechname, unk);
		}
		static bool is_ped_in_current_conversation(type::ped ped) {
			return invoker::invoke<bool>(0x049e937f18f4020c, ped);
		}
		static void set_ped_is_drunk(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x95d2d383d5396b8a, ped, toggle);
		}
		static void _0xee066c7006c49c0a(type::entity entity, int unk, const char* speech) {
			invoker::invoke<void>(0xee066c7006c49c0a, entity, unk, speech);
		}
		static bool _0xc265df9fb44a9fbd(type::any p0) {
			return invoker::invoke<bool>(0xc265df9fb44a9fbd, p0);
		}
		static void set_animal_mood(type::ped animal, int mood) {
			invoker::invoke<void>(0xcc97b29285b1dc3b, animal, mood);
		}
		static bool is_mobile_phone_radio_active() {
			return invoker::invoke<bool>(0xb35ce999e8ef317e);
		}
		static void set_mobile_phone_radio_state(bool state) {
			invoker::invoke<void>(0xbf286c554784f3df, state);
		}
		static int get_player_radio_station_index() {
			return invoker::invoke<int>(0xe8af77c4c06adc93);
		}
		static const char* get_player_radio_station_name() {
			return invoker::invoke<const char*>(0xf6d733c32076ad03);
		}
		static const char* get_radio_station_name(int radiostation) {
			return invoker::invoke<const char*>(0xb28eca15046ca8b9, radiostation);
		}
		static type::any get_player_radio_station_genre() {
			return invoker::invoke<type::any>(0xa571991a7fe6cceb);
		}
		static bool is_radio_retuning() {
			return invoker::invoke<bool>(0xa151a7394a214e65);
		}
		static bool _0x0626a247d2405330() {
			return invoker::invoke<bool>(0x0626a247d2405330);
		}
		static void _0xff266d1d0eb1195d() {
			invoker::invoke<void>(0xff266d1d0eb1195d);
		}
		static void _0xdd6bcf9e94425df9() {
			invoker::invoke<void>(0xdd6bcf9e94425df9);
		}
		static void set_radio_to_station_name(const char* stationname) {
			invoker::invoke<void>(0xc69eda28699d5107, stationname);
		}
		static void set_veh_radio_station(type::vehicle vehicle, const char* radiostation) {
			invoker::invoke<void>(0x1b9c0099cb942ac6, vehicle, radiostation);
		}
		static void _set_vehicle_as_ambient_emmitter(type::vehicle vehicle) {
			invoker::invoke<void>(0xc1805d05e6d4fe10, vehicle);
		}
		static void set_emitter_radio_station(const char* emittername, const char* radiostation) {
			invoker::invoke<void>(0xacf57305b12af907, emittername, radiostation);
		}
		static void set_static_emitter_enabled(const char* emittername, bool toggle) {
			invoker::invoke<void>(0x399d2d3b33f1b8eb, emittername, toggle);
		}
		static void set_radio_to_station_index(int radiostation) {
			invoker::invoke<void>(0xa619b168b8a8570f, radiostation);
		}
		static void set_frontend_radio_active(bool active) {
			invoker::invoke<void>(0xf7f26c6e9cc9ebb8, active);
		}
		static void unlock_mission_news_story(int newsstory) {
			invoker::invoke<void>(0xb165ab7c248b2dc1, newsstory);
		}
		static int get_number_of_passenger_voice_variations(type::any p0) {
			return invoker::invoke<int>(0x66e49bf55b4b1874, p0);
		}
		static int get_audible_music_track_text_id() {
			return invoker::invoke<int>(0x50b196fc9ed6545b);
		}
		static void play_end_credits_music(bool play) {
			invoker::invoke<void>(0xcd536c4d33dcc900, play);
		}
		static void skip_radio_forward() {
			invoker::invoke<void>(0x6ddbbdd98e2e9c25);
		}
		static void freeze_radio_station(const char* radiostation) {
			invoker::invoke<void>(0x344f393b027e38c3, radiostation);
		}
		static void unfreeze_radio_station(const char* radiostation) {
			invoker::invoke<void>(0xfc00454cf60b91dd, radiostation);
		}
		static void set_radio_auto_unfreeze(bool toggle) {
			invoker::invoke<void>(0xc1aa9f53ce982990, toggle);
		}
		static void set_initial_player_station(const char* radiostation) {
			invoker::invoke<void>(0x88795f13facda88d, radiostation);
		}
		static void set_user_radio_control_enabled(bool toggle) {
			invoker::invoke<void>(0x19f21e63ae6eae4e, toggle);
		}
		static void set_radio_track(const char* radiostation, const char* radiotrack) {
			invoker::invoke<void>(0xb39786f201fee30b, radiostation, radiotrack);
		}
		static void set_vehicle_radio_loud(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xbb6f1caec68b0bce, vehicle, toggle);
		}
		static bool _is_vehicle_radio_loud(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x032a116663a4d5ac, vehicle);
		}
		static void set_mobile_radio_enabled_during_gameplay(bool toggle) {
			invoker::invoke<void>(0x1098355a16064bb3, toggle);
		}
		static bool _0x109697e2ffbac8a1() {
			return invoker::invoke<bool>(0x109697e2ffbac8a1);
		}
		static bool _is_player_vehicle_radio_enabled() {
			return invoker::invoke<bool>(0x5f43d83fd6738741);
		}
		static void set_vehicle_radio_enabled(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x3b988190c0aa6c0b, vehicle, toggle);
		}
		static void _0x4e404a9361f75bb2(const char* radiostation, const char* p1, bool p2) {
			invoker::invoke<void>(0x4e404a9361f75bb2, radiostation, p1, p2);
		}
		static void _0x1654f24a88a8e3fe(const char* radiostation) {
			invoker::invoke<void>(0x1654f24a88a8e3fe, radiostation);
		}
		static int _max_radio_station_index() {
			return invoker::invoke<int>(0xf1620ecb50e01de7);
		}
		static int find_radio_station_index(int station) {
			return invoker::invoke<int>(0x8d67489793ff428b, station);
		}
		static void _0x774bd811f656a122(const char* radiostation, bool p1) {
			invoker::invoke<void>(0x774bd811f656a122, radiostation, p1);
		}
		static void _0x2c96cdb04fca358e(float p0) {
			invoker::invoke<void>(0x2c96cdb04fca358e, p0);
		}
		static void _0x031acb6aba18c729(const char* radiostation, const char* p1) {
			invoker::invoke<void>(0x031acb6aba18c729, radiostation, p1);
		}
		static void _0xf3365489e0dd50f9(type::any p0, bool p1) {
			invoker::invoke<void>(0xf3365489e0dd50f9, p0, p1);
		}
		static void set_ambient_zone_state(type::any* p0, bool p1, bool p2) {
			invoker::invoke<void>(0xbda07e5950085e46, p0, p1, p2);
		}
		static void clear_ambient_zone_state(const char* zonename, bool p1) {
			invoker::invoke<void>(0x218dd44aaac964ff, zonename, p1);
		}
		static void set_ambient_zone_list_state(const char* p0, bool p1, bool p2) {
			invoker::invoke<void>(0x9748fa4de50cce3e, p0, p1, p2);
		}
		static void clear_ambient_zone_list_state(type::any* p0, bool p1) {
			invoker::invoke<void>(0x120c48c614909fa4, p0, p1);
		}
		static void set_ambient_zone_state_persistent(const char* ambientzone, bool p1, bool p2) {
			invoker::invoke<void>(0x1d6650420cec9d3b, ambientzone, p1, p2);
		}
		static void set_ambient_zone_list_state_persistent(const char* ambientzone, bool p1, bool p2) {
			invoker::invoke<void>(0xf3638dae8c4045e1, ambientzone, p1, p2);
		}
		static bool is_ambient_zone_enabled(const char* ambientzone) {
			return invoker::invoke<bool>(0x01e2817a479a7f9b, ambientzone);
		}
		static void set_cutscene_audio_override(const char* p0) {
			invoker::invoke<void>(0x3b4bf5f0859204d9, p0);
		}
		static void get_player_headset_sound_alternate(const char* p0, float p1) {
			invoker::invoke<void>(0xbcc29f935ed07688, p0, p1);
		}
		static type::any play_police_report(const char* name, float p1) {
			return invoker::invoke<type::any>(0xdfebd56d9bd1eb16, name, p1);
		}
		static void _disable_police_reports() {
			invoker::invoke<void>(0xb4f90faf7670b16f);
		}
		static void blip_siren(type::vehicle vehicle) {
			invoker::invoke<void>(0x1b9025bda76822b6, vehicle);
		}
		static void override_veh_horn(type::vehicle vehicle, bool mute, int p2) {
			invoker::invoke<void>(0x3cdc1e622cce0356, vehicle, mute, p2);
		}
		static bool is_horn_active(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x9d6bfc12b05c6121, vehicle);
		}
		static void set_aggressive_horns(bool toggle) {
			invoker::invoke<void>(0x395bf71085d1b1d9, toggle);
		}
		static void _0x02e93c796abd3a97(bool p0) {
			invoker::invoke<void>(0x02e93c796abd3a97, p0);
		}
		static void _0x58bb377bec7cd5f4(bool p0, bool p1) {
			invoker::invoke<void>(0x58bb377bec7cd5f4, p0, p1);
		}
		static bool is_stream_playing() {
			return invoker::invoke<bool>(0xd11fa52eb849d978);
		}
		static int get_stream_play_time() {
			return invoker::invoke<int>(0x4e72bbdbca58a3db);
		}
		static bool load_stream(const char* streamname, const char* soundset) {
			return invoker::invoke<bool>(0x1f1f957154ec51df, streamname, soundset);
		}
		static bool load_stream_with_start_offset(const char* streamname, int startoffset, const char* soundset) {
			return invoker::invoke<bool>(0x59c16b79f53b3712, streamname, startoffset, soundset);
		}
		static void play_stream_from_ped(type::ped ped) {
			invoker::invoke<void>(0x89049dd63c08b5d1, ped);
		}
		static void play_stream_from_vehicle(type::vehicle vehicle) {
			invoker::invoke<void>(0xb70374a758007dfa, vehicle);
		}
		static void play_stream_from_object(type::object object) {
			invoker::invoke<void>(0xebaa9b64d76356fd, object);
		}
		static void play_stream_frontend() {
			invoker::invoke<void>(0x58fce43488f9f5f4);
		}
		static void special_frontend_equal(float x, float y, float z) {
			invoker::invoke<void>(0x21442f412e8de56b, x, y, z);
		}
		static void stop_stream() {
			invoker::invoke<void>(0xa4718a1419d18151);
		}
		static void stop_ped_speaking(type::ped ped, bool speaking) {
			invoker::invoke<void>(0x9d64d7405520e3d3, ped, speaking);
		}
		static void disable_ped_pain_audio(type::ped ped, bool toggle) {
			invoker::invoke<void>(0xa9a41c1e940fb0e8, ped, toggle);
		}
		static bool is_ambient_speech_disabled(type::ped ped) {
			return invoker::invoke<bool>(0x932c2d096a2c3fff, ped);
		}
		static void set_siren_with_no_driver(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x1fef0683b96ebcf2, vehicle, toggle);
		}
		static void _sound_vehicle_horn_this_frame(type::vehicle vehicle) {
			invoker::invoke<void>(0x9c11908013ea4715, vehicle);
		}
		static void set_horn_enabled(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x76d683c108594d0e, vehicle, toggle);
		}
		static void set_audio_vehicle_priority(type::vehicle vehicle, type::any p1) {
			invoker::invoke<void>(0xe5564483e407f914, vehicle, p1);
		}
		static void _0x9d3af56e94c9ae98(type::any p0, float p1) {
			invoker::invoke<void>(0x9d3af56e94c9ae98, p0, p1);
		}
		static void use_siren_as_horn(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xfa932de350266ef8, vehicle, toggle);
		}
		static void _force_vehicle_engine_audio(type::vehicle vehicle, const char* audioname) {
			invoker::invoke<void>(0x4f0c413926060b38, vehicle, audioname);
		}
		static void _0xf1f8157b8c3f171c(type::any p0, const char* p1, const char* p2) {
			invoker::invoke<void>(0xf1f8157b8c3f171c, p0, p1, p2);
		}
		static void _0xd2dccd8e16e20997(type::any p0) {
			invoker::invoke<void>(0xd2dccd8e16e20997, p0);
		}
		static bool _0x5db8010ee71fdef2(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x5db8010ee71fdef2, vehicle);
		}
		static void _0x59e7b488451f4d3a(type::any p0, float p1) {
			invoker::invoke<void>(0x59e7b488451f4d3a, p0, p1);
		}
		static void _0x01bb4d577d38bd9e(type::any p0, float p1) {
			invoker::invoke<void>(0x01bb4d577d38bd9e, p0, p1);
		}
		static void _0x1c073274e065c6d2(type::any p0, bool p1) {
			invoker::invoke<void>(0x1c073274e065c6d2, p0, p1);
		}
		static void _0x2be4bc731d039d5a(type::any p0, bool p1) {
			invoker::invoke<void>(0x2be4bc731d039d5a, p0, p1);
		}
		static void set_vehicle_boost_active(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0x4a04de7cab2739a1, vehicle, toggle);
		}
		static void _0x6fddad856e36988a(type::any p0, bool p1) {
			invoker::invoke<void>(0x6fddad856e36988a, p0, p1);
		}
		static void _0x06c0023bed16dd6b(type::any p0, bool p1) {
			invoker::invoke<void>(0x06c0023bed16dd6b, p0, p1);
		}
		static void play_vehicle_door_open_sound(type::vehicle vehicle, int p1) {
			invoker::invoke<void>(0x3a539d52857ea82d, vehicle, p1);
		}
		static void play_vehicle_door_close_sound(type::vehicle vehicle, int p1) {
			invoker::invoke<void>(0x62a456aa4769ef34, vehicle, p1);
		}
		static void _0xc15907d667f7cfb2(type::vehicle vehicle, bool toggle) {
			invoker::invoke<void>(0xc15907d667f7cfb2, vehicle, toggle);
		}
		static bool is_game_in_control_of_music() {
			return invoker::invoke<bool>(0x6d28dc1671e334fd);
		}
		static void set_gps_active(bool active) {
			invoker::invoke<void>(0x3bd3f52ba9b1e4e8, active);
		}
		static void play_mission_complete_audio(const char* audioname) {
			invoker::invoke<void>(0xb138aab8a70d3c69, audioname);
		}
		static bool is_mission_complete_playing() {
			return invoker::invoke<bool>(0x19a30c23f5827f8a);
		}
		static bool _0x6f259f82d873b8b8() {
			return invoker::invoke<bool>(0x6f259f82d873b8b8);
		}
		static void _0xf154b8d1775b2dec(bool p0) {
			invoker::invoke<void>(0xf154b8d1775b2dec, p0);
		}
		static bool start_audio_scene(const char* scene) {
			return invoker::invoke<bool>(0x013a80fc08f6e4f2, scene);
		}
		static void stop_audio_scene(const char* scene) {
			invoker::invoke<void>(0xdfe8422b3b94e688, scene);
		}
		static void stop_audio_scenes() {
			invoker::invoke<void>(0xbac7fc81a75ec1a1);
		}
		static bool is_audio_scene_active(const char* scene) {
			return invoker::invoke<bool>(0xb65b60556e2a9225, scene);
		}
		static void set_audio_scene_variable(const char* scene, const char* variable, float value) {
			invoker::invoke<void>(0xef21a9ef089a2668, scene, variable, value);
		}
		static void _0xa5f377b175a699c5(type::any p0) {
			invoker::invoke<void>(0xa5f377b175a699c5, p0);
		}
		static void _dynamic_mixer_related_fn(type::entity p0, const char* p1, float p2) {
			invoker::invoke<void>(0x153973ab99fe8980, p0, p1, p2);
		}
		static void _0x18eb48cfc41f2ea0(type::any p0, float p1) {
			invoker::invoke<void>(0x18eb48cfc41f2ea0, p0, p1);
		}
		static bool audio_is_scripted_music_playing() {
			return invoker::invoke<bool>(0x845ffc3a4feefa3e);
		}
		static bool prepare_music_event(const char* eventname) {
			return invoker::invoke<bool>(0x1e5185b72ef5158a, eventname);
		}
		static bool cancel_music_event(const char* eventname) {
			return invoker::invoke<bool>(0x5b17a90291133da5, eventname);
		}
		static bool trigger_music_event(const char* eventname) {
			return invoker::invoke<bool>(0x706d57b0f50da710, eventname);
		}
		static type::any _0xa097ab275061fb21() {
			return invoker::invoke<type::any>(0xa097ab275061fb21);
		}
		static type::any get_music_playtime() {
			return invoker::invoke<type::any>(0xe7a0d23dc414507b);
		}
		static void _0xfbe20329593dec9d(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0xfbe20329593dec9d, p0, p1, p2, p3);
		}
		static void clear_all_broken_glass() {
			invoker::invoke<void>(0xb32209effdc04913);
		}
		static void _0x70b8ec8fc108a634(bool p0, type::any p1) {
			invoker::invoke<void>(0x70b8ec8fc108a634, p0, p1);
		}
		static void _0x149aee66f0cb3a99(float p0, float p1) {
			invoker::invoke<void>(0x149aee66f0cb3a99, p0, p1);
		}
		static void _0x8bf907833be275de(float p0, float p1) {
			invoker::invoke<void>(0x8bf907833be275de, p0, p1);
		}
		static void _0x062d5ead4da2fa6a() {
			invoker::invoke<void>(0x062d5ead4da2fa6a);
		}
		static bool prepare_alarm(const char* alarmname) {
			return invoker::invoke<bool>(0x9d74ae343db65533, alarmname);
		}
		static void start_alarm(const char* alarmname, bool p2) {
			invoker::invoke<void>(0x0355ef116c4c97b2, alarmname, p2);
		}
		static void stop_alarm(const char* alarmname, bool toggle) {
			invoker::invoke<void>(0xa1caddcd98415a41, alarmname, toggle);
		}
		static void stop_all_alarms(bool stop) {
			invoker::invoke<void>(0x2f794a877add4c92, stop);
		}
		static bool is_alarm_playing(const char* alarmname) {
			return invoker::invoke<bool>(0x226435cb96ccfc8c, alarmname);
		}
		static type::hash get_vehicle_default_horn(type::vehicle vehicle) {
			return invoker::invoke<type::hash>(0x02165d55000219ac, vehicle);
		}
		static type::hash _get_vehicle_horn_hash(type::vehicle vehicle) {
			return invoker::invoke<type::hash>(0xacb5dcca1ec76840, vehicle);
		}
		static void reset_ped_audio_flags(type::ped ped) {
			invoker::invoke<void>(0xf54bb7b61036f335, ped);
		}
		static void _0xd2cc78cd3d0b50f9(type::any p0, bool p1) {
			invoker::invoke<void>(0xd2cc78cd3d0b50f9, p0, p1);
		}
		static void _0xbf4dc1784be94dfa(type::any p0, bool p1, type::any p2) {
			invoker::invoke<void>(0xbf4dc1784be94dfa, p0, p1, p2);
		}
		static void _0x75773e11ba459e90(type::any p0, bool p1) {
			invoker::invoke<void>(0x75773e11ba459e90, p0, p1);
		}
		static void _0xd57aaae0e2214d11() {
			invoker::invoke<void>(0xd57aaae0e2214d11);
		}
		static void _force_ambient_siren(bool value) {
			invoker::invoke<void>(0x552369f549563ad5, value);
		}
		static void _0x43fa0dfc5df87815(type::vehicle vehicle, bool p1) {
			invoker::invoke<void>(0x43fa0dfc5df87815, vehicle, p1);
		}
		static void set_audio_flag(const char* flagname, bool toggle) {
			invoker::invoke<void>(0xb9efd5c25018725a, flagname, toggle);
		}
		static type::any prepare_synchronized_audio_event(const char* audioname, bool unk) {
			return invoker::invoke<type::any>(0xc7abcaca4985a766, audioname, unk);
		}
		static bool prepare_synchronized_audio_event_for_scene(int sceneid, const char* audioname) {
			return invoker::invoke<bool>(0x029fe7cd1b7e2e75, sceneid, audioname);
		}
		static bool play_synchronized_audio_event(int sceneid) {
			return invoker::invoke<bool>(0x8b2fd4560e55dd2d, sceneid);
		}
		static bool stop_synchronized_audio_event(int sceneid) {
			return invoker::invoke<bool>(0x92d6a88e64a94430, sceneid);
		}
		static void _0xc8ede9bdbccba6d4(type::any* p0, float p1, float p2, float p3) {
			invoker::invoke<void>(0xc8ede9bdbccba6d4, p0, p1, p2, p3);
		}
		static void _set_synchronized_audio_event_position_this_frame(const char* p0, type::entity p1) {
			invoker::invoke<void>(0x950a154b8dab6185, p0, p1);
		}
		static void _0x12561fcbb62d5b9c(int p0) {
			invoker::invoke<void>(0x12561fcbb62d5b9c, p0);
		}
		static void _0x044dbad7a7fa2be5(const char* p0, const char* p1) {
			invoker::invoke<void>(0x044dbad7a7fa2be5, p0, p1);
		}
		static void _0xb4bbfd9cd8b3922b(const char* p0) {
			invoker::invoke<void>(0xb4bbfd9cd8b3922b, p0);
		}
		static void _0xe4e6dd5566d28c82() {
			invoker::invoke<void>(0xe4e6dd5566d28c82);
		}
		static bool _0x3a48ab4445d499be() {
			return invoker::invoke<bool>(0x3a48ab4445d499be);
		}
		static void _set_ped_talk(type::ped ped) {
			invoker::invoke<void>(0x4ada3f19be4a6047, ped);
		}
		static void _0x0150b6ff25a9e2e5() {
			invoker::invoke<void>(0x0150b6ff25a9e2e5);
		}
		static void _0xbef34b1d9624d5dd(bool p0) {
			invoker::invoke<void>(0xbef34b1d9624d5dd, p0);
		}
		static void _0x806058bbdc136e06() {
			invoker::invoke<void>(0x806058bbdc136e06);
		}
		static bool _0x544810ed9db6bbe6() {
			return invoker::invoke<bool>(0x544810ed9db6bbe6);
		}
		static bool _0x5b50abb1fe3746f4() {
			return invoker::invoke<bool>(0x5b50abb1fe3746f4);
		}
	}

	namespace cutscene {
		static void request_cutscene(const char* cutscenename, int p1) {
			invoker::invoke<void>(0x7a86743f475d9e09, cutscenename, p1);
		}
		static void _request_cutscene_ex(const char* cutscenename, int p1, int p2) {
			invoker::invoke<void>(0xc23de0e91c30b58c, cutscenename, p1, p2);
		}
		static void remove_cutscene() {
			invoker::invoke<void>(0x440af51a3462b86f);
		}
		static bool has_cutscene_loaded() {
			return invoker::invoke<bool>(0xc59f528e9ab9f339);
		}
		static bool has_this_cutscene_loaded(const char* cutscenename) {
			return invoker::invoke<bool>(0x228d3d94f8a11c3c, cutscenename);
		}
		static void _0x8d9df6eca8768583(int p0) {
			invoker::invoke<void>(0x8d9df6eca8768583, p0);
		}
		static bool _0xb56bbbcc2955d9cb() {
			return invoker::invoke<bool>(0xb56bbbcc2955d9cb);
		}
		static bool _0x71b74d2ae19338d0(int p0) {
			return invoker::invoke<bool>(0x71b74d2ae19338d0, p0);
		}
		static void _0x4c61c75bee8184c2(const char* p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0x4c61c75bee8184c2, p0, p1, p2);
		}
		static void _0x06a3524161c502ba(type::any* p0) {
			invoker::invoke<void>(0x06a3524161c502ba, p0);
		}
		static bool _0xa1c996c2a744262e(type::any* p0) {
			return invoker::invoke<bool>(0xa1c996c2a744262e, p0);
		}
		static void _0xd00d76a7dfc9d852(type::any* p0) {
			invoker::invoke<void>(0xd00d76a7dfc9d852, p0);
		}
		static type::any _0x0abc54de641dc0fc(type::any* p0) {
			return invoker::invoke<type::any>(0x0abc54de641dc0fc, p0);
		}
		static void start_cutscene(int p0) {
			invoker::invoke<void>(0x186d5cb5e7b0ff7b, p0);
		}
		static void start_cutscene_at_coords(float x, float y, float z, int p3) {
			invoker::invoke<void>(0x1c9adda3244a1fbf, x, y, z, p3);
		}
		static void stop_cutscene(bool p0) {
			invoker::invoke<void>(0xc7272775b4dc786e, p0);
		}
		static void stop_cutscene_immediately() {
			invoker::invoke<void>(0xd220bdd222ac4a1e);
		}
		static void set_cutscene_origin(float x, float y, float z, float p3, int p4) {
			invoker::invoke<void>(0xb812b3fd1c01cf27, x, y, z, p3, p4);
		}
		static void _0x011883f41211432a(float x1, float y1, float z1, float x2, float y2, float z2, int p6) {
			invoker::invoke<void>(0x011883f41211432a, x1, y1, z1, x2, y2, z2, p6);
		}
		static int get_cutscene_time() {
			return invoker::invoke<int>(0xe625beabbaffdab9);
		}
		static int get_cutscene_total_duration() {
			return invoker::invoke<int>(0xee53b14a19e480d4);
		}
		static bool was_cutscene_skipped() {
			return invoker::invoke<bool>(0x40c8656edaedd569);
		}
		static bool has_cutscene_finished() {
			return invoker::invoke<bool>(0x7c0a893088881d57);
		}
		static bool is_cutscene_active() {
			return invoker::invoke<bool>(0x991251afc3981f84);
		}
		static bool is_cutscene_playing() {
			return invoker::invoke<bool>(0xd3c2e180a40f031e);
		}
		static int get_cutscene_section_playing() {
			return invoker::invoke<int>(0x49010a6a396553d8);
		}
		static type::entity get_entity_index_of_cutscene_entity(const char* cutsceneentname, type::hash modelhash) {
			return invoker::invoke<type::entity>(0x0a2e9fdb9a8c62f6, cutsceneentname, modelhash);
		}
		static int _0x583df8e3d4afbd98() {
			return invoker::invoke<int>(0x583df8e3d4afbd98);
		}
		static bool _0x4cebc1ed31e8925e(const char* cutscenename) {
			return invoker::invoke<bool>(0x4cebc1ed31e8925e, cutscenename);
		}
		static void register_entity_for_cutscene(type::ped cutsceneped, const char* cutsceneentname, int p2, type::hash modelhash, int p4) {
			invoker::invoke<void>(0xe40c1c56df95c2e8, cutsceneped, cutsceneentname, p2, modelhash, p4);
		}
		static type::entity get_entity_index_of_registered_entity(const char* cutsceneentname, type::hash modelhash) {
			return invoker::invoke<type::entity>(0xc0741a26499654cd, cutsceneentname, modelhash);
		}
		static void _0x7f96f23fa9b73327(type::hash modelhash) {
			invoker::invoke<void>(0x7f96f23fa9b73327, modelhash);
		}
		static void set_cutscene_trigger_area(float p0, float p1, float p2, float p3, float p4, float p5) {
			invoker::invoke<void>(0x9896ce4721be84ba, p0, p1, p2, p3, p4, p5);
		}
		static bool can_set_enter_state_for_registered_entity(const char* cutsceneentname, type::hash modelhash) {
			return invoker::invoke<bool>(0x645d0b458d8e17b5, cutsceneentname, modelhash);
		}
		static bool can_set_exit_state_for_registered_entity(const char* cutsceneentname, type::hash modelhash) {
			return invoker::invoke<bool>(0x4c6a6451c79e4662, cutsceneentname, modelhash);
		}
		static bool can_set_exit_state_for_camera(bool p0) {
			return invoker::invoke<bool>(0xb2cbcd0930dfb420, p0);
		}
		static void _0xc61b86c9f61eb404(bool toggle) {
			invoker::invoke<void>(0xc61b86c9f61eb404, toggle);
		}
		static void set_cutscene_fade_values(bool p0, bool p1, bool p2, bool p3) {
			invoker::invoke<void>(0x8093f23abaccc7d4, p0, p1, p2, p3);
		}
		static void _0x20746f7b1032a3c7(bool p0, bool p1, bool p2, bool p3) {
			invoker::invoke<void>(0x20746f7b1032a3c7, p0, p1, p2, p3);
		}
		static void _0x06ee9048fd080382(bool p0) {
			invoker::invoke<void>(0x06ee9048fd080382, p0);
		}
		static int _0xa0fe76168a189ddb() {
			return invoker::invoke<int>(0xa0fe76168a189ddb);
		}
		static void _0x2f137b508de238f2(bool p0) {
			invoker::invoke<void>(0x2f137b508de238f2, p0);
		}
		static void _0xe36a98d8ab3d3c66(bool p0) {
			invoker::invoke<void>(0xe36a98d8ab3d3c66, p0);
		}
		static bool _0x5edef0cf8c1dab3c() {
			return invoker::invoke<bool>(0x5edef0cf8c1dab3c);
		}
		static void _0x41faa8fb2ece8720(bool p0) {
			invoker::invoke<void>(0x41faa8fb2ece8720, p0);
		}
		static void register_synchronised_script_speech() {
			invoker::invoke<void>(0x2131046957f31b04);
		}
		static void set_cutscene_ped_component_variation(const char* cutsceneentname, int p1, int p2, int p3, type::hash modelhash) {
			invoker::invoke<void>(0xba01e7b6deefbbc9, cutsceneentname, p1, p2, p3, modelhash);
		}
		static void _0x2a56c06ebef2b0d9(const char* cutsceneentname, type::ped ped, type::hash modelhash) {
			invoker::invoke<void>(0x2a56c06ebef2b0d9, cutsceneentname, ped, modelhash);
		}
		static bool does_cutscene_entity_exist(const char* cutsceneentname, type::hash modelhash) {
			return invoker::invoke<bool>(0x499ef20c5db25c59, cutsceneentname, modelhash);
		}
		static void set_cutscene_ped_prop_variation(const char* cutsceneentname, int p1, int p2, int p3, type::hash modelhash) {
			invoker::invoke<void>(0x0546524ade2e9723, cutsceneentname, p1, p2, p3, modelhash);
		}
		static bool _0x708bdd8cd795b043() {
			return invoker::invoke<bool>(0x708bdd8cd795b043);
		}
	}

	namespace interior {
		static int get_interior_group_id(int interiorid) {
			return invoker::invoke<int>(0xe4a84abf135ef91a, interiorid);
		}
		static PVector3 get_offset_from_interior_in_world_coords(int interiorid, float x, float y, float z) {
			return invoker::invoke<PVector3>(0x9e3b3e6d66f6e22f, interiorid, x, y, z);
		}
		static bool is_interior_scene() {
			return invoker::invoke<bool>(0xbc72b5d7a1cbd54d);
		}
		static bool is_valid_interior(int interiorid) {
			return invoker::invoke<bool>(0x26b0e73d7eaaf4d3, interiorid);
		}
		static void clear_room_for_entity(type::entity entity) {
			invoker::invoke<void>(0xb365fc0c4e27ffa7, entity);
		}
		static void force_room_for_entity(type::entity entity, int interiorid, type::hash roomhashkey) {
			invoker::invoke<void>(0x52923c4710dd9907, entity, interiorid, roomhashkey);
		}
		static type::hash get_room_key_from_entity(type::entity entity) {
			return invoker::invoke<type::hash>(0x47c2a06d4f5f424b, entity);
		}
		static type::hash get_key_for_entity_in_room(type::entity entity) {
			return invoker::invoke<type::hash>(0x399685db942336bc, entity);
		}
		static int get_interior_from_entity(type::entity entity) {
			return invoker::invoke<int>(0x2107ba504071a6bb, entity);
		}
		static void _0x82ebb79e258fa2b7(type::entity entity, int interiorid) {
			invoker::invoke<void>(0x82ebb79e258fa2b7, entity, interiorid);
		}
		static void _0x920d853f3e17f1da(int interiorid, type::hash roomhashkey) {
			invoker::invoke<void>(0x920d853f3e17f1da, interiorid, roomhashkey);
		}
		static void _0xaf348afcb575a441(const char* roomname) {
			invoker::invoke<void>(0xaf348afcb575a441, roomname);
		}
		static void _0x405dc2aef6af95b9(type::hash roomhashkey) {
			invoker::invoke<void>(0x405dc2aef6af95b9, roomhashkey);
		}
		static type::hash _get_room_key_from_gameplay_cam() {
			return invoker::invoke<type::hash>(0xa6575914d2a0b450);
		}
		static void _0x23b59d8912f94246() {
			invoker::invoke<void>(0x23b59d8912f94246);
		}
		static int get_interior_at_coords(float x, float y, float z) {
			return invoker::invoke<int>(0xb0f7f8663821d9c3, x, y, z);
		}
		static void add_pickup_to_interior_room_by_name(type::pickup pickup, const char* roomname) {
			invoker::invoke<void>(0x3f6167f351168730, pickup, roomname);
		}
		static void _load_interior(int interiorid) {
			invoker::invoke<void>(0x2ca429c029ccf247, interiorid);
		}
		static void unpin_interior(int interiorid) {
			invoker::invoke<void>(0x261cce7eed010641, interiorid);
		}
		static bool is_interior_ready(int interiorid) {
			return invoker::invoke<bool>(0x6726bdccc1932f0e, interiorid);
		}
		static type::any _0x4c2330e61d3deb56(int interiorid) {
			return invoker::invoke<type::any>(0x4c2330e61d3deb56, interiorid);
		}
		static int get_interior_at_coords_with_type(float x, float y, float z, const char* interiortype) {
			return invoker::invoke<int>(0x05b7a89bd78797fc, x, y, z, interiortype);
		}
		static int _unk_get_interior_at_coords(float x, float y, float z, int unk) {
			return invoker::invoke<int>(0xf0f77adb9f67e79d, x, y, z, unk);
		}
		static bool _are_coords_colliding_with_exterior(float x, float y, float z) {
			return invoker::invoke<bool>(0xeea5ac2eda7c33e8, x, y, z);
		}
		static int get_interior_from_collision(float x, float y, float z) {
			return invoker::invoke<int>(0xec4cf9fcb29a4424, x, y, z);
		}
		static void _enable_interior_prop(int interiorid, const char* propname) {
			invoker::invoke<void>(0x55e86af2712b36a1, interiorid, propname);
		}
		static void _disable_interior_prop(int interiorid, const char* propname) {
			invoker::invoke<void>(0x420bd37289eee162, interiorid, propname);
		}
		static bool _is_interior_prop_enabled(int interiorid, const char* propname) {
			return invoker::invoke<bool>(0x35f7dd45e8c0a16d, interiorid, propname);
		}
		static void refresh_interior(int interiorid) {
			invoker::invoke<void>(0x41f37c3427c75ae0, interiorid);
		}
		static void _hide_map_object_this_frame(type::hash mapobjecthash) {
			invoker::invoke<void>(0xa97f257d0151a6ab, mapobjecthash);
		}
		static void disable_interior(int interiorid, bool toggle) {
			invoker::invoke<void>(0x6170941419d7d8ec, interiorid, toggle);
		}
		static bool is_interior_disabled(int interiorid) {
			return invoker::invoke<bool>(0xbc5115a5a939dd15, interiorid);
		}
		static void cap_interior(int interiorid, bool toggle) {
			invoker::invoke<void>(0xd9175f941610db54, interiorid, toggle);
		}
		static bool is_interior_capped(int interiorid) {
			return invoker::invoke<bool>(0x92bac8acf88cec26, interiorid);
		}
		static void _0x9e6542f0ce8e70a3(bool toggle) {
			invoker::invoke<void>(0x9e6542f0ce8e70a3, toggle);
		}
	}

	namespace cam {
		static void render_script_cams(bool render, bool ease, int easetime, bool p3, bool p4) {
			invoker::invoke<void>(0x07e5b515db0636fc, render, ease, easetime, p3, p4);
		}
		static void _render_first_person_cam(bool render, float p1, int p2) {
			invoker::invoke<void>(0xc819f3cbb62bf692, render, p1, p2);
		}
		static type::cam create_cam(const char* camname, bool unk) {
			return invoker::invoke<type::cam>(0xc3981dce61d9e13f, camname, unk);
		}
		static type::cam create_cam_with_params(const char* camname, float posx, float posy, float posz, float rotx, float roty, float rotz, float fov, bool p8, int p9) {
			return invoker::invoke<type::cam>(0xb51194800b257161, camname, posx, posy, posz, rotx, roty, rotz, fov, p8, p9);
		}
		static type::cam create_camera(type::hash camhash, bool p1) {
			return invoker::invoke<type::cam>(0x5e3cf89c6bcca67d, camhash, p1);
		}
		static type::cam create_camera_with_params(type::hash camhash, float posx, float posy, float posz, float rotx, float roty, float rotz, float fov, bool p8, type::any p9) {
			return invoker::invoke<type::cam>(0x6abfa3e16460f22d, camhash, posx, posy, posz, rotx, roty, rotz, fov, p8, p9);
		}
		static void destroy_cam(type::cam cam, bool thisscriptcheck) {
			invoker::invoke<void>(0x865908c81a2c22e9, cam, thisscriptcheck);
		}
		static void destroy_all_cams(bool thisscriptcheck) {
			invoker::invoke<void>(0x8e5fb15663f79120, thisscriptcheck);
		}
		static bool does_cam_exist(type::cam cam) {
			return invoker::invoke<bool>(0xa7a932170592b50e, cam);
		}
		static void set_cam_active(type::cam cam, bool active) {
			invoker::invoke<void>(0x026fb97d0a425f84, cam, active);
		}
		static bool is_cam_active(type::cam cam) {
			return invoker::invoke<bool>(0xdfb2b516207d3534, cam);
		}
		static bool is_cam_rendering(type::cam cam) {
			return invoker::invoke<bool>(0x02ec0af5c5a49b7a, cam);
		}
		static type::cam get_rendering_cam() {
			return invoker::invoke<type::cam>(0x5234f9f10919eaba);
		}
		static PVector3 get_cam_coord(type::cam cam) {
			return invoker::invoke<PVector3>(0xbac038f7459ae5ae, cam);
		}
		static PVector3 get_cam_rot(type::cam cam, int rotationorder) {
			return invoker::invoke<PVector3>(0x7d304c1c955e3e12, cam, rotationorder);
		}
		static float get_cam_fov(type::cam cam) {
			return invoker::invoke<float>(0xc3330a45cccdb26a, cam);
		}
		static float get_cam_near_clip(type::cam cam) {
			return invoker::invoke<float>(0xc520a34dafbf24b1, cam);
		}
		static float get_cam_far_clip(type::cam cam) {
			return invoker::invoke<float>(0xb60a9cfeb21ca6aa, cam);
		}
		static float get_cam_far_dof(type::cam cam) {
			return invoker::invoke<float>(0x255f8dafd540d397, cam);
		}
		static void set_cam_params(type::cam cam, float posx, float posy, float posz, float rotx, float roty, float rotz, float fieldofview, type::any p8, int p9, int p10, int p11) {
			invoker::invoke<void>(0xbfd8727aea3cceba, cam, posx, posy, posz, rotx, roty, rotz, fieldofview, p8, p9, p10, p11);
		}
		static void set_cam_coord(type::cam cam, float posx, float posy, float posz) {
			invoker::invoke<void>(0x4d41783fb745e42e, cam, posx, posy, posz);
		}
		static void set_cam_rot(type::cam cam, float pitch, float roll, float yaw, int rotationorder) {
			invoker::invoke<void>(0x85973643155d0b07, cam, pitch, roll, yaw, rotationorder);
		}
		static void set_cam_fov(type::cam cam, float fieldofview) {
			invoker::invoke<void>(0xb13c14f66a00d047, cam, fieldofview);
		}
		static void set_cam_near_clip(type::cam cam, float nearclip) {
			invoker::invoke<void>(0xc7848efccc545182, cam, nearclip);
		}
		static void set_cam_far_clip(type::cam cam, float farclip) {
			invoker::invoke<void>(0xae306f2a904bf86e, cam, farclip);
		}
		static void set_cam_motion_blur_strength(type::cam cam, float strength) {
			invoker::invoke<void>(0x6f0f77fba9a8f2e6, cam, strength);
		}
		static void set_cam_near_dof(type::cam cam, float neardof) {
			invoker::invoke<void>(0x3fa4bf0a7ab7de2c, cam, neardof);
		}
		static void set_cam_far_dof(type::cam cam, float fardof) {
			invoker::invoke<void>(0xedd91296cd01aee0, cam, fardof);
		}
		static void set_cam_dof_strength(type::cam cam, float dofstrength) {
			invoker::invoke<void>(0x5ee29b4d7d5df897, cam, dofstrength);
		}
		static void set_cam_dof_planes(type::cam cam, float p1, float p2, float p3, float p4) {
			invoker::invoke<void>(0x3cf48f6f96e749dc, cam, p1, p2, p3, p4);
		}
		static void set_cam_use_shallow_dof_mode(type::cam cam, bool toggle) {
			invoker::invoke<void>(0x16a96863a17552bb, cam, toggle);
		}
		static void set_use_hi_dof() {
			invoker::invoke<void>(0xa13b0222f3d94a94);
		}
		static void _0xf55e4046f6f831dc(type::any p0, float p1) {
			invoker::invoke<void>(0xf55e4046f6f831dc, p0, p1);
		}
		static void _0xe111a7c0d200cbc5(type::any p0, float p1) {
			invoker::invoke<void>(0xe111a7c0d200cbc5, p0, p1);
		}
		static void _set_cam_dof_fnumber_of_lens(type::cam camera, float p1) {
			invoker::invoke<void>(0x7dd234d6f3914c5b, camera, p1);
		}
		static void _set_cam_dof_focus_distance_bias(type::cam camera, float p1) {
			invoker::invoke<void>(0xc669eea5d031b7de, camera, p1);
		}
		static void _set_cam_dof_max_near_in_focus_distance(type::cam camera, float p1) {
			invoker::invoke<void>(0xc3654a441402562d, camera, p1);
		}
		static void _set_cam_dof_max_near_in_focus_distance_blend_level(type::cam camera, float p1) {
			invoker::invoke<void>(0x2c654b4943bddf7c, camera, p1);
		}
		static void attach_cam_to_entity(type::cam cam, type::entity entity, float xoffset, float yoffset, float zoffset, bool isrelative) {
			invoker::invoke<void>(0xfedb7d269e8c60e3, cam, entity, xoffset, yoffset, zoffset, isrelative);
		}
		static void attach_cam_to_ped_bone(type::cam cam, type::ped ped, int boneindex, float x, float y, float z, bool heading) {
			invoker::invoke<void>(0x61a3dba14ab7f411, cam, ped, boneindex, x, y, z, heading);
		}
		static void detach_cam(type::cam cam) {
			invoker::invoke<void>(0xa2fabbe87f4bad82, cam);
		}
		static void set_cam_inherit_roll_vehicle(type::cam cam, bool p1) {
			invoker::invoke<void>(0x45f1de9c34b93ae6, cam, p1);
		}
		static void point_cam_at_coord(type::cam cam, float x, float y, float z) {
			invoker::invoke<void>(0xf75497bb865f0803, cam, x, y, z);
		}
		static void point_cam_at_entity(type::cam cam, type::entity entity, float p2, float p3, float p4, bool p5) {
			invoker::invoke<void>(0x5640bff86b16e8dc, cam, entity, p2, p3, p4, p5);
		}
		static void point_cam_at_ped_bone(type::cam cam, int ped, int boneindex, float x, float y, float z, bool p6) {
			invoker::invoke<void>(0x68b2b5f33ba63c41, cam, ped, boneindex, x, y, z, p6);
		}
		static void stop_cam_pointing(type::cam cam) {
			invoker::invoke<void>(0xf33ab75780ba57de, cam);
		}
		static void set_cam_affects_aiming(type::cam cam, bool toggle) {
			invoker::invoke<void>(0x8c1dc7770c51dc8d, cam, toggle);
		}
		static void _0x661b5c8654add825(type::any p0, bool p1) {
			invoker::invoke<void>(0x661b5c8654add825, p0, p1);
		}
		static void _0xa2767257a320fc82(type::any p0, bool p1) {
			invoker::invoke<void>(0xa2767257a320fc82, p0, p1);
		}
		static void _0x271017b9ba825366(type::any p0, bool p1) {
			invoker::invoke<void>(0x271017b9ba825366, p0, p1);
		}
		static void set_cam_debug_name(type::cam camera, const char* name) {
			invoker::invoke<void>(0x1b93e0107865dd40, camera, name);
		}
		static void add_cam_spline_node(type::cam camera, float x, float y, float z, float xrot, float yrot, float zrot, int length, int p8, int transitiontype) {
			invoker::invoke<void>(0x8609c75ec438fb3b, camera, x, y, z, xrot, yrot, zrot, length, p8, transitiontype);
		}
		static void _0x0a9f2a468b328e74(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x0a9f2a468b328e74, p0, p1, p2, p3);
		}
		static void _0x0fb82563989cf4fb(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x0fb82563989cf4fb, p0, p1, p2, p3);
		}
		static void _0x609278246a29ca34(type::any p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0x609278246a29ca34, p0, p1, p2);
		}
		static void set_cam_spline_phase(type::cam cam, float p1) {
			invoker::invoke<void>(0x242b5874f0a4e052, cam, p1);
		}
		static float get_cam_spline_phase(type::cam cam) {
			return invoker::invoke<float>(0xb5349e36c546509a, cam);
		}
		static float get_cam_spline_node_phase(type::cam cam) {
			return invoker::invoke<float>(0xd9d0e694c8282c96, cam);
		}
		static void set_cam_spline_duration(int cam, int timeduration) {
			invoker::invoke<void>(0x1381539fee034cda, cam, timeduration);
		}
		static void _0xd1b0f412f109ea5d(type::any p0, type::any p1) {
			invoker::invoke<void>(0xd1b0f412f109ea5d, p0, p1);
		}
		static int get_cam_spline_node_index(type::cam cam) {
			return invoker::invoke<int>(0xb22b17df858716a6, cam);
		}
		static void _0x83b8201ed82a9a2d(type::any p0, type::any p1, type::any p2, float p3) {
			invoker::invoke<void>(0x83b8201ed82a9a2d, p0, p1, p2, p3);
		}
		static void _0xa6385deb180f319f(type::any p0, type::any p1, float p2) {
			invoker::invoke<void>(0xa6385deb180f319f, p0, p1, p2);
		}
		static void override_cam_spline_velocity(type::cam cam, int p1, float p2, float p3) {
			invoker::invoke<void>(0x40b62fa033eb0346, cam, p1, p2, p3);
		}
		static void override_cam_spline_motion_blur(type::cam cam, int p1, float p2, float p3) {
			invoker::invoke<void>(0x7dcf7c708d292d55, cam, p1, p2, p3);
		}
		static void _0x7bf1a54ae67ac070(type::any p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0x7bf1a54ae67ac070, p0, p1, p2);
		}
		static bool is_cam_spline_paused(type::any p0) {
			return invoker::invoke<bool>(0x0290f35c0ad97864, p0);
		}
		static void set_cam_active_with_interp(type::cam camto, type::cam camfrom, int duration, int easelocation, int easerotation) {
			invoker::invoke<void>(0x9fbda379383a52a4, camto, camfrom, duration, easelocation, easerotation);
		}
		static bool is_cam_interpolating(type::cam cam) {
			return invoker::invoke<bool>(0x036f97c908c2b52c, cam);
		}
		static void shake_cam(type::cam cam, const char* type, float amplitude) {
			invoker::invoke<void>(0x6a25241c340d3822, cam, type, amplitude);
		}
		static void animated_shake_cam(type::cam cam, const char* p1, const char* p2, const char* p3, float amplitude) {
			invoker::invoke<void>(0xa2746eeae3e577cd, cam, p1, p2, p3, amplitude);
		}
		static bool is_cam_shaking(type::cam cam) {
			return invoker::invoke<bool>(0x6b24bfe83a2be47b, cam);
		}
		static void set_cam_shake_amplitude(type::cam cam, float amplitude) {
			invoker::invoke<void>(0xd93db43b82bc0d00, cam, amplitude);
		}
		static void stop_cam_shaking(type::cam cam, bool p1) {
			invoker::invoke<void>(0xbdecf64367884ac3, cam, p1);
		}
		static void _0xf4c8cf9e353afeca(const char* p0, float p1) {
			invoker::invoke<void>(0xf4c8cf9e353afeca, p0, p1);
		}
		static void _0xc2eae3fb8cdbed31(const char* p0, const char* p1, const char* p2, float p3) {
			invoker::invoke<void>(0xc2eae3fb8cdbed31, p0, p1, p2, p3);
		}
		static bool is_script_global_shaking() {
			return invoker::invoke<bool>(0xc912af078af19212);
		}
		static void stop_script_global_shaking(bool p0) {
			invoker::invoke<void>(0x1c9d7949fa533490, p0);
		}
		static bool play_cam_anim(type::cam cam, const char* animname, const char* animdictionary, float x, float y, float z, float xrot, float yrot, float zrot, bool p9, int p10) {
			return invoker::invoke<bool>(0x9a2d0fb2e7852392, cam, animname, animdictionary, x, y, z, xrot, yrot, zrot, p9, p10);
		}
		static bool is_cam_playing_anim(type::cam cam, const char* animname, const char* animdictionary) {
			return invoker::invoke<bool>(0xc90621d8a0ceecf2, cam, animname, animdictionary);
		}
		static void set_cam_anim_current_phase(type::cam cam, float phase) {
			invoker::invoke<void>(0x4145a4c44ff3b5a6, cam, phase);
		}
		static float get_cam_anim_current_phase(type::cam cam) {
			return invoker::invoke<float>(0xa10b2db49e92a6b0, cam);
		}
		static bool play_synchronized_cam_anim(type::cam camera, int sceneid, const char* animname, const char* animdictionary) {
			return invoker::invoke<bool>(0xe32efe9ab4a9aa0c, camera, sceneid, animname, animdictionary);
		}
		static void set_fly_cam_horizontal_response(type::any p0, float p1, float p2, float p3) {
			invoker::invoke<void>(0x503f5920162365b2, p0, p1, p2, p3);
		}
		static void set_fly_cam_max_height(type::cam cam, float range) {
			invoker::invoke<void>(0xf9d02130ecdd1d77, cam, range);
		}
		static void set_fly_cam_coord_and_constrain(type::any p0, float p1, float p2, float p3) {
			invoker::invoke<void>(0xc91c6c55199308ca, p0, p1, p2, p3);
		}
		static void _0xc8b5c4a79cc18b94(type::cam p0) {
			invoker::invoke<void>(0xc8b5c4a79cc18b94, p0);
		}
		static bool _0x5c48a1d6e3b33179(type::any p0) {
			return invoker::invoke<bool>(0x5c48a1d6e3b33179, p0);
		}
		static bool is_screen_faded_out() {
			return invoker::invoke<bool>(0xb16fce9ddc7ba182);
		}
		static bool is_screen_faded_in() {
			return invoker::invoke<bool>(0x5a859503b0c08678);
		}
		static bool is_screen_fading_out() {
			return invoker::invoke<bool>(0x797ac7cb535ba28f);
		}
		static bool is_screen_fading_in() {
			return invoker::invoke<bool>(0x5c544bc6c57ac575);
		}
		static void do_screen_fade_in(int duration) {
			invoker::invoke<void>(0xd4e8e24955024033, duration);
		}
		static void do_screen_fade_out(int duration) {
			invoker::invoke<void>(0x891b5b39ac6302af, duration);
		}
		static void set_widescreen_borders(bool toggle, int duration) {
			invoker::invoke<void>(0xdcd4ea924f42d01a, toggle, duration);
		}
		static PVector3 get_gameplay_cam_coord() {
			return invoker::invoke<PVector3>(0x14d6f5678d8f1b37);
		}
		static PVector3 get_gameplay_cam_rot(int rotationorder) {
			return invoker::invoke<PVector3>(0x837765a25378f0bb, rotationorder);
		}
		static float get_gameplay_cam_fov() {
			return invoker::invoke<float>(0x65019750a0324133);
		}
		static void custom_menu_coordinates(float p0) {
			invoker::invoke<void>(0x487a82c650eb7799, p0);
		}
		static void _0x0225778816fdc28c(float p0) {
			invoker::invoke<void>(0x0225778816fdc28c, p0);
		}
		static float get_gameplay_cam_relative_heading() {
			return invoker::invoke<float>(0x743607648add4587);
		}
		static void set_gameplay_cam_relative_heading(float heading) {
			invoker::invoke<void>(0xb4ec2312f4e5b1f1, heading);
		}
		static float get_gameplay_cam_relative_pitch() {
			return invoker::invoke<float>(0x3a6867b4845beda2);
		}
		static void set_gameplay_cam_relative_pitch(float x, float value2) {
			invoker::invoke<void>(0x6d0858b8edfd2b7d, x, value2);
		}
		static void _set_gameplay_cam_raw_yaw(float yaw) {
			invoker::invoke<void>(0x103991d4a307d472, yaw);
		}
		static void _set_gameplay_cam_raw_pitch(float pitch) {
			invoker::invoke<void>(0x759e13ebc1c15c5a, pitch);
		}
		static void _0x469f2ecdec046337(bool p0) {
			invoker::invoke<void>(0x469f2ecdec046337, p0);
		}
		static void shake_gameplay_cam(const char* shakename, float intensity) {
			invoker::invoke<void>(0xfd55e49555e017cf, shakename, intensity);
		}
		static bool is_gameplay_cam_shaking() {
			return invoker::invoke<bool>(0x016c090630df1f89);
		}
		static void set_gameplay_cam_shake_amplitude(float amplitude) {
			invoker::invoke<void>(0xa87e00932db4d85d, amplitude);
		}
		static void stop_gameplay_cam_shaking(bool p0) {
			invoker::invoke<void>(0x0ef93e9f3d08c178, p0);
		}
		static void _0x8bbacbf51da047a8(type::any p0) {
			invoker::invoke<void>(0x8bbacbf51da047a8, p0);
		}
		static bool is_gameplay_cam_rendering() {
			return invoker::invoke<bool>(0x39b5d1b10383f0c8);
		}
		static bool _0x3044240d2e0fa842() {
			return invoker::invoke<bool>(0x3044240d2e0fa842);
		}
		static bool _0x705a276ebff3133d() {
			return invoker::invoke<bool>(0x705a276ebff3133d);
		}
		static void _0xdb90c6cca48940f1(bool p0) {
			invoker::invoke<void>(0xdb90c6cca48940f1, p0);
		}
		static void _enable_crosshair_this_frame() {
			invoker::invoke<void>(0xea7f0ad7e9ba676f);
		}
		static bool is_gameplay_cam_looking_behind() {
			return invoker::invoke<bool>(0x70fda869f3317ea9);
		}
		static void _0x2aed6301f67007d5(type::entity entity) {
			invoker::invoke<void>(0x2aed6301f67007d5, entity);
		}
		static void _0x49482f9fcd825aaa(type::entity entity) {
			invoker::invoke<void>(0x49482f9fcd825aaa, entity);
		}
		static void _0xfd3151cd37ea2245(type::any p0) {
			invoker::invoke<void>(0xfd3151cd37ea2245, p0);
		}
		static void _0xdd79df9f4d26e1c9() {
			invoker::invoke<void>(0xdd79df9f4d26e1c9);
		}
		static bool is_sphere_visible(float x, float y, float z, float radius) {
			return invoker::invoke<bool>(0xe33d59da70b58fdf, x, y, z, radius);
		}
		static bool is_follow_ped_cam_active() {
			return invoker::invoke<bool>(0xc6d3d26810c8e0f9);
		}
		static bool set_follow_ped_cam_cutscene_chat(const char* p0, int p1) {
			return invoker::invoke<bool>(0x44a113dd6ffc48d1, p0, p1);
		}
		static void _0x271401846bd26e92(bool p0, bool p1) {
			invoker::invoke<void>(0x271401846bd26e92, p0, p1);
		}
		static void _0xc8391c309684595a() {
			invoker::invoke<void>(0xc8391c309684595a);
		}
		static void _clamp_gameplay_cam_yaw(float minimum, float maximum) {
			invoker::invoke<void>(0x8f993d26e0ca5e8e, minimum, maximum);
		}
		static void _clamp_gameplay_cam_pitch(float minimum, float maximum) {
			invoker::invoke<void>(0xa516c198b7dca1e1, minimum, maximum);
		}
		static void _animate_gameplay_cam_zoom(float p0, float distance) {
			invoker::invoke<void>(0xdf2e1f7742402e81, p0, distance);
		}
		static void _0xe9ea16d6e54cdca4(type::vehicle p0, int p1) {
			invoker::invoke<void>(0xe9ea16d6e54cdca4, p0, p1);
		}
		static void _disable_first_person_cam_this_frame() {
			invoker::invoke<void>(0xde2ef5da284cc8df);
		}
		static void _0x59424bd75174c9b1() {
			invoker::invoke<void>(0x59424bd75174c9b1);
		}
		static int get_follow_ped_cam_zoom_level() {
			return invoker::invoke<int>(0x33e6c8efd0cd93e9);
		}
		static int get_follow_ped_cam_view_mode() {
			return invoker::invoke<int>(0x8d4d46230b2c353a);
		}
		static void set_follow_ped_cam_view_mode(int viewmode) {
			invoker::invoke<void>(0x5a4f9edf1673f704, viewmode);
		}
		static bool is_follow_vehicle_cam_active() {
			return invoker::invoke<bool>(0xcbbde6d335d6d496);
		}
		static void _0x91ef6ee6419e5b97(bool p0) {
			invoker::invoke<void>(0x91ef6ee6419e5b97, p0);
		}
		static void set_time_idle_drop(bool p0, bool p1) {
			invoker::invoke<void>(0x9dfe13ecdc1ec196, p0, p1);
		}
		static int get_follow_vehicle_cam_zoom_level() {
			return invoker::invoke<int>(0xee82280ab767b690);
		}
		static void set_follow_vehicle_cam_zoom_level(int zoomlevel) {
			invoker::invoke<void>(0x19464cb6e4078c8a, zoomlevel);
		}
		static int get_follow_vehicle_cam_view_mode() {
			return invoker::invoke<int>(0xa4ff579ac0e3aaae);
		}
		static void set_follow_vehicle_cam_view_mode(int viewmode) {
			invoker::invoke<void>(0xac253d7842768f48, viewmode);
		}
		static type::any _0xee778f8c7e1142e2(type::any p0) {
			return invoker::invoke<type::any>(0xee778f8c7e1142e2, p0);
		}
		static void _set_cam_perspective(type::cam p0, type::any perspective) {
			invoker::invoke<void>(0x2a2173e46daecd12, p0, perspective);
		}
		static type::any _0x19cafa3c87f7c2ff() {
			return invoker::invoke<type::any>(0x19cafa3c87f7c2ff);
		}
		static bool is_aim_cam_active() {
			return invoker::invoke<bool>(0x68edda28a5976d07);
		}
		static bool _0x74bd83ea840f6bc9() {
			return invoker::invoke<bool>(0x74bd83ea840f6bc9);
		}
		static bool is_first_person_aim_cam_active() {
			return invoker::invoke<bool>(0x5e346d934122613f);
		}
		static void disable_aim_cam_this_update() {
			invoker::invoke<void>(0x1a31fe0049e542f6);
		}
		static float _get_gameplay_cam_zoom() {
			return invoker::invoke<float>(0x7ec52cc40597d170);
		}
		static void _0x70894bd0915c5bca(float p0) {
			invoker::invoke<void>(0x70894bd0915c5bca, p0);
		}
		static void _0xced08cbe8ebb97c7(float p0, float p1) {
			invoker::invoke<void>(0xced08cbe8ebb97c7, p0, p1);
		}
		static void _0x2f7f2b26dd3f18ee(float p0, float p1) {
			invoker::invoke<void>(0x2f7f2b26dd3f18ee, p0, p1);
		}
		static void _set_first_person_cam_pitch_range(float minangle, float maxangle) {
			invoker::invoke<void>(0xbcfc632db7673bf0, minangle, maxangle);
		}
		static void _set_first_person_cam_near_clip(float distance) {
			invoker::invoke<void>(0x0af7b437918103b3, distance);
		}
		static void _set_third_person_aim_cam_near_clip(float distance) {
			invoker::invoke<void>(0x42156508606de65e, distance);
		}
		static void _0x4008edf7d6e48175(bool p0) {
			invoker::invoke<void>(0x4008edf7d6e48175, p0);
		}
		static PVector3 _get_gameplay_cam_coords() {
			return invoker::invoke<PVector3>(0xa200eb1ee790f448);
		}
		static PVector3 _get_gameplay_cam_rot(int rotationorder) {
			return invoker::invoke<PVector3>(0x5b4e4c817fcc2dfb, rotationorder);
		}
		static PVector3 _0x26903d9cd1175f2c(type::any p0, type::any p1) {
			return invoker::invoke<PVector3>(0x26903d9cd1175f2c, p0, p1);
		}
		static float _0x80ec114669daeff4() {
			return invoker::invoke<float>(0x80ec114669daeff4);
		}
		static float _0x5f35f6732c3fbba0(type::any p0) {
			return invoker::invoke<float>(0x5f35f6732c3fbba0, p0);
		}
		static float _0xd0082607100d7193() {
			return invoker::invoke<float>(0xd0082607100d7193);
		}
		static float _get_gameplay_cam_far_clip() {
			return invoker::invoke<float>(0xdfc8cbc606fdb0fc);
		}
		static float _get_gameplay_cam_near_dof() {
			return invoker::invoke<float>(0xa03502fc581f7d9b);
		}
		static float _get_gameplay_cam_far_dof() {
			return invoker::invoke<float>(0x9780f32bcaf72431);
		}
		static float _0x162f9d995753dc19() {
			return invoker::invoke<float>(0x162f9d995753dc19);
		}
		static void set_gameplay_coord_hint(float x, float y, float z, int duration, int blendoutduration, int blendinduration, int unk) {
			invoker::invoke<void>(0xd51adcd2d8bc0fb3, x, y, z, duration, blendoutduration, blendinduration, unk);
		}
		static void set_gameplay_ped_hint(type::ped p0, float x1, float y1, float z1, bool p4, type::any p5, type::any p6, type::any p7) {
			invoker::invoke<void>(0x2b486269acd548d3, p0, x1, y1, z1, p4, p5, p6, p7);
		}
		static void set_gameplay_vehicle_hint(type::vehicle p0, float xoffset, float yoffset, float zoffset, bool p4, int duration, int easeinduration, int easeoutduration) {
			invoker::invoke<void>(0xa2297e18f3e71c2e, p0, xoffset, yoffset, zoffset, p4, duration, easeinduration, easeoutduration);
		}
		static void set_gameplay_object_hint(type::any p0, float p1, float p2, float p3, bool p4, type::any p5, type::any p6, type::any p7) {
			invoker::invoke<void>(0x83e87508a2ca2ac6, p0, p1, p2, p3, p4, p5, p6, p7);
		}
		static void set_gameplay_entity_hint(type::entity entity, float xoffset, float yoffset, float zoffset, bool p4, int duration, int fadeintime, int fadeouttime, int flags) {
			invoker::invoke<void>(0x189e955a8313e298, entity, xoffset, yoffset, zoffset, p4, duration, fadeintime, fadeouttime, flags);
		}
		static bool is_gameplay_hint_active() {
			return invoker::invoke<bool>(0xe520ff1ad2785b40);
		}
		static void stop_gameplay_hint(bool p0) {
			invoker::invoke<void>(0xf46c581c61718916, p0);
		}
		static void _0xccd078c2665d2973(bool p0) {
			invoker::invoke<void>(0xccd078c2665d2973, p0);
		}
		static void _0x247acbc4abbc9d1c(bool p0) {
			invoker::invoke<void>(0x247acbc4abbc9d1c, p0);
		}
		static type::any _0xbf72910d0f26f025() {
			return invoker::invoke<type::any>(0xbf72910d0f26f025);
		}
		static void set_gameplay_hint_fov(float fov) {
			invoker::invoke<void>(0x513403fb9c56211f, fov);
		}
		static void _0xf8bdbf3d573049a1(float p0) {
			invoker::invoke<void>(0xf8bdbf3d573049a1, p0);
		}
		static void _0xd1f8363dfad03848(float p0) {
			invoker::invoke<void>(0xd1f8363dfad03848, p0);
		}
		static void _0x5d7b620dae436138(float p0) {
			invoker::invoke<void>(0x5d7b620dae436138, p0);
		}
		static void _0xc92717ef615b6704(float p0) {
			invoker::invoke<void>(0xc92717ef615b6704, p0);
		}
		static void get_is_multiplayer_brief(bool p0) {
			invoker::invoke<void>(0xe3433eadaaf7ee40, p0);
		}
		static void set_cinematic_button_active(bool p0) {
			invoker::invoke<void>(0x51669f7d1fb53d9f, p0);
		}
		static bool is_cinematic_cam_rendering() {
			return invoker::invoke<bool>(0xb15162cb5826e9e8);
		}
		static void shake_cinematic_cam(const char* p0, float p1) {
			invoker::invoke<void>(0xdce214d9ed58f3cf, p0, p1);
		}
		static bool is_cinematic_cam_shaking() {
			return invoker::invoke<bool>(0xbbc08f6b4cb8ff0a);
		}
		static void set_cinematic_cam_shake_amplitude(float p0) {
			invoker::invoke<void>(0xc724c701c30b2fe7, p0);
		}
		static void stop_cinematic_cam_shaking(bool p0) {
			invoker::invoke<void>(0x2238e588e588a6d7, p0);
		}
		static void _disable_vehicle_first_person_cam_this_frame() {
			invoker::invoke<void>(0xadff1b2a555f5fba);
		}
		static void _0x62ecfcfdee7885d6() {
			invoker::invoke<void>(0x62ecfcfdee7885d6);
		}
		static void _0x9e4cfff989258472() {
			invoker::invoke<void>(0x9e4cfff989258472);
		}
		static void _f4f2c0d4ee209e20() {
			invoker::invoke<void>(0xf4f2c0d4ee209e20);
		}
		static bool _0xca9d2aa3e326d720() {
			return invoker::invoke<bool>(0xca9d2aa3e326d720);
		}
		static bool _is_in_vehicle_cam_disabled() {
			return invoker::invoke<bool>(0x4f32c0d5a90a9b40);
		}
		static void create_cinematic_shot(type::any p0, int p1, type::any p2, type::entity entity) {
			invoker::invoke<void>(0x741b0129d4560f31, p0, p1, p2, entity);
		}
		static bool is_cinematic_shot_active(type::any p0) {
			return invoker::invoke<bool>(0xcc9f3371a7c28bc9, p0);
		}
		static void stop_cinematic_shot(type::any p0) {
			invoker::invoke<void>(0x7660c6e75d3a078e, p0);
		}
		static void _0xa41bcd7213805aac(bool p0) {
			invoker::invoke<void>(0xa41bcd7213805aac, p0);
		}
		static void _0xdc9da9e8789f5246() {
			invoker::invoke<void>(0xdc9da9e8789f5246);
		}
		static void set_cinematic_mode_active(bool p0) {
			invoker::invoke<void>(0xdcf0754ac3d6fd4e, p0);
		}
		static type::any _0x1f2300cb7fa7b7f6() {
			return invoker::invoke<type::any>(0x1f2300cb7fa7b7f6);
		}
		static type::any _0x17fca7199a530203() {
			return invoker::invoke<type::any>(0x17fca7199a530203);
		}
		static void stop_cutscene_cam_shaking() {
			invoker::invoke<void>(0xdb629ffd9285fa06);
		}
		static void _0x12ded8ca53d47ea5(float p0) {
			invoker::invoke<void>(0x12ded8ca53d47ea5, p0);
		}
		static type::entity _0x89215ec747df244a(float p0, int p1, float p2, float p3, float p4, float p5, float p6, int p7, int p8) {
			return invoker::invoke<type::entity>(0x89215ec747df244a, p0, p1, p2, p3, p4, p5, p6, p7, p8);
		}
		static void _0x5a43c76f7fc7ba5f() {
			invoker::invoke<void>(0x5a43c76f7fc7ba5f);
		}
		static void _set_cam_effect(int p0) {
			invoker::invoke<void>(0x80c8b1846639bb19, p0);
		}
		static void _0x5c41e6babc9e2112(type::any p0) {
			invoker::invoke<void>(0x5c41e6babc9e2112, p0);
		}
		static void _0x21e253a7f8da5dfb(const char* vehiclename) {
			invoker::invoke<void>(0x21e253a7f8da5dfb, vehiclename);
		}
		static void _0x11fa5d3479c7dd47(type::any p0) {
			invoker::invoke<void>(0x11fa5d3479c7dd47, p0);
		}
		static type::any _0xeaf0fa793d05c592() {
			return invoker::invoke<type::any>(0xeaf0fa793d05c592);
		}
		static float _get_replay_free_cam_max_range() {
			return invoker::invoke<float>(0x8bfceb5ea1b161b6);
		}
	}

	namespace weapon {
		static void enable_laser_sight_rendering(bool toggle) {
			invoker::invoke<void>(0xc8b46d7727d864aa, toggle);
		}
		static type::hash get_weapon_component_type_model(type::hash componenthash) {
			return invoker::invoke<type::hash>(0x0db57b41ec1db083, componenthash);
		}
		static type::hash get_weapontype_model(type::hash weaponhash) {
			return invoker::invoke<type::hash>(0xf46cdc33180fda94, weaponhash);
		}
		static type::hash get_weapontype_slot(type::hash weaponhash) {
			return invoker::invoke<type::hash>(0x4215460b9b8b7fa0, weaponhash);
		}
		static type::hash get_weapontype_group(type::hash weaponhash) {
			return invoker::invoke<type::hash>(0xc3287ee3050fb74c, weaponhash);
		}
		static void set_current_ped_weapon(type::ped ped, type::hash weaponhash, bool equipnow) {
			invoker::invoke<void>(0xadf692b254977c0c, ped, weaponhash, equipnow);
		}
		static bool get_current_ped_weapon(type::ped ped, type::hash* weaponhash, bool unused) {
			return invoker::invoke<bool>(0x3a87e44bb9a01d54, ped, weaponhash, unused);
		}
		static type::entity get_current_ped_weapon_entity_index(type::ped ped) {
			return invoker::invoke<type::entity>(0x3b390a939af0b5fc, ped);
		}
		static type::hash get_best_ped_weapon(type::ped ped, bool p1) {
			return invoker::invoke<type::hash>(0x8483e98e8b888ae2, ped, p1);
		}
		static bool set_current_ped_vehicle_weapon(type::ped ped, type::hash weaponhash) {
			return invoker::invoke<bool>(0x75c55983c2c39daa, ped, weaponhash);
		}
		static bool get_current_ped_vehicle_weapon(type::ped ped, type::hash* weaponhash) {
			return invoker::invoke<bool>(0x1017582bcd3832dc, ped, weaponhash);
		}
		static bool is_ped_armed(type::ped ped, int p1) {
			return invoker::invoke<bool>(0x475768a975d5ad17, ped, p1);
		}
		static bool is_weapon_valid(type::hash weaponhash) {
			return invoker::invoke<bool>(0x937c71165cf334b3, weaponhash);
		}
		static bool has_ped_got_weapon(type::ped ped, type::hash weaponhash, bool p2) {
			return invoker::invoke<bool>(0x8decb02f88f428bc, ped, weaponhash, p2);
		}
		static bool is_ped_weapon_ready_to_shoot(type::ped ped) {
			return invoker::invoke<bool>(0xb80ca294f2f26749, ped);
		}
		static type::hash get_ped_weapontype_in_slot(type::ped ped, type::hash weaponslot) {
			return invoker::invoke<type::hash>(0xeffed78e9011134d, ped, weaponslot);
		}
		static int get_ammo_in_ped_weapon(type::ped ped, type::hash weaponhash) {
			return invoker::invoke<int>(0x015a522136d7f951, ped, weaponhash);
		}
		static void add_ammo_to_ped(type::ped ped, type::hash weaponhash, int ammo) {
			invoker::invoke<void>(0x78f0424c34306220, ped, weaponhash, ammo);
		}
		static void set_ped_ammo(type::ped ped, type::hash weaponhash, int ammo) {
			invoker::invoke<void>(0x14e56bc5b5db6a19, ped, weaponhash, ammo);
		}
		static void set_ped_infinite_ammo(type::ped ped, bool toggle, type::hash weaponhash) {
			invoker::invoke<void>(0x3edcb0505123623b, ped, toggle, weaponhash);
		}
		static void set_ped_infinite_ammo_clip(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x183dadc6aa953186, ped, toggle);
		}
		static void give_weapon_to_ped(type::ped ped, type::hash weaponhash, int ammocount, bool ishidden, bool equipnow) {
			invoker::invoke<void>(0xbf0fd6e56c964fcb, ped, weaponhash, ammocount, ishidden, equipnow);
		}
		static void give_delayed_weapon_to_ped(type::ped ped, type::hash weaponhash, int ammocount, bool equipnow) {
			invoker::invoke<void>(0xb282dc6ebd803c75, ped, weaponhash, ammocount, equipnow);
		}
		static void remove_all_ped_weapons(type::ped ped, bool unused) {
			invoker::invoke<void>(0xf25df915fa38c5f3, ped, unused);
		}
		static void remove_weapon_from_ped(type::ped ped, type::hash weaponhash) {
			invoker::invoke<void>(0x4899cb088edf59b8, ped, weaponhash);
		}
		static void hide_ped_weapon_for_scripted_cutscene(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x6f6981d2253c208f, ped, toggle);
		}
		static void set_ped_current_weapon_visible(type::ped ped, bool visible, bool deselectweapon, bool p3, bool p4) {
			invoker::invoke<void>(0x0725a4ccfded9a70, ped, visible, deselectweapon, p3, p4);
		}
		static void set_ped_drops_weapons_when_dead(type::ped ped, bool toggle) {
			invoker::invoke<void>(0x476ae72c1d19d1a8, ped, toggle);
		}
		static bool has_ped_been_damaged_by_weapon(type::ped ped, type::hash weaponhash, int weapontype) {
			return invoker::invoke<bool>(0x2d343d2219cd027a, ped, weaponhash, weapontype);
		}
		static void clear_ped_last_weapon_damage(type::ped ped) {
			invoker::invoke<void>(0x0e98f88a24c5f4b8, ped);
		}
		static bool has_entity_been_damaged_by_weapon(type::entity entity, type::hash weaponhash, int weapontype) {
			return invoker::invoke<bool>(0x131d401334815e94, entity, weaponhash, weapontype);
		}
		static void clear_entity_last_weapon_damage(type::entity entity) {
			invoker::invoke<void>(0xac678e40be7c74d2, entity);
		}
		static void set_ped_drops_weapon(type::ped ped) {
			invoker::invoke<void>(0x6b7513d9966fbec0, ped);
		}
		static void set_ped_drops_inventory_weapon(type::ped ped, type::hash weaponhash, float xoffset, float yoffset, float zoffset, int ammocount) {
			invoker::invoke<void>(0x208a1888007fc0e6, ped, weaponhash, xoffset, yoffset, zoffset, ammocount);
		}
		static int get_max_ammo_in_clip(type::ped ped, type::hash weaponhash, bool p2) {
			return invoker::invoke<int>(0xa38dcffcea8962fa, ped, weaponhash, p2);
		}
		static bool get_ammo_in_clip(type::ped ped, type::hash weaponhash, int* ammo) {
			return invoker::invoke<bool>(0x2e1202248937775c, ped, weaponhash, ammo);
		}
		static bool set_ammo_in_clip(type::ped ped, type::hash weaponhash, int ammo) {
			return invoker::invoke<bool>(0xdcd2a934d65cb497, ped, weaponhash, ammo);
		}
		static bool get_max_ammo(type::ped ped, type::hash weaponhash, int* ammo) {
			return invoker::invoke<bool>(0xdc16122c7a20c933, ped, weaponhash, ammo);
		}
		static void set_ped_ammo_by_type(type::ped ped, type::hash ammotype, int ammo) {
			invoker::invoke<void>(0x5fd1e1f011e76d7e, ped, ammotype, ammo);
		}
		static int get_ped_ammo_by_type(type::ped ped, type::hash ammotype) {
			return invoker::invoke<int>(0x39d22031557946c1, ped, ammotype);
		}
		static void set_ped_ammo_to_drop(type::any ammotype, int ammo) {
			invoker::invoke<void>(0xa4efef9440a5b0ef, ammotype, ammo);
		}
		static void _0xe620fd3512a04f18(float p0) {
			invoker::invoke<void>(0xe620fd3512a04f18, p0);
		}
		static type::hash get_ped_ammo_type_from_weapon(type::ped ped, type::hash weaponhash) {
			return invoker::invoke<type::hash>(0x7fead38b326b9f74, ped, weaponhash);
		}
		static bool get_ped_last_weapon_impact_coord(type::ped ped, PVector3* coords) {
			return invoker::invoke<bool>(0x6c4d0409ba1a2bc2, ped, coords);
		}
		static void set_ped_gadget(type::ped ped, type::hash gadgethash, bool p2) {
			invoker::invoke<void>(0xd0d7b1e680ed4a1a, ped, gadgethash, p2);
		}
		static bool get_is_ped_gadget_equipped(type::ped ped, type::hash gadgethash) {
			return invoker::invoke<bool>(0xf731332072f5156c, ped, gadgethash);
		}
		static type::hash get_selected_ped_weapon(type::ped ped) {
			return invoker::invoke<type::hash>(0x0a6db4965674d243, ped);
		}
		static void explode_projectiles(type::ped ped, type::hash weaponhash, bool p2) {
			invoker::invoke<void>(0xfc4bd125de7611e4, ped, weaponhash, p2);
		}
		static void remove_all_projectiles_of_type(type::hash weaponhash, bool p1) {
			invoker::invoke<void>(0xfc52e0f37e446528, weaponhash, p1);
		}
		static float _get_lockon_range_of_current_ped_weapon(type::ped ped) {
			return invoker::invoke<float>(0x840f03e9041e2c9c, ped);
		}
		static float get_max_range_of_current_ped_weapon(type::ped ped) {
			return invoker::invoke<float>(0x814c9d19dfd69679, ped);
		}
		static bool has_vehicle_got_projectile_attached(type::ped driver, type::vehicle vehicle, type::hash weaponhash, type::any p3) {
			return invoker::invoke<bool>(0x717c8481234e3b88, driver, vehicle, weaponhash, p3);
		}
		static void give_weapon_component_to_ped(type::ped ped, type::hash weaponhash, type::hash componenthash) {
			invoker::invoke<void>(0xd966d51aa5b28bb9, ped, weaponhash, componenthash);
		}
		static void remove_weapon_component_from_ped(type::ped ped, type::hash weaponhash, type::hash componenthash) {
			invoker::invoke<void>(0x1e8be90c74fb4c09, ped, weaponhash, componenthash);
		}
		static bool has_ped_got_weapon_component(type::ped ped, type::hash weaponhash, type::hash componenthash) {
			return invoker::invoke<bool>(0xc593212475fae340, ped, weaponhash, componenthash);
		}
		static bool is_ped_weapon_component_active(type::ped ped, type::hash weaponhash, type::hash componenthash) {
			return invoker::invoke<bool>(0x0d78de0572d3969e, ped, weaponhash, componenthash);
		}
		static bool _ped_skip_next_reloading(type::ped ped) {
			return invoker::invoke<bool>(0x8c0d57ea686fad87, ped);
		}
		static bool make_ped_reload(type::ped ped) {
			return invoker::invoke<bool>(0x20ae33f3ac9c0033, ped);
		}
		static void request_weapon_asset(type::hash weaponhash, int p1, int p2) {
			invoker::invoke<void>(0x5443438f033e29c3, weaponhash, p1, p2);
		}
		static bool has_weapon_asset_loaded(type::hash weaponhash) {
			return invoker::invoke<bool>(0x36e353271f0e90ee, weaponhash);
		}
		static void remove_weapon_asset(type::hash weaponhash) {
			invoker::invoke<void>(0xaa08ef13f341c8fc, weaponhash);
		}
		static type::object create_weapon_object(type::hash weaponhash, int ammocount, float x, float y, float z, bool showworldmodel, float heading, type::any p7) {
			return invoker::invoke<type::object>(0x9541d3cf0d398f36, weaponhash, ammocount, x, y, z, showworldmodel, heading, p7);
		}
		static void give_weapon_component_to_weapon_object(type::object weaponobject, type::hash addonhash) {
			invoker::invoke<void>(0x33e179436c0b31db, weaponobject, addonhash);
		}
		static void remove_weapon_component_from_weapon_object(type::object weaponobject, type::hash component) {
			invoker::invoke<void>(0xf7d82b0d66777611, weaponobject, component);
		}
		static bool has_weapon_got_weapon_component(type::object weapon, type::hash addonhash) {
			return invoker::invoke<bool>(0x76a18844e743bf91, weapon, addonhash);
		}
		static void give_weapon_object_to_ped(type::object weaponobject, type::ped ped) {
			invoker::invoke<void>(0xb1fa61371af7c4b7, weaponobject, ped);
		}
		static bool does_weapon_take_weapon_component(type::hash weaponhash, type::hash componenthash) {
			return invoker::invoke<bool>(0x5cee3df569cecab0, weaponhash, componenthash);
		}
		static type::object get_weapon_object_from_ped(type::ped ped, bool p1) {
			return invoker::invoke<type::object>(0xcae1dc9a0e22a16d, ped, p1);
		}
		static void set_ped_weapon_tint_index(type::ped ped, type::hash weaponhash, int tintindex) {
			invoker::invoke<void>(0x50969b9b89ed5738, ped, weaponhash, tintindex);
		}
		static int get_ped_weapon_tint_index(type::ped ped, type::hash weaponhash) {
			return invoker::invoke<int>(0x2b9eedc07bd06b9f, ped, weaponhash);
		}
		static void set_weapon_object_tint_index(type::object weapon, int tintindex) {
			invoker::invoke<void>(0xf827589017d4e4a9, weapon, tintindex);
		}
		static int get_weapon_object_tint_index(type::object weapon) {
			return invoker::invoke<int>(0xcd183314f7cd2e57, weapon);
		}
		static int get_weapon_tint_count(type::hash weaponhash) {
			return invoker::invoke<int>(0x5dcf6c5cab2e9bf7, weaponhash);
		}
		static bool get_weapon_hud_stats(type::hash weaponhash, type::any* outdata) {
			return invoker::invoke<bool>(0xd92c739ee34c9eba, weaponhash, outdata);
		}
		static bool get_weapon_component_hud_stats(type::hash componenthash, int* outdata) {
			return invoker::invoke<bool>(0xb3caf387ae12e9f8, componenthash, outdata);
		}
		static float _0x3133b907d8b32053(type::hash weapon, int p1) {
			return invoker::invoke<float>(0x3133b907d8b32053, weapon, p1);
		}
		static int get_weapon_clip_size(type::hash weaponhash) {
			return invoker::invoke<int>(0x583be370b1ec6eb4, weaponhash);
		}
		static void set_ped_chance_of_firing_blanks(type::ped ped, float xbias, float ybias) {
			invoker::invoke<void>(0x8378627201d5497d, ped, xbias, ybias);
		}
		static type::entity _0xb4c8d77c80c0421e(type::ped ped, bool p1) {
			return invoker::invoke<type::entity>(0xb4c8d77c80c0421e, ped, p1);
		}
		static void request_weapon_high_detail_model(type::entity weaponobject) {
			invoker::invoke<void>(0x48164dbb970ac3f0, weaponobject);
		}
		static bool is_ped_current_weapon_silenced(type::ped ped) {
			return invoker::invoke<bool>(0x65f0c5ae05943ec7, ped);
		}
		static bool set_weapon_smokegrenade_assigned(type::ped ped) {
			return invoker::invoke<bool>(0x4b7620c47217126c, ped);
		}
		static type::any set_flash_light_fade_distance(float distance) {
			return invoker::invoke<type::any>(0xcea66dad478cd39b, distance);
		}
		static void set_weapon_animation_override(type::ped ped, type::hash animstyle) {
			invoker::invoke<void>(0x1055ac3a667f09d9, ped, animstyle);
		}
		static int get_weapon_damage_type(type::hash weaponhash) {
			return invoker::invoke<int>(0x3be0bb12d25fb305, weaponhash);
		}
		static void _0xe4dcec7fd5b739a5(type::ped ped) {
			invoker::invoke<void>(0xe4dcec7fd5b739a5, ped);
		}
		static bool can_use_weapon_on_parachute(type::hash weaponhash) {
			return invoker::invoke<bool>(0xbc7be5abc0879f74, weaponhash);
		}
	}

	namespace itemset {
		static type::vehicle create_itemset(type::vehicle distri) {
			return invoker::invoke<type::vehicle>(0x35ad299f50d91b24, distri);
		}
		static void destroy_itemset(type::any p0) {
			invoker::invoke<void>(0xde18220b1c183eda, p0);
		}
		static bool is_itemset_valid(type::any p0) {
			return invoker::invoke<bool>(0xb1b1ea596344dfab, p0);
		}
		static bool add_to_itemset(type::any p0, type::any p1) {
			return invoker::invoke<bool>(0xe3945201f14637dd, p0, p1);
		}
		static void remove_from_itemset(type::any p0, type::any p1) {
			invoker::invoke<void>(0x25e68244b0177686, p0, p1);
		}
		static type::any get_itemset_size(type::scrhandle x) {
			return invoker::invoke<type::any>(0xd9127e83abf7c631, x);
		}
		static type::any get_indexed_item_in_itemset(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x7a197e2521ee2bab, p0, p1);
		}
		static bool is_in_itemset(type::any p0, type::any p1) {
			return invoker::invoke<bool>(0x2d0fc594d1e9c107, p0, p1);
		}
		static void clean_itemset(type::any p0) {
			invoker::invoke<void>(0x41bc0d722fc04221, p0);
		}
	}

	namespace streaming {
		static void load_all_objects_now() {
			invoker::invoke<void>(0xbd6e84632dd4cb3f);
		}
		static void load_scene(float x, float y, float z) {
			invoker::invoke<void>(0x4448eb75b4904bdb, x, y, z);
		}
		static PVector3* network_update_load_scene() {
			return invoker::invoke<PVector3*>(0xc4582015556d1c46);
		}
		static void network_stop_load_scene() {
			invoker::invoke<void>(0x64e630faf5f60f44);
		}
		static bool is_network_loading_scene() {
			return invoker::invoke<bool>(0x41ca5a33160ea4ab);
		}
		static void set_interior_active(int interiorid, bool toggle) {
			invoker::invoke<void>(0xe37b76c387be28ed, interiorid, toggle);
		}
		static void request_model(type::hash model) {
			invoker::invoke<void>(0x963d27a58df860ac, model);
		}
		static void request_menu_ped_model(type::player model) {
			invoker::invoke<void>(0xa0261aef7acfc51e, model);
		}
		static bool has_model_loaded(type::hash model) {
			return invoker::invoke<bool>(0x98a4eb5d89a0c952, model);
		}
		static void _request_interior_room_by_name(int interiorid, const char* roomname) {
			invoker::invoke<void>(0x8a7a40100edfec58, interiorid, roomname);
		}
		static void set_model_as_no_longer_needed(type::hash model) {
			invoker::invoke<void>(0xe532f5d78798daab, model);
		}
		static bool is_model_in_cdimage(type::hash model) {
			return invoker::invoke<bool>(0x35b9e0803292b641, model);
		}
		static bool is_model_valid(type::hash model) {
			return invoker::invoke<bool>(0xc0296a2edf545e92, model);
		}
		static bool is_model_a_vehicle(type::hash model) {
			return invoker::invoke<bool>(0x19aac8f07bfec53e, model);
		}
		static void request_collision_at_coord(float x, float y, float z) {
			invoker::invoke<void>(0x07503f7948f491a7, x, y, z);
		}
		static void request_collision_at_coord_v3(PVector3 coords) {
			invoker::invoke<void>(0x07503f7948f491a7, coords);
		}
		static void request_collision_for_model(type::hash model) {
			invoker::invoke<void>(0x923cb32a3b874fcb, model);
		}
		static bool has_collision_for_model_loaded(type::hash model) {
			return invoker::invoke<bool>(0x22cca434e368f03a, model);
		}
		static void request_additional_collision_at_coord(float x, float y, float z) {
			invoker::invoke<void>(0xc9156dc11411a9ea, x, y, z);
		}
		static bool does_anim_dict_exist(const char* animdict) {
			return invoker::invoke<bool>(0x2da49c3b79856961, animdict);
		}
		static void request_anim_dict(const char* animdict) {
			invoker::invoke<void>(0xd3bd40951412fef6, animdict);
		}
		static bool has_anim_dict_loaded(const char* animdict) {
			return invoker::invoke<bool>(0xd031a9162d01088c, animdict);
		}
		static void remove_anim_dict(const char* animdict) {
			invoker::invoke<void>(0xf66a602f829e2a06, animdict);
		}
		static void request_anim_set(const char* animset) {
			invoker::invoke<void>(0x6ea47dae7fad0eed, animset);
		}
		static bool has_anim_set_loaded(const char* animset) {
			return invoker::invoke<bool>(0xc4ea073d86fb29b0, animset);
		}
		static void remove_anim_set(const char* animset) {
			invoker::invoke<void>(0x16350528f93024b3, animset);
		}
		static void request_clip_set(const char* clipset) {
			invoker::invoke<void>(0xd2a71e1a77418a49, clipset);
		}
		static bool has_clip_set_loaded(const char* clipset) {
			return invoker::invoke<bool>(0x318234f4f3738af3, clipset);
		}
		static void remove_clip_set(const char* clipset) {
			invoker::invoke<void>(0x01f73a131c18cd94, clipset);
		}
		static void request_ipl(const char* iplname) {
			invoker::invoke<void>(0x41b4893843bbdb74, iplname);
		}
		static void remove_ipl(const char* iplname) {
			invoker::invoke<void>(0xee6c5ad3ece0a82d, iplname);
		}
		static bool is_ipl_active(const char* iplname) {
			return invoker::invoke<bool>(0x88a741e44a2b3495, iplname);
		}
		static void set_streaming(bool toggle) {
			invoker::invoke<void>(0x6e0c692677008888, toggle);
		}
		static void set_game_pauses_for_streaming(bool toggle) {
			invoker::invoke<void>(0x717cd6e6faebbedc, toggle);
		}
		static void set_reduce_ped_model_budget(bool toggle) {
			invoker::invoke<void>(0x77b5f9a36bf96710, toggle);
		}
		static void set_reduce_vehicle_model_budget(bool toggle) {
			invoker::invoke<void>(0x80c527893080ccf3, toggle);
		}
		static void set_ditch_police_models(bool toggle) {
			invoker::invoke<void>(0x42cbe54462d92634, toggle);
		}
		static int get_number_of_streaming_requests() {
			return invoker::invoke<int>(0x4060057271cebc89);
		}
		static void request_ptfx_asset() {
			invoker::invoke<void>(0x944955fb2a3935c8);
		}
		static bool has_ptfx_asset_loaded() {
			return invoker::invoke<bool>(0xca7d9b86eca7481b);
		}
		static void remove_ptfx_asset() {
			invoker::invoke<void>(0x88c6814073dd4a73);
		}
		static void request_named_ptfx_asset(const char* assetname) {
			invoker::invoke<void>(0xb80d8756b4668ab6, assetname);
		}
		static bool has_named_ptfx_asset_loaded(const char* assetname) {
			return invoker::invoke<bool>(0x8702416e512ec454, assetname);
		}
		static void _remove_named_ptfx_asset(const char* assetname) {
			invoker::invoke<void>(0x5f61ebbe1a00f96d, assetname);
		}
		static void set_vehicle_population_budget(int budget) {
			invoker::invoke<void>(0xcb9e1eb3be2af4e9, budget);
		}
		static void set_ped_population_budget(int budget) {
			invoker::invoke<void>(0x8c95333cfc3340f3, budget);
		}
		static void clear_focus() {
			invoker::invoke<void>(0x31b73d1ea9f01da2);
		}
		static void set_focus_pos_and_vel(float x, float y, float z, float offsetx, float offsety, float offsetz) {
			invoker::invoke<void>(0xbb7454baff08fe25, x, y, z, offsetx, offsety, offsetz);
		}
		static void set_focus_entity(type::entity entity) {
			invoker::invoke<void>(0x198f77705fa0931d, entity);
		}
		static bool is_entity_focus(type::entity entity) {
			return invoker::invoke<bool>(0x2ddff3fb9075d747, entity);
		}
		static void _0x0811381ef5062fec(type::entity p0) {
			invoker::invoke<void>(0x0811381ef5062fec, p0);
		}
		static void _0xaf12610c644a35c9(const char* p0, bool p1) {
			invoker::invoke<void>(0xaf12610c644a35c9, p0, p1);
		}
		static void _0x4e52e752c76e7e7a(type::any p0) {
			invoker::invoke<void>(0x4e52e752c76e7e7a, p0);
		}
		static type::any format_focus_heading(float x, float y, float z, float rad, type::any p4, type::any p5) {
			return invoker::invoke<type::any>(0x219c7b8d53e429fd, x, y, z, rad, p4, p5);
		}
		static type::any _0x1f3f018bc3afa77c(float p0, float p1, float p2, float p3, float p4, float p5, float p6, type::any p7, type::any p8) {
			return invoker::invoke<type::any>(0x1f3f018bc3afa77c, p0, p1, p2, p3, p4, p5, p6, p7, p8);
		}
		static type::any _0x0ad9710cee2f590f(float p0, float p1, float p2, float p3, float p4, float p5, type::any p6) {
			return invoker::invoke<type::any>(0x0ad9710cee2f590f, p0, p1, p2, p3, p4, p5, p6);
		}
		static void _0x1ee7d8df4425f053(type::any p0) {
			invoker::invoke<void>(0x1ee7d8df4425f053, p0);
		}
		static type::any _0x7d41e9d2d17c5b2d(type::any p0) {
			return invoker::invoke<type::any>(0x7d41e9d2d17c5b2d, p0);
		}
		static type::any _0x07c313f94746702c(type::any p0) {
			return invoker::invoke<type::any>(0x07c313f94746702c, p0);
		}
		static type::any _0xbc9823ab80a3dcac() {
			return invoker::invoke<type::any>(0xbc9823ab80a3dcac);
		}
		static bool new_load_scene_start(float p0, float p1, float p2, float p3, float p4, float p5, float p6, type::any p7) {
			return invoker::invoke<bool>(0x212a8d0d2babfac2, p0, p1, p2, p3, p4, p5, p6, p7);
		}
		static bool new_load_scene_start_sphere(float x, float y, float z, float radius, type::any p4) {
			return invoker::invoke<bool>(0xaccfb4acf53551b0, x, y, z, radius, p4);
		}
		static void new_load_scene_stop() {
			invoker::invoke<void>(0xc197616d221ff4a4);
		}
		static bool is_new_load_scene_active() {
			return invoker::invoke<bool>(0xa41a05b6cb741b85);
		}
		static bool is_new_load_scene_loaded() {
			return invoker::invoke<bool>(0x01b8247a7a8b9ad1);
		}
		static bool _0x71e7b2e657449aad() {
			return invoker::invoke<bool>(0x71e7b2e657449aad);
		}
		static void start_player_switch(type::ped from, type::ped to, int flags, int switchtype) {
			invoker::invoke<void>(0xfaa23f2cba159d67, from, to, flags, switchtype);
		}
		static void stop_player_switch() {
			invoker::invoke<void>(0x95c0a5bbdc189aa1);
		}
		static bool is_player_switch_in_progress() {
			return invoker::invoke<bool>(0xd9d2cfff49fab35f);
		}
		static int get_player_switch_type() {
			return invoker::invoke<int>(0xb3c94a90d9fc9e62);
		}
		static int get_ideal_player_switch_type(float x1, float y1, float z1, float x2, float y2, float z2) {
			return invoker::invoke<int>(0xb5d7b26b45720e05, x1, y1, z1, x2, y2, z2);
		}
		static int get_player_switch_state() {
			return invoker::invoke<int>(0x470555300d10b2a5);
		}
		static int get_player_short_switch_state() {
			return invoker::invoke<int>(0x20f898a5d9782800);
		}
		static void _0x5f2013f8bc24ee69(type::any* p0) {
			invoker::invoke<void>(0x5f2013f8bc24ee69, p0);
		}
		static int _0x78c0d93253149435() {
			return invoker::invoke<int>(0x78c0d93253149435);
		}
		static void set_player_switch_outro(float camcoordx, float camcoordy, float camcoordz, float camrotx, float camroty, float camrotz, float camfov, float camfarclip, int p8) {
			invoker::invoke<void>(0xc208b673ce446b61, camcoordx, camcoordy, camcoordz, camrotx, camroty, camrotz, camfov, camfarclip, p8);
		}
		static void _0x0fde9dbfc0a6bc65(const char* p0) {
			invoker::invoke<void>(0x0fde9dbfc0a6bc65, p0);
		}
		static void _0x43d1680c6d19a8e9() {
			invoker::invoke<void>(0x43d1680c6d19a8e9);
		}
		static void _0x74de2e8739086740() {
			invoker::invoke<void>(0x74de2e8739086740);
		}
		static void _0x8e2a065abdae6994() {
			invoker::invoke<void>(0x8e2a065abdae6994);
		}
		static void _0xad5fdf34b81bfe79() {
			invoker::invoke<void>(0xad5fdf34b81bfe79);
		}
		static type::any _0xdfa80cb25d0a19b3() {
			return invoker::invoke<type::any>(0xdfa80cb25d0a19b3);
		}
		static void _0xd4793dff3af2abcd() {
			invoker::invoke<void>(0xd4793dff3af2abcd);
		}
		static void _0xbd605b8e0e18b3bb() {
			invoker::invoke<void>(0xbd605b8e0e18b3bb);
		}
		static void _switch_out_player(type::ped ped, int flags, int switchtype) {
			invoker::invoke<void>(0xaab3200ed59016bc, ped, flags, switchtype);
		}
		static void _switch_in_player(type::ped ped) {
			invoker::invoke<void>(0xd8295af639fd9cb8, ped);
		}
		static bool _0x933bbeeb8c61b5f4() {
			return invoker::invoke<bool>(0x933bbeeb8c61b5f4);
		}
		static int set_player_inverted_up() {
			return invoker::invoke<int>(0x08c2d6c52a3104bb);
		}
		static type::any _0x5b48a06dd0e792a5() {
			return invoker::invoke<type::any>(0x5b48a06dd0e792a5);
		}
		static type::any destroy_player_in_pause_menu() {
			return invoker::invoke<type::any>(0x5b74ea8cfd5e3e7e);
		}
		static void _0x1e9057a74fd73e23() {
			invoker::invoke<void>(0x1e9057a74fd73e23);
		}
		static type::any _0x0c15b0e443b2349d() {
			return invoker::invoke<type::any>(0x0c15b0e443b2349d);
		}
		static void _0xa76359fc80b2438e(float p0) {
			invoker::invoke<void>(0xa76359fc80b2438e, p0);
		}
		static void _0xbed8ca5ff5e04113(float p0, float p1, float p2, float p3) {
			invoker::invoke<void>(0xbed8ca5ff5e04113, p0, p1, p2, p3);
		}
		static void _0x472397322e92a856() {
			invoker::invoke<void>(0x472397322e92a856);
		}
		static void _0x40aefd1a244741f2(bool p0) {
			invoker::invoke<void>(0x40aefd1a244741f2, p0);
		}
		static void _0x03f1a106bda7dd3e() {
			invoker::invoke<void>(0x03f1a106bda7dd3e);
		}
		static void _0x95a7dabddbb78ae7(type::any* p0, type::any* p1) {
			invoker::invoke<void>(0x95a7dabddbb78ae7, p0, p1);
		}
		static void _0x63eb2b972a218cac() {
			invoker::invoke<void>(0x63eb2b972a218cac);
		}
		static type::any _0xfb199266061f820a() {
			return invoker::invoke<type::any>(0xfb199266061f820a);
		}
		static void _0xf4a0dadb70f57fa6() {
			invoker::invoke<void>(0xf4a0dadb70f57fa6);
		}
		static type::any _0x5068f488ddb54dd8() {
			return invoker::invoke<type::any>(0x5068f488ddb54dd8);
		}
		static void prefetch_srl(const char* srl) {
			invoker::invoke<void>(0x3d245789ce12982c, srl);
		}
		static bool is_srl_loaded() {
			return invoker::invoke<bool>(0xd0263801a4c5b0bb);
		}
		static void begin_srl() {
			invoker::invoke<void>(0x9baddc94ef83b823);
		}
		static void end_srl() {
			invoker::invoke<void>(0x0a41540e63c9ee17);
		}
		static void set_srl_time(float p0) {
			invoker::invoke<void>(0xa74a541c6884e7b8, p0);
		}
		static void _0xef39ee20c537e98c(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4, type::any p5) {
			invoker::invoke<void>(0xef39ee20c537e98c, p0, p1, p2, p3, p4, p5);
		}
		static void _0xbeb2d9a1d9a8f55a(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0xbeb2d9a1d9a8f55a, p0, p1, p2, p3);
		}
		static void _0x20c6c7e4eb082a7f(bool p0) {
			invoker::invoke<void>(0x20c6c7e4eb082a7f, p0);
		}
		static void _0xf8155a7f03ddfc8e(type::any p0) {
			invoker::invoke<void>(0xf8155a7f03ddfc8e, p0);
		}
		static void set_hd_area(float x, float y, float z, float radius) {
			invoker::invoke<void>(0xb85f26619073e775, x, y, z, radius);
		}
		static void clear_hd_area() {
			invoker::invoke<void>(0xce58b1cfb9290813);
		}
		static void _load_mission_creator_data() {
			invoker::invoke<void>(0xb5a4db34fe89b88a);
		}
		static void shutdown_creator_budget() {
			invoker::invoke<void>(0xcce26000e9a6fad7);
		}
		static bool _0x0bc3144deb678666(type::hash modelhash) {
			return invoker::invoke<bool>(0x0bc3144deb678666, modelhash);
		}
		static void _0xf086ad9354fac3a3(type::any p0) {
			invoker::invoke<void>(0xf086ad9354fac3a3, p0);
		}
		static type::any _0x3d3d8b3be5a83d35() {
			return invoker::invoke<type::any>(0x3d3d8b3be5a83d35);
		}
	}

	namespace script {
		static void request_script(const char* scriptname) {
			invoker::invoke<void>(0x6eb5f71aa68f2e8e, scriptname);
		}
		static void set_script_as_no_longer_needed(const char* scriptname) {
			invoker::invoke<void>(0xc90d2dcacd56184c, scriptname);
		}
		static bool has_script_loaded(const char* scriptname) {
			return invoker::invoke<bool>(0xe6cc9f3ba0fb9ef1, scriptname);
		}
		static bool does_script_exist(const char* scriptname) {
			return invoker::invoke<bool>(0xfc04745fbe67c19a, scriptname);
		}
		static void request_script_with_name_hash(type::hash scripthash) {
			invoker::invoke<void>(0xd62a67d26d9653e6, scripthash);
		}
		static void set_script_with_name_hash_as_no_longer_needed(type::hash scripthash) {
			invoker::invoke<void>(0xc5bc038960e9db27, scripthash);
		}
		static bool has_script_with_name_hash_loaded(type::hash scripthash) {
			return invoker::invoke<bool>(0x5f0f0c783eb16c04, scripthash);
		}
		static bool _does_script_with_name_hash_exist(type::hash scripthash) {
			return invoker::invoke<bool>(0xf86aa3c56ba31381, scripthash);
		}
		static void terminate_thread(int threadid) {
			invoker::invoke<void>(0xc8b189ed9138bcd4, threadid);
		}
		static bool is_thread_active(int threadid) {
			return invoker::invoke<bool>(0x46e9ae36d8fa6417, threadid);
		}
		static const char* _get_name_of_thread(int threadid) {
			return invoker::invoke<const char*>(0x05a42ba9fc8da96b, threadid);
		}
		static void _begin_enumerating_threads() {
			invoker::invoke<void>(0xdadfada5a20143a8);
		}
		static int _get_id_of_next_thread_in_enumeration() {
			return invoker::invoke<int>(0x30b4fa1c82dd4b9f);
		}
		static int get_id_of_this_thread() {
			return invoker::invoke<int>(0xc30338e8088e2e21);
		}
		static void terminate_this_thread() {
			invoker::invoke<void>(0x1090044ad1da76fa);
		}
		static int _get_number_of_instances_of_script_with_name_hash(type::hash scripthash) {
			return invoker::invoke<int>(0x2c83a9da6bffc4f9, scripthash);
		}
		static const char* get_this_script_name() {
			return invoker::invoke<const char*>(0x442e0a7ede4a738a);
		}
		static type::hash get_hash_of_this_script_name() {
			return invoker::invoke<type::hash>(0x8a1c8b1738ffe87e);
		}
		static int get_number_of_events(int eventgroup) {
			return invoker::invoke<int>(0x5f92a689a06620aa, eventgroup);
		}
		static bool get_event_exists(int eventgroup, int eventindex) {
			return invoker::invoke<bool>(0x936e6168a9bcedb5, eventgroup, eventindex);
		}
		static int get_event_at_index(int eventgroup, int eventindex) {
			return invoker::invoke<int>(0xd8f66a3a60c62153, eventgroup, eventindex);
		}
		static bool get_event_data(int eventgroup, int eventindex, int* argstruct, int argstructsize) {
			return invoker::invoke<bool>(0x2902843fcd2b2d79, eventgroup, eventindex, argstruct, argstructsize);
		}
		static void trigger_script_event(int eventgroup, type::any* args, int argcount, type::any bit) {
			invoker::invoke<void>(0x5ae99c571d5bbe5d, eventgroup, args, argcount, bit);
		}
		static void shutdown_loading_screen() {
			invoker::invoke<void>(0x078ebe9809ccd637);
		}
		static void set_no_loading_screen(bool toggle) {
			invoker::invoke<void>(0x5262cc1995d07e09, toggle);
		}
		static bool _get_no_loading_screen() {
			return invoker::invoke<bool>(0x18c1270ea7f199bc);
		}
		static void _0xb1577667c3708f9b() {
			invoker::invoke<void>(0xb1577667c3708f9b);
		}
	}

	namespace ui {
		static void begin_text_command_busyspinner_on(const char* string) {
			invoker::invoke<void>(0xaba17d7ce615adbf, string);
		}
		static void _end_text_command_busy_string(int busyspinnertype) {
			invoker::invoke<void>(0xbd12f8228410d9b4, busyspinnertype);
		}
		static void _remove_loading_prompt() {
			invoker::invoke<void>(0x10d373323e5b9c0d);
		}
		static void _0xc65ab383cd91df98() {
			invoker::invoke<void>(0xc65ab383cd91df98);
		}
		static bool _is_loading_prompt_being_displayed() {
			return invoker::invoke<bool>(0xd422fcc5f239a915);
		}
		static type::pickup _0xb2a592b04648a9cb() {
			return invoker::invoke<type::pickup>(0xb2a592b04648a9cb);
		}
		static void _0x9245e81072704b8a(bool p0) {
			invoker::invoke<void>(0x9245e81072704b8a, p0);
		}
		static void _show_cursor_this_frame() {
			invoker::invoke<void>(0xaae7ce1d63167423);
		}
		static void _set_cursor_sprite(int spriteid) {
			invoker::invoke<void>(0x8db8cffd58b62552, spriteid);
		}
		static void _0x98215325a695e78a(bool p0) {
			invoker::invoke<void>(0x98215325a695e78a, p0);
		}
		static bool _0x3d9acb1eb139e702() {
			return invoker::invoke<bool>(0x3d9acb1eb139e702);
		}
		static bool _0x632b2940c67f4ea9(int scaleformhandle, type::any* p1, type::any* p2, type::any* p3) {
			return invoker::invoke<bool>(0x632b2940c67f4ea9, scaleformhandle, p1, p2, p3);
		}
		static void _0x6f1554b0cc2089fa(bool p0) {
			invoker::invoke<void>(0x6f1554b0cc2089fa, p0);
		}
		static void _clear_notifications_pos(float pos) {
			invoker::invoke<void>(0x55598d21339cb998, pos);
		}
		static void _0x25f87b30c382fca7() {
			invoker::invoke<void>(0x25f87b30c382fca7);
		}
		static void _0xa8fdb297a8d25fba() {
			invoker::invoke<void>(0xa8fdb297a8d25fba);
		}
		static void _remove_notification(int notificationid) {
			invoker::invoke<void>(0xbe4390cb40b3e627, notificationid);
		}
		static void _0xa13c11e1b5c06bfc() {
			invoker::invoke<void>(0xa13c11e1b5c06bfc);
		}
		static void _0x583049884a2eee3c() {
			invoker::invoke<void>(0x583049884a2eee3c);
		}
		static void thefeed_pause() {
			invoker::invoke<void>(0xfdb423997fa30340);
		}
		static void thefeed_resume() {
			invoker::invoke<void>(0xe1cd1e48e025e661);
		}
		static bool thefeed_is_paused() {
			return invoker::invoke<bool>(0xa9cbfd40b3fa3010);
		}
		static void _0xd4438c0564490e63() {
			invoker::invoke<void>(0xd4438c0564490e63);
		}
		static void _0xb695e2cd0a2da9ee() {
			invoker::invoke<void>(0xb695e2cd0a2da9ee);
		}
		static int _get_current_notification() {
			return invoker::invoke<int>(0x82352748437638ca);
		}
		static void _0x56c8b608cfd49854() {
			invoker::invoke<void>(0x56c8b608cfd49854);
		}
		static void _0xaded7f5748acafe6() {
			invoker::invoke<void>(0xaded7f5748acafe6);
		}
		static void _set_notification_background_color(int hudindex) {
			invoker::invoke<void>(0x92f0da1e27db96dc, hudindex);
		}
		static void _set_notification_flash_color(int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0x17430b918701c342, red, green, blue, alpha);
		}
		static void _0x17ad8c9706bdd88a(type::any p0) {
			invoker::invoke<void>(0x17ad8c9706bdd88a, p0);
		}
		static void _0x4a0c7c9bb10abb36(bool p0) {
			invoker::invoke<void>(0x4a0c7c9bb10abb36, p0);
		}
		static void _0xfdd85225b2dea55e() {
			invoker::invoke<void>(0xfdd85225b2dea55e);
		}
		static void _0xfdec055ab549e328() {
			invoker::invoke<void>(0xfdec055ab549e328);
		}
		static void _0x80fe4f3ab4e1b62a() {
			invoker::invoke<void>(0x80fe4f3ab4e1b62a);
		}
		static void _0xbae4f9b97cd43b30(bool p0) {
			invoker::invoke<void>(0xbae4f9b97cd43b30, p0);
		}
		static void _0x317eba71d7543f52(type::any* p0, type::any* p1, type::any* p2, type::any* p3) {
			invoker::invoke<void>(0x317eba71d7543f52, p0, p1, p2, p3);
		}
		static void _set_notification_text_entry(const char* text) {
			invoker::invoke<void>(0x202709f4c58a0424, text);
		}
		static int _set_notification_message(const char* picname1, const char* picname2, bool flash, int icontype, bool _nonexistent, const char* sender, const char* subject) {
			return invoker::invoke<int>(0x2b7e9a4eaaa93c89, picname1, picname2, flash, icontype, _nonexistent, sender, subject);
		}
		static int _set_notification_message_2(const char* picname1, const char* picname2, bool flash, int icontype, const char* sender, const char* subject) {
			return invoker::invoke<int>(0x1ccd9a37359072cf, picname1, picname2, flash, icontype, sender, subject);
		}
		static int _set_notification_message_3(const char* picname1, const char* picname2, bool p2, type::any p3, const char* p4, const char* p5) {
			return invoker::invoke<int>(0xc6f580e4c94926ac, picname1, picname2, p2, p3, p4, p5);
		}
		static int _set_notification_message_4(const char* picname1, const char* picname2, bool flash, int icontype, const char* sender, const char* subject, float duration) {
			return invoker::invoke<int>(0x1e6611149db3db6b, picname1, picname2, flash, icontype, sender, subject, duration);
		}
		static int _set_notification_message_clan_tag(const char* picname1, const char* picname2, bool flash, int icontype, const char* sender, const char* subject, float duration, const char* clantag) {
			return invoker::invoke<int>(0x5cbf7bade20db93e, picname1, picname2, flash, icontype, sender, subject, duration, clantag);
		}
		static int _set_notification_message_clan_tag_2(const char* picname1, const char* picname2, bool flash, int icontype1, const char* sender, const char* subject, float duration, const char* clantag, int icontype2, int p9) {
			return invoker::invoke<int>(0x531b84e7da981fb6, picname1, picname2, flash, icontype1, sender, subject, duration, clantag, icontype2, p9);
		}
		static int end_text_command_thefeed_post_ticker(bool blink, bool showinbrief) {
			return invoker::invoke<int>(0x2ed7843f8f801023, blink, showinbrief);
		}
		static int end_text_command_thefeed_post_ticker_forced(bool blink, bool p1) {
			return invoker::invoke<int>(0x44fa03975424a0ee, blink, p1);
		}
		static int _draw_notification_3(bool blink, bool p1) {
			return invoker::invoke<int>(0x378e809bf61ec840, blink, p1);
		}
		static int _draw_notification_award(const char* p0, const char* p1, int p2, int p3, const char* p4) {
			return invoker::invoke<int>(0xaa295b6f28bd587d, p0, p1, p2, p3, p4);
		}
		static int _draw_notification_apartment_invite(bool p0, bool p1, int* p2, int p3, bool isleader, bool unk0, int clandesc, int r, int g, int b) {
			return invoker::invoke<int>(0x97c9e4e7024a8f2c, p0, p1, p2, p3, isleader, unk0, clandesc, r, g, b);
		}
		static int end_text_command_thefeed_post_crewtag_with_game_name(bool p0, bool p1, int* p2, int p3, bool isleader, bool unk0, int clandesc, const char* playername, int r, int g, int b) {
			return invoker::invoke<int>(0x137bc35589e34e1e, p0, p1, p2, p3, isleader, unk0, clandesc, playername, r, g, b);
		}
		static int _draw_notification_unlock(const char* unlocklabel, int icontype, const char* unk2) {
			return invoker::invoke<int>(0x33ee12743ccd6343, unlocklabel, icontype, unk2);
		}
		static int _draw_notification_unlock_2(const char* unlocklabel, int icontype, const char* unk2, int fadeintime) {
			return invoker::invoke<int>(0xc8f3aaf93d0600bf, unlocklabel, icontype, unk2, fadeintime);
		}
		static int end_text_command_thefeed_post_unlock_tu_with_color(const char* primarytext, int icontype, const char* unk2, int fadeintime, int primarystyle, bool ignoreloc) {
			return invoker::invoke<int>(0x7ae0589093a2e088, primarytext, icontype, unk2, fadeintime, primarystyle, ignoreloc);
		}
		static int _draw_notification_4(bool blink, bool p1) {
			return invoker::invoke<int>(0xf020c96915705b3a, blink, p1);
		}
		static type::any _0x8efccf6ec66d85e4(type::any* p0, type::any* p1, type::any* p2, bool p3, bool p4) {
			return invoker::invoke<type::any>(0x8efccf6ec66d85e4, p0, p1, p2, p3, p4);
		}
		static type::any _0xb6871b0555b02996(type::any* p0, type::any* p1, type::any p2, type::any* p3, type::any* p4, type::any p5) {
			return invoker::invoke<type::any>(0xb6871b0555b02996, p0, p1, p2, p3, p4, p5);
		}
		static int _draw_notification_with_icon(int type, int image, const char* text) {
			return invoker::invoke<int>(0xd202b92cbf1d816f, type, image, text);
		}
		static int _draw_notification_with_button(int type, const char* button, const char* text) {
			return invoker::invoke<int>(0xdd6cb2cce7c2735c, type, button, text);
		}
		static void begin_text_command_print(const char* gxtentry) {
			invoker::invoke<void>(0xb87a37eeb7faa67d, gxtentry);
		}
		static void end_text_command_print(int duration, bool drawimmediately) {
			invoker::invoke<void>(0x9d77056a530643f6, duration, drawimmediately);
		}
		static void begin_text_command_is_message_displayed(const char* text) {
			invoker::invoke<void>(0x853648fd1063a213, text);
		}
		static bool end_text_command_is_message_displayed() {
			return invoker::invoke<bool>(0x8a9ba1ab3e237613);
		}
		static void begin_text_command_display_text(const char* text) {
			invoker::invoke<void>(0x25fbb336df1804cb, text);
		}
		static void end_text_command_display_text(float x, float y) {
			invoker::invoke<void>(0xcd015e5bb0d96a57, x, y);
		}
		static void _begin_text_command_width(const char* text) {
			invoker::invoke<void>(0x54ce8ac98e120cab, text);
		}
		static float _end_text_command_get_width(bool p0) {
			return invoker::invoke<float>(0x85f061da64ed2f67, p0);
		}
		static void _begin_text_command_line_count(const char* entry) {
			invoker::invoke<void>(0x521fb041d93dd0e4, entry);
		}
		static int _get_text_screen_line_count(float x, float y) {
			return invoker::invoke<int>(0x9040dfb09be75706, x, y);
		}
		static void begin_text_command_display_help(const char* inputtype) {
			invoker::invoke<void>(0x8509b634fbe7da11, inputtype);
		}
		static void end_text_command_display_help(type::any p0, bool loop, bool beep, int duration) {
			invoker::invoke<void>(0x238ffe5c7b0498a6, p0, loop, beep, duration);
		}
		static void begin_text_command_is_this_help_message_being_displayed(const char* labelname) {
			invoker::invoke<void>(0x0a24da3a41b718f5, labelname);
		}
		static bool end_text_command_is_this_help_message_being_displayed(int p0) {
			return invoker::invoke<bool>(0x10bddbfc529428dd, p0);
		}
		static void begin_text_command_set_blip_name(const char* gxtentry) {
			invoker::invoke<void>(0xf9113a30de5c6670, gxtentry);
		}
		static void end_text_command_set_blip_name(type::blip blip) {
			invoker::invoke<void>(0xbc38b49bcb83bc9b, blip);
		}
		static void _begin_text_command_objective(const char* p0) {
			invoker::invoke<void>(0x23d69e0465570028, p0);
		}
		static void _end_text_command_objective(bool p0) {
			invoker::invoke<void>(0xcfdbdf5ae59ba0f4, p0);
		}
		static void begin_text_command_clear_print(const char* text) {
			invoker::invoke<void>(0xe124fa80a759019c, text);
		}
		static void end_text_command_clear_print() {
			invoker::invoke<void>(0xfcc75460aba29378);
		}
		static void begin_text_command_override_button_text(const char* p0) {
			invoker::invoke<void>(0x8f9ee5687f8eeccd, p0);
		}
		static void end_text_command_override_button_text(bool p0) {
			invoker::invoke<void>(0xa86911979638106f, p0);
		}
		static void add_text_component_integer(int value) {
			invoker::invoke<void>(0x03b504cf259931bc, value);
		}
		static void add_text_component_float(float value, int decimalplaces) {
			invoker::invoke<void>(0xe7dcb5b874bcd96e, value, decimalplaces);
		}
		static void add_text_component_substring_text_label(const char* labelname) {
			invoker::invoke<void>(0xc63cd5d2920acbe7, labelname);
		}
		static void add_text_component_substring_text_label_hash_key(type::hash gxtentryhash) {
			invoker::invoke<void>(0x17299b63c7683a2b, gxtentryhash);
		}
		static void add_text_component_substring_blip_name(type::blip blip) {
			invoker::invoke<void>(0x80ead8e2e1d5d52e, blip);
		}
		static void add_text_component_substring_player_name(const char* text) {
			invoker::invoke<void>(0x6c188be134e074aa, text);
		}
		static void add_text_component_substring_time(int timestamp, int flags) {
			invoker::invoke<void>(0x1115f16b8ab9e8bf, timestamp, flags);
		}
		static void add_text_component_formatted_integer(int value, bool commaseparated) {
			invoker::invoke<void>(0x0e4c749ff9de9cc4, value, commaseparated);
		}
		static void add_text_component_substring_phone_number(const char* p0, int p1) {
			invoker::invoke<void>(0x761b77454205a61d, p0, p1);
		}
		static void add_text_component_substring_website(const char* website) {
			invoker::invoke<void>(0x94cf4ac034c9c986, website);
		}
		static void add_text_component_substring_keyboard_display(const char* p0) {
			invoker::invoke<void>(0x5f68520888e69014, p0);
		}
		static void _set_notification_color_next(int hudindex) {
			invoker::invoke<void>(0x39bbf623fc803eac, hudindex);
		}
		static const char* _get_text_substring(const char* text, int position, int length) {
			return invoker::invoke<const char*>(0x169bd9382084c8c0, text, position, length);
		}
		static const char* _get_text_substring_safe(const char* text, int position, int length, int maxlength) {
			return invoker::invoke<const char*>(0xb2798643312205c5, text, position, length, maxlength);
		}
		static const char* _get_text_substring_slice(const char* text, int startposition, int endposition) {
			return invoker::invoke<const char*>(0xce94aeba5d82908a, text, startposition, endposition);
		}
		static const char* _get_label_text(const char* labelname) {
			return invoker::invoke<const char*>(0x7b5280eba9840c72, labelname);
		}
		static void clear_prints() {
			invoker::invoke<void>(0xcc33fa791322b9d9);
		}
		static void clear_brief() {
			invoker::invoke<void>(0x9d292f73adbd9313);
		}
		static void clear_all_help_messages() {
			invoker::invoke<void>(0x6178f68a87a4d3a0);
		}
		static void clear_this_print(const char* p0) {
			invoker::invoke<void>(0xcf708001e1e536dd, p0);
		}
		static void clear_small_prints() {
			invoker::invoke<void>(0x2cea2839313c09ac);
		}
		static bool does_text_block_exist(const char* gxt) {
			return invoker::invoke<bool>(0x1c7302e725259789, gxt);
		}
		static void request_additional_text(const char* gxt, int slot) {
			invoker::invoke<void>(0x71a78003c8e71424, gxt, slot);
		}
		static void _request_additional_text_2(const char* gxt, int slot) {
			invoker::invoke<void>(0x6009f9f1ae90d8a6, gxt, slot);
		}
		static bool has_additional_text_loaded(int slot) {
			return invoker::invoke<bool>(0x02245fe4bed318b8, slot);
		}
		static void clear_additional_text(int p0, bool p1) {
			invoker::invoke<void>(0x2a179df17ccf04cd, p0, p1);
		}
		static bool is_streaming_additional_text(int p0) {
			return invoker::invoke<bool>(0x8b6817b71b85ebf0, p0);
		}
		static bool has_this_additional_text_loaded(const char* gxt, int slot) {
			return invoker::invoke<bool>(0xadbf060e2b30c5bc, gxt, slot);
		}
		static bool is_message_being_displayed() {
			return invoker::invoke<bool>(0x7984c03aa5cc2f41);
		}
		static bool does_text_label_exist(const char* gxt) {
			return invoker::invoke<bool>(0xac09ca973c564252, gxt);
		}
		static int get_length_of_string_with_this_text_label(const char* gxt) {
			return invoker::invoke<int>(0x801bd273d3a23f74, gxt);
		}
		static int get_length_of_literal_string(const char* string) {
			return invoker::invoke<int>(0xf030907ccbb8a9fd, string);
		}
		static int _get_length_of_string(const char* string) {
			return invoker::invoke<int>(0x43e4111189e54f0e, string);
		}
		static const char* get_street_name_from_hash_key(type::hash hash) {
			return invoker::invoke<const char*>(0xd0ef8a959b8a4cb9, hash);
		}
		static bool is_hud_preference_switched_on() {
			return invoker::invoke<bool>(0x1930dfa731813ec4);
		}
		static bool is_radar_preference_switched_on() {
			return invoker::invoke<bool>(0x9eb6522ea68f22fe);
		}
		static bool is_subtitle_preference_switched_on() {
			return invoker::invoke<bool>(0xad6daca4ba53e0a4);
		}
		static void display_hud(bool toggle) {
			invoker::invoke<void>(0xa6294919e56ff02a, toggle);
		}
		static void _0x7669f9e39dc17063() {
			invoker::invoke<void>(0x7669f9e39dc17063);
		}
		static void _0x402f9ed62087e898() {
			invoker::invoke<void>(0x402f9ed62087e898);
		}
		static void display_radar(bool toggle) {
			invoker::invoke<void>(0xa0ebb943c300e693, toggle);
		}
		static bool is_hud_hidden() {
			return invoker::invoke<bool>(0xa86478c6958735c5);
		}
		static bool is_radar_hidden() {
			return invoker::invoke<bool>(0x157f93b036700462);
		}
		static bool _is_radar_enabled() {
			return invoker::invoke<bool>(0xaf754f20eb5cd51a);
		}
		static void set_blip_route(type::blip blip, bool enabled) {
			invoker::invoke<void>(0x4f7d8a9bfb0b43e9, blip, enabled);
		}
		static void set_blip_route_colour(type::blip blip, int colour) {
			invoker::invoke<void>(0x837155cd2f63da09, blip, colour);
		}
		static void add_next_message_to_previous_briefs(bool p0) {
			invoker::invoke<void>(0x60296af4ba14abc5, p0);
		}
		static void _0x57d760d55f54e071(bool p0) {
			invoker::invoke<void>(0x57d760d55f54e071, p0);
		}
		static void responding_as_temp(float p0) {
			invoker::invoke<void>(0xbd12c5eee184c337, p0);
		}
		static void set_radar_zoom(int zoomlevel) {
			invoker::invoke<void>(0x096ef57a0c999bba, zoomlevel);
		}
		static void _0xf98e4b3e56afc7b1(type::any p0, float p1) {
			invoker::invoke<void>(0xf98e4b3e56afc7b1, p0, p1);
		}
		static void _set_radar_zoom_level_this_frame(float zoomlevel) {
			invoker::invoke<void>(0xcb7cc0d58405ad41, zoomlevel);
		}
		static void _0xd2049635deb9c375() {
			invoker::invoke<void>(0xd2049635deb9c375);
		}
		static void get_hud_colour(int hudcolorindex, int* r, int* g, int* b, int* a) {
			invoker::invoke<void>(0x7c9c91ab74a0360f, hudcolorindex, r, g, b, a);
		}
		static void _0xd68a5ff8a3a89874(int r, int g, int b, int a) {
			invoker::invoke<void>(0xd68a5ff8a3a89874, r, g, b, a);
		}
		static void _0x16a304e6cb2bfab9(int r, int g, int b, int a) {
			invoker::invoke<void>(0x16a304e6cb2bfab9, r, g, b, a);
		}
		static void _set_hud_colours_switch(int hudcolorindex, int hudcolorindex2) {
			invoker::invoke<void>(0x1ccc708f0f850613, hudcolorindex, hudcolorindex2);
		}
		static void _set_hud_colour(int hudcolorindex, int r, int g, int b, int a) {
			invoker::invoke<void>(0xf314cf4f0211894e, hudcolorindex, r, g, b, a);
		}
		static void flash_ability_bar(bool toggle) {
			invoker::invoke<void>(0x02cfba0c9e9275ce, toggle);
		}
		static void set_ability_bar_value(float value, float maxvalue) {
			invoker::invoke<void>(0x9969599ccff5d85e, value, maxvalue);
		}
		static void flash_wanted_display(bool p0) {
			invoker::invoke<void>(0xa18afb39081b6a1f, p0);
		}
		static void _0xba8d65c1c65702e5(bool p0) {
			invoker::invoke<void>(0xba8d65c1c65702e5, p0);
		}
		static float _get_text_scale_height(float size, int font) {
			return invoker::invoke<float>(0xdb88a37483346780, size, font);
		}
		static void set_text_scale(float unk, float scale) {
			invoker::invoke<void>(0x07c837f9a01c34c9, unk, scale);
		}
		static void set_text_colour(int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0xbe6b23ffa53fb442, red, green, blue, alpha);
		}
		static void set_text_centre(bool align) {
			invoker::invoke<void>(0xc02f4dbfb51d988b, align);
		}
		static void set_text_right_justify(bool toggle) {
			invoker::invoke<void>(0x6b3c4650bc8bee47, toggle);
		}
		static void set_text_justification(int justifytype) {
			invoker::invoke<void>(0x4e096588b13ffeca, justifytype);
		}
		static void set_text_wrap(float start, float end) {
			invoker::invoke<void>(0x63145d9c883a1a70, start, end);
		}
		static void set_text_leading(bool p0) {
			invoker::invoke<void>(0xa50abc31e3cdfaff, p0);
		}
		static void set_text_proportional(bool p0) {
			invoker::invoke<void>(0x038c1f517d7fdcf8, p0);
		}
		static void set_text_font(int fonttype) {
			invoker::invoke<void>(0x66e0276cc5f6b9da, fonttype);
		}
		static void set_text_drop_shadow() {
			invoker::invoke<void>(0x1ca3e9eac9d93e5e);
		}
		static void set_text_dropshadow(int distance, int r, int g, int b, int a) {
			invoker::invoke<void>(0x465c84bc39f1c351, distance, r, g, b, a);
		}
		static void set_text_outline() {
			invoker::invoke<void>(0x2513dfb0fb8400fe);
		}
		static void set_text_edge(int p0, int r, int g, int b, int a) {
			invoker::invoke<void>(0x441603240d202fa6, p0, r, g, b, a);
		}
		static void set_text_render_id(int renderid) {
			invoker::invoke<void>(0x5f15302936e07111, renderid);
		}
		static int get_default_script_rendertarget_render_id() {
			return invoker::invoke<int>(0x52f0982d7fd156b6);
		}
		static bool register_named_rendertarget(const char* p0, bool p1) {
			return invoker::invoke<bool>(0x57d9c12635e25ce3, p0, p1);
		}
		static bool is_named_rendertarget_registered(const char* p0) {
			return invoker::invoke<bool>(0x78dcdc15c9f116b4, p0);
		}
		static bool release_named_rendertarget(type::any* p0) {
			return invoker::invoke<bool>(0xe9f6ffe837354dd4, p0);
		}
		static void link_named_rendertarget(type::hash hash) {
			invoker::invoke<void>(0xf6c09e276aeb3f2d, hash);
		}
		static type::any get_named_rendertarget_render_id(const char* p0) {
			return invoker::invoke<type::any>(0x1a6478b61c6bdc3b, p0);
		}
		static bool is_named_rendertarget_linked(type::hash hash) {
			return invoker::invoke<bool>(0x113750538fa31298, hash);
		}
		static void clear_help(bool toggle) {
			invoker::invoke<void>(0x8dfced7a656f8802, toggle);
		}
		static bool is_help_message_on_screen() {
			return invoker::invoke<bool>(0xdad37f45428801ae);
		}
		static bool _0x214cd562a939246a() {
			return invoker::invoke<bool>(0x214cd562a939246a);
		}
		static bool is_help_message_being_displayed() {
			return invoker::invoke<bool>(0x4d79439a6b55ac67);
		}
		static bool is_help_message_fading_out() {
			return invoker::invoke<bool>(0x327edeeeac55c369);
		}
		static bool _0x4a9923385bdb9dad() {
			return invoker::invoke<bool>(0x4a9923385bdb9dad);
		}
		static int _get_blip_info_id_iterator() {
			return invoker::invoke<int>(0x186e5d252fa50e7d);
		}
		static int get_number_of_active_blips() {
			return invoker::invoke<int>(0x9a3ff3de163034e8);
		}
		static type::blip get_next_blip_info_id(int blipsprite) {
			return invoker::invoke<type::blip>(0x14f96aa50d6fbea7, blipsprite);
		}
		static type::blip get_first_blip_info_id(int blipsprite) {
			return invoker::invoke<type::blip>(0x1bede233e6cd2a1f, blipsprite);
		}
		static PVector3 get_blip_info_id_coord(type::blip blip) {
			return invoker::invoke<PVector3>(0xfa7c7f0aadf25d09, blip);
		}
		static int get_blip_info_id_display(type::blip blip) {
			return invoker::invoke<int>(0x1e314167f701dc3b, blip);
		}
		static int get_blip_info_id_type(type::blip blip) {
			return invoker::invoke<int>(0xbe9b0959ffd0779b, blip);
		}
		static type::entity get_blip_info_id_entity_index(type::blip blip) {
			return invoker::invoke<type::entity>(0x4ba4e2553afedc2c, blip);
		}
		static type::pickup get_blip_info_id_pickup_index(type::blip blip) {
			return invoker::invoke<type::pickup>(0x9b6786e4c03dd382, blip);
		}
		static type::blip get_blip_from_entity(type::entity entity) {
			return invoker::invoke<type::blip>(0xbc8dbdca2436f7e8, entity);
		}
		static type::blip add_blip_for_radius(float posx, float posy, float posz, float radius) {
			return invoker::invoke<type::blip>(0x46818d79b1f7499a, posx, posy, posz, radius);
		}
		static type::blip add_blip_for_entity(type::entity entity) {
			return invoker::invoke<type::blip>(0x5cde92c702a8fce7, entity);
		}
		static type::blip add_blip_for_pickup(type::pickup pickup) {
			return invoker::invoke<type::blip>(0xbe339365c863bd36, pickup);
		}
		static type::blip add_blip_for_coord(float x, float y, float z) {
			return invoker::invoke<type::blip>(0x5a039bb0bca604b6, x, y, z);
		}
		static void _0x72dd432f3cdfc0ee(float posx, float posy, float posz, float radius, int p4) {
			invoker::invoke<void>(0x72dd432f3cdfc0ee, posx, posy, posz, radius, p4);
		}
		static void _0x60734cc207c9833c(bool p0) {
			invoker::invoke<void>(0x60734cc207c9833c, p0);
		}
		static void set_blip_coords(type::blip blip, float posx, float posy, float posz) {
			invoker::invoke<void>(0xae2af67e9d9af65d, blip, posx, posy, posz);
		}
		static PVector3 get_blip_coords(type::blip blip) {
			return invoker::invoke<PVector3>(0x586afe3ff72d996e, blip);
		}
		static void set_blip_sprite(type::blip blip, int spriteid) {
			invoker::invoke<void>(0xdf735600a4696daf, blip, spriteid);
		}
		static int get_blip_sprite(type::blip blip) {
			return invoker::invoke<int>(0x1fc877464a04fc4f, blip);
		}
		static void set_blip_name_from_text_file(type::blip blip, const char* gxtentry) {
			invoker::invoke<void>(0xeaa0ffe120d92784, blip, gxtentry);
		}
		static void set_blip_name_to_player_name(type::blip blip, type::player player) {
			invoker::invoke<void>(0x127de7b20c60a6a3, blip, player);
		}
		static void set_blip_alpha(type::blip blip, int alpha) {
			invoker::invoke<void>(0x45ff974eee1c8734, blip, alpha);
		}
		static int get_blip_alpha(type::blip blip) {
			return invoker::invoke<int>(0x970f608f0ee6c885, blip);
		}
		static void set_blip_fade(type::blip blip, int opacity, int duration) {
			invoker::invoke<void>(0x2aee8f8390d2298c, blip, opacity, duration);
		}
		static void set_blip_rotation(type::blip blip, int rotation) {
			invoker::invoke<void>(0xf87683cdf73c3f6e, blip, rotation);
		}
		static void set_blip_flash_timer(type::blip blip, int duration) {
			invoker::invoke<void>(0xd3cd6fd297ae87cc, blip, duration);
		}
		static void set_blip_flash_interval(type::blip blip, type::any p1) {
			invoker::invoke<void>(0xaa51db313c010a7e, blip, p1);
		}
		static void set_blip_colour(type::blip blip, int color) {
			invoker::invoke<void>(0x03d7fb09e75d6b7e, blip, color);
		}
		static void set_blip_secondary_colour(type::blip blip, float r, float g, float b) {
			invoker::invoke<void>(0x14892474891e09eb, blip, r, g, b);
		}
		static int get_blip_colour(type::blip blip) {
			return invoker::invoke<int>(0xdf729e8d20cf7327, blip);
		}
		static int get_blip_hud_colour(type::blip blip) {
			return invoker::invoke<int>(0x729b5f1efbc0aaee, blip);
		}
		static bool is_blip_short_range(type::blip blip) {
			return invoker::invoke<bool>(0xda5f8727eb75b926, blip);
		}
		static bool is_blip_on_minimap(type::blip blip) {
			return invoker::invoke<bool>(0xe41ca53051197a27, blip);
		}
		static bool _0xdd2238f57b977751(type::any p0) {
			return invoker::invoke<bool>(0xdd2238f57b977751, p0);
		}
		static void _0x54318c915d27e4ce(type::any p0, bool p1) {
			invoker::invoke<void>(0x54318c915d27e4ce, p0, p1);
		}
		static void set_blip_high_detail(type::blip blip, bool toggle) {
			invoker::invoke<void>(0xe2590bc29220cebb, blip, toggle);
		}
		static void set_blip_as_mission_creator_blip(type::blip blip, bool toggle) {
			invoker::invoke<void>(0x24ac0137444f9fd5, blip, toggle);
		}
		static bool is_mission_creator_blip(type::blip blip) {
			return invoker::invoke<bool>(0x26f49bf3381d933d, blip);
		}
		static type::blip disable_blip_name_for_var() {
			return invoker::invoke<type::blip>(0x5c90988e7c8e1af4);
		}
		static bool _0x4167efe0527d706e() {
			return invoker::invoke<bool>(0x4167efe0527d706e);
		}
		static void _0xf1a6c18b35bcade6(bool p0) {
			invoker::invoke<void>(0xf1a6c18b35bcade6, p0);
		}
		static void set_blip_flashes(type::blip blip, bool toggle) {
			invoker::invoke<void>(0xb14552383d39ce3e, blip, toggle);
		}
		static void set_blip_flashes_alternate(type::blip blip, bool toggle) {
			invoker::invoke<void>(0x2e8d9498c56dd0d1, blip, toggle);
		}
		static bool is_blip_flashing(type::blip blip) {
			return invoker::invoke<bool>(0xa5e41fd83ad6cef0, blip);
		}
		static void set_blip_as_short_range(type::blip blip, bool toggle) {
			invoker::invoke<void>(0xbe8be4fe60e27b72, blip, toggle);
		}
		static void set_blip_scale(type::blip blip, float scale) {
			invoker::invoke<void>(0xd38744167b2fa257, blip, scale);
		}
		static void set_blip_priority(type::blip blip, int priority) {
			invoker::invoke<void>(0xae9fc9ef6a9fac79, blip, priority);
		}
		static void set_blip_display(type::blip blip, int displayid) {
			invoker::invoke<void>(0x9029b2f3da924928, blip, displayid);
		}
		static void set_blip_category(type::blip blip, int index) {
			invoker::invoke<void>(0x234cdd44d996fd9a, blip, index);
		}
		static void remove_blip(type::blip* blip) {
			invoker::invoke<void>(0x86a652570e5f25dd, blip);
		}
		static void set_blip_as_friendly(type::blip blip, bool toggle) {
			invoker::invoke<void>(0x6f6f290102c02ab4, blip, toggle);
		}
		static void pulse_blip(type::blip blip) {
			invoker::invoke<void>(0x742d6fd43115af73, blip);
		}
		static void show_number_on_blip(type::blip blip, int number) {
			invoker::invoke<void>(0xa3c0b359dcb848b6, blip, number);
		}
		static void hide_number_on_blip(type::blip blip) {
			invoker::invoke<void>(0x532cff637ef80148, blip);
		}
		static void _0x75a16c3da34f1245(type::blip blip, bool p1) {
			invoker::invoke<void>(0x75a16c3da34f1245, blip, p1);
		}
		static void show_tick_on_blip(type::blip blip, bool toggle) {
			invoker::invoke<void>(0x74513ea3e505181e, blip, toggle);
		}
		static void show_heading_indicator_on_blip(type::blip blip, bool toggle) {
			invoker::invoke<void>(0x5fbca48327b914df, blip, toggle);
		}
		static void _set_blip_friendly(type::blip blip, bool toggle) {
			invoker::invoke<void>(0xb81656bc81fe24d1, blip, toggle);
		}
		static void _set_blip_friend(type::blip blip, bool toggle) {
			invoker::invoke<void>(0x23c3eb807312f01a, blip, toggle);
		}
		static void _0xdcfb5d4db8bf367e(type::any p0, bool p1) {
			invoker::invoke<void>(0xdcfb5d4db8bf367e, p0, p1);
		}
		static void _0xc4278f70131baa6d(type::any p0, bool p1) {
			invoker::invoke<void>(0xc4278f70131baa6d, p0, p1);
		}
		static void _set_blip_shrink(type::blip blip, bool toggle) {
			invoker::invoke<void>(0x2b6d467dab714e8d, blip, toggle);
		}
		static void _0x25615540d894b814(type::any p0, bool p1) {
			invoker::invoke<void>(0x25615540d894b814, p0, p1);
		}
		static bool does_blip_exist(type::blip blip) {
			return invoker::invoke<bool>(0xa6db27d19ecbb7da, blip);
		}
		static void set_waypoint_off() {
			invoker::invoke<void>(0xa7e4e2d361c2627f);
		}
		static void _0xd8e694757bcea8e9() {
			invoker::invoke<void>(0xd8e694757bcea8e9);
		}
		static void refresh_waypoint() {
			invoker::invoke<void>(0x81fa173f170560d1);
		}
		static bool is_waypoint_active() {
			return invoker::invoke<bool>(0x1dd1f58f493f1da5);
		}
		static void set_new_waypoint(float x, float y) {
			invoker::invoke<void>(0xfe43368d2aa4f2fc, x, y);
		}
		static void set_blip_bright(type::blip blip, bool toggle) {
			invoker::invoke<void>(0xb203913733f27884, blip, toggle);
		}
		static void set_blip_show_cone(type::blip blip, bool toggle) {
			invoker::invoke<void>(0x13127ec3665e8ee1, blip, toggle);
		}
		static void _0xc594b315edf2d4af(type::ped ped) {
			invoker::invoke<void>(0xc594b315edf2d4af, ped);
		}
		static type::any set_minimap_component(int component, bool toggle, int componentcolor) {
			return invoker::invoke<type::any>(0x75a9a10948d1dea6, component, toggle, componentcolor);
		}
		static void _0x60e892ba4f5bdca4() {
			invoker::invoke<void>(0x60e892ba4f5bdca4);
		}
		static type::blip get_main_player_blip_id() {
			return invoker::invoke<type::blip>(0xdcd4ec3f419d02fa);
		}
		static void _0x41350b4fc28e3941(bool p0) {
			invoker::invoke<void>(0x41350b4fc28e3941, p0);
		}
		static void hide_loading_on_fade_this_frame() {
			invoker::invoke<void>(0x4b0311d3cdc4648f);
		}
		static void set_radar_as_interior_this_frame(type::hash interior, float x, float y, int heading, int zoom) {
			invoker::invoke<void>(0x59e727a1c9d3e31a, interior, x, y, heading, zoom);
		}
		static void set_radar_as_exterior_this_frame() {
			invoker::invoke<void>(0xe81b7d2a3dab2d81);
		}
		static void _set_player_blip_position_this_frame(float x, float y) {
			invoker::invoke<void>(0x77e2dd177910e1cf, x, y);
		}
		static type::any _0x9049fe339d5f6f6f() {
			return invoker::invoke<type::any>(0x9049fe339d5f6f6f);
		}
		static void _disable_radar_this_frame() {
			invoker::invoke<void>(0x5fbae526203990c9);
		}
		static void _0x20fe7fdfeead38c0() {
			invoker::invoke<void>(0x20fe7fdfeead38c0);
		}
		static void _center_player_on_radar_this_frame() {
			invoker::invoke<void>(0x6d14bfdc33b34f55);
		}
		static void set_widescreen_format(type::any p0) {
			invoker::invoke<void>(0xc3b07ba00a83b0f1, p0);
		}
		static void display_area_name(bool toggle) {
			invoker::invoke<void>(0x276b6ce369c33678, toggle);
		}
		static void display_cash(bool toggle) {
			invoker::invoke<void>(0x96dec8d5430208b7, toggle);
		}
		static void _update_display_cash(bool toggle) {
			invoker::invoke<void>(0x170f541e1cadd1de, toggle);
		}
		static void _set_player_cash_change(int cash, int bank) {
			invoker::invoke<void>(0x0772df77852c2e30, cash, bank);
		}
		static void display_ammo_this_frame(bool display) {
			invoker::invoke<void>(0xa5e78ba2b1331c55, display);
		}
		static void display_sniper_scope_this_frame() {
			invoker::invoke<void>(0x73115226f4814e62);
		}
		static void hide_hud_and_radar_this_frame() {
			invoker::invoke<void>(0x719ff505f097fd20);
		}
		static void _0xe67c6dfd386ea5e7(bool p0) {
			invoker::invoke<void>(0xe67c6dfd386ea5e7, p0);
		}
		static void _set_display_cash() {
			invoker::invoke<void>(0xc2d15bef167e27bc);
		}
		static void _remove_display_cash() {
			invoker::invoke<void>(0x95cf81bd06ee1887);
		}
		static void set_multiplayer_bank_cash() {
			invoker::invoke<void>(0xdd21b55df695cd0a);
		}
		static void remove_multiplayer_bank_cash() {
			invoker::invoke<void>(0xc7c6789aa1cfedd0);
		}
		static void set_multiplayer_hud_cash(int p0, int p1) {
			invoker::invoke<void>(0xfd1d220394bcb824, p0, p1);
		}
		static void remove_multiplayer_hud_cash() {
			invoker::invoke<void>(0x968f270e39141eca);
		}
		static void hide_help_text_this_frame() {
			invoker::invoke<void>(0xd46923fc481ca285);
		}
		static void display_help_text_this_frame(const char* message, bool p1) {
			invoker::invoke<void>(0x960c9ff8f616e41c, message, p1);
		}
		static void _show_weapon_wheel(bool forcedshow) {
			invoker::invoke<void>(0xeb354e5376bc81a7, forcedshow);
		}
		static void _block_weapon_wheel_this_frame() {
			invoker::invoke<void>(0x0afc4af510774b47);
		}
		static type::hash _0xa48931185f0536fe() {
			return invoker::invoke<type::hash>(0xa48931185f0536fe);
		}
		static void _0x72c1056d678bb7d8(type::hash weaponhash) {
			invoker::invoke<void>(0x72c1056d678bb7d8, weaponhash);
		}
		static type::any _0xa13e93403f26c812(type::any p0) {
			return invoker::invoke<type::any>(0xa13e93403f26c812, p0);
		}
		static void _0x14c9fdcc41f81f63(bool p0) {
			invoker::invoke<void>(0x14c9fdcc41f81f63, p0);
		}
		static void set_gps_flags(int p0, float p1) {
			invoker::invoke<void>(0x5b440763a4c8d15b, p0, p1);
		}
		static void clear_gps_flags() {
			invoker::invoke<void>(0x21986729d6a3a830);
		}
		static void _0x1eac5f91bcbc5073(bool p0) {
			invoker::invoke<void>(0x1eac5f91bcbc5073, p0);
		}
		static void clear_gps_race_track() {
			invoker::invoke<void>(0x7aa5b4ce533c858b);
		}
		static void _0xdb34e8d56fc13b08(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0xdb34e8d56fc13b08, p0, p1, p2);
		}
		static void _0x311438a071dd9b1a(float x, float y, float z) {
			invoker::invoke<void>(0x311438a071dd9b1a, x, y, z);
		}
		static void _0x900086f371220b6f(bool p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0x900086f371220b6f, p0, p1, p2);
		}
		static void _0xe6de0561d9232a64() {
			invoker::invoke<void>(0xe6de0561d9232a64);
		}
		static void _0x3d3d15af7bcaaf83(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x3d3d15af7bcaaf83, p0, p1, p2);
		}
		static void _0xa905192a6781c41b(float x, float y, float z) {
			invoker::invoke<void>(0xa905192a6781c41b, x, y, z);
		}
		static void _0x3dda37128dd1aca8(bool p0) {
			invoker::invoke<void>(0x3dda37128dd1aca8, p0);
		}
		static void _0x67eedea1b9bafd94() {
			invoker::invoke<void>(0x67eedea1b9bafd94);
		}
		static void clear_gps_player_waypoint() {
			invoker::invoke<void>(0xff4fb7c8cdfa3da7);
		}
		static void set_gps_flashes(bool toggle) {
			invoker::invoke<void>(0x320d0e0d936a0e9b, toggle);
		}
		static void _0x7b21e0bb01e8224a(type::any p0) {
			invoker::invoke<void>(0x7b21e0bb01e8224a, p0);
		}
		static void flash_minimap_display() {
			invoker::invoke<void>(0xf2dd778c22b15bda);
		}
		static void _0x6b1de27ee78e6a19(type::any p0) {
			invoker::invoke<void>(0x6b1de27ee78e6a19, p0);
		}
		static void toggle_stealth_radar(bool toggle) {
			invoker::invoke<void>(0x6afdfb93754950c7, toggle);
		}
		static void key_hud_colour(bool p0, type::any p1) {
			invoker::invoke<void>(0x1a5cd7752dd28cd3, p0, p1);
		}
		static void set_mission_name(bool p0, const char* name) {
			invoker::invoke<void>(0x5f28ecf5fc84772f, p0, name);
		}
		static void _set_mission_name_2(bool p0, const char* name) {
			invoker::invoke<void>(0xe45087d85f468bc2, p0, name);
		}
		static void _0x817b86108eb94e51(bool p0, type::any* p1, type::any* p2, type::any* p3, type::any* p4, type::any* p5, type::any* p6, type::any* p7, type::any* p8) {
			invoker::invoke<void>(0x817b86108eb94e51, p0, p1, p2, p3, p4, p5, p6, p7, p8);
		}
		static void set_minimap_block_waypoint(bool toggle) {
			invoker::invoke<void>(0x58fadded207897dc, toggle);
		}
		static void _set_north_yankton_map(bool toggle) {
			invoker::invoke<void>(0x9133955f1a2da957, toggle);
		}
		static void _set_minimap_revealed(bool toggle) {
			invoker::invoke<void>(0xf8dee0a5600cbb93, toggle);
		}
		static float _0xe0130b41d3cf4574() {
			return invoker::invoke<float>(0xe0130b41d3cf4574);
		}
		static bool _is_minimap_area_revealed(float x, float y, float radius) {
			return invoker::invoke<bool>(0x6e31b91145873922, x, y, radius);
		}
		static void _0x62e849b7eb28e770(bool p0) {
			invoker::invoke<void>(0x62e849b7eb28e770, p0);
		}
		static void _0x0923dbf87dff735e(float x, float y, float z) {
			invoker::invoke<void>(0x0923dbf87dff735e, x, y, z);
		}
		static void set_minimap_golf_course(int hole) {
			invoker::invoke<void>(0x71bdb63dbaf8da59, hole);
		}
		static void _0x35edd5b2e3ff01c0() {
			invoker::invoke<void>(0x35edd5b2e3ff01c0);
		}
		static void lock_minimap_angle(int angle) {
			invoker::invoke<void>(0x299faebb108ae05b, angle);
		}
		static void unlock_minimap_angle() {
			invoker::invoke<void>(0x8183455e16c42e3a);
		}
		static void lock_minimap_position(float x, float y) {
			invoker::invoke<void>(0x1279e861a329e73f, x, y);
		}
		static void unlock_minimap_position() {
			invoker::invoke<void>(0x3e93e06db8ef1f30);
		}
		static void _set_minimap_attitude_indicator_level(float altitude, bool p1) {
			invoker::invoke<void>(0xd201f3ff917a506d, altitude, p1);
		}
		static void _0x3f5cc444dcaaa8f2(type::any p0, type::any p1, bool p2) {
			invoker::invoke<void>(0x3f5cc444dcaaa8f2, p0, p1, p2);
		}
		static void _0x975d66a0bc17064c(type::any p0) {
			invoker::invoke<void>(0x975d66a0bc17064c, p0);
		}
		static void _0x06a320535f5f0248(type::any p0) {
			invoker::invoke<void>(0x06a320535f5f0248, p0);
		}
		static void _set_radar_bigmap_enabled(bool togglebigmap, bool showfullmap) {
			invoker::invoke<void>(0x231c8f89d0539d8f, togglebigmap, showfullmap);
		}
		static bool is_hud_component_active(int id) {
			return invoker::invoke<bool>(0xbc4c9ea5391ecc0d, id);
		}
		static bool is_scripted_hud_component_active(int id) {
			return invoker::invoke<bool>(0xdd100eb17a94ff65, id);
		}
		static void hide_scripted_hud_component_this_frame(int id) {
			invoker::invoke<void>(0xe374c498d8badc14, id);
		}
		static bool _0x09c0403ed9a751c2(type::any p0) {
			return invoker::invoke<bool>(0x09c0403ed9a751c2, p0);
		}
		static void hide_hud_component_this_frame(int id) {
			invoker::invoke<void>(0x6806c51ad12b83b8, id);
		}
		static void show_hud_component_this_frame(int id) {
			invoker::invoke<void>(0x0b4df1fa60c0e664, id);
		}
		static void _0xa4dede28b1814289() {
			invoker::invoke<void>(0xa4dede28b1814289);
		}
		static void reset_reticule_values() {
			invoker::invoke<void>(0x12782ce0a636e9f0);
		}
		static void reset_hud_component_values(int id) {
			invoker::invoke<void>(0x450930e616475d0d, id);
		}
		static void set_hud_component_position(int id, float x, float y) {
			invoker::invoke<void>(0xaabb1f56e2a17ced, id, x, y);
		}
		static PVector3 get_hud_component_position(int id) {
			return invoker::invoke<PVector3>(0x223ca69a8c4417fd, id);
		}
		static void clear_reminder_message() {
			invoker::invoke<void>(0xb57d8dd645cfa2cf);
		}
		static bool _get_screen_coord_from_world_coord(float worldx, float worldy, float worldz, float* screenx, float* screeny) {
			return invoker::invoke<bool>(0xf9904d11f1acbec3, worldx, worldy, worldz, screenx, screeny);
		}
		static void _display_job_report() {
			invoker::invoke<void>(0x523a590c1a3cc0d3);
		}
		static void _0xee4c0e6dbc6f2c6f() {
			invoker::invoke<void>(0xee4c0e6dbc6f2c6f);
		}
		static type::any _0x9135584d09a3437e() {
			return invoker::invoke<type::any>(0x9135584d09a3437e);
		}
		static bool _0x2432784aca090da4(type::any p0) {
			return invoker::invoke<bool>(0x2432784aca090da4, p0);
		}
		static void _0x7679cc1bcebe3d4c(type::any p0, float p1, float p2) {
			invoker::invoke<void>(0x7679cc1bcebe3d4c, p0, p1, p2);
		}
		static void _0x784ba7e0eceb4178(type::any p0, float x, float y, float z) {
			invoker::invoke<void>(0x784ba7e0eceb4178, p0, x, y, z);
		}
		static void _0xb094bc1db4018240(type::any p0, type::any p1, float p2, float p3) {
			invoker::invoke<void>(0xb094bc1db4018240, p0, p1, p2, p3);
		}
		static void _0x788e7fd431bd67f1(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4, type::any p5) {
			invoker::invoke<void>(0x788e7fd431bd67f1, p0, p1, p2, p3, p4, p5);
		}
		static void clear_floating_help(type::any p0, bool p1) {
			invoker::invoke<void>(0x50085246abd3fefa, p0, p1);
		}
		static void _set_mp_gamer_tag_color(int headdisplayid, const char* username, bool pointedclantag, bool isrockstarclan, const char* clantag, type::any p5, int r, int g, int b) {
			invoker::invoke<void>(0x6dd05e9d83efa4c9, headdisplayid, username, pointedclantag, isrockstarclan, clantag, p5, r, g, b);
		}
		static bool _has_mp_gamer_tag() {
			return invoker::invoke<bool>(0x6e0eb3eb47c8d7aa);
		}
		static int _create_mp_gamer_tag(type::ped ped, const char* username, bool pointedclantag, bool isrockstarclan, const char* clantag, type::any p5) {
			return invoker::invoke<int>(0xbfefe3321a3f5015, ped, username, pointedclantag, isrockstarclan, clantag, p5);
		}
		static void remove_mp_gamer_tag(int gamertagid) {
			invoker::invoke<void>(0x31698aa80e0223f8, gamertagid);
		}
		static bool is_mp_gamer_tag_active(int gamertagid) {
			return invoker::invoke<bool>(0x4e929e7a5796fd26, gamertagid);
		}
		static bool add_trevor_random_modifier(int gamertagid) {
			return invoker::invoke<bool>(0x595b5178e412e199, gamertagid);
		}
		static void set_mp_gamer_tag_visibility(int gamertagid, int component, bool toggle) {
			invoker::invoke<void>(0x63bb75abedc1f6a0, gamertagid, component, toggle);
		}
		static void _set_mp_gamer_tag_(int headdisplayid, bool p1) {
			invoker::invoke<void>(0xee76ff7e6a0166b0, headdisplayid, p1);
		}
		static void _set_mp_gamer_tag_icons(int headdisplayid, bool p1) {
			invoker::invoke<void>(0xa67f9c46d612b6f1, headdisplayid, p1);
		}
		static void set_mp_gamer_tag_colour(int gamertagid, int flag, int color) {
			invoker::invoke<void>(0x613ed644950626ae, gamertagid, flag, color);
		}
		static void set_mp_gamer_tag_health_bar_colour(int headdisplayid, int color) {
			invoker::invoke<void>(0x3158c77a7e888ab4, headdisplayid, color);
		}
		static void set_mp_gamer_tag_alpha(int gamertagid, int component, int alpha) {
			invoker::invoke<void>(0xd48fe545cd46f857, gamertagid, component, alpha);
		}
		static void set_mp_gamer_tag_wanted_level(int gamertagid, int wantedlvl) {
			invoker::invoke<void>(0xcf228e2aa03099c3, gamertagid, wantedlvl);
		}
		static void set_mp_gamer_tag_name(int gamertagid, const char* string) {
			invoker::invoke<void>(0xdea2b8283baa3944, gamertagid, string);
		}
		static bool _has_mp_gamer_tag_2(int gamertagid) {
			return invoker::invoke<bool>(0xeb709a36958abe0d, gamertagid);
		}
		static void _set_mp_gamer_tag_chatting(int gamertagid, const char* string) {
			invoker::invoke<void>(0x7b7723747ccb55b6, gamertagid, string);
		}
		static int _get_active_website_id() {
			return invoker::invoke<int>(0x01a358d9128b7a86);
		}
		static int get_current_website_id() {
			return invoker::invoke<int>(0x97d47996fc48cbad);
		}
		static type::any _0xe3b05614dce1d014(type::any p0) {
			return invoker::invoke<type::any>(0xe3b05614dce1d014, p0);
		}
		static void _0xb99c4e4d9499df29(bool p0) {
			invoker::invoke<void>(0xb99c4e4d9499df29, p0);
		}
		static type::any _0xaf42195a42c63bba() {
			return invoker::invoke<type::any>(0xaf42195a42c63bba);
		}
		static void set_warning_message(const char* entryline1, int instructionalkey, const char* entryline2, bool p3, type::any p4, type::any* p5, type::any* p6, bool background) {
			invoker::invoke<void>(0x7b1776b3b53f8d74, entryline1, instructionalkey, entryline2, p3, p4, p5, p6, background);
		}
		static void set_warning_message_with_header(const char* entryheader, const char* entryline1, int instructionalkey, const char* entryline2, bool p4, type::any p5, type::any* p6, type::any* p7, bool background) {
			invoker::invoke<void>(0xdc38cc1e35b6a5d7, entryheader, entryline1, instructionalkey, entryline2, p4, p5, p6, p7, background);
		}
		static void _set_warning_message_3(const char* entryheader, const char* entryline1, type::any instructionalkey, const char* entryline2, bool p4, type::any p5, type::any p6, type::any* p7, type::any* p8, bool p9) {
			invoker::invoke<void>(0x701919482c74b5ab, entryheader, entryline1, instructionalkey, entryline2, p4, p5, p6, p7, p8, p9);
		}
		static bool _0x0c5a80a9e096d529(type::any p0, const char* text, type::any p2, type::any p3, type::any p4, type::any p5) {
			return invoker::invoke<bool>(0x0c5a80a9e096d529, p0, text, p2, p3, p4, p5);
		}
		static bool _0xdaf87174be7454ff(type::any p0) {
			return invoker::invoke<bool>(0xdaf87174be7454ff, p0);
		}
		static void _0x6ef54ab721dc6242() {
			invoker::invoke<void>(0x6ef54ab721dc6242);
		}
		static bool is_warning_message_active() {
			return invoker::invoke<bool>(0xe18b138fabc53103);
		}
		static void _0x7792424aa0eac32e() {
			invoker::invoke<void>(0x7792424aa0eac32e);
		}
		static void _set_map_full_screen(bool toggle) {
			invoker::invoke<void>(0x5354c5ba2ea868a4, toggle);
		}
		static void _0x1eae6dd17b7a5efa(type::any p0) {
			invoker::invoke<void>(0x1eae6dd17b7a5efa, p0);
		}
		static type::any _0x551df99658db6ee8(float p0, float p1, float p2) {
			return invoker::invoke<type::any>(0x551df99658db6ee8, p0, p1, p2);
		}
		static void _0x2708fc083123f9ff() {
			invoker::invoke<void>(0x2708fc083123f9ff);
		}
		static type::any _0x1121bfa1a1a522a8() {
			return invoker::invoke<type::any>(0x1121bfa1a1a522a8);
		}
		static void _0x82cedc33687e1f50(bool p0) {
			invoker::invoke<void>(0x82cedc33687e1f50, p0);
		}
		static void _0x211c4ef450086857() {
			invoker::invoke<void>(0x211c4ef450086857);
		}
		static void _0xbf4f34a85ca2970c() {
			invoker::invoke<void>(0xbf4f34a85ca2970c);
		}
		static void activate_frontend_menu(type::hash menuhash, bool toggle_pause, int component) {
			invoker::invoke<void>(0xef01d36b9c9d0c7b, menuhash, toggle_pause, component);
		}
		static void restart_frontend_menu(type::hash menuhash, int p1) {
			invoker::invoke<void>(0x10706dc6ad2d49c0, menuhash, p1);
		}
		static type::hash _get_current_frontend_menu() {
			return invoker::invoke<type::hash>(0x2309595ad6145265);
		}
		static void set_pause_menu_active(bool toggle) {
			invoker::invoke<void>(0xdf47fc56c71569cf, toggle);
		}
		static void disable_frontend_this_frame() {
			invoker::invoke<void>(0x6d3465a73092f0e6);
		}
		static void _0xba751764f0821256() {
			invoker::invoke<void>(0xba751764f0821256);
		}
		static void _0xcc3fdded67bcfc63() {
			invoker::invoke<void>(0xcc3fdded67bcfc63);
		}
		static void set_frontend_active(bool active) {
			invoker::invoke<void>(0x745711a75ab09277, active);
		}
		static bool is_pause_menu_active() {
			return invoker::invoke<bool>(0xb0034a223497ffcb);
		}
		static type::any _0x2f057596f2bd0061() {
			return invoker::invoke<type::any>(0x2f057596f2bd0061);
		}
		static int get_pause_menu_state() {
			return invoker::invoke<int>(0x272acd84970869c5);
		}
		static PVector3 _0x5bff36d6ed83e0ae() {
			return invoker::invoke<PVector3>(0x5bff36d6ed83e0ae);
		}
		static bool is_pause_menu_restarting() {
			return invoker::invoke<bool>(0x1c491717107431c7);
		}
		static void _log_debug_info(const char* p0) {
			invoker::invoke<void>(0x2162c446dfdf38fd, p0);
		}
		static void _0x77f16b447824da6c(int p0) {
			invoker::invoke<void>(0x77f16b447824da6c, p0);
		}
		static void _0xcdca26e80faecb8f() {
			invoker::invoke<void>(0xcdca26e80faecb8f);
		}
		static void pause_menu_activate_context(type::hash contexthash) {
			invoker::invoke<void>(0xdd564bdd0472c936, contexthash);
		}
		static void object_decal_toggle(type::hash contexthash) {
			invoker::invoke<void>(0x444d8cf241ec25c5, contexthash);
		}
		static bool _0x84698ab38d0c6636(type::hash hash) {
			return invoker::invoke<bool>(0x84698ab38d0c6636, hash);
		}
		static type::any _0x2a25adc48f87841f() {
			return invoker::invoke<type::any>(0x2a25adc48f87841f);
		}
		static type::any _0xde03620f8703a9df() {
			return invoker::invoke<type::any>(0xde03620f8703a9df);
		}
		static type::any _0x359af31a4b52f5ed() {
			return invoker::invoke<type::any>(0x359af31a4b52f5ed);
		}
		static type::any _0x13c4b962653a5280() {
			return invoker::invoke<type::any>(0x13c4b962653a5280);
		}
		static bool _0xc8e1071177a23be5(type::any* p0, type::any* p1, type::any* p2) {
			return invoker::invoke<bool>(0xc8e1071177a23be5, p0, p1, p2);
		}
		static void enable_deathblood_seethrough(bool p0) {
			invoker::invoke<void>(0x4895bdea16e7c080, p0);
		}
		static void _0xc78e239ac5b2ddb9(bool p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0xc78e239ac5b2ddb9, p0, p1, p2);
		}
		static void _0xf06ebb91a81e09e3(bool p0) {
			invoker::invoke<void>(0xf06ebb91a81e09e3, p0);
		}
		static type::any _0x3bab9a4e4f2ff5c7() {
			return invoker::invoke<type::any>(0x3bab9a4e4f2ff5c7);
		}
		static void _0xec9264727eec0f28() {
			invoker::invoke<void>(0xec9264727eec0f28);
		}
		static void _0x14621bb1df14e2b2() {
			invoker::invoke<void>(0x14621bb1df14e2b2);
		}
		static type::any _0x66e7cb63c97b7d20() {
			return invoker::invoke<type::any>(0x66e7cb63c97b7d20);
		}
		static type::any _0x593feae1f73392d4() {
			return invoker::invoke<type::any>(0x593feae1f73392d4);
		}
		static type::any _0x4e3cd0ef8a489541() {
			return invoker::invoke<type::any>(0x4e3cd0ef8a489541);
		}
		static type::any _0xf284ac67940c6812() {
			return invoker::invoke<type::any>(0xf284ac67940c6812);
		}
		static bool _0x2e22fefa0100275e() {
			return invoker::invoke<bool>(0x2e22fefa0100275e);
		}
		static void _0x0cf54f20de43879c(type::any p0) {
			invoker::invoke<void>(0x0cf54f20de43879c, p0);
		}
		static void _0x36c1451a88a09630(type::any* p0, type::any* p1) {
			invoker::invoke<void>(0x36c1451a88a09630, p0, p1);
		}
		static void _0x7e17be53e1aaabaf(int* p0, int* p1, int* p2) {
			invoker::invoke<void>(0x7e17be53e1aaabaf, p0, p1, p2);
		}
		static bool _0xa238192f33110615(int* p0, int* p1, int* p2) {
			return invoker::invoke<bool>(0xa238192f33110615, p0, p1, p2);
		}
		static bool set_userids_uihidden(type::any p0, type::any* p1) {
			return invoker::invoke<bool>(0xef4ced81cebedc6d, p0, p1);
		}
		static bool _0xca6b2f7ce32ab653(type::any p0, type::any* p1, type::any p2) {
			return invoker::invoke<bool>(0xca6b2f7ce32ab653, p0, p1, p2);
		}
		static bool _0x90a6526cf0381030(type::any p0, type::any* p1, type::any p2, type::any p3) {
			return invoker::invoke<bool>(0x90a6526cf0381030, p0, p1, p2, p3);
		}
		static bool _0x24a49beaf468dc90(type::any p0, type::any* p1, type::any p2, type::any p3, type::any p4) {
			return invoker::invoke<bool>(0x24a49beaf468dc90, p0, p1, p2, p3, p4);
		}
		static bool _0x5fbd7095fe7ae57f(type::any p0, float* p1) {
			return invoker::invoke<bool>(0x5fbd7095fe7ae57f, p0, p1);
		}
		static bool _0x8f08017f9d7c47bd(type::any p0, type::any* p1, type::any p2) {
			return invoker::invoke<bool>(0x8f08017f9d7c47bd, p0, p1, p2);
		}
		static bool _0x052991e59076e4e4(type::hash p0, type::any* p1) {
			return invoker::invoke<bool>(0x052991e59076e4e4, p0, p1);
		}
		static void clear_ped_in_pause_menu() {
			invoker::invoke<void>(0x5e62be5dc58e9e06);
		}
		static void give_ped_to_pause_menu(type::ped ped, int p1) {
			invoker::invoke<void>(0xac0bfbdc3be00e14, ped, p1);
		}
		static void _0x3ca6050692bc61b0(bool p0) {
			invoker::invoke<void>(0x3ca6050692bc61b0, p0);
		}
		static void _0xecf128344e9ff9f1(bool p0) {
			invoker::invoke<void>(0xecf128344e9ff9f1, p0);
		}
		static void _show_social_club_legal_screen() {
			invoker::invoke<void>(0x805d7cbb36fd6c4c);
		}
		static type::any _0xf13fe2a80c05c561() {
			return invoker::invoke<type::any>(0xf13fe2a80c05c561);
		}
		static int _0x6f72cd94f7b5b68c() {
			return invoker::invoke<int>(0x6f72cd94f7b5b68c);
		}
		static void _0x75d3691713c3b05a() {
			invoker::invoke<void>(0x75d3691713c3b05a);
		}
		static void _0xd2b32be3fc1626c6() {
			invoker::invoke<void>(0xd2b32be3fc1626c6);
		}
		static void _0x9e778248d6685fe0(const char* p0) {
			invoker::invoke<void>(0x9e778248d6685fe0, p0);
		}
		static bool is_social_club_active() {
			return invoker::invoke<bool>(0xc406be343fc4b9af);
		}
		static void _0x1185a8087587322c(bool p0) {
			invoker::invoke<void>(0x1185a8087587322c, p0);
		}
		static void _0x8817605c2ba76200() {
			invoker::invoke<void>(0x8817605c2ba76200);
		}
		static bool _is_text_chat_active() {
			return invoker::invoke<bool>(0xb118af58b5f332a1);
		}
		static void _abort_text_chat() {
			invoker::invoke<void>(0x1ac8f4ad40e22127);
		}
		static void _set_text_chat_unk(bool p0) {
			invoker::invoke<void>(0x1db21a44b09e8ba3, p0);
		}
		static void _0xcef214315d276fd1(bool p0) {
			invoker::invoke<void>(0xcef214315d276fd1, p0);
		}
		static void _set_ped_ai_blip(int pedhandle, bool showviewcones) {
			invoker::invoke<void>(0xd30c50df888d58b5, pedhandle, showviewcones);
		}
		static bool does_ped_have_ai_blip(type::ped ped) {
			return invoker::invoke<bool>(0x15b8ecf844ee67ed, ped);
		}
		static void _set_ai_blip_type(type::ped ped, int type) {
			invoker::invoke<void>(0xe52b8e7f85d39a08, ped, type);
		}
		static void hide_special_ability_lockon_operation(type::any p0, bool p1) {
			invoker::invoke<void>(0x3eed80dff7325caa, p0, p1);
		}
		static void _is_ai_blip_always_shown(type::ped ped, bool flag) {
			invoker::invoke<void>(0x0c4bbf625ca98c4e, ped, flag);
		}
		static void _set_ai_blip_max_distance(type::ped ped, float distance) {
			invoker::invoke<void>(0x97c65887d4b37fa9, ped, distance);
		}
		static type::blip* _0x7cd934010e115c2c(type::ped ped) {
			return invoker::invoke<type::blip*>(0x7cd934010e115c2c, ped);
		}
		static type::blip _get_ai_blip(type::ped ped) {
			return invoker::invoke<type::blip>(0x56176892826a4fe8, ped);
		}
		static type::any _0xa277800a9eae340e() {
			return invoker::invoke<type::any>(0xa277800a9eae340e);
		}
		static void _0x2632482fd6b9ab87() {
			invoker::invoke<void>(0x2632482fd6b9ab87);
		}
		static void _set_director_mode(bool toggle) {
			invoker::invoke<void>(0x808519373fd336a3, toggle);
		}
		static void _0x04655f9d075d0ae5(bool p0) {
			invoker::invoke<void>(0x04655f9d075d0ae5, p0);
		}
	}

	namespace graphics {
		static void set_debug_lines_and_spheres_drawing_active(bool enabled) {
			invoker::invoke<void>(0x175b6bfc15cdd0c5, enabled);
		}
		static void draw_debug_line(float x1, float y1, float z1, float x2, float y2, float z2, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0x7fdfade676aa3cb0, x1, y1, z1, x2, y2, z2, red, green, blue, alpha);
		}
		static void draw_debug_line_with_two_colours(float x1, float y1, float z1, float x2, float y2, float z2, int r1, int g1, int b1, int r2, int g2, int b2, int alpha1, int alpha2) {
			invoker::invoke<void>(0xd8b9a8ac5608ff94, x1, y1, z1, x2, y2, z2, r1, g1, b1, r2, g2, b2, alpha1, alpha2);
		}
		static void draw_debug_sphere(float x, float y, float z, float radius, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0xaad68e1ab39da632, x, y, z, radius, red, green, blue, alpha);
		}
		static void draw_debug_box(float x1, float y1, float z1, float x2, float y2, float z2, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0x083a2ca4f2e573bd, x1, y1, z1, x2, y2, z2, red, green, blue, alpha);
		}
		static void draw_debug_cross(float x, float y, float z, float size, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0x73b1189623049839, x, y, z, size, red, green, blue, alpha);
		}
		static void draw_debug_text(const char* text, float x, float y, float z, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0x3903e216620488e8, text, x, y, z, red, green, blue, alpha);
		}
		static void draw_debug_text_2d(const char* text, float x, float y, float z, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0xa3bb2e9555c05a8f, text, x, y, z, red, green, blue, alpha);
		}
		static void draw_line(float x1, float y1, float z1, float x2, float y2, float z2, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0x6b7256074ae34680, x1, y1, z1, x2, y2, z2, red, green, blue, alpha);
		}
		static void draw_poly(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0xac26716048436851, x1, y1, z1, x2, y2, z2, x3, y3, z3, red, green, blue, alpha);
		}
		static void draw_box(float x1, float y1, float z1, float x2, float y2, float z2, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0xd3a9971cadac7252, x1, y1, z1, x2, y2, z2, red, green, blue, alpha);
		}
		static void _0x23ba6b0c2ad7b0d3(bool toggle) {
			invoker::invoke<void>(0x23ba6b0c2ad7b0d3, toggle);
		}
		static bool _0x1dd2139a9a20dce8() {
			return invoker::invoke<bool>(0x1dd2139a9a20dce8);
		}
		static int _0x90a78ecaa4e78453() {
			return invoker::invoke<int>(0x90a78ecaa4e78453);
		}
		static void _0x0a46af8a78dc5e0a() {
			invoker::invoke<void>(0x0a46af8a78dc5e0a);
		}
		static bool _0x4862437a486f91b0(const char* p0, type::any* p1, type::any* p2, bool p3) {
			return invoker::invoke<bool>(0x4862437a486f91b0, p0, p1, p2, p3);
		}
		static int _0x1670f8d05056f257(type::any p0) {
			return invoker::invoke<int>(0x1670f8d05056f257, p0);
		}
		static bool _0x7fa5d82b8f58ec06() {
			return invoker::invoke<bool>(0x7fa5d82b8f58ec06);
		}
		static int _0x5b0316762afd4a64() {
			return invoker::invoke<int>(0x5b0316762afd4a64);
		}
		static void _0x346ef3ecaaab149e() {
			invoker::invoke<void>(0x346ef3ecaaab149e);
		}
		static bool _0xa67c35c56eb1bd9d() {
			return invoker::invoke<bool>(0xa67c35c56eb1bd9d);
		}
		static int _0x0d6ca79eeebd8ca3() {
			return invoker::invoke<int>(0x0d6ca79eeebd8ca3);
		}
		static void _0xd801cc02177fa3f1() {
			invoker::invoke<void>(0xd801cc02177fa3f1);
		}
		static void _0x1bbc135a4d25edde(bool p0) {
			invoker::invoke<void>(0x1bbc135a4d25edde, p0);
		}
		static bool _0x3dec726c25a11bac(int p0) {
			return invoker::invoke<bool>(0x3dec726c25a11bac, p0);
		}
		static int _0x0c0c4e81e1ac60a0() {
			return invoker::invoke<int>(0x0c0c4e81e1ac60a0);
		}
		static bool _0x759650634f07b6b4(int p0) {
			return invoker::invoke<bool>(0x759650634f07b6b4, p0);
		}
		static int _0xcb82a0bf0e3e3265(int p0) {
			return invoker::invoke<int>(0xcb82a0bf0e3e3265, p0);
		}
		static void _0x6a12d88881435dca() {
			invoker::invoke<void>(0x6a12d88881435dca);
		}
		static void _0x1072f115dab0717e(bool p0, bool p1) {
			invoker::invoke<void>(0x1072f115dab0717e, p0, p1);
		}
		static int get_maximum_number_of_photos() {
			return invoker::invoke<int>(0x34d23450f028b0bf);
		}
		static int _get_maximum_number_of_photos_2() {
			return invoker::invoke<int>(0xdc54a7af8b3a14ef);
		}
		static int _get_number_of_photos() {
			return invoker::invoke<int>(0x473151ebc762c6da);
		}
		static bool _0x2a893980e96b659a(bool p0) {
			return invoker::invoke<bool>(0x2a893980e96b659a, p0);
		}
		static int _0xf5bed327cea362b1(bool p0) {
			return invoker::invoke<int>(0xf5bed327cea362b1, p0);
		}
		static void _0x4af92acd3141d96c() {
			invoker::invoke<void>(0x4af92acd3141d96c);
		}
		static type::any _0xe791df1f73ed2c8b(type::any p0) {
			return invoker::invoke<type::any>(0xe791df1f73ed2c8b, p0);
		}
		static type::any _0xec72c258667be5ea(type::any p0) {
			return invoker::invoke<type::any>(0xec72c258667be5ea, p0);
		}
		static int _return_two(type::any p0) {
			return invoker::invoke<int>(0x40afb081f8add4ee, p0);
		}
		static void _draw_light_with_range_and_shadow(float x, float y, float z, int r, int g, int b, float range, float intensity, float shadow) {
			invoker::invoke<void>(0xf49e9a9716a04595, x, y, z, r, g, b, range, intensity, shadow);
		}
		static void draw_light_with_range(float posx, float posy, float posz, int colorr, int colorg, int colorb, float range, float intensity) {
			invoker::invoke<void>(0xf2a1b2771a01dbd4, posx, posy, posz, colorr, colorg, colorb, range, intensity);
		}
		static void draw_spot_light(float posx, float posy, float posz, float dirx, float diry, float dirz, int colorr, int colorg, int colorb, float distance, float brightness, float hardness, float radius, float falloff) {
			invoker::invoke<void>(0xd0f64b265c8c8b33, posx, posy, posz, dirx, diry, dirz, colorr, colorg, colorb, distance, brightness, hardness, radius, falloff);
		}
		static void _draw_spot_light_with_shadow(float posx, float posy, float posz, float dirx, float diry, float dirz, int colorr, int colorg, int colorb, float distance, float brightness, float roundness, float radius, float falloff, int shadowid) {
			invoker::invoke<void>(0x5bca583a583194db, posx, posy, posz, dirx, diry, dirz, colorr, colorg, colorb, distance, brightness, roundness, radius, falloff, shadowid);
		}
		static void _0xc9b18b4619f48f7b(float p0) {
			invoker::invoke<void>(0xc9b18b4619f48f7b, p0);
		}
		static void _entity_description_text(type::entity entity) {
			invoker::invoke<void>(0xdeadc0dedeadc0de, entity);
		}
		static void draw_marker(int type, float posx, float posy, float posz, float dirx, float diry, float dirz, float rotx, float roty, float rotz, float scalex, float scaley, float scalez, int red, int green, int blue, int alpha, bool bobupanddown, bool facecamera, int p19, bool rotate, const char* texturedict, const char* texturename, bool drawonents) {
			invoker::invoke<void>(0x28477ec23d892089, type, posx, posy, posz, dirx, diry, dirz, rotx, roty, rotz, scalex, scaley, scalez, red, green, blue, alpha, bobupanddown, facecamera, p19, rotate, texturedict, texturename, drawonents);
		}
		static int create_checkpoint(int type, float posx1, float posy1, float posz1, float posx2, float posy2, float posz2, float radius, int red, int green, int blue, int alpha, int reserved) {
			return invoker::invoke<int>(0x0134f0835ab6bfcb, type, posx1, posy1, posz1, posx2, posy2, posz2, radius, red, green, blue, alpha, reserved);
		}
		static void _set_checkpoint_icon_height(int checkpoint, float iconheight) {
			invoker::invoke<void>(0x4b5b4da5d79f1943, checkpoint, iconheight);
		}
		static void set_checkpoint_cylinder_height(int checkpoint, float nearheight, float farheight, float radius) {
			invoker::invoke<void>(0x2707aae9d9297d89, checkpoint, nearheight, farheight, radius);
		}
		static void set_checkpoint_rgba(int checkpoint, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0x7167371e8ad747f7, checkpoint, red, green, blue, alpha);
		}
		static void _set_checkpoint_icon_rgba(int checkpoint, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0xb9ea40907c680580, checkpoint, red, green, blue, alpha);
		}
		static void _0xf51d36185993515d(int checkpoint, float posx, float posy, float posz, float unkx, float unky, float unkz) {
			invoker::invoke<void>(0xf51d36185993515d, checkpoint, posx, posy, posz, unkx, unky, unkz);
		}
		static void _0x615d3925e87a3b26(int checkpoint) {
			invoker::invoke<void>(0x615d3925e87a3b26, checkpoint);
		}
		static void delete_checkpoint(int checkpoint) {
			invoker::invoke<void>(0xf5ed37f54cd4d52e, checkpoint);
		}
		static void _0x22a249a53034450a(bool p0) {
			invoker::invoke<void>(0x22a249a53034450a, p0);
		}
		static void _0xdc459cfa0cce245b(bool p0) {
			invoker::invoke<void>(0xdc459cfa0cce245b, p0);
		}
		static void request_streamed_texture_dict(const char* texturedict, bool unused) {
			invoker::invoke<void>(0xdfa2ef8e04127dd5, texturedict, unused);
		}
		static bool has_streamed_texture_dict_loaded(const char* texturedict) {
			return invoker::invoke<bool>(0x0145f696aaaad2e4, texturedict);
		}
		static void set_streamed_texture_dict_as_no_longer_needed(const char* texturedict) {
			invoker::invoke<void>(0xbe2caccf5a8aa805, texturedict);
		}
		static void draw_rect(float x, float y, float width, float height, int r, int g, int b, int a) {
			invoker::invoke<void>(0x3a618a217e5154f0, x, y, width, height, r, g, b, a);
		}
		static void set_script_gfx_draw_behind_pausemenu(bool p0) {
			invoker::invoke<void>(0xc6372ecd45d73bcd, p0);
		}
		static void _set_ui_layer(int layer) {
			invoker::invoke<void>(0x61bb1d9b3a95d802, layer);
		}
		static void set_script_gfx_align(int x, int y) {
			invoker::invoke<void>(0xb8a850f20a067eb6, x, y);
		}
		static void reset_script_gfx_align() {
			invoker::invoke<void>(0xe3a3db414a373dab);
		}
		static void _screen_draw_position_ratio(float x, float y, float p2, float p3) {
			invoker::invoke<void>(0xf5a2c681787e579d, x, y, p2, p3);
		}
		static void _0x6dd8f5aa635eb4b2(float p0, float p1, float* p2, float* p3) {
			invoker::invoke<void>(0x6dd8f5aa635eb4b2, p0, p1, p2, p3);
		}
		static float get_safe_zone_size() {
			return invoker::invoke<float>(0xbaf107b6bb2c97f0);
		}
		static void draw_sprite(const char* texturedict, const char* texturename, float screenx, float screeny, float width, float height, float heading, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0xe7ffae5ebf23d890, texturedict, texturename, screenx, screeny, width, height, heading, red, green, blue, alpha);
		}
		static type::any add_entity_icon(type::entity entity, const char* icon) {
			return invoker::invoke<type::any>(0x9cd43eee12bf4dd0, entity, icon);
		}
		static void set_entity_icon_visibility(type::entity entity, bool toggle) {
			invoker::invoke<void>(0xe0e8beecca96ba31, entity, toggle);
		}
		static void set_entity_icon_color(type::entity entity, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0x1d5f595ccae2e238, entity, red, green, blue, alpha);
		}
		static void set_draw_origin(float x, float y, float z, type::any p3) {
			invoker::invoke<void>(0xaa0008f3bbb8f416, x, y, z, p3);
		}
		static void clear_draw_origin() {
			invoker::invoke<void>(0xff0b610f6be0d7af);
		}
		static void attach_tv_audio_to_entity(type::entity entity) {
			invoker::invoke<void>(0x845bad77cc770633, entity);
		}
		static void set_tv_audio_frontend(bool toggle) {
			invoker::invoke<void>(0x113d2c5dc57e1774, toggle);
		}
		static int load_movie_mesh_set(const char* moviemeshsetname) {
			return invoker::invoke<int>(0xb66064452270e8f1, moviemeshsetname);
		}
		static void release_movie_mesh_set(int moviemeshset) {
			invoker::invoke<void>(0xeb119aa014e89183, moviemeshset);
		}
		static type::any _0x9b6e70c5ceef4eeb(type::any p0) {
			return invoker::invoke<type::any>(0x9b6e70c5ceef4eeb, p0);
		}
		static void get_screen_resolution(int* x, int* y) {
			invoker::invoke<void>(0x888d57e407e63624, x, y);
		}
		static void _get_active_screen_resolution(int* x, int* y) {
			invoker::invoke<void>(0x873c9f3104101dd3, x, y);
		}
		static float _get_aspect_ratio(bool b) {
			return invoker::invoke<float>(0xf1307ef624a80d87, b);
		}
		static type::any _0xb2ebe8cbc58b90e9() {
			return invoker::invoke<type::any>(0xb2ebe8cbc58b90e9);
		}
		static bool get_is_widescreen() {
			return invoker::invoke<bool>(0x30cf4bda4fcb1905);
		}
		static bool get_is_hidef() {
			return invoker::invoke<bool>(0x84ed31191cc5d2c9);
		}
		static void _0xefabc7722293da7c() {
			invoker::invoke<void>(0xefabc7722293da7c);
		}
		static void set_nightvision(bool toggle) {
			invoker::invoke<void>(0x18f621f7a5b1f85d, toggle);
		}
		static type::any _0x35fb78dc42b7bd21() {
			return invoker::invoke<type::any>(0x35fb78dc42b7bd21);
		}
		static bool _is_nightvision_active() {
			return invoker::invoke<bool>(0x2202a3f42c8e5f79);
		}
		static void _0xef398beee4ef45f9(bool p0) {
			invoker::invoke<void>(0xef398beee4ef45f9, p0);
		}
		static void set_noiseoveride(bool toggle) {
			invoker::invoke<void>(0xe787bf1c5cf823c9, toggle);
		}
		static void set_noisinessoveride(float value) {
			invoker::invoke<void>(0xcb6a7c3bb17a0c67, value);
		}
		static bool get_screen_coord_from_world_coord(float worldx, float worldy, float worldz, float* screenx, float* screeny) {
			return invoker::invoke<bool>(0x34e82f05df2974f5, worldx, worldy, worldz, screenx, screeny);
		}
		static PVector3 get_texture_resolution(const char* texturedict, const char* texturename) {
			return invoker::invoke<PVector3>(0x35736ee65bd00c11, texturedict, texturename);
		}
		static void _0xe2892e7e55d7073a(float p0) {
			invoker::invoke<void>(0xe2892e7e55d7073a, p0);
		}
		static void set_flash(float p0, float p1, float fadein, float duration, float fadeout) {
			invoker::invoke<void>(0x0ab84296fed9cfc6, p0, p1, fadein, duration, fadeout);
		}
		static void _0x3669f1b198dcaa4f() {
			invoker::invoke<void>(0x3669f1b198dcaa4f);
		}
		static void set_artificial_lights_state(bool state) {
			invoker::invoke<void>(0x1268615ace24d504, state);
		}
		static void _0xc35a6d07c93802b2() {
			invoker::invoke<void>(0xc35a6d07c93802b2);
		}
		static int create_tracked_point() {
			return invoker::invoke<int>(0xe2c9439ed45dea60);
		}
		static void set_tracked_point_info(int point, float x, float y, float z, float radius) {
			invoker::invoke<void>(0x164ecbb3cf750cb0, point, x, y, z, radius);
		}
		static bool is_tracked_point_visible(int point) {
			return invoker::invoke<bool>(0xc45ccdaac9221ca8, point);
		}
		static void destroy_tracked_point(int point) {
			invoker::invoke<void>(0xb25dc90bad56ca42, point);
		}
		static type::any _0xbe197eaa669238f4(type::any p0, type::any p1, type::any p2, type::any p3) {
			return invoker::invoke<type::any>(0xbe197eaa669238f4, p0, p1, p2, p3);
		}
		static void _0x61f95e5bb3e0a8c6(type::any p0) {
			invoker::invoke<void>(0x61f95e5bb3e0a8c6, p0);
		}
		static void _0xae51bc858f32ba66(type::any p0, float p1, float p2, float p3, float p4) {
			invoker::invoke<void>(0xae51bc858f32ba66, p0, p1, p2, p3, p4);
		}
		static void _0x649c97d52332341a(type::any p0) {
			invoker::invoke<void>(0x649c97d52332341a, p0);
		}
		static type::any _0x2c42340f916c5930(type::any p0) {
			return invoker::invoke<type::any>(0x2c42340f916c5930, p0);
		}
		static void _0x14fc5833464340a8() {
			invoker::invoke<void>(0x14fc5833464340a8);
		}
		static void _0x0218ba067d249dea() {
			invoker::invoke<void>(0x0218ba067d249dea);
		}
		static void _0x1612c45f9e3e0d44() {
			invoker::invoke<void>(0x1612c45f9e3e0d44);
		}
		static void _0x5debd9c4dc995692() {
			invoker::invoke<void>(0x5debd9c4dc995692);
		}
		static void _0x6d955f6a9e0295b1(float x, float y, float z, float p3, float p4, float p5, float p6) {
			invoker::invoke<void>(0x6d955f6a9e0295b1, x, y, z, p3, p4, p5, p6);
		}
		static void _0x302c91ab2d477f7e() {
			invoker::invoke<void>(0x302c91ab2d477f7e);
		}
		static void _0x03fc694ae06c5a20() {
			invoker::invoke<void>(0x03fc694ae06c5a20);
		}
		static void _0xd2936cab8b58fcbd(int p0, bool p1, float x, float y, float z, float p5, bool p6, bool p7) {
			invoker::invoke<void>(0xd2936cab8b58fcbd, p0, p1, x, y, z, p5, p6, p7);
		}
		static void _0x5f0f3f56635809ef(float p0) {
			invoker::invoke<void>(0x5f0f3f56635809ef, p0);
		}
		static void _0x5e9daf5a20f15908(float p0) {
			invoker::invoke<void>(0x5e9daf5a20f15908, p0);
		}
		static void _0x36f6626459d91457(float p0) {
			invoker::invoke<void>(0x36f6626459d91457, p0);
		}
		static void _set_far_shadows_suppressed(bool toggle) {
			invoker::invoke<void>(0x80ecbc0c856d3b0b, toggle);
		}
		static void _0x25fc3e33a31ad0c9(bool p0) {
			invoker::invoke<void>(0x25fc3e33a31ad0c9, p0);
		}
		static void _0xb11d94bc55f41932(const char* p0) {
			invoker::invoke<void>(0xb11d94bc55f41932, p0);
		}
		static void _0x27cb772218215325() {
			invoker::invoke<void>(0x27cb772218215325);
		}
		static void _0x6ddbf9dffc4ac080(bool p0) {
			invoker::invoke<void>(0x6ddbf9dffc4ac080, p0);
		}
		static void _0xd39d13c9febf0511(bool p0) {
			invoker::invoke<void>(0xd39d13c9febf0511, p0);
		}
		static void _0x02ac28f3a01fa04a(float p0) {
			invoker::invoke<void>(0x02ac28f3a01fa04a, p0);
		}
		static void _0x0ae73d8df3a762b2(bool p0) {
			invoker::invoke<void>(0x0ae73d8df3a762b2, p0);
		}
		static void _0xa51c4b86b71652ae(bool p0) {
			invoker::invoke<void>(0xa51c4b86b71652ae, p0);
		}
		static void _0x312342e1a4874f3f(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, bool p8) {
			invoker::invoke<void>(0x312342e1a4874f3f, p0, p1, p2, p3, p4, p5, p6, p7, p8);
		}
		static void _0x2485d34e50a22e84(float p0, float p1, float p2) {
			invoker::invoke<void>(0x2485d34e50a22e84, p0, p1, p2);
		}
		static void _0x12995f2e53ffa601(int p0, int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10, int p11) {
			invoker::invoke<void>(0x12995f2e53ffa601, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
		}
		static void _0xdbaa5ec848ba2d46(type::any p0, type::any p1) {
			invoker::invoke<void>(0xdbaa5ec848ba2d46, p0, p1);
		}
		static void _0xc0416b061f2b7e5e(bool p0) {
			invoker::invoke<void>(0xc0416b061f2b7e5e, p0);
		}
		static void _0xb1bb03742917a5d6(int type, float xpos, float ypos, float zpos, float p4, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0xb1bb03742917a5d6, type, xpos, ypos, zpos, p4, red, green, blue, alpha);
		}
		static void _0x9cfdd90b2b844bf7(float p0, float p1, float p2, float p3, float p4) {
			invoker::invoke<void>(0x9cfdd90b2b844bf7, p0, p1, p2, p3, p4);
		}
		static void _0x06f761ea47c1d3ed(bool p0) {
			invoker::invoke<void>(0x06f761ea47c1d3ed, p0);
		}
		static type::any _0xa4819f5e23e2ffad() {
			return invoker::invoke<type::any>(0xa4819f5e23e2ffad);
		}
		static PVector3 _0xa4664972a9b8f8ba(type::any p0) {
			return invoker::invoke<PVector3>(0xa4664972a9b8f8ba, p0);
		}
		static void set_seethrough(bool toggle) {
			invoker::invoke<void>(0x7e08924259e08ce0, toggle);
		}
		static bool _is_seethrough_active() {
			return invoker::invoke<bool>(0x44b80abab9d80bd3);
		}
		static void _0xd7d0b00177485411(type::any p0, float p1) {
			invoker::invoke<void>(0xd7d0b00177485411, p0, p1);
		}
		static void _0xb3c641f3630bf6da(float p0) {
			invoker::invoke<void>(0xb3c641f3630bf6da, p0);
		}
		static type::any _0xe59343e9e96529e7() {
			return invoker::invoke<type::any>(0xe59343e9e96529e7);
		}
		static void _0xe63d7c6eececb66b(bool p0) {
			invoker::invoke<void>(0xe63d7c6eececb66b, p0);
		}
		static void _0xe3e2c1b4c59dbc77(int unk) {
			invoker::invoke<void>(0xe3e2c1b4c59dbc77, unk);
		}
		static bool _transition_to_blurred(float transitiontime) {
			return invoker::invoke<bool>(0xa328a24aaa6b7fdc, transitiontime);
		}
		static bool _transition_from_blurred(float transitiontime) {
			return invoker::invoke<bool>(0xefacc8aef94430d5, transitiontime);
		}
		static void _0xde81239437e8c5a8() {
			invoker::invoke<void>(0xde81239437e8c5a8);
		}
		static float is_particle_fx_delayed_blink() {
			return invoker::invoke<float>(0x5ccabffca31dde33);
		}
		static type::any _0x7b226c785a52a0a9() {
			return invoker::invoke<type::any>(0x7b226c785a52a0a9);
		}
		static void _toggle_pause_render_phases(bool toggle) {
			invoker::invoke<void>(0xdfc252d8a3e15ab7, toggle);
		}
		static bool _0xeb3dac2c86001e5e() {
			return invoker::invoke<bool>(0xeb3dac2c86001e5e);
		}
		static void _0xe1c8709406f2c41c() {
			invoker::invoke<void>(0xe1c8709406f2c41c);
		}
		static void _0x851cd923176eba7c() {
			invoker::invoke<void>(0x851cd923176eba7c);
		}
		static void _0xba3d65906822bed5(bool p0, bool p1, float p2, float p3, float p4, float p5) {
			invoker::invoke<void>(0xba3d65906822bed5, p0, p1, p2, p3, p4, p5);
		}
		static bool _0x7ac24eab6d74118d(bool p0) {
			return invoker::invoke<bool>(0x7ac24eab6d74118d, p0);
		}
		static type::any _0xbcedb009461da156() {
			return invoker::invoke<type::any>(0xbcedb009461da156);
		}
		static bool _0x27feb5254759cde3(const char* texturedict, bool p1) {
			return invoker::invoke<bool>(0x27feb5254759cde3, texturedict, p1);
		}
		static int start_particle_fx_non_looped_at_coord(const char* effectname, float xpos, float ypos, float zpos, float xrot, float yrot, float zrot, float scale, bool xaxis, bool yaxis, bool zaxis) {
			return invoker::invoke<int>(0x25129531f77b9ed3, effectname, xpos, ypos, zpos, xrot, yrot, zrot, scale, xaxis, yaxis, zaxis);
		}
		static bool start_networked_particle_fx_non_looped_at_coord(const char* effectname, float xpos, float ypos, float zpos, float xrot, float yrot, float zrot, float scale, bool xaxis, bool yaxis, bool zaxis) {
			return invoker::invoke<bool>(0xf56b8137df10135d, effectname, xpos, ypos, zpos, xrot, yrot, zrot, scale, xaxis, yaxis, zaxis);
		}
		static bool start_particle_fx_non_looped_on_ped_bone(const char* effectname, type::ped ped, float offsetx, float offsety, float offsetz, float rotx, float roty, float rotz, int boneindex, float scale, bool axisx, bool axisy, bool axisz) {
			return invoker::invoke<bool>(0x0e7e72961ba18619, effectname, ped, offsetx, offsety, offsetz, rotx, roty, rotz, boneindex, scale, axisx, axisy, axisz);
		}
		static bool start_networked_particle_fx_non_looped_on_ped_bone(const char* effectname, type::ped ped, float offsetx, float offsety, float offsetz, float rotx, float roty, float rotz, int boneindex, float scale, bool axisx, bool axisy, bool axisz) {
			return invoker::invoke<bool>(0xa41b6a43642ac2cf, effectname, ped, offsetx, offsety, offsetz, rotx, roty, rotz, boneindex, scale, axisx, axisy, axisz);
		}
		static bool start_particle_fx_non_looped_on_entity(const char* effectname, type::entity entity, float offsetx, float offsety, float offsetz, float rotx, float roty, float rotz, float scale, bool axisx, bool axisy, bool axisz) {
			return invoker::invoke<bool>(0x0d53a3b8da0809d2, effectname, entity, offsetx, offsety, offsetz, rotx, roty, rotz, scale, axisx, axisy, axisz);
		}
		static bool _start_networked_particle_fx_non_looped_on_entity(const char* effectname, type::entity entity, float offsetx, float offsety, float offsetz, float rotx, float roty, float rotz, float scale, bool axisx, bool axisy, bool axisz) {
			return invoker::invoke<bool>(0xc95eb1db6e92113d, effectname, entity, offsetx, offsety, offsetz, rotx, roty, rotz, scale, axisx, axisy, axisz);
		}
		static void set_particle_fx_non_looped_colour(float r, float g, float b) {
			invoker::invoke<void>(0x26143a59ef48b262, r, g, b);
		}
		static void set_particle_fx_non_looped_alpha(float alpha) {
			invoker::invoke<void>(0x77168d722c58b2fc, alpha);
		}
		static void _0x8cde909a0370bb3a(bool p0) {
			invoker::invoke<void>(0x8cde909a0370bb3a, p0);
		}
		static int start_particle_fx_looped_at_coord(const char* effectname, float x, float y, float z, float xrot, float yrot, float zrot, float scale, bool xaxis, bool yaxis, bool zaxis, bool p11) {
			return invoker::invoke<int>(0xe184f4f0dc5910e7, effectname, x, y, z, xrot, yrot, zrot, scale, xaxis, yaxis, zaxis, p11);
		}
		static int start_particle_fx_looped_on_ped_bone(const char* effectname, type::ped ped, float xoffset, float yoffset, float zoffset, float xrot, float yrot, float zrot, int boneindex, float scale, bool xaxis, bool yaxis, bool zaxis) {
			return invoker::invoke<int>(0xf28da9f38cd1787c, effectname, ped, xoffset, yoffset, zoffset, xrot, yrot, zrot, boneindex, scale, xaxis, yaxis, zaxis);
		}
		static int start_particle_fx_looped_on_entity(const char* effectname, type::entity entity, float xoffset, float yoffset, float zoffset, float xrot, float yrot, float zrot, float scale, bool xaxis, bool yaxis, bool zaxis) {
			return invoker::invoke<int>(0x1ae42c1660fd6517, effectname, entity, xoffset, yoffset, zoffset, xrot, yrot, zrot, scale, xaxis, yaxis, zaxis);
		}
		static int _start_particle_fx_looped_on_entity_bone(const char* effectname, type::entity entity, float xoffset, float yoffset, float zoffset, float xrot, float yrot, float zrot, int boneindex, float scale, bool xaxis, bool yaxis, bool zaxis) {
			return invoker::invoke<int>(0xc6eb449e33977f0b, effectname, entity, xoffset, yoffset, zoffset, xrot, yrot, zrot, boneindex, scale, xaxis, yaxis, zaxis);
		}
		static int start_networked_particle_fx_looped_on_entity(const char* effectname, type::entity entity, float xoffset, float yoffset, float zoffset, float xrot, float yrot, float zrot, float scale, bool xaxis, bool yaxis, bool zaxis) {
			return invoker::invoke<int>(0x6f60e89a7b64ee1d, effectname, entity, xoffset, yoffset, zoffset, xrot, yrot, zrot, scale, xaxis, yaxis, zaxis);
		}
		static int _start_networked_particle_fx_looped_on_entity_bone(const char* effectname, type::entity entity, float xoffset, float yoffset, float zoffset, float xrot, float yrot, float zrot, int boneindex, float scale, bool xaxis, bool yaxis, bool zaxis) {
			return invoker::invoke<int>(0xdde23f30cc5a0f03, effectname, entity, xoffset, yoffset, zoffset, xrot, yrot, zrot, boneindex, scale, xaxis, yaxis, zaxis);
		}
		static void stop_particle_fx_looped(int ptfxhandle, bool p1) {
			invoker::invoke<void>(0x8f75998877616996, ptfxhandle, p1);
		}
		static void remove_particle_fx(int ptfxhandle, bool p1) {
			invoker::invoke<void>(0xc401503dfe8d53cf, ptfxhandle, p1);
		}
		static void remove_particle_fx_from_entity(type::entity entity) {
			invoker::invoke<void>(0xb8feaeebcc127425, entity);
		}
		static void remove_particle_fx_in_range(float x, float y, float z, float radius) {
			invoker::invoke<void>(0xdd19fa1c6d657305, x, y, z, radius);
		}
		static bool does_particle_fx_looped_exist(int ptfxhandle) {
			return invoker::invoke<bool>(0x74afef0d2e1e409b, ptfxhandle);
		}
		static void set_particle_fx_looped_offsets(int ptfxhandle, float x, float y, float z, float rotx, float roty, float rotz) {
			invoker::invoke<void>(0xf7ddebec43483c43, ptfxhandle, x, y, z, rotx, roty, rotz);
		}
		static void set_particle_fx_looped_evolution(int ptfxhandle, const char* propertyname, float amount, bool id) {
			invoker::invoke<void>(0x5f0c4b5b1c393be2, ptfxhandle, propertyname, amount, id);
		}
		static void set_particle_fx_looped_colour(int ptfxhandle, float r, float g, float b, bool p4) {
			invoker::invoke<void>(0x7f8f65877f88783b, ptfxhandle, r, g, b, p4);
		}
		static void set_particle_fx_looped_alpha(int ptfxhandle, float alpha) {
			invoker::invoke<void>(0x726845132380142e, ptfxhandle, alpha);
		}
		static void set_particle_fx_looped_scale(int ptfxhandle, float scale) {
			invoker::invoke<void>(0xb44250aaa456492d, ptfxhandle, scale);
		}
		static void set_particle_fx_looped_far_clip_dist(int ptfxhandle, float dist) {
			invoker::invoke<void>(0xdcb194b85ef7b541, ptfxhandle, dist);
		}
		static void set_particle_fx_cam_inside_vehicle(bool p0) {
			invoker::invoke<void>(0xeec4047028426510, p0);
		}
		static void set_particle_fx_cam_inside_nonplayer_vehicle(type::any p0, bool p1) {
			invoker::invoke<void>(0xacee6f360fc1f6b6, p0, p1);
		}
		static void set_particle_fx_shootout_boat(type::any p0) {
			invoker::invoke<void>(0x96ef97daeb89bef5, p0);
		}
		static void set_particle_fx_blood_scale(bool p0) {
			invoker::invoke<void>(0x5f6df3d92271e8a1, p0);
		}
		static void enable_clown_blood_vfx(bool toggle) {
			invoker::invoke<void>(0xd821490579791273, toggle);
		}
		static void enable_alien_blood_vfx(bool toggle) {
			invoker::invoke<void>(0x9dce1f0f78260875, toggle);
		}
		static void _0x27e32866e9a5c416(float p0) {
			invoker::invoke<void>(0x27e32866e9a5c416, p0);
		}
		static void _0xbb90e12cac1dab25(float p0) {
			invoker::invoke<void>(0xbb90e12cac1dab25, p0);
		}
		static void _0xca4ae345a153d573(bool p0) {
			invoker::invoke<void>(0xca4ae345a153d573, p0);
		}
		static void _0x54e22ea2c1956a8d(float p0) {
			invoker::invoke<void>(0x54e22ea2c1956a8d, p0);
		}
		static void _0x949f397a288b28b3(float p0) {
			invoker::invoke<void>(0x949f397a288b28b3, p0);
		}
		static void _0x9b079e5221d984d3(bool p0) {
			invoker::invoke<void>(0x9b079e5221d984d3, p0);
		}
		static void use_particle_fx_asset(const char* name) {
			invoker::invoke<void>(0x6c38af3693a69a91, name);
		}
		static void _set_particle_fx_asset_old_to_new(const char* oldasset, const char* newasset) {
			invoker::invoke<void>(0xea1e2d93f6f75ed9, oldasset, newasset);
		}
		static void _reset_particle_fx_asset_old_to_new(const char* name) {
			invoker::invoke<void>(0x89c8553dd3274aae, name);
		}
		static void _0xa46b73faa3460ae1(bool p0) {
			invoker::invoke<void>(0xa46b73faa3460ae1, p0);
		}
		static void _0xf78b803082d4386f(float p0) {
			invoker::invoke<void>(0xf78b803082d4386f, p0);
		}
		static void wash_decals_in_range(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4) {
			invoker::invoke<void>(0x9c30613d50a6adef, p0, p1, p2, p3, p4);
		}
		static void wash_decals_from_vehicle(type::vehicle vehicle, float p1) {
			invoker::invoke<void>(0x5b712761429dbc14, vehicle, p1);
		}
		static void fade_decals_in_range(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4) {
			invoker::invoke<void>(0xd77edadb0420e6e0, p0, p1, p2, p3, p4);
		}
		static void remove_decals_in_range(float x, float y, float z, float range) {
			invoker::invoke<void>(0x5d6b2d4830a67c62, x, y, z, range);
		}
		static void remove_decals_from_object(type::object obj) {
			invoker::invoke<void>(0xccf71cbddf5b6cb9, obj);
		}
		static void remove_decals_from_object_facing(type::object obj, float x, float y, float z) {
			invoker::invoke<void>(0xa6f6f70fdc6d144c, obj, x, y, z);
		}
		static void remove_decals_from_vehicle(type::vehicle vehicle) {
			invoker::invoke<void>(0xe91f1b65f2b48d57, vehicle);
		}
		static type::object add_decal(int decaltype, float posx, float posy, float posz, float p4, float p5, float p6, float p7, float p8, float p9, float width, float height, float rcoef, float gcoef, float bcoef, float opacity, float timeout, bool p17, bool p18, bool p19) {
			return invoker::invoke<type::object>(0xb302244a1839bdad, decaltype, posx, posy, posz, p4, p5, p6, p7, p8, p9, width, height, rcoef, gcoef, bcoef, opacity, timeout, p17, p18, p19);
		}
		static type::any add_petrol_decal(float x, float y, float z, float groundlvl, float width, float transparency) {
			return invoker::invoke<type::any>(0x4f5212c7ad880df8, x, y, z, groundlvl, width, transparency);
		}
		static void _0x99ac7f0d8b9c893d(float p0) {
			invoker::invoke<void>(0x99ac7f0d8b9c893d, p0);
		}
		static void _0x967278682cb6967a(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x967278682cb6967a, p0, p1, p2, p3);
		}
		static void _0x0a123435a26c36cd() {
			invoker::invoke<void>(0x0a123435a26c36cd);
		}
		static void remove_decal(type::object decal) {
			invoker::invoke<void>(0xed3f346429ccd659, decal);
		}
		static bool is_decal_alive(type::object decal) {
			return invoker::invoke<bool>(0xc694d74949cafd0c, decal);
		}
		static float get_decal_wash_level(int decal) {
			return invoker::invoke<float>(0x323f647679a09103, decal);
		}
		static void _0xd9454b5752c857dc() {
			invoker::invoke<void>(0xd9454b5752c857dc);
		}
		static void _0x27cfb1b1e078cb2d() {
			invoker::invoke<void>(0x27cfb1b1e078cb2d);
		}
		static void _0x4b5cfc83122df602() {
			invoker::invoke<void>(0x4b5cfc83122df602);
		}
		static bool _0x2f09f7976c512404(float xcoord, float ycoord, float zcoord, float p3) {
			return invoker::invoke<bool>(0x2f09f7976c512404, xcoord, ycoord, zcoord, p3);
		}
		static void _override_decal_texture(int decaltype, const char* texturedict, const char* texturename) {
			invoker::invoke<void>(0x8a35c742130c6080, decaltype, texturedict, texturename);
		}
		static void _0xb7ed70c49521a61d(int decaltype) {
			invoker::invoke<void>(0xb7ed70c49521a61d, decaltype);
		}
		static void move_vehicle_decals(type::vehicle vehicle1, type::vehicle vehicle2) {
			invoker::invoke<void>(0x84c8d7c2d30d3280, vehicle1, vehicle2);
		}
		static bool _add_clan_decal_to_vehicle(type::vehicle vehicle, type::ped ped, int boneindex, float x1, float x2, float x3, float y1, float y2, float y3, float z1, float z2, float z3, float scale, int decalindex, int alpha) {
			return invoker::invoke<bool>(0x428bdcb9da58da53, vehicle, ped, boneindex, x1, x2, x3, y1, y2, y3, z1, z2, z3, scale, decalindex, alpha);
		}
		static void _0xd2300034310557e4(type::vehicle vehicle, type::any p1) {
			invoker::invoke<void>(0xd2300034310557e4, vehicle, p1);
		}
		static int _0xfe26117a5841b2ff(type::vehicle vehicle, type::any p1) {
			return invoker::invoke<int>(0xfe26117a5841b2ff, vehicle, p1);
		}
		static bool _does_vehicle_have_decal(type::vehicle vehicle, int decalindex) {
			return invoker::invoke<bool>(0x060d935d3981a275, vehicle, decalindex);
		}
		static void _0x0e4299c549f0d1f1(bool p0) {
			invoker::invoke<void>(0x0e4299c549f0d1f1, p0);
		}
		static void _0x02369d5c8a51fdcf(bool p0) {
			invoker::invoke<void>(0x02369d5c8a51fdcf, p0);
		}
		static void _0x46d1a61a21f566fc(float p0) {
			invoker::invoke<void>(0x46d1a61a21f566fc, p0);
		}
		static void _0x2a2a52824db96700(type::any* p0) {
			invoker::invoke<void>(0x2a2a52824db96700, p0);
		}
		static void _0x1600fd8cf72ebc12(float p0) {
			invoker::invoke<void>(0x1600fd8cf72ebc12, p0);
		}
		static void _0xefb55e7c25d3b3be() {
			invoker::invoke<void>(0xefb55e7c25d3b3be);
		}
		static void _0xa44ff770dfbc5dae() {
			invoker::invoke<void>(0xa44ff770dfbc5dae);
		}
		static void disable_vehicle_distantlights(bool toggle) {
			invoker::invoke<void>(0xc9f98ac1884e73a2, toggle);
		}
		static void _0x03300b57fcac6ddb(bool p0) {
			invoker::invoke<void>(0x03300b57fcac6ddb, p0);
		}
		static void _0x98edf76a7271e4f2() {
			invoker::invoke<void>(0x98edf76a7271e4f2);
		}
		static void _set_force_ped_footsteps_tracks(bool toggle) {
			invoker::invoke<void>(0xaeedad1420c65cc0, toggle);
		}
		static void _set_force_vehicle_trails(bool toggle) {
			invoker::invoke<void>(0x4cc7f0fea5283fe0, toggle);
		}
		static void _preset_interior_ambient_cache(const char* tcmodifiername) {
			invoker::invoke<void>(0xd7021272eb0a451e, tcmodifiername);
		}
		static void set_timecycle_modifier(const char* modifiername) {
			invoker::invoke<void>(0x2c933abf17a1df41, modifiername);
		}
		static void set_timecycle_modifier_strength(float strength) {
			invoker::invoke<void>(0x82e7ffcd5b2326b3, strength);
		}
		static void set_transition_timecycle_modifier(const char* modifiername, float transition) {
			invoker::invoke<void>(0x3bcf567485e1971c, modifiername, transition);
		}
		static void _0x1cba05ae7bd7ee05(float p0) {
			invoker::invoke<void>(0x1cba05ae7bd7ee05, p0);
		}
		static void clear_timecycle_modifier() {
			invoker::invoke<void>(0x0f07e7745a236711);
		}
		static int get_timecycle_modifier_index() {
			return invoker::invoke<int>(0xfdf3d97c674afb66);
		}
		static type::any _0x459fd2c8d0ab78bc() {
			return invoker::invoke<type::any>(0x459fd2c8d0ab78bc);
		}
		static void push_timecycle_modifier() {
			invoker::invoke<void>(0x58f735290861e6b4);
		}
		static void pop_timecycle_modifier() {
			invoker::invoke<void>(0x3c8938d7d872211e);
		}
		static void _0xbbf327ded94e4deb(const char* p0) {
			invoker::invoke<void>(0xbbf327ded94e4deb, p0);
		}
		static void _0xbdeb86f4d5809204(float p0) {
			invoker::invoke<void>(0xbdeb86f4d5809204, p0);
		}
		static void _0xbf59707b3e5ed531(const char* p0) {
			invoker::invoke<void>(0xbf59707b3e5ed531, p0);
		}
		static void _0x1a8e2c8b9cf4549c(type::any* p0, type::any* p1) {
			invoker::invoke<void>(0x1a8e2c8b9cf4549c, p0, p1);
		}
		static void _0x15e33297c3e8dc60(type::any p0) {
			invoker::invoke<void>(0x15e33297c3e8dc60, p0);
		}
		static void _0x5096fd9ccb49056d(const char* p0) {
			invoker::invoke<void>(0x5096fd9ccb49056d, p0);
		}
		static void _0x92ccc17a7a2285da() {
			invoker::invoke<void>(0x92ccc17a7a2285da);
		}
		static type::any _0xbb0527ec6341496d() {
			return invoker::invoke<type::any>(0xbb0527ec6341496d);
		}
		static void _0x2c328af17210f009(float p0) {
			invoker::invoke<void>(0x2c328af17210f009, p0);
		}
		static void _0x2bf72ad5b41aa739() {
			invoker::invoke<void>(0x2bf72ad5b41aa739);
		}
		static int request_scaleform_movie(const char* scaleformname) {
			return invoker::invoke<int>(0x11fe353cf9733e6f, scaleformname);
		}
		static int request_scaleform_movie_instance(const char* scaleformname) {
			return invoker::invoke<int>(0xc514489cfb8af806, scaleformname);
		}
		static int _request_scaleform_movie_interactive(const char* scaleformname) {
			return invoker::invoke<int>(0xbd06c611bb9048c2, scaleformname);
		}
		static bool has_scaleform_movie_loaded(int scaleformhandle) {
			return invoker::invoke<bool>(0x85f01b8d5b90570e, scaleformhandle);
		}
		static bool _has_named_scaleform_movie_loaded(const char* scaleformname) {
			return invoker::invoke<bool>(0x0c1c5d756fb5f337, scaleformname);
		}
		static bool has_scaleform_container_movie_loaded_into_parent(int scaleformhandle) {
			return invoker::invoke<bool>(0x8217150e1217ebfd, scaleformhandle);
		}
		static void set_scaleform_movie_as_no_longer_needed(int* scaleformhandle) {
			invoker::invoke<void>(0x1d132d614dd86811, scaleformhandle);
		}
		static void set_scaleform_movie_to_use_system_time(int scaleform, bool toggle) {
			invoker::invoke<void>(0x6d8eb211944dce08, scaleform, toggle);
		}
		static void draw_scaleform_movie(int scaleformhandle, float x, float y, float width, float height, int red, int green, int blue, int alpha, int unk) {
			invoker::invoke<void>(0x54972adaf0294a93, scaleformhandle, x, y, width, height, red, green, blue, alpha, unk);
		}
		static void draw_scaleform_movie_fullscreen(int scaleform, int red, int green, int blue, int alpha, int unk) {
			invoker::invoke<void>(0x0df606929c105be1, scaleform, red, green, blue, alpha, unk);
		}
		static void draw_scaleform_movie_fullscreen_masked(int scaleform1, int scaleform2, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0xcf537fde4fbd4ce5, scaleform1, scaleform2, red, green, blue, alpha);
		}
		static void draw_scaleform_movie_3d(int scaleform, float posx, float posy, float posz, float rotx, float roty, float rotz, float p7, float sharpness, float p9, float scalex, float scaley, float scalez, type::any p13) {
			invoker::invoke<void>(0x87d51d72255d4e78, scaleform, posx, posy, posz, rotx, roty, rotz, p7, sharpness, p9, scalex, scaley, scalez, p13);
		}
		static void _draw_scaleform_movie_3d_non_additive(int scaleform, float posx, float posy, float posz, float rotx, float roty, float rotz, float p7, float p8, float p9, float scalex, float scaley, float scalez, type::any p13) {
			invoker::invoke<void>(0x1ce592fdc749d6f5, scaleform, posx, posy, posz, rotx, roty, rotz, p7, p8, p9, scalex, scaley, scalez, p13);
		}
		static void call_scaleform_movie_method(int scaleform, const char* method) {
			invoker::invoke<void>(0xfbd96d87ac96d533, scaleform, method);
		}
		static void _call_scaleform_movie_function_float_params(int scaleform, const char* functionname, float param1, float param2, float param3, float param4, float param5) {
			invoker::invoke<void>(0xd0837058ae2e4bee, scaleform, functionname, param1, param2, param3, param4, param5);
		}
		static void _call_scaleform_movie_function_string_params(int scaleform, const char* functionname, const char* param1, const char* param2, const char* param3, const char* param4, const char* param5) {
			invoker::invoke<void>(0x51bc1ed3cc44e8f7, scaleform, functionname, param1, param2, param3, param4, param5);
		}
		static void _call_scaleform_movie_function_mixed_params(int scaleform, const char* functionname, float floatparam1, float floatparam2, float floatparam3, float floatparam4, float floatparam5, const char* stringparam1, const char* stringparam2, const char* stringparam3, const char* stringparam4, const char* stringparam5) {
			invoker::invoke<void>(0xef662d8d57e290b1, scaleform, functionname, floatparam1, floatparam2, floatparam3, floatparam4, floatparam5, stringparam1, stringparam2, stringparam3, stringparam4, stringparam5);
		}
		static bool _begin_scaleform_movie_method_hud_component(int hudcomponent, const char* functionname) {
			return invoker::invoke<bool>(0x98c494fd5bdfbfd5, hudcomponent, functionname);
		}
		static bool begin_scaleform_movie_method(int scaleform, const char* functionname) {
			return invoker::invoke<bool>(0xf6e48914c7a8694e, scaleform, functionname);
		}
		static bool _begin_scaleform_movie_method_n(const char* functionname) {
			return invoker::invoke<bool>(0xab58c27c2e6123c6, functionname);
		}
		static bool _begin_scaleform_movie_method_v(const char* functionname) {
			return invoker::invoke<bool>(0xb9449845f73f5e9c, functionname);
		}
		static void end_scaleform_movie_method() {
			invoker::invoke<void>(0xc6796a8ffa375e53);
		}
		static int _end_scaleform_movie_method_return() {
			return invoker::invoke<int>(0xc50aa39a577af886);
		}
		static bool _get_scaleform_movie_function_return_bool(int method_return) {
			return invoker::invoke<bool>(0x768ff8961ba904d6, method_return);
		}
		static int _get_scaleform_movie_function_return_int(int method_return) {
			return invoker::invoke<int>(0x2de7efa66b906036, method_return);
		}
		static const char* sitting_tv(int method_return) {
			return invoker::invoke<const char*>(0xe1e258829a885245, method_return);
		}
		static void _add_scaleform_movie_method_parameter_int(int value) {
			invoker::invoke<void>(0xc3d0841a0cc546a6, value);
		}
		static void _add_scaleform_movie_method_parameter_float(float value) {
			invoker::invoke<void>(0xd69736aae04db51a, value);
		}
		static void _add_scaleform_movie_method_parameter_bool(bool value) {
			invoker::invoke<void>(0xc58424ba936eb458, value);
		}
		static void begin_text_command_scaleform_string(const char* componenttype) {
			invoker::invoke<void>(0x80338406f3475e55, componenttype);
		}
		static void end_text_command_scaleform_string() {
			invoker::invoke<void>(0x362e2d3fe93a9959);
		}
		static void _end_text_command_scaleform_string_2() {
			invoker::invoke<void>(0xae4e8157d9ecf087);
		}
		static void _add_scaleform_movie_method_parameter_string(const char* value) {
			invoker::invoke<void>(0xba7148484bd90365, value);
		}
		static void _add_scaleform_movie_method_parameter_button_name(const char* button) {
			invoker::invoke<void>(0xe83a3e3557a56640, button);
		}
		static bool _0x5e657ef1099edd65(int p0) {
			return invoker::invoke<bool>(0x5e657ef1099edd65, p0);
		}
		static void _0xec52c631a1831c03(int p0) {
			invoker::invoke<void>(0xec52c631a1831c03, p0);
		}
		static void _request_hud_scaleform(int hudcomponent) {
			invoker::invoke<void>(0x9304881d6f6537ea, hudcomponent);
		}
		static bool _has_hud_scaleform_loaded(int hudcomponent) {
			return invoker::invoke<bool>(0xdf6e5987d2b4d140, hudcomponent);
		}
		static void _0xf44a5456ac3f4f97(type::any p0) {
			invoker::invoke<void>(0xf44a5456ac3f4f97, p0);
		}
		static bool _0xd1c7cb175e012964(int scaleformhandle) {
			return invoker::invoke<bool>(0xd1c7cb175e012964, scaleformhandle);
		}
		static void set_tv_channel(int channel) {
			invoker::invoke<void>(0xbaabbb23eb6e484e, channel);
		}
		static int get_tv_channel() {
			return invoker::invoke<int>(0xfc1e275a90d39995);
		}
		static void set_tv_volume(float volume) {
			invoker::invoke<void>(0x2982bf73f66e9ddc, volume);
		}
		static float get_tv_volume() {
			return invoker::invoke<float>(0x2170813d3dd8661b);
		}
		static void draw_tv_channel(float xpos, float ypos, float xscale, float yscale, float rotation, int red, int green, int blue, int alpha) {
			invoker::invoke<void>(0xfddc2b4ed3c69df0, xpos, ypos, xscale, yscale, rotation, red, green, blue, alpha);
		}
		static void set_tv_channel_playlist(int channel, const char* playlist, bool frombeginning) {
			invoker::invoke<void>(0xf7b38b8305f1fe8b, channel, playlist, frombeginning);
		}
		static void _0x2201c576facaebe8(type::any p0, const char* p1, type::any p2) {
			invoker::invoke<void>(0x2201c576facaebe8, p0, p1, p2);
		}
		static void _0xbeb3d46bb7f043c0(type::any p0) {
			invoker::invoke<void>(0xbeb3d46bb7f043c0, p0);
		}
		static bool _is_tv_playlist_item_playing(type::hash videoclip) {
			return invoker::invoke<bool>(0x0ad973ca1e077b60, videoclip);
		}
		static void _0x74c180030fde4b69(bool p0) {
			invoker::invoke<void>(0x74c180030fde4b69, p0);
		}
		static void _0xd1c55b110e4df534(type::any p0) {
			invoker::invoke<void>(0xd1c55b110e4df534, p0);
		}
		static void enable_movie_subtitles(bool toggle) {
			invoker::invoke<void>(0x873fa65c778ad970, toggle);
		}
		static bool _0xd3a10fc7fd8d98cd() {
			return invoker::invoke<bool>(0xd3a10fc7fd8d98cd);
		}
		static bool _0xf1cea8a4198d8e9a(const char* p0) {
			return invoker::invoke<bool>(0xf1cea8a4198d8e9a, p0);
		}
		static bool _draw_showroom(const char* p0, type::ped ped, int p2, float posx, float posy, float posz) {
			return invoker::invoke<bool>(0x98c4fe6ec34154ca, p0, ped, p2, posx, posy, posz);
		}
		static void _0x7a42b2e236e71415() {
			invoker::invoke<void>(0x7a42b2e236e71415);
		}
		static void _0x108be26959a9d9bb(bool p0) {
			invoker::invoke<void>(0x108be26959a9d9bb, p0);
		}
		static void _0xa356990e161c9e65(bool p0) {
			invoker::invoke<void>(0xa356990e161c9e65, p0);
		}
		static void _0x1c4fc5752bcd8e48(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11, float p12) {
			invoker::invoke<void>(0x1c4fc5752bcd8e48, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
		}
		static void _0x5ce62918f8d703c7(int p0, int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10, int p11) {
			invoker::invoke<void>(0x5ce62918f8d703c7, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
		}
		static void animpostfx_play(const char* effectname, int duration, bool looped) {
			invoker::invoke<void>(0x2206bf9a37b7f724, effectname, duration, looped);
		}
		static void animpostfx_stop(const char* effectname) {
			invoker::invoke<void>(0x068e835a1d0dc0e3, effectname);
		}
		static bool animpostfx_is_running(const char* effectname) {
			return invoker::invoke<bool>(0x36ad3e690da5aceb, effectname);
		}
		static void animpostfx_stop_all() {
			invoker::invoke<void>(0xb4eddc19532bfb85);
		}
		static void _0xd2209be128b5418c(const char* graphicsname) {
			invoker::invoke<void>(0xd2209be128b5418c, graphicsname);
		}
	}

	namespace stats {
		static type::any stat_clear_slot_for_reload(int statslot) {
			return invoker::invoke<type::any>(0xeb0a72181d4aa4ad, statslot);
		}
		static bool stat_load(int p0) {
			return invoker::invoke<bool>(0xa651443f437b1ce6, p0);
		}
		static bool stat_save(int p0, bool p1, int p2) {
			return invoker::invoke<bool>(0xe07bca305b82d2fd, p0, p1, p2);
		}
		static void _0x5688585e6d563cd8(type::any p0) {
			invoker::invoke<void>(0x5688585e6d563cd8, p0);
		}
		static bool stat_load_pending(type::any p0) {
			return invoker::invoke<bool>(0xa1750ffafa181661, p0);
		}
		static type::any stat_save_pending() {
			return invoker::invoke<type::any>(0x7d3a583856f2c5ac);
		}
		static type::any stat_save_pending_or_requested() {
			return invoker::invoke<type::any>(0xbbb6ad006f1bbea3);
		}
		static type::any stat_delete_slot(type::any p0) {
			return invoker::invoke<type::any>(0x49a49bed12794d70, p0);
		}
		static bool stat_slot_is_loaded(type::any p0) {
			return invoker::invoke<bool>(0x0d0a9f0e7bd91e3c, p0);
		}
		static bool _0x7f2c4cdf2e82df4c(type::any p0) {
			return invoker::invoke<bool>(0x7f2c4cdf2e82df4c, p0);
		}
		static type::any _0xe496a53ba5f50a56(type::any p0) {
			return invoker::invoke<type::any>(0xe496a53ba5f50a56, p0);
		}
		static void _0xf434a10ba01c37d0(bool p0) {
			invoker::invoke<void>(0xf434a10ba01c37d0, p0);
		}
		static bool _0x7e6946f68a38b74f(type::any p0) {
			return invoker::invoke<bool>(0x7e6946f68a38b74f, p0);
		}
		static void _0xa8733668d1047b51(type::any p0) {
			invoker::invoke<void>(0xa8733668d1047b51, p0);
		}
		static bool _0xecb41ac6ab754401() {
			return invoker::invoke<bool>(0xecb41ac6ab754401);
		}
		static void _0x9b4bd21d69b1e609() {
			invoker::invoke<void>(0x9b4bd21d69b1e609);
		}
		static type::any _0xc0e0d686ddfc6eae() {
			return invoker::invoke<type::any>(0xc0e0d686ddfc6eae);
		}
		static bool stat_set_int(type::hash statname, int value, bool save) {
			return invoker::invoke<bool>(0xb3271d7ab655b441, statname, value, save);
		}
		static bool stat_set_float(type::hash statname, float value, bool save) {
			return invoker::invoke<bool>(0x4851997f37fe9b3c, statname, value, save);
		}
		static bool stat_set_bool(type::hash statname, bool value, bool save) {
			return invoker::invoke<bool>(0x4b33c4243de0c432, statname, value, save);
		}
		static bool stat_set_gxt_label(type::hash statname, const char* value, bool save) {
			return invoker::invoke<bool>(0x17695002fd8b2ae0, statname, value, save);
		}
		static bool stat_set_date(type::hash statname, type::any* value, int numfields, bool save) {
			return invoker::invoke<bool>(0x2c29bfb64f4fcbe4, statname, value, numfields, save);
		}
		static bool stat_set_string(type::hash statname, const char* value, bool save) {
			return invoker::invoke<bool>(0xa87b2335d12531d7, statname, value, save);
		}
		static bool stat_set_pos(type::hash statname, float x, float y, float z, bool save) {
			return invoker::invoke<bool>(0xdb283fde680fe72e, statname, x, y, z, save);
		}
		static bool stat_set_masked_int(type::hash statname, type::any p1, type::any p2, int p3, bool save) {
			return invoker::invoke<bool>(0x7bbb1b54583ed410, statname, p1, p2, p3, save);
		}
		static bool stat_set_user_id(type::hash statname, const char* value, bool save) {
			return invoker::invoke<bool>(0x8cddf1e452babe11, statname, value, save);
		}
		static bool stat_set_current_posix_time(type::hash statname, bool p1) {
			return invoker::invoke<bool>(0xc2f84b7f9c4d0c61, statname, p1);
		}
		static bool stat_get_int(type::hash stathash, int* outvalue, int p2) {
			return invoker::invoke<bool>(0x767fbc2ac802ef3d, stathash, outvalue, p2);
		}
		static bool stat_get_float(type::hash stathash, float* outvalue, type::any p2) {
			return invoker::invoke<bool>(0xd7ae6c9c9c6ac54c, stathash, outvalue, p2);
		}
		static bool stat_get_bool(type::hash stathash, bool* outvalue, type::any p2) {
			return invoker::invoke<bool>(0x11b5e6d2ae73f48e, stathash, outvalue, p2);
		}
		static bool stat_get_date(type::hash stathash, type::any* p1, type::any p2, type::any p3) {
			return invoker::invoke<bool>(0x8b0facefc36c824b, stathash, p1, p2, p3);
		}
		static const char* stat_get_string(type::hash stathash, int p1) {
			return invoker::invoke<const char*>(0xe50384acc2c3db74, stathash, p1);
		}
		static bool stat_get_pos(type::hash stathash, float* x, float* y, float* z, int p4) {
			return invoker::invoke<bool>(0x350f82ccb186aa1b, stathash, x, y, z, p4);
		}
		static bool stat_get_masked_int(type::hash stathash, int* outvalue, type::any p2, type::any p3, type::any p4) {
			return invoker::invoke<bool>(0x655185a06d9eeaab, stathash, outvalue, p2, p3, p4);
		}
		static const char* stat_get_user_id(type::any p0) {
			return invoker::invoke<const char*>(0x2365c388e393bbe2, p0);
		}
		static const char* stat_get_license_plate(type::hash statname) {
			return invoker::invoke<const char*>(0x5473d4195058b2e4, statname);
		}
		static bool stat_set_license_plate(type::hash statname, const char* str) {
			return invoker::invoke<bool>(0x69ff13266d7296da, statname, str);
		}
		static void stat_increment(type::hash statname, float value) {
			invoker::invoke<void>(0x9b5a68c6489e9909, statname, value);
		}
		static bool _0x5a556b229a169402() {
			return invoker::invoke<bool>(0x5a556b229a169402);
		}
		static bool _0xb1d2bb1e1631f5b1() {
			return invoker::invoke<bool>(0xb1d2bb1e1631f5b1);
		}
		static bool _0xbed9f5693f34ed17(type::hash statname, int p1, float* outvalue) {
			return invoker::invoke<bool>(0xbed9f5693f34ed17, statname, p1, outvalue);
		}
		static void _0x26d7399b9587fe89(int characterindex) {
			invoker::invoke<void>(0x26d7399b9587fe89, characterindex);
		}
		static void _0xa78b8fa58200da56(int p0) {
			invoker::invoke<void>(0xa78b8fa58200da56, p0);
		}
		static int stat_get_number_of_days(type::hash statname) {
			return invoker::invoke<int>(0xe0e854f5280fb769, statname);
		}
		static int stat_get_number_of_hours(type::hash statname) {
			return invoker::invoke<int>(0xf2d4b2fe415aafc3, statname);
		}
		static int stat_get_number_of_minutes(type::hash statname) {
			return invoker::invoke<int>(0x7583b4be4c5a41b5, statname);
		}
		static int stat_get_number_of_seconds(type::hash statname) {
			return invoker::invoke<int>(0x2ce056ff3723f00b, statname);
		}
		static void stat_set_profile_setting_value(int profilesetting, int value) {
			invoker::invoke<void>(0x68f01422be1d838f, profilesetting, value);
		}
		static int _0xf4d8e7ac2a27758c(int p0) {
			return invoker::invoke<int>(0xf4d8e7ac2a27758c, p0);
		}
		static int _0x94f12abf9c79e339(int p0) {
			return invoker::invoke<int>(0x94f12abf9c79e339, p0);
		}
		static type::hash _get_pstat_bool_hash(int index, bool spstat, bool charstat, int character) {
			return invoker::invoke<type::hash>(0x80c75307b1c42837, index, spstat, charstat, character);
		}
		static type::hash _get_pstat_int_hash(int index, bool spstat, bool charstat, int character) {
			return invoker::invoke<type::hash>(0x61e111e323419e07, index, spstat, charstat, character);
		}
		static type::hash _get_tupstat_bool_hash(int index, bool spstat, bool charstat, int character) {
			return invoker::invoke<type::hash>(0xc4bb08ee7907471e, index, spstat, charstat, character);
		}
		static type::hash _get_tupstat_int_hash(int index, bool spstat, bool charstat, int character) {
			return invoker::invoke<type::hash>(0xd16c2ad6b8e32854, index, spstat, charstat, character);
		}
		static type::hash _get_ngstat_bool_hash(int index, bool spstat, bool charstat, int character, const char* section) {
			return invoker::invoke<type::hash>(0xba52ff538ed2bc71, index, spstat, charstat, character, section);
		}
		static type::hash _get_ngstat_int_hash(int index, bool spstat, bool charstat, int character, const char* section) {
			return invoker::invoke<type::hash>(0x2b4cdca6f07ff3da, index, spstat, charstat, character, section);
		}
		static bool stat_get_bool_masked(type::hash statname, int mask, int p2) {
			return invoker::invoke<bool>(0x10fe3f1b79f9b071, statname, mask, p2);
		}
		static bool stat_set_bool_masked(type::hash statname, bool value, int mask, bool save) {
			return invoker::invoke<bool>(0x5bc62ec1937b9e5b, statname, value, mask, save);
		}
		static void _0x5009dfd741329729(const char* p0, type::any p1) {
			invoker::invoke<void>(0x5009dfd741329729, p0, p1);
		}
		static void playstats_npc_invite(type::any* p0) {
			invoker::invoke<void>(0x93054c88e6aa7c44, p0);
		}
		static void playstats_award_xp(int xp, type::hash hash1, type::hash hash2) {
			invoker::invoke<void>(0x46f917f6b4128fe4, xp, hash1, hash2);
		}
		static void playstats_rank_up(type::player player) {
			invoker::invoke<void>(0xc7f2de41d102bfb4, player);
		}
		static void _0x098760c7461724cd() {
			invoker::invoke<void>(0x098760c7461724cd);
		}
		static void _0xa071e0ed98f91286(type::any p0, type::any p1) {
			invoker::invoke<void>(0xa071e0ed98f91286, p0, p1);
		}
		static void _0xc5be134ec7ba96a0(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4) {
			invoker::invoke<void>(0xc5be134ec7ba96a0, p0, p1, p2, p3, p4);
		}
		static void playstats_mission_started(type::any* p0, type::any p1, type::any p2, bool p3) {
			invoker::invoke<void>(0xc19a2925c34d2231, p0, p1, p2, p3);
		}
		static void playstats_mission_over(type::any* p0, type::any p1, type::any p2, bool p3, bool p4, bool p5) {
			invoker::invoke<void>(0x7c4bb33a8ced7324, p0, p1, p2, p3, p4, p5);
		}
		static void playstats_mission_checkpoint(type::any* p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0xc900596a63978c1d, p0, p1, p2, p3);
		}
		static void playstats_random_mission_done(const char* name, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x71862b1d855f32e1, name, p1, p2, p3);
		}
		static void playstats_ros_bet(int amount, int act, type::player player, float cm) {
			invoker::invoke<void>(0x121fb4dddc2d5291, amount, act, player, cm);
		}
		static void playstats_race_checkpoint(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4) {
			invoker::invoke<void>(0x9c375c315099dde4, p0, p1, p2, p3, p4);
		}
		static bool _0x6dee77aff8c21bd1(type::any* p0, type::any* p1) {
			return invoker::invoke<bool>(0x6dee77aff8c21bd1, p0, p1);
		}
		static void playstats_match_started(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4, type::any p5, type::any p6) {
			invoker::invoke<void>(0xbc80e22ded931e3d, p0, p1, p2, p3, p4, p5, p6);
		}
		static void playstats_shop_item(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4) {
			invoker::invoke<void>(0x176852acaac173d1, p0, p1, p2, p3, p4);
		}
		static void _0x1cae5d2e3f9a07f0(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4, type::any p5) {
			invoker::invoke<void>(0x1cae5d2e3f9a07f0, p0, p1, p2, p3, p4, p5);
		}
		static void _playstats_ambient_mission_crate_created(float p0, float p1, float p2) {
			invoker::invoke<void>(0xafc7e5e075a96f46, p0, p1, p2);
		}
		static void _0xcb00196b31c39eb1(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0xcb00196b31c39eb1, p0, p1, p2, p3);
		}
		static void _0x2b69f5074c894811(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x2b69f5074c894811, p0, p1, p2, p3);
		}
		static void _0x7eec2a316c250073(type::any p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0x7eec2a316c250073, p0, p1, p2);
		}
		static void _0xaddd1c754e2e2914(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4, type::any p5, type::any p6, type::any p7, type::any p8, type::any p9) {
			invoker::invoke<void>(0xaddd1c754e2e2914, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
		}
		static void playstats_acquired_hidden_package(type::any p0) {
			invoker::invoke<void>(0x79ab33f0fbfac40c, p0);
		}
		static void playstats_website_visited(type::hash scaleformhash, int p1) {
			invoker::invoke<void>(0xddf24d535060f811, scaleformhash, p1);
		}
		static void playstats_friend_activity(type::any p0, type::any p1) {
			invoker::invoke<void>(0x0f71de29ab2258f1, p0, p1);
		}
		static void playstats_oddjob_done(type::any p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0x69dea3e9db727b4c, p0, p1, p2);
		}
		static void playstats_prop_change(type::any p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0xba739d6d5a05d6e7, p0, p1, p2, p3);
		}
		static void playstats_cloth_change(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4) {
			invoker::invoke<void>(0x34b973047a2268b9, p0, p1, p2, p3, p4);
		}
		static void _0xe95c8a1875a02ca4(type::any p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0xe95c8a1875a02ca4, p0, p1, p2);
		}
		static void playstats_cheat_applied(const char* cheat) {
			invoker::invoke<void>(0x6058665d72302d3f, cheat);
		}
		static void _0xf8c54a461c3e11dc(type::any* p0, type::any* p1, type::any* p2, type::any* p3) {
			invoker::invoke<void>(0xf8c54a461c3e11dc, p0, p1, p2, p3);
		}
		static void _0xf5bb8dac426a52c0(type::any* p0, type::any* p1, type::any* p2, type::any* p3) {
			invoker::invoke<void>(0xf5bb8dac426a52c0, p0, p1, p2, p3);
		}
		static void _0xa736cf7fb7c5bff4(type::any* p0, type::any* p1, type::any* p2, type::any* p3) {
			invoker::invoke<void>(0xa736cf7fb7c5bff4, p0, p1, p2, p3);
		}
		static void _0x14e0b2d1ad1044e0(type::any* p0, type::any* p1, type::any* p2, type::any* p3) {
			invoker::invoke<void>(0x14e0b2d1ad1044e0, p0, p1, p2, p3);
		}
		static void playstats_quickfix_tool(int element, const char* item) {
			invoker::invoke<void>(0x90d0622866e80445, element, item);
		}
		static void playstats_idle_kick(int time) {
			invoker::invoke<void>(0x5da3a8de8cb6226f, time);
		}
		static void _0xd1032e482629049e(bool p0) {
			invoker::invoke<void>(0xd1032e482629049e, p0);
		}
		static void _0xf4ff020a08bc8863(type::any p0, type::any p1) {
			invoker::invoke<void>(0xf4ff020a08bc8863, p0, p1);
		}
		static void _0x46326e13da4e0546(type::any* p0) {
			invoker::invoke<void>(0x46326e13da4e0546, p0);
		}
		static type::any leaderboards_get_number_of_columns(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x117b45156d7eff2e, p0, p1);
		}
		static type::any leaderboards_get_column_id(type::any p0, type::any p1, type::any p2) {
			return invoker::invoke<type::any>(0xc4b5467a1886ea7e, p0, p1, p2);
		}
		static type::any leaderboards_get_column_type(type::any p0, type::any p1, type::any p2) {
			return invoker::invoke<type::any>(0xbf4fef46db7894d3, p0, p1, p2);
		}
		static type::any leaderboards_read_clear_all() {
			return invoker::invoke<type::any>(0xa34cb6e6f0df4a0b);
		}
		static type::any leaderboards_read_clear(type::any p0, type::any p1, type::any p2) {
			return invoker::invoke<type::any>(0x7cce5c737a665701, p0, p1, p2);
		}
		static bool leaderboards_read_pending(type::any p0, type::any p1, type::any p2) {
			return invoker::invoke<bool>(0xac392c8483342ac2, p0, p1, p2);
		}
		static type::any _0xa31fd15197b192bd() {
			return invoker::invoke<type::any>(0xa31fd15197b192bd);
		}
		static bool leaderboards_read_successful(type::any p0, type::any p1, type::any p2) {
			return invoker::invoke<bool>(0x2fb19228983e832c, p0, p1, p2);
		}
		static bool leaderboards2_read_friends_by_row(type::any* p0, type::any* p1, type::any p2, bool p3, type::any p4, type::any p5) {
			return invoker::invoke<bool>(0x918b101666f9cb83, p0, p1, p2, p3, p4, p5);
		}
		static bool leaderboards2_read_by_handle(type::any* p0, type::any* p1) {
			return invoker::invoke<bool>(0xc30713a383bfbf0e, p0, p1);
		}
		static bool leaderboards2_read_by_row(type::any* p0, type::any* p1, type::any p2, type::any* p3, type::any p4, type::any* p5, type::any p6) {
			return invoker::invoke<bool>(0xa9cdb1e3f0a49883, p0, p1, p2, p3, p4, p5, p6);
		}
		static bool leaderboards2_read_by_rank(type::any* p0, type::any p1, type::any p2) {
			return invoker::invoke<bool>(0xba2c7db0c129449a, p0, p1, p2);
		}
		static bool leaderboards2_read_by_radius(type::any* p0, type::any p1, type::any* p2) {
			return invoker::invoke<bool>(0x5ce587fb5a42c8c4, p0, p1, p2);
		}
		static bool leaderboards2_read_by_score_int(type::any* p0, type::any p1, type::any p2) {
			return invoker::invoke<bool>(0x7eec7e4f6984a16a, p0, p1, p2);
		}
		static bool leaderboards2_read_by_score_float(type::any* p0, float p1, type::any p2) {
			return invoker::invoke<bool>(0xe662c8b759d08f3c, p0, p1, p2);
		}
		static bool _0xc38dc1e90d22547c(type::any* p0, type::any* p1, type::any* p2) {
			return invoker::invoke<bool>(0xc38dc1e90d22547c, p0, p1, p2);
		}
		static bool _0xf1ae5dcdbfca2721(type::any* p0, type::any* p1, type::any* p2) {
			return invoker::invoke<bool>(0xf1ae5dcdbfca2721, p0, p1, p2);
		}
		static bool _0xa0f93d5465b3094d(type::any* p0) {
			return invoker::invoke<bool>(0xa0f93d5465b3094d, p0);
		}
		static void _0x71b008056e5692d6() {
			invoker::invoke<void>(0x71b008056e5692d6);
		}
		static bool _0x34770b9ce0e03b91(type::any p0, type::any* p1) {
			return invoker::invoke<bool>(0x34770b9ce0e03b91, p0, p1);
		}
		static type::any _0x88578f6ec36b4a3a(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x88578f6ec36b4a3a, p0, p1);
		}
		static float _0x38491439b6ba7f7d(type::any p0, type::any p1) {
			return invoker::invoke<float>(0x38491439b6ba7f7d, p0, p1);
		}
		static bool leaderboards2_write_data(type::any* p0) {
			return invoker::invoke<bool>(0xae2206545888ae49, p0);
		}
		static void _0x0bca1d2c47b0d269(type::any p0, type::any p1, float p2) {
			invoker::invoke<void>(0x0bca1d2c47b0d269, p0, p1, p2);
		}
		static void _0x2e65248609523599(type::any p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0x2e65248609523599, p0, p1, p2);
		}
		static bool leaderboards_cache_data_row(type::any* p0) {
			return invoker::invoke<bool>(0xb9bb18e2c40142ed, p0);
		}
		static void leaderboards_clear_cache_data() {
			invoker::invoke<void>(0xd4b02a6b476e1fdc);
		}
		static void _0x8ec74ceb042e7cff(type::any p0) {
			invoker::invoke<void>(0x8ec74ceb042e7cff, p0);
		}
		static bool leaderboards_get_cache_exists(type::any p0) {
			return invoker::invoke<bool>(0x9c51349be6cdfe2c, p0);
		}
		static type::any leaderboards_get_cache_time(type::any p0) {
			return invoker::invoke<type::any>(0xf04c1c27da35f6c8, p0);
		}
		static type::any _0x58a651cd201d89ad(type::any p0) {
			return invoker::invoke<type::any>(0x58a651cd201d89ad, p0);
		}
		static bool leaderboards_get_cache_data_row(type::any p0, type::any p1, type::any* p2) {
			return invoker::invoke<bool>(0x9120e8dba3d69273, p0, p1, p2);
		}
		static void _0x11ff1c80276097ed(const char* p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0x11ff1c80276097ed, p0, p1, p2);
		}
		static void _0x30a6614c1f7799b8(type::any p0, float p1, type::any p2) {
			invoker::invoke<void>(0x30a6614c1f7799b8, p0, p1, p2);
		}
		static void _0x6483c25849031c4f(type::any p0, type::any p1, type::any p2, type::any* p3) {
			invoker::invoke<void>(0x6483c25849031c4f, p0, p1, p2, p3);
		}
		static bool _0x5ead2bf6484852e4() {
			return invoker::invoke<bool>(0x5ead2bf6484852e4);
		}
		static void _0xc141b8917e0017ec() {
			invoker::invoke<void>(0xc141b8917e0017ec);
		}
		static void _0xb475f27c6a994d65() {
			invoker::invoke<void>(0xb475f27c6a994d65);
		}
		static void _0xf1a1803d3476f215(int value) {
			invoker::invoke<void>(0xf1a1803d3476f215, value);
		}
		static void _0x38baaa5dd4c9d19f(int value) {
			invoker::invoke<void>(0x38baaa5dd4c9d19f, value);
		}
		static void _0x55384438fc55ad8e(int value) {
			invoker::invoke<void>(0x55384438fc55ad8e, value);
		}
		static void _0x723c1ce13fbfdb67(type::any p0, type::any p1) {
			invoker::invoke<void>(0x723c1ce13fbfdb67, p0, p1);
		}
		static void _0x0d01d20616fc73fb(type::any p0, type::any p1) {
			invoker::invoke<void>(0x0d01d20616fc73fb, p0, p1);
		}
		static void _leaderboards_deaths(type::hash statname, float value) {
			invoker::invoke<void>(0x428eaf89e24f6c36, statname, value);
		}
		static void _0x047cbed6f6f8b63c() {
			invoker::invoke<void>(0x047cbed6f6f8b63c);
		}
		static bool _leaderboards2_write_data_ex(type::any* p0, type::any* p1) {
			return invoker::invoke<bool>(0xc980e62e33df1d5c, p0, p1);
		}
		static void _0x6f361b8889a792a3() {
			invoker::invoke<void>(0x6f361b8889a792a3);
		}
		static void _0xc847b43f369ac0b5() {
			invoker::invoke<void>(0xc847b43f369ac0b5);
		}
		static bool _0xa5c80d8e768a9e66(type::any* p0) {
			return invoker::invoke<bool>(0xa5c80d8e768a9e66, p0);
		}
		static type::any _0x9a62ec95ae10e011() {
			return invoker::invoke<type::any>(0x9a62ec95ae10e011);
		}
		static type::any _0x4c89fe2bdeb3f169() {
			return invoker::invoke<type::any>(0x4c89fe2bdeb3f169);
		}
		static type::any _0xc6e0e2616a7576bb() {
			return invoker::invoke<type::any>(0xc6e0e2616a7576bb);
		}
		static type::any _0x5bd5f255321c4aaf(type::any p0) {
			return invoker::invoke<type::any>(0x5bd5f255321c4aaf, p0);
		}
		static type::any _0xdeaaf77eb3687e97(type::any p0, type::any* p1) {
			return invoker::invoke<type::any>(0xdeaaf77eb3687e97, p0, p1);
		}
		static type::any _0xc70ddce56d0d3a99() {
			return invoker::invoke<type::any>(0xc70ddce56d0d3a99);
		}
		static type::any _0x886913bbeaca68c1(type::any* p0) {
			return invoker::invoke<type::any>(0x886913bbeaca68c1, p0);
		}
		static type::any _0x4fef53183c3c6414() {
			return invoker::invoke<type::any>(0x4fef53183c3c6414);
		}
		static type::any _0x567384dfa67029e6() {
			return invoker::invoke<type::any>(0x567384dfa67029e6);
		}
		static bool _0x3270f67eed31fbc1(type::any p0, type::any* p1, type::any* p2) {
			return invoker::invoke<bool>(0x3270f67eed31fbc1, p0, p1, p2);
		}
		static type::any _0xce5aa445aba8dee0(type::any* p0) {
			return invoker::invoke<type::any>(0xce5aa445aba8dee0, p0);
		}
		static void _0x98e2bc1ca26287c3() {
			invoker::invoke<void>(0x98e2bc1ca26287c3);
		}
		static void _0x629526aba383bcaa() {
			invoker::invoke<void>(0x629526aba383bcaa);
		}
		static type::any _0xb3da2606774a8e2d() {
			return invoker::invoke<type::any>(0xb3da2606774a8e2d);
		}
		static void _unlock_exclus_content(type::any bitset) {
			invoker::invoke<void>(0xdac073c7901f9e15, bitset);
		}
		static void _0xf6792800ac95350d(type::any p0) {
			invoker::invoke<void>(0xf6792800ac95350d, p0);
		}
	}

	namespace brain {
		static void add_script_to_random_ped(const char* name, type::hash model, float p2, float p3) {
			invoker::invoke<void>(0x4ee5367468a65ccc, name, model, p2, p3);
		}
		static void register_object_script_brain(const char* scriptname, type::hash objectname, int p2, float p3, int p4, int p5) {
			invoker::invoke<void>(0x0be84c318ba6ec22, scriptname, objectname, p2, p3, p4, p5);
		}
		static bool is_object_within_brain_activation_range(type::object object) {
			return invoker::invoke<bool>(0xccba154209823057, object);
		}
		static void register_world_point_script_brain(type::any* p0, float p1, type::any p2) {
			invoker::invoke<void>(0x3cdc7136613284bd, p0, p1, p2);
		}
		static bool is_world_point_within_brain_activation_range() {
			return invoker::invoke<bool>(0xc5042cc6f5e3d450);
		}
		static void enable_script_brain_set(int brainset) {
			invoker::invoke<void>(0x67aa4d73f0cfa86b, brainset);
		}
		static void disable_script_brain_set(int brainset) {
			invoker::invoke<void>(0x14d8518e9760f08f, brainset);
		}
		static void _0x0b40ed49d7d6ff84() {
			invoker::invoke<void>(0x0b40ed49d7d6ff84);
		}
		static void _0x4d953df78ebf8158() {
			invoker::invoke<void>(0x4d953df78ebf8158);
		}
		static void _0x6d6840cee8845831(const char* action) {
			invoker::invoke<void>(0x6d6840cee8845831, action);
		}
		static void _0x6e91b04e08773030(const char* action) {
			invoker::invoke<void>(0x6e91b04e08773030, action);
		}
	}

	namespace mobile {
		static void create_mobile_phone(int p4) {
			invoker::invoke<void>(0xa4e8e696c532fbc7, p4);
		}
		static void destroy_mobile_phone() {
			invoker::invoke<void>(0x3bc861df703e5097);
		}
		static void set_mobile_phone_scale(float scale) {
			invoker::invoke<void>(0xcbdd322a73d6d932, scale);
		}
		static void set_mobile_phone_rotation(float rotx, float roty, float rotz, type::any p3) {
			invoker::invoke<void>(0xbb779c0ca917e865, rotx, roty, rotz, p3);
		}
		static void get_mobile_phone_rotation(PVector3* rotation, type::vehicle p1) {
			invoker::invoke<void>(0x1cefb61f193070ae, rotation, p1);
		}
		static void set_mobile_phone_position(float posx, float posy, float posz) {
			invoker::invoke<void>(0x693a5c6d6734085b, posx, posy, posz);
		}
		static void get_mobile_phone_position(PVector3* position) {
			invoker::invoke<void>(0x584fdfda48805b86, position);
		}
		static void script_is_moving_mobile_phone_offscreen(bool toggle) {
			invoker::invoke<void>(0xf511f759238a5122, toggle);
		}
		static bool can_phone_be_seen_on_screen() {
			return invoker::invoke<bool>(0xc4e2813898c97a4b);
		}
		static void _move_finger(int direction) {
			invoker::invoke<void>(0x95c9e72f3d7dec9b, direction);
		}
		static void _set_phone_lean(bool toggle) {
			invoker::invoke<void>(0x44e44169ef70138e, toggle);
		}
		static void cell_cam_activate(bool p0, bool p1) {
			invoker::invoke<void>(0xfde8f069c542d126, p0, p1);
		}
		static void _disable_phone_this_frame(bool toggle) {
			invoker::invoke<void>(0x015c49a93e3e086e, toggle);
		}
		static void _0xa2ccbe62cd4c91a4(int* toggle) {
			invoker::invoke<void>(0xa2ccbe62cd4c91a4, toggle);
		}
		static void _0x1b0b4aeed5b9b41c(float p0) {
			invoker::invoke<void>(0x1b0b4aeed5b9b41c, p0);
		}
		static void _0x53f4892d18ec90a4(float p0) {
			invoker::invoke<void>(0x53f4892d18ec90a4, p0);
		}
		static void _0x3117d84efa60f77b(float p0) {
			invoker::invoke<void>(0x3117d84efa60f77b, p0);
		}
		static void _0x15e69e2802c24b8d(float p0) {
			invoker::invoke<void>(0x15e69e2802c24b8d, p0);
		}
		static void _0xac2890471901861c(float p0) {
			invoker::invoke<void>(0xac2890471901861c, p0);
		}
		static void _0xd6ade981781fca09(float p0) {
			invoker::invoke<void>(0xd6ade981781fca09, p0);
		}
		static void _0xf1e22dc13f5eebad(float p0) {
			invoker::invoke<void>(0xf1e22dc13f5eebad, p0);
		}
		static void _0x466da42c89865553(float p0) {
			invoker::invoke<void>(0x466da42c89865553, p0);
		}
		static bool cell_cam_is_char_visible_no_face_check(type::entity entity) {
			return invoker::invoke<bool>(0x439e9bc95b7e7fbe, entity);
		}
		static void get_mobile_phone_render_id(int* renderid) {
			invoker::invoke<void>(0xb4a53e05f68b6fa1, renderid);
		}
		static bool _network_shop_does_item_exist(const char* name) {
			return invoker::invoke<bool>(0xbd4d7eaf8a30f637, name);
		}
		static bool _network_shop_does_item_exist_hash(type::hash hash) {
			return invoker::invoke<bool>(0x247f0f73a182ea0b, hash);
		}
	}

	namespace app {
		static type::hash app_data_valid() {
			return invoker::invoke<type::hash>(0x846aa8e7d55ee5b6);
		}
		static int* app_get_int(type::scrhandle property) {
			return invoker::invoke<int*>(0xd3a58a12c77d9d4b, property);
		}
		static float app_get_float(const char* property) {
			return invoker::invoke<float>(0x1514fb24c02c2322, property);
		}
		static const char* app_get_string(const char* property) {
			return invoker::invoke<const char*>(0x749b023950d2311c, property);
		}
		static void app_set_int(const char* property, int value) {
			invoker::invoke<void>(0x607e8e3d3e4f9611, property, value);
		}
		static void app_set_float(const char* property, float value) {
			invoker::invoke<void>(0x25d7687c68e0daa4, property, value);
		}
		static void app_set_string(const char* property, const char* value) {
			invoker::invoke<void>(0x3ff2fcec4b7721b4, property, value);
		}
		static void app_set_app(const char* appname) {
			invoker::invoke<void>(0xcfd0406adaf90d2b, appname);
		}
		static void app_set_block(const char* blockname) {
			invoker::invoke<void>(0x262ab456a3d21f93, blockname);
		}
		static void app_clear_block() {
			invoker::invoke<void>(0x5fe1df3342db7dba);
		}
		static void app_close_app() {
			invoker::invoke<void>(0xe41c65e07a5f05fc);
		}
		static void app_close_block() {
			invoker::invoke<void>(0xe8e3fcf72eac0ef8);
		}
		static bool app_has_linked_social_club_account() {
			return invoker::invoke<bool>(0x71eee69745088da0);
		}
		static bool app_has_synced_data(const char* appname) {
			return invoker::invoke<bool>(0xca52279a7271517f, appname);
		}
		static void app_save_data() {
			invoker::invoke<void>(0x95c5d356cda6e85f);
		}
		static type::any app_get_deleted_file_status() {
			return invoker::invoke<type::any>(0xc9853a2be3ded1a6);
		}
		static bool app_delete_app_data(const char* appname) {
			return invoker::invoke<bool>(0x44151aea95c8a003, appname);
		}
	}

	namespace time {
		static void set_clock_time(int hour, int minute, int second) {
			invoker::invoke<void>(0x47c3b5848c3e45d8, hour, minute, second);
		}
		static void pause_clock(bool toggle) {
			invoker::invoke<void>(0x4055e40bd2dbec1d, toggle);
		}
		static void advance_clock_time_to(int hour, int minute, int second) {
			invoker::invoke<void>(0xc8ca9670b9d83b3b, hour, minute, second);
		}
		static void add_to_clock_time(int hours, int minutes, int seconds) {
			invoker::invoke<void>(0xd716f30d8c8980e2, hours, minutes, seconds);
		}
		static int get_clock_hours() {
			return invoker::invoke<int>(0x25223ca6b4d20b7f);
		}
		static int get_clock_minutes() {
			return invoker::invoke<int>(0x13d2b8add79640f2);
		}
		static int get_clock_seconds() {
			return invoker::invoke<int>(0x494e97c2ef27c470);
		}
		static void set_clock_date(int day, int month, int year) {
			invoker::invoke<void>(0xb096419df0d06ce7, day, month, year);
		}
		static int get_clock_day_of_week() {
			return invoker::invoke<int>(0xd972e4bd7aeb235f);
		}
		static int get_clock_day_of_month() {
			return invoker::invoke<int>(0x3d10bc92a4db1d35);
		}
		static int get_clock_month() {
			return invoker::invoke<int>(0xbbc72712e80257a1);
		}
		static int get_clock_year() {
			return invoker::invoke<int>(0x961777e64bdaf717);
		}
		static int get_milliseconds_per_game_minute() {
			return invoker::invoke<int>(0x2f8b4d1c595b11db);
		}
		static void get_posix_time(int* year, int* month, int* day, int* hour, int* minute, int* second) {
			invoker::invoke<void>(0xda488f299a5b164e, year, month, day, hour, minute, second);
		}
		static void _get_utc_time(int* year, int* month, int* day, int* hour, int* minute, int* second) {
			invoker::invoke<void>(0x8117e09a19eef4d3, year, month, day, hour, minute, second);
		}
		static void get_local_time(int* year, int* month, int* day, int* hour, int* minute, int* second) {
			invoker::invoke<void>(0x50c7a99057a69748, year, month, day, hour, minute, second);
		}
	}

	namespace pathfind {
		static void set_roads_in_area(float x1, float y1, float z1, float x2, float y2, float z2, bool unknown1, bool unknown2) {
			invoker::invoke<void>(0xbf1a602b5ba52fee, x1, y1, z1, x2, y2, z2, unknown1, unknown2);
		}
		static void set_roads_in_angled_area(float x1, float y1, float z1, float x2, float y2, float z2, float angle, bool unknown1, bool unknown2, bool unknown3) {
			invoker::invoke<void>(0x1a5aa1208af5db59, x1, y1, z1, x2, y2, z2, angle, unknown1, unknown2, unknown3);
		}
		static void set_ped_paths_in_area(float x1, float y1, float z1, float x2, float y2, float z2, bool unknown) {
			invoker::invoke<void>(0x34f060f4bf92e018, x1, y1, z1, x2, y2, z2, unknown);
		}
		static bool get_safe_coord_for_ped(float x, float y, float z, bool onground, PVector3* outposition, int flags) {
			return invoker::invoke<bool>(0xb61c8e878a4199ca, x, y, z, onground, outposition, flags);
		}
		static bool get_closest_vehicle_node(float x, float y, float z, PVector3* outposition, int nodetype, float p5, float p6) {
			return invoker::invoke<bool>(0x240a18690ae96513, x, y, z, outposition, nodetype, p5, p6);
		}
		static bool get_closest_major_vehicle_node(float x, float y, float z, PVector3* outposition, float p5, int p6) {
			return invoker::invoke<bool>(0x2eabe3b06f58c1be, x, y, z, outposition, p5, p6);
		}
		static bool get_closest_vehicle_node_with_heading(float x, float y, float z, PVector3* outposition, float* outheading, int nodetype, float p6, int p7) {
			return invoker::invoke<bool>(0xff071fb798b803b0, x, y, z, outposition, outheading, nodetype, p6, p7);
		}
		static bool get_nth_closest_vehicle_node(float x, float y, float z, int nthclosest, PVector3* outposition, bool p6, float p7, float p8) {
			return invoker::invoke<bool>(0xe50e52416ccf948b, x, y, z, nthclosest, outposition, p6, p7, p8);
		}
		static int get_nth_closest_vehicle_node_id(float x, float y, float z, int nth, int nodetype, float p5, float p6) {
			return invoker::invoke<int>(0x22d7275a79fe8215, x, y, z, nth, nodetype, p5, p6);
		}
		static bool get_nth_closest_vehicle_node_with_heading(float x, float y, float z, int nthclosest, PVector3* outposition, float* outheading, int* outint, int p6, float p7, float p8) {
			return invoker::invoke<bool>(0x80ca6a8b6c094cc4, x, y, z, nthclosest, outposition, outheading, outint, p6, p7, p8);
		}
		static type::any get_nth_closest_vehicle_node_id_with_heading(float x, float y, float z, int nthclosest, PVector3* outposition, float* outheading, int nodetype, float p7, float p8) {
			return invoker::invoke<type::any>(0x6448050e9c2a7207, x, y, z, nthclosest, outposition, outheading, nodetype, p7, p8);
		}
		static bool get_nth_closest_vehicle_node_favour_direction(float x, float y, float z, float desiredx, float desiredy, float desiredz, int nthclosest, PVector3* outposition, float* outheading, int nodetype, type::any p10, type::any p11) {
			return invoker::invoke<bool>(0x45905be8654ae067, x, y, z, desiredx, desiredy, desiredz, nthclosest, outposition, outheading, nodetype, p10, p11);
		}
		static bool get_vehicle_node_properties(float x, float y, float z, int* density, int* flags) {
			return invoker::invoke<bool>(0x0568566acbb5dedc, x, y, z, density, flags);
		}
		static bool is_vehicle_node_id_valid(int vehiclenodeid) {
			return invoker::invoke<bool>(0x1eaf30fcfbf5af74, vehiclenodeid);
		}
		static void get_vehicle_node_position(int nodeid, PVector3* outposition) {
			invoker::invoke<void>(0x703123e5e7d429c2, nodeid, outposition);
		}
		static bool _get_supports_gps_route_flag(int nodeid) {
			return invoker::invoke<bool>(0xa2ae5c478b96e3b6, nodeid);
		}
		static bool _get_is_slow_road_flag(int nodeid) {
			return invoker::invoke<bool>(0x4f5070aa58f69279, nodeid);
		}
		static type::any get_closest_road(float posx, float posy, float posz, float p3, int p4, PVector3* p5, PVector3* p6, int* p7, int* p8, float* p9, int p10) {
			return invoker::invoke<type::any>(0x132f52bba570fe92, posx, posy, posz, p3, p4, p5, p6, p7, p8, p9, p10);
		}
		static bool load_all_path_nodes(bool keepinmemory) {
			return invoker::invoke<bool>(0x80e4a6eddb0be8d9, keepinmemory);
		}
		static void _0x228e5c6ad4d74bfd(bool p0) {
			invoker::invoke<void>(0x228e5c6ad4d74bfd, p0);
		}
		static bool _0xf7b79a50b905a30d(float p0, float p1, float p2, float p3) {
			return invoker::invoke<bool>(0xf7b79a50b905a30d, p0, p1, p2, p3);
		}
		static bool _0x07fb139b592fa687(float p0, float p1, float p2, float p3) {
			return invoker::invoke<bool>(0x07fb139b592fa687, p0, p1, p2, p3);
		}
		static void set_roads_back_to_original(float p0, float p1, float p2, float p3, float p4, float p5) {
			invoker::invoke<void>(0x1ee7063b80ffc77c, p0, p1, p2, p3, p4, p5);
		}
		static void set_roads_back_to_original_in_angled_area(float x1, float y1, float z1, float x2, float y2, float z2, float p6) {
			invoker::invoke<void>(0x0027501b9f3b407e, x1, y1, z1, x2, y2, z2, p6);
		}
		static void _0x0b919e1fb47cc4e0(float p0) {
			invoker::invoke<void>(0x0b919e1fb47cc4e0, p0);
		}
		static void _0xaa76052dda9bfc3e(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4, type::any p5, type::any p6) {
			invoker::invoke<void>(0xaa76052dda9bfc3e, p0, p1, p2, p3, p4, p5, p6);
		}
		static void set_ped_paths_back_to_original(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4, type::any p5) {
			invoker::invoke<void>(0xe04b48f2cc926253, p0, p1, p2, p3, p4, p5);
		}
		static bool get_random_vehicle_node(float x, float y, float z, float radius, bool p4, bool p5, bool p6, PVector3* outposition, int* nodeid) {
			return invoker::invoke<bool>(0x93e0db8440b73a7d, x, y, z, radius, p4, p5, p6, outposition, nodeid);
		}
		static void get_street_name_at_coord(float x, float y, float z, type::hash* streetname, type::hash* crossingroad) {
			invoker::invoke<void>(0x2eb41072b4c1e4c0, x, y, z, streetname, crossingroad);
		}
		static int generate_directions_to_coord(float x, float y, float z, bool p3, float* direction, float* vehicle, float* disttonxjunction) {
			return invoker::invoke<int>(0xf90125f1f79ecdf8, x, y, z, p3, direction, vehicle, disttonxjunction);
		}
		static void set_ignore_no_gps_flag(bool ignore) {
			invoker::invoke<void>(0x72751156e7678833, ignore);
		}
		static void _0x1fc289a0c3ff470f(bool p0) {
			invoker::invoke<void>(0x1fc289a0c3ff470f, p0);
		}
		static void set_gps_disabled_zone(float x1, float y1, float z1, float x2, float y2, float z2) {
			invoker::invoke<void>(0xdc20483cd3dd5201, x1, y1, z1, x2, y2, z2);
		}
		static type::any _0xbbb45c3cf5c8aa85() {
			return invoker::invoke<type::any>(0xbbb45c3cf5c8aa85);
		}
		static type::any _0x869daacbbe9fa006() {
			return invoker::invoke<type::any>(0x869daacbbe9fa006);
		}
		static type::any _0x16f46fb18c8009e4(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4) {
			return invoker::invoke<type::any>(0x16f46fb18c8009e4, p0, p1, p2, p3, p4);
		}
		static bool is_point_on_road(float x, float y, float z, type::vehicle vehicle) {
			return invoker::invoke<bool>(0x125bf4abfc536b09, x, y, z, vehicle);
		}
		static type::any _0xd3a6a0ef48823a8c() {
			return invoker::invoke<type::any>(0xd3a6a0ef48823a8c);
		}
		static void _0xd0bc1c6fb18ee154(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4, type::any p5, type::any p6) {
			invoker::invoke<void>(0xd0bc1c6fb18ee154, p0, p1, p2, p3, p4, p5, p6);
		}
		static void _0x2801d0012266df07(type::any p0) {
			invoker::invoke<void>(0x2801d0012266df07, p0);
		}
		static void add_navmesh_required_region(float x, float y, float radius) {
			invoker::invoke<void>(0x387ead7ee42f6685, x, y, radius);
		}
		static void remove_navmesh_required_regions() {
			invoker::invoke<void>(0x916f0a3cdec3445e);
		}
		static void disable_navmesh_in_area(float x1, float y1, float z1, float x2, float y2, float z2, bool disable) {
			invoker::invoke<void>(0x4c8872d8cdbe1b8b, x1, y1, z1, x2, y2, z2, disable);
		}
		static bool are_all_navmesh_regions_loaded() {
			return invoker::invoke<bool>(0x8415d95b194a3aea);
		}
		static bool is_navmesh_loaded_in_area(float x1, float y1, float z1, float x2, float y2, float z2) {
			return invoker::invoke<bool>(0xf813c7e63f9062a5, x1, y1, z1, x2, y2, z2);
		}
		static type::any _0x01708e8dd3ff8c65(float p0, float p1, float p2, float p3, float p4, float p5) {
			return invoker::invoke<type::any>(0x01708e8dd3ff8c65, p0, p1, p2, p3, p4, p5);
		}
		static type::any add_navmesh_blocking_object(float p0, float p1, float p2, float p3, float p4, float p5, float p6, bool p7, type::any p8) {
			return invoker::invoke<type::any>(0xfcd5c8e06e502f5a, p0, p1, p2, p3, p4, p5, p6, p7, p8);
		}
		static void update_navmesh_blocking_object(type::any p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, type::any p8) {
			invoker::invoke<void>(0x109e99373f290687, p0, p1, p2, p3, p4, p5, p6, p7, p8);
		}
		static void remove_navmesh_blocking_object(type::any p0) {
			invoker::invoke<void>(0x46399a7895957c0e, p0);
		}
		static bool does_navmesh_blocking_object_exist(type::any p0) {
			return invoker::invoke<bool>(0x0eaeb0db4b132399, p0);
		}
		static float _0x29c24bfbed8ab8fb(float p0, float p1) {
			return invoker::invoke<float>(0x29c24bfbed8ab8fb, p0, p1);
		}
		static float _0x8abe8608576d9ce3(float p0, float p1, float p2, float p3) {
			return invoker::invoke<float>(0x8abe8608576d9ce3, p0, p1, p2, p3);
		}
		static float _0x336511a34f2e5185(float left, float right) {
			return invoker::invoke<float>(0x336511a34f2e5185, left, right);
		}
		static float _0x3599d741c9ac6310(float p0, float p1, float p2, float p3) {
			return invoker::invoke<float>(0x3599d741c9ac6310, p0, p1, p2, p3);
		}
		static float calculate_travel_distance_between_points(float x1, float y1, float z1, float x2, float y2, float z2) {
			return invoker::invoke<float>(0xadd95c7005c4a197, x1, y1, z1, x2, y2, z2);
		}
	}

	namespace controls {
		static bool is_control_enabled(int inputgroup, int control) {
			return invoker::invoke<bool>(0x1cea6bfdf248e5d9, inputgroup, control);
		}
		static bool is_control_pressed(int inputgroup, int contorl) {
			return invoker::invoke<bool>(0xf3a21bcd95725a4a, inputgroup, contorl);
		}
		static bool is_control_released(int inputgroup, int control) {
			return invoker::invoke<bool>(0x648ee3e7f38877dd, inputgroup, control);
		}
		static bool is_control_just_pressed(int inputgroup, int control) {
			return invoker::invoke<bool>(0x580417101ddb492f, inputgroup, control);
		}
		static bool is_control_just_released(int inputgroup, int control) {
			return invoker::invoke<bool>(0x50f940259d3841e6, inputgroup, control);
		}
		static int get_control_value(int inputgroup, int control) {
			return invoker::invoke<int>(0xd95e79e8686d2c27, inputgroup, control);
		}
		static float get_control_normal(int inputgroup, int control) {
			return invoker::invoke<float>(0xec3c9b8d5327b563, inputgroup, control);
		}
		static void _0x5b73c77d9eb66e24(bool p0) {
			invoker::invoke<void>(0x5b73c77d9eb66e24, p0);
		}
		static float _0x5b84d09cec5209c5(int inputgroup, int control) {
			return invoker::invoke<float>(0x5b84d09cec5209c5, inputgroup, control);
		}
		static bool _set_control_normal(int inputgroup, int control, float amount) {
			return invoker::invoke<bool>(0xe8a25867fba3b05e, inputgroup, control, amount);
		}
		static bool is_disabled_control_pressed(int inputgroup, int control) {
			return invoker::invoke<bool>(0xe2587f8cbbd87b1d, inputgroup, control);
		}
		static bool is_disabled_control_just_pressed(int inputgroup, int control) {
			return invoker::invoke<bool>(0x91aef906bca88877, inputgroup, control);
		}
		static bool is_disabled_control_just_released(int inputgroup, int control) {
			return invoker::invoke<bool>(0x305c8dcd79da8b0f, inputgroup, control);
		}
		static float get_disabled_control_normal(int inputgroup, int control) {
			return invoker::invoke<float>(0x11e65974a982637c, inputgroup, control);
		}
		static float _0x4f8a26a890fd62fb(int inputgroup, int control) {
			return invoker::invoke<float>(0x4f8a26a890fd62fb, inputgroup, control);
		}
		static int _0xd7d22f5592aed8ba(int p0) {
			return invoker::invoke<int>(0xd7d22f5592aed8ba, p0);
		}
		static bool _is_input_disabled(int inputgroup) {
			return invoker::invoke<bool>(0xa571d46727e2b718, inputgroup);
		}
		static bool _is_input_just_disabled(int inputgroup) {
			return invoker::invoke<bool>(0x13337b38db572509, inputgroup);
		}
		static bool _set_cursor_location(float x, float y) {
			return invoker::invoke<bool>(0xfc695459d4d0e219, x, y);
		}
		static bool _0x23f09eadc01449d6(bool p0) {
			return invoker::invoke<bool>(0x23f09eadc01449d6, p0);
		}
		static bool _0x6cd79468a1e595c6(int inputgroup) {
			return invoker::invoke<bool>(0x6cd79468a1e595c6, inputgroup);
		}
		static const char* get_control_instructional_button(int inputgroup, int control, type::player p2) {
			return invoker::invoke<const char*>(0x0499d7b09fc9b407, inputgroup, control, p2);
		}
		static const char* _0x80c2fd58d720c801(int inputgroup, int control, bool p2) {
			return invoker::invoke<const char*>(0x80c2fd58d720c801, inputgroup, control, p2);
		}
		static void _0x8290252fff36acb5(int p0, int red, int green, int blue) {
			invoker::invoke<void>(0x8290252fff36acb5, p0, red, green, blue);
		}
		static void _0xcb0360efefb2580d(type::any p0) {
			invoker::invoke<void>(0xcb0360efefb2580d, p0);
		}
		static void set_pad_shake(int p0, int duration, int frequency) {
			invoker::invoke<void>(0x48b3886c1358d0d5, p0, duration, frequency);
		}
		static void _0x14d29bb12d47f68c(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4) {
			invoker::invoke<void>(0x14d29bb12d47f68c, p0, p1, p2, p3, p4);
		}
		static void stop_pad_shake(type::any p0) {
			invoker::invoke<void>(0x38c16a305e8cdc8d, p0);
		}
		static void _0xf239400e16c23e08(type::any p0, type::any p1) {
			invoker::invoke<void>(0xf239400e16c23e08, p0, p1);
		}
		static void _0xa0cefcea390aab9b(type::any p0) {
			invoker::invoke<void>(0xa0cefcea390aab9b, p0);
		}
		static bool is_look_inverted() {
			return invoker::invoke<bool>(0x77b612531280010d);
		}
		static bool _0xe1615ec03b3bb4fd() {
			return invoker::invoke<bool>(0xe1615ec03b3bb4fd);
		}
		static int get_local_player_aim_state() {
			return invoker::invoke<int>(0xbb41afbbbc0a0287);
		}
		static type::any _0x59b9a7af4c95133c() {
			return invoker::invoke<type::any>(0x59b9a7af4c95133c);
		}
		static bool _0x0f70731baccfbb96() {
			return invoker::invoke<bool>(0x0f70731baccfbb96);
		}
		static bool _0xfc859e2374407556() {
			return invoker::invoke<bool>(0xfc859e2374407556);
		}
		static void set_playerpad_shakes_when_controller_disabled(bool toggle) {
			invoker::invoke<void>(0x798fdeb5b1575088, toggle);
		}
		static void set_input_exclusive(int inputgroup, int control) {
			invoker::invoke<void>(0xede476e5ee29edb1, inputgroup, control);
		}
		static void disable_control_action(int inputgroup, int control, bool disable) {
			invoker::invoke<void>(0xfe99b66d079cf6bc, inputgroup, control, disable);
		}
		static void enable_control_action(int inputgroup, int control, bool enable) {
			invoker::invoke<void>(0x351220255d64c155, inputgroup, control, enable);
		}
		static void disable_all_control_actions(int inputgroup) {
			invoker::invoke<void>(0x5f4b6931816e599b, inputgroup);
		}
		static void enable_all_control_actions(int inputgroup) {
			invoker::invoke<void>(0xa5ffe9b05f199de7, inputgroup);
		}
		static bool _0x3d42b92563939375(const char* p0) {
			return invoker::invoke<bool>(0x3d42b92563939375, p0);
		}
		static bool _0x4683149ed1dde7a1(const char* p0) {
			return invoker::invoke<bool>(0x4683149ed1dde7a1, p0);
		}
		static void _0x643ed62d5ea3bebd() {
			invoker::invoke<void>(0x643ed62d5ea3bebd);
		}
		static void _disable_input_group(int inputgroup) {
			invoker::invoke<void>(0x7f4724035fdca1dd, inputgroup);
		}
	}

	namespace datafile {
		static void _0xad6875bbc0fc899c(type::blip x) {
			invoker::invoke<void>(0xad6875bbc0fc899c, x);
		}
		static void _0x6cc86e78358d5119() {
			invoker::invoke<void>(0x6cc86e78358d5119);
		}
		static bool _0xfccae5b92a830878(type::any p0) {
			return invoker::invoke<bool>(0xfccae5b92a830878, p0);
		}
		static bool _0x15ff52b809db2353(type::any p0) {
			return invoker::invoke<bool>(0x15ff52b809db2353, p0);
		}
		static bool _0xf8cc1ebe0b62e29f(type::any p0) {
			return invoker::invoke<bool>(0xf8cc1ebe0b62e29f, p0);
		}
		static bool _0x22da66936e0fff37(type::any p0) {
			return invoker::invoke<bool>(0x22da66936e0fff37, p0);
		}
		static bool _0x8f5ea1c01d65a100(type::any p0) {
			return invoker::invoke<bool>(0x8f5ea1c01d65a100, p0);
		}
		static bool _0xc84527e235fca219(const char* p0, bool p1, const char* p2, type::any* p3, type::any* p4, const char* type, bool p6) {
			return invoker::invoke<bool>(0xc84527e235fca219, p0, p1, p2, p3, p4, type, p6);
		}
		static bool _0xa5efc3e847d60507(const char* p0, const char* p1, const char* p2, const char* p3, bool p4) {
			return invoker::invoke<bool>(0xa5efc3e847d60507, p0, p1, p2, p3, p4);
		}
		static bool _0x648e7a5434af7969(const char* p0, type::any* p1, bool p2, type::any* p3, type::any* p4, type::any* p5, const char* type) {
			return invoker::invoke<bool>(0x648e7a5434af7969, p0, p1, p2, p3, p4, p5, type);
		}
		static bool _0x4645de9980999e93(const char* p0, const char* p1, const char* p2, const char* p3, const char* type) {
			return invoker::invoke<bool>(0x4645de9980999e93, p0, p1, p2, p3, type);
		}
		static bool _0x692d808c34a82143(const char* p0, float p1, const char* type) {
			return invoker::invoke<bool>(0x692d808c34a82143, p0, p1, type);
		}
		static bool _0xa69ac4ade82b57a4(int p0) {
			return invoker::invoke<bool>(0xa69ac4ade82b57a4, p0);
		}
		static bool _0x9cb0bfa7a9342c3d(int p0, bool p1) {
			return invoker::invoke<bool>(0x9cb0bfa7a9342c3d, p0, p1);
		}
		static bool _0x52818819057f2b40(int p0) {
			return invoker::invoke<bool>(0x52818819057f2b40, p0);
		}
		static bool _0x01095c95cd46b624(int p0) {
			return invoker::invoke<bool>(0x01095c95cd46b624, p0);
		}
		static bool _load_ugc_file(const char* filename) {
			return invoker::invoke<bool>(0xc5238c011af405e4, filename);
		}
		static void datafile_create() {
			invoker::invoke<void>(0xd27058a1ca2b13ee);
		}
		static void datafile_delete() {
			invoker::invoke<void>(0x9ab9c1cfc8862dfb);
		}
		static void _0x2ed61456317b8178() {
			invoker::invoke<void>(0x2ed61456317b8178);
		}
		static void _0xc55854c7d7274882() {
			invoker::invoke<void>(0xc55854c7d7274882);
		}
		static const char* datafile_get_file_dict() {
			return invoker::invoke<const char*>(0x906b778ca1dc72b6);
		}
		static bool _0x83bcce3224735f05(const char* filename) {
			return invoker::invoke<bool>(0x83bcce3224735f05, filename);
		}
		static bool _0x4dfdd9eb705f8140(bool* p0) {
			return invoker::invoke<bool>(0x4dfdd9eb705f8140, p0);
		}
		static bool datafile_is_save_pending() {
			return invoker::invoke<bool>(0xbedb96a7584aa8cf);
		}
		static void _object_value_add_boolean(type::any* objectdata, const char* key, bool value) {
			invoker::invoke<void>(0x35124302a556a325, objectdata, key, value);
		}
		static void _object_value_add_integer(type::any* objectdata, const char* key, int value) {
			invoker::invoke<void>(0xe7e035450a7948d5, objectdata, key, value);
		}
		static void _object_value_add_float(type::any* objectdata, const char* key, float value) {
			invoker::invoke<void>(0xc27e1cc2d795105e, objectdata, key, value);
		}
		static void _object_value_add_string(type::any* objectdata, const char* key, const char* value) {
			invoker::invoke<void>(0x8ff3847dadd8e30c, objectdata, key, value);
		}
		static void _object_value_add_PVector3(type::any* objectdata, const char* key, float valuex, float valuey, float valuez) {
			invoker::invoke<void>(0x4cd49b76338c7dee, objectdata, key, valuex, valuey, valuez);
		}
		static type::any* _object_value_add_object(type::any* objectdata, const char* key) {
			return invoker::invoke<type::any*>(0xa358f56f10732ee1, objectdata, key);
		}
		static type::any* _object_value_add_array(type::any* objectdata, const char* key) {
			return invoker::invoke<type::any*>(0x5b11728527ca6e5f, objectdata, key);
		}
		static bool _object_value_get_boolean(type::any* objectdata, const char* key) {
			return invoker::invoke<bool>(0x1186940ed72ffeec, objectdata, key);
		}
		static int _object_value_get_integer(type::any* objectdata, const char* key) {
			return invoker::invoke<int>(0x78f06f6b1fb5a80c, objectdata, key);
		}
		static float _object_value_get_float(type::any* objectdata, const char* key) {
			return invoker::invoke<float>(0x06610343e73b9727, objectdata, key);
		}
		static const char* _object_value_get_string(type::any* objectdata, const char* key) {
			return invoker::invoke<const char*>(0x3d2fd9e763b24472, objectdata, key);
		}
		static PVector3 _object_value_get_PVector3(type::any* objectdata, const char* key) {
			return invoker::invoke<PVector3>(0x46cd3cb66e0825cc, objectdata, key);
		}
		static type::any* _object_value_get_object(type::any* scloudfile, const char* key) {
			return invoker::invoke<type::any*>(0xb6b9ddc412fceee2, scloudfile, key);
		}
		static type::any* _object_value_get_array(type::any* objectdata, const char* key) {
			return invoker::invoke<type::any*>(0x7a983aa9da2659ed, objectdata, key);
		}
		static int _object_value_get_type(type::any* objectdata, const char* key) {
			return invoker::invoke<int>(0x031c55ed33227371, objectdata, key);
		}
		static void _array_value_add_boolean(type::any* arraydata, bool value) {
			invoker::invoke<void>(0xf8b0f5a43e928c76, arraydata, value);
		}
		static void _array_value_add_integer(type::any* arraydata, int value) {
			invoker::invoke<void>(0xcabdb751d86fe93b, arraydata, value);
		}
		static void _array_value_add_float(type::any* arraydata, float value) {
			invoker::invoke<void>(0x57a995fd75d37f56, arraydata, value);
		}
		static void _array_value_add_string(type::any* arraydata, const char* value) {
			invoker::invoke<void>(0x2f0661c155aeeeaa, arraydata, value);
		}
		static void _array_value_add_PVector3(type::any* arraydata, float valuex, float valuey, float valuez) {
			invoker::invoke<void>(0x407f8d034f70f0c2, arraydata, valuex, valuey, valuez);
		}
		static type::any* _array_value_add_object(type::any* arraydata) {
			return invoker::invoke<type::any*>(0x6889498b3e19c797, arraydata);
		}
		static bool _array_value_get_boolean(type::any* arraydata, int arrayindex) {
			return invoker::invoke<bool>(0x50c1b2874e50c114, arraydata, arrayindex);
		}
		static int _array_value_get_integer(type::any* arraydata, int arrayindex) {
			return invoker::invoke<int>(0x3e5ae19425cd74be, arraydata, arrayindex);
		}
		static float _array_value_get_float(type::any* arraydata, int arrayindex) {
			return invoker::invoke<float>(0xc0c527b525d7cfb5, arraydata, arrayindex);
		}
		static const char* _array_value_get_string(type::any* arraydata, int arrayindex) {
			return invoker::invoke<const char*>(0xd3f2ffeb8d836f52, arraydata, arrayindex);
		}
		static PVector3 _array_value_get_PVector3(type::any* arraydata, int arrayindex) {
			return invoker::invoke<PVector3>(0x8d2064e5b64a628a, arraydata, arrayindex);
		}
		static type::any* _array_value_get_object(type::any* arraydata, int arrayindex) {
			return invoker::invoke<type::any*>(0x8b5fadcc4e3a145f, arraydata, arrayindex);
		}
		static int _array_value_get_size(type::any* arraydata) {
			return invoker::invoke<int>(0x065db281590cea2d, arraydata);
		}
		static int _array_value_get_type(type::any* arraydata, int arrayindex) {
			return invoker::invoke<int>(0x3a0014adb172a3c5, arraydata, arrayindex);
		}
	}

	namespace fire {
		static type::hash start_script_fire(float x, float y, float z, int maxchildren, bool isgasfire) {
			return invoker::invoke<type::hash>(0x6b83617e04503888, x, y, z, maxchildren, isgasfire);
		}
		static void remove_script_fire(int firehandle) {
			invoker::invoke<void>(0x7ff548385680673f, firehandle);
		}
		static type::ped start_entity_fire(type::ped entity) {
			return invoker::invoke<type::ped>(0xf6a9d9708f6f23df, entity);
		}
		static void stop_entity_fire(type::entity entity) {
			invoker::invoke<void>(0x7f0dd2ebbb651aff, entity);
		}
		static bool is_entity_on_fire(type::entity entity) {
			return invoker::invoke<bool>(0x28d3fed7190d3a0b, entity);
		}
		static int get_number_of_fires_in_range(float x, float y, float z, float radius) {
			return invoker::invoke<int>(0x50cad495a460b305, x, y, z, radius);
		}
		static void stop_fire_in_range(float x, float y, float z, float radius) {
			invoker::invoke<void>(0x056a8a219b8e829f, x, y, z, radius);
		}
		static bool get_closest_fire_pos(PVector3* outposition, float x, float y, float z) {
			return invoker::invoke<bool>(0x352a9f6bcf90081f, outposition, x, y, z);
		}
		static void add_explosion(float x, float y, float z, int explosiontype, float damagescale, bool isaudible, bool isinvisible, float camerashake) {
			invoker::invoke<void>(0xe3ad2bdbaee269ac, x, y, z, explosiontype, damagescale, isaudible, isinvisible, camerashake);
		}
		static void add_owned_explosion(type::ped ped, float x, float y, float z, int explosiontype, float damagescale, bool isaudible, bool isinvisible, float camerashake) {
			invoker::invoke<void>(0x172aa1b624fa1013, ped, x, y, z, explosiontype, damagescale, isaudible, isinvisible, camerashake);
		}
		static void add_explosion_with_user_vfx(type::entity x, type::entity y, type::entity z, int explosiontype, type::hash explosionfx, float damagescale, bool isaudible, bool isinvisible, float camerashake) {
			invoker::invoke<void>(0x36dd3fe58b5e5212, x, y, z, explosiontype, explosionfx, damagescale, isaudible, isinvisible, camerashake);
		}
		static bool is_explosion_in_area(int explosiontype, float x1, float y1, float z1, float x2, float y2, float z2) {
			return invoker::invoke<bool>(0x2e2eba0ee7ced0e0, explosiontype, x1, y1, z1, x2, y2, z2);
		}
		static int _0x6070104b699b2ef4(int explosiontype, float x1, float y1, float z1, float x2, float y2, float z2) {
			return invoker::invoke<int>(0x6070104b699b2ef4, explosiontype, x1, y1, z1, x2, y2, z2);
		}
		static bool is_explosion_in_sphere(int explosiontype, float x, float y, float z, float radius) {
			return invoker::invoke<bool>(0xab0f816885b0e483, explosiontype, x, y, z, radius);
		}
		static bool is_explosion_in_angled_area(int explosiontype, float x1, float y1, float z1, float x2, float y2, float z2, float angle) {
			return invoker::invoke<bool>(0xa079a6c51525dc4b, explosiontype, x1, y1, z1, x2, y2, z2, angle);
		}
		static type::entity _get_ped_inside_explosion_area(int explosiontype, float x1, float y1, float z1, float x2, float y2, float z2, float radius) {
			return invoker::invoke<type::entity>(0x14ba4ba137af6cec, explosiontype, x1, y1, z1, x2, y2, z2, radius);
		}
	}

	namespace decisionevent {
		static void set_decision_maker(type::ped ped, type::hash name) {
			invoker::invoke<void>(0xb604a2942aded0ee, ped, name);
		}
		static void clear_decision_maker_event_response(type::hash name, int type) {
			invoker::invoke<void>(0x4fc9381a7aee8968, name, type);
		}
		static void block_decision_maker_event(type::hash name, int type) {
			invoker::invoke<void>(0xe42fcdfd0e4196f7, name, type);
		}
		static void unblock_decision_maker_event(type::hash name, int type) {
			invoker::invoke<void>(0xd7cd9cf34f2c99e8, name, type);
		}
		static type::scrhandle add_shocking_event_at_position(int type, float x, float y, float z, float duration) {
			return invoker::invoke<type::scrhandle>(0xd9f8455409b525e9, type, x, y, z, duration);
		}
		static type::scrhandle add_shocking_event_for_entity(int type, type::entity entity, float duration) {
			return invoker::invoke<type::scrhandle>(0x7fd8f3be76f89422, type, entity, duration);
		}
		static bool is_shocking_event_in_sphere(int type, float x, float y, float z, float radius) {
			return invoker::invoke<bool>(0x1374abb7c15bab92, type, x, y, z, radius);
		}
		static bool remove_shocking_event(type::scrhandle event) {
			return invoker::invoke<bool>(0x2cda538c44c6cce5, event);
		}
		static void remove_all_shocking_events(bool p0) {
			invoker::invoke<void>(0xeaabe8fdfa21274c, p0);
		}
		static void remove_shocking_event_spawn_blocking_areas() {
			invoker::invoke<void>(0x340f1415b68aeade);
		}
		static void suppress_shocking_events_next_frame() {
			invoker::invoke<void>(0x2f9a292ad0a3bd89);
		}
		static void suppress_shocking_event_type_next_frame(int type) {
			invoker::invoke<void>(0x3fd2ec8bf1f1cf30, type);
		}
		static void suppress_agitation_events_next_frame() {
			invoker::invoke<void>(0x5f3b7749c112d552);
		}
	}

	namespace zone {
		static int get_zone_at_coords(float x, float y, float z) {
			return invoker::invoke<int>(0x27040c25de6cb2f4, x, y, z);
		}
		static int get_zone_from_name_id(const char* zonename) {
			return invoker::invoke<int>(0x98cd1d2934b76cc1, zonename);
		}
		static int get_zone_popschedule(int zoneid) {
			return invoker::invoke<int>(0x4334bc40aa0cb4bb, zoneid);
		}
		static const char* get_name_of_zone(float x, float y, float z) {
			return invoker::invoke<const char*>(0xcd90657d4c30e1ca, x, y, z);
		}
		static void set_zone_enabled(int zoneid, bool toggle) {
			invoker::invoke<void>(0xba5eceea120e5611, zoneid, toggle);
		}
		static int get_zone_scumminess(int zoneid) {
			return invoker::invoke<int>(0x5f7b268d15ba0739, zoneid);
		}
		static void override_popschedule_vehicle_model(int scheduleid, type::hash vehiclehash) {
			invoker::invoke<void>(0x5f7d596bac2e7777, scheduleid, vehiclehash);
		}
		static void clear_popschedule_override_vehicle_model(int scheduleid) {
			invoker::invoke<void>(0x5c0de367aa0d911c, scheduleid);
		}
		static type::hash get_hash_of_map_area_at_coords(float x, float y, float z) {
			return invoker::invoke<type::hash>(0x7ee64d51e8498728, x, y, z);
		}
	}

	namespace rope {
		static type::object add_rope(float x, float y, float z, float rotx, float roty, float rotz, float length, int ropetype, float maxlength, float minlength, float p10, bool p11, bool p12, bool rigid, float p14, bool breakwhenshot, type::any* unkptr) {
			return invoker::invoke<type::object>(0xe832d760399eb220, x, y, z, rotx, roty, rotz, length, ropetype, maxlength, minlength, p10, p11, p12, rigid, p14, breakwhenshot, unkptr);
		}
		static void delete_rope(type::object* rope) {
			invoker::invoke<void>(0x52b4829281364649, rope);
		}
		static void delete_child_rope(type::object rope) {
			invoker::invoke<void>(0xaa5d6b1888e4db20, rope);
		}
		static bool does_rope_exist(type::object* rope) {
			return invoker::invoke<bool>(0xfd5448be3111ed96, rope);
		}
		static void rope_draw_shadow_enabled(type::object* rope, bool toggle) {
			invoker::invoke<void>(0xf159a63806bb5ba8, rope, toggle);
		}
		static void load_rope_data(type::object rope, const char* rope_preset) {
			invoker::invoke<void>(0xcbb203c04d1abd27, rope, rope_preset);
		}
		static void pin_rope_vertex(type::object rope, int vertex, float x, float y, float z) {
			invoker::invoke<void>(0x2b320cf14146b69a, rope, vertex, x, y, z);
		}
		static void unpin_rope_vertex(type::object rope, int vertex) {
			invoker::invoke<void>(0x4b5ae2eee4a8f180, rope, vertex);
		}
		static int get_rope_vertex_count(type::object rope) {
			return invoker::invoke<int>(0x3655f544cd30f0b5, rope);
		}
		static void attach_entities_to_rope(type::object rope, type::entity ent1, type::entity ent2, float ent1_x, float ent1_y, float ent1_z, float ent2_x, float ent2_y, float ent2_z, float length, bool p10, bool p11, const char* bonename1, const char* bonename2) {
			invoker::invoke<void>(0x3d95ec8b6d940ac3, rope, ent1, ent2, ent1_x, ent1_y, ent1_z, ent2_x, ent2_y, ent2_z, length, p10, p11, bonename1, bonename2);
		}
		static void attach_rope_to_entity(type::object rope, type::entity entity, float x, float y, float z, bool p5) {
			invoker::invoke<void>(0x4b490a6832559a65, rope, entity, x, y, z, p5);
		}
		static void detach_rope_from_entity(type::object rope, type::entity entity) {
			invoker::invoke<void>(0xbcf3026912a8647d, rope, entity);
		}
		static void rope_set_update_pinverts(type::object rope) {
			invoker::invoke<void>(0xc8d667ee52114aba, rope);
		}
		static void _hide_rope(type::object rope, int value) {
			invoker::invoke<void>(0xdc57a637a20006ed, rope, value);
		}
		static void _0x36ccb9be67b970fd(type::object rope, bool p1) {
			invoker::invoke<void>(0x36ccb9be67b970fd, rope, p1);
		}
		static bool _0x84de3b5fb3e666f0(type::object rope) {
			return invoker::invoke<bool>(0x84de3b5fb3e666f0, rope);
		}
		static PVector3 get_rope_last_vertex_coord(type::object rope) {
			return invoker::invoke<PVector3>(0x21bb0fbd3e217c2d, rope);
		}
		static PVector3 get_rope_vertex_coord(type::object rope, int vertex) {
			return invoker::invoke<PVector3>(0xea61ca8e80f09e4d, rope, vertex);
		}
		static void start_rope_winding(type::object rope) {
			invoker::invoke<void>(0x1461c72c889e343e, rope);
		}
		static void stop_rope_winding(type::object rope) {
			invoker::invoke<void>(0xcb2d4ab84a19aa7c, rope);
		}
		static void start_rope_unwinding_front(type::object rope) {
			invoker::invoke<void>(0x538d1179ec1aa9a9, rope);
		}
		static void stop_rope_unwinding_front(type::object rope) {
			invoker::invoke<void>(0xfff3a50779efbbb3, rope);
		}
		static void rope_convert_to_simple(type::object rope) {
			invoker::invoke<void>(0x5389d48efa2f079a, rope);
		}
		static void rope_load_textures() {
			invoker::invoke<void>(0x9b9039dbf2d258c1);
		}
		static bool rope_are_textures_loaded() {
			return invoker::invoke<bool>(0xf2d0e6a75cc05597);
		}
		static void rope_unload_textures() {
			invoker::invoke<void>(0x6ce36c35c1ac8163);
		}
		static bool _0x271c9d3aca5d6409(type::object rope) {
			return invoker::invoke<bool>(0x271c9d3aca5d6409, rope);
		}
		static void _0xbc0ce682d4d05650(type::object rope, int unk, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, float x4, float y4, float z4) {
			invoker::invoke<void>(0xbc0ce682d4d05650, rope, unk, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4);
		}
		static void _0xb1b6216ca2e7b55e(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0xb1b6216ca2e7b55e, p0, p1, p2);
		}
		static void _0xb743f735c03d7810(type::any p0, type::any p1) {
			invoker::invoke<void>(0xb743f735c03d7810, p0, p1);
		}
		static float _get_rope_length(type::object rope) {
			return invoker::invoke<float>(0x73040398dff9a4a6, rope);
		}
		static void rope_force_length(type::object rope, float length) {
			invoker::invoke<void>(0xd009f759a723db1b, rope, length);
		}
		static void rope_reset_length(type::object rope, float length) {
			invoker::invoke<void>(0xc16de94d9bea14a0, rope, length);
		}
		static void apply_impulse_to_cloth(float posx, float posy, float posz, float vecx, float vecy, float vecz, float impulse) {
			invoker::invoke<void>(0xe37f721824571784, posx, posy, posz, vecx, vecy, vecz, impulse);
		}
		static void set_damping(type::object ropeorobject, int vertex, float value) {
			invoker::invoke<void>(0xeea3b200a6feb65b, ropeorobject, vertex, value);
		}
		static void activate_physics(type::entity entity) {
			invoker::invoke<void>(0x710311adf0e20730, entity);
		}
		static void set_cgoffset(type::object rope, float x, float y, float z) {
			invoker::invoke<void>(0xd8fa3908d7b86904, rope, x, y, z);
		}
		static PVector3 get_cgoffset(type::object rope) {
			return invoker::invoke<PVector3>(0x8214a4b5a7a33612, rope);
		}
		static void set_cg_at_boundcenter(type::object rope) {
			invoker::invoke<void>(0xbe520d9761ff811f, rope);
		}
		static void break_entity_glass(type::object object, float posx, float posy, float posz, float p4, float offsetx, float offsety, float offsetz, float p8, int p9, bool p10) {
			invoker::invoke<void>(0x2e648d16f6e308f3, object, posx, posy, posz, p4, offsetx, offsety, offsetz, p8, p9, p10);
		}
		static void set_disable_breaking(type::object rope, bool enabled) {
			invoker::invoke<void>(0x5cec1a84620e7d5b, rope, enabled);
		}
		static void _0xcc6e963682533882(type::object object) {
			invoker::invoke<void>(0xcc6e963682533882, object);
		}
		static void set_disable_frag_damage(type::object object, bool toggle) {
			invoker::invoke<void>(0x01ba3aed21c16cfb, object, toggle);
		}
	}

	namespace water {
		static bool get_water_height(float x, float y, float a, float* height) {
			return invoker::invoke<bool>(0xf6829842c06ae524, x, y, a, height);
		}
		static bool get_water_height_no_waves(float b, float y, float z, float* height) {
			return invoker::invoke<bool>(0x8ee6b53ce13a9794, b, y, z, height);
		}
		static bool test_probe_against_water(float x1, float y1, float z1, float x2, float y2, float z2, PVector3* result) {
			return invoker::invoke<bool>(0xffa5d878809819db, x1, y1, z1, x2, y2, z2, result);
		}
		static int test_probe_against_all_water(float x1, float y1, float z1, float x2, float y2, float z2, int type, PVector3* result) {
			return invoker::invoke<int>(0x8974647ed222ea5f, x1, y1, z1, x2, y2, z2, type, result);
		}
		static bool test_vertical_probe_against_all_water(float x, float y, float z, float p3, float* height) {
			return invoker::invoke<bool>(0x2b3451fa1e3142e2, x, y, z, p3, height);
		}
		static void modify_water(float x, float y, float radius, float height) {
			invoker::invoke<void>(0xc443fd757c3ba637, x, y, radius, height);
		}
		static int _add_current_rise(float xlow, float ylow, float xhigh, float yhigh, float height) {
			return invoker::invoke<int>(0xfdbf4cdbc07e1706, xlow, ylow, xhigh, yhigh, height);
		}
		static void _remove_current_rise(int risehandle) {
			invoker::invoke<void>(0xb1252e3e59a82aaf, risehandle);
		}
		static void _set_current_intensity(float intensity) {
			invoker::invoke<void>(0xb96b00e976be977f, intensity);
		}
		static float _get_current_intensity() {
			return invoker::invoke<float>(0x2b2a2cc86778b619);
		}
		static void _reset_current_intensity() {
			invoker::invoke<void>(0x5e5e99285ae812db);
		}
	}

	namespace worldprobe {
		static int start_shape_test_los_probe(float x1, float y1, float z1, float x2, float y2, float z2, int flags, type::entity ent, int p8) {
			return invoker::invoke<int>(0x7ee9f5d83dd4f90e, x1, y1, z1, x2, y2, z2, flags, ent, p8);
		}
		static int _start_shape_test_ray(float x1, float y1, float z1, float x2, float y2, float z2, int flags, type::entity entity, int p8) {
			return invoker::invoke<int>(0x377906d8a31e5586, x1, y1, z1, x2, y2, z2, flags, entity, p8);
		}
		static int start_shape_test_bounding_box(type::entity entity, int flags1, int flags2) {
			return invoker::invoke<int>(0x052837721a854ec7, entity, flags1, flags2);
		}
		static int start_shape_test_box(float x, float y, float z, float x1, float y2, float z2, float rotx, float roty, float rotz, type::any p9, type::any p10, type::any entity, type::any p12) {
			return invoker::invoke<int>(0xfe466162c4401d18, x, y, z, x1, y2, z2, rotx, roty, rotz, p9, p10, entity, p12);
		}
		static int start_shape_test_bound(type::entity entity, int flags1, int flags2) {
			return invoker::invoke<int>(0x37181417ce7c8900, entity, flags1, flags2);
		}
		static int start_shape_test_capsule(float x1, float y1, float z1, float x2, float y2, float z2, float radius, int flags, type::entity entity, int p9) {
			return invoker::invoke<int>(0x28579d1b8f8aac80, x1, y1, z1, x2, y2, z2, radius, flags, entity, p9);
		}
		static int _start_shape_test_capsule_2(float x1, float y1, float z1, float x2, float y2, float z2, float radius, int flags, type::entity entity, type::any p9) {
			return invoker::invoke<int>(0xe6ac6c45fbe83004, x1, y1, z1, x2, y2, z2, radius, flags, entity, p9);
		}
		static int _start_shape_test_surrounding_coords(PVector3* pvec1, PVector3* pvec2, int flag, type::entity entity, int flag2) {
			return invoker::invoke<int>(0xff6be494c7987f34, pvec1, pvec2, flag, entity, flag2);
		}
		static int get_shape_test_result(int rayhandle, bool* hit, PVector3* endcoords, PVector3* surfacenormal, type::entity* entityhit) {
			return invoker::invoke<int>(0x3d87450e15d98694, rayhandle, hit, endcoords, surfacenormal, entityhit);
		}
		static int _get_shape_test_result_ex(int rayhandle, bool* hit, PVector3* endcoords, PVector3* surfacenormal, type::hash* materialhash, type::entity* entityhit) {
			return invoker::invoke<int>(0x65287525d951f6be, rayhandle, hit, endcoords, surfacenormal, materialhash, entityhit);
		}
		static void _shape_test_result_entity(type::hash entityhit) {
			invoker::invoke<void>(0x2b3334bca57cd799, entityhit);
		}
	}

	namespace network {
		static bool network_is_signed_in() {
			return invoker::invoke<bool>(0x054354a99211eb96);
		}
		static bool network_is_signed_online() {
			return invoker::invoke<bool>(0x1077788e268557c2);
		}
		static bool _0xbd545d44cce70597() {
			return invoker::invoke<bool>(0xbd545d44cce70597);
		}
		static bool _0xebcab9e5048434f4() {
			return invoker::invoke<bool>(0xebcab9e5048434f4);
		}
		static bool _0x74fb3e29e6d10fa9() {
			return invoker::invoke<bool>(0x74fb3e29e6d10fa9);
		}
		static type::any _0x7808619f31ff22db() {
			return invoker::invoke<type::any>(0x7808619f31ff22db);
		}
		static type::any _0xa0fa4ec6a05da44e() {
			return invoker::invoke<type::any>(0xa0fa4ec6a05da44e);
		}
		static bool _network_are_ros_available() {
			return invoker::invoke<bool>(0x85443ff4c328f53b);
		}
		static bool _network_is_np_available() {
			return invoker::invoke<bool>(0x8d11e61a4abf49cc);
		}
		static bool network_is_cloud_available() {
			return invoker::invoke<bool>(0x9a4cf4f48ad77302);
		}
		static bool _0x67a5589628e0cff6() {
			return invoker::invoke<bool>(0x67a5589628e0cff6);
		}
		static type::any _0xba9775570db788cf() {
			return invoker::invoke<type::any>(0xba9775570db788cf);
		}
		static bool network_is_host() {
			return invoker::invoke<bool>(0x8db296b814edda07);
		}
		static type::any _0xa306f470d1660581() {
			return invoker::invoke<type::any>(0xa306f470d1660581);
		}
		static bool _0x4237e822315d8ba9() {
			return invoker::invoke<bool>(0x4237e822315d8ba9);
		}
		static bool network_have_online_privileges() {
			return invoker::invoke<bool>(0x25cb5a9f37bfd063);
		}
		static bool _network_has_restricted_profile() {
			return invoker::invoke<bool>(0x1353f87e89946207);
		}
		static type::any _72d918c99bcacc54(type::any p0) {
			return invoker::invoke<type::any>(0x72d918c99bcacc54, p0);
		}
		static bool _0xaeef48cdf5b6ce7c(type::any p0, type::any p1) {
			return invoker::invoke<bool>(0xaeef48cdf5b6ce7c, p0, p1);
		}
		static bool _0x78321bea235fd8cd(type::any p0, bool p1) {
			return invoker::invoke<bool>(0x78321bea235fd8cd, p0, p1);
		}
		static bool _0x595f028698072dd9(type::any p0, type::any p1, bool p2) {
			return invoker::invoke<bool>(0x595f028698072dd9, p0, p1, p2);
		}
		static bool _0x83f28ce49fbbffba(type::any p0, type::any p1, bool p2) {
			return invoker::invoke<bool>(0x83f28ce49fbbffba, p0, p1, p2);
		}
		static type::any* _0x76bf03fadbf154f5() {
			return invoker::invoke<type::any*>(0x76bf03fadbf154f5);
		}
		static type::any _0x9614b71f8adb982b() {
			return invoker::invoke<type::any>(0x9614b71f8adb982b);
		}
		static type::any _0x5ea784d197556507() {
			return invoker::invoke<type::any>(0x5ea784d197556507);
		}
		static type::any _0xa8acb6459542a8c8() {
			return invoker::invoke<type::any>(0xa8acb6459542a8c8);
		}
		static void _0x83fe8d7229593017() {
			invoker::invoke<void>(0x83fe8d7229593017);
		}
		static bool network_can_bail() {
			return invoker::invoke<bool>(0x580ce4438479cc61);
		}
		static void network_bail() {
			invoker::invoke<void>(0x95914459a87eba28);
		}
		static void _0x283b6062a2c01e9b() {
			invoker::invoke<void>(0x283b6062a2c01e9b);
		}
		static bool network_can_access_multiplayer(int* loadingstate) {
			return invoker::invoke<bool>(0xaf50da1a3f8b1ba4, loadingstate);
		}
		static bool network_is_multiplayer_disabled() {
			return invoker::invoke<bool>(0x9747292807126eda);
		}
		static bool network_can_enter_multiplayer() {
			return invoker::invoke<bool>(0x7e782a910c362c25);
		}
		static type::any network_session_enter(type::any p0, type::any p1, type::any p2, int maxplayers, type::any p4, type::any p5) {
			return invoker::invoke<type::any>(0x330ed4d05491934f, p0, p1, p2, maxplayers, p4, p5);
		}
		static bool network_session_friend_matchmaking(int p0, int p1, int maxplayers, bool p3) {
			return invoker::invoke<bool>(0x2cfc76e0d087c994, p0, p1, maxplayers, p3);
		}
		static bool network_session_crew_matchmaking(int p0, int p1, int p2, int maxplayers, bool p4) {
			return invoker::invoke<bool>(0x94bc51e9449d917f, p0, p1, p2, maxplayers, p4);
		}
		static bool network_session_activity_quickmatch(type::any p0, type::any p1, type::any p2, type::any p3) {
			return invoker::invoke<bool>(0xbe3e347a87aceb82, p0, p1, p2, p3);
		}
		static bool network_session_host(int p0, int maxplayers, bool p2) {
			return invoker::invoke<bool>(0x6f3d4ed9bee4e61d, p0, maxplayers, p2);
		}
		static bool network_session_host_closed(int p0, int maxplayers) {
			return invoker::invoke<bool>(0xed34c0c02c098bb7, p0, maxplayers);
		}
		static bool network_session_host_friends_only(int p0, int maxplayers) {
			return invoker::invoke<bool>(0xb9cfd27a5d578d83, p0, maxplayers);
		}
		static bool network_session_is_closed_friends() {
			return invoker::invoke<bool>(0xfbcfa2ea2e206890);
		}
		static bool network_session_is_closed_crew() {
			return invoker::invoke<bool>(0x74732c6ca90da2b4);
		}
		static bool network_session_is_solo() {
			return invoker::invoke<bool>(0xf3929c2379b60cce);
		}
		static bool network_session_is_private() {
			return invoker::invoke<bool>(0xcef70aa5b3f89ba1);
		}
		static bool network_session_end(bool p0, bool p1) {
			return invoker::invoke<bool>(0xa02e59562d711006, p0, p1);
		}
		static void network_session_kick_player(type::player player) {
			invoker::invoke<void>(0xfa8904dc5f304220, player);
		}
		static bool _network_session_is_player_voted_to_kick(type::player player) {
			return invoker::invoke<bool>(0xd6d09a6f32f49ef1, player);
		}
		static type::any _0x59df79317f85a7e0() {
			return invoker::invoke<type::any>(0x59df79317f85a7e0);
		}
		static type::any _0xffe1e5b792d92b34() {
			return invoker::invoke<type::any>(0xffe1e5b792d92b34);
		}
		static void _network_sctv_slots(int p0) {
			invoker::invoke<void>(0x49ec8030f5015f8b, p0);
		}
		static void _network_session_set_max_players(int playertype, int playercount) {
			invoker::invoke<void>(0x8b6a4dd0af9ce215, playertype, playercount);
		}
		static int _network_session_get_unk(int p0) {
			return invoker::invoke<int>(0x56ce820830ef040b, p0);
		}
		static void _0xcae55f48d3d7875c(type::any p0) {
			invoker::invoke<void>(0xcae55f48d3d7875c, p0);
		}
		static void _0xf49abc20d8552257(type::any p0) {
			invoker::invoke<void>(0xf49abc20d8552257, p0);
		}
		static void _0x4811bbac21c5fcd5(type::any p0) {
			invoker::invoke<void>(0x4811bbac21c5fcd5, p0);
		}
		static void _0x5539c3ebf104a53a(bool p0) {
			invoker::invoke<void>(0x5539c3ebf104a53a, p0);
		}
		static void _0x702bc4d605522539(type::any p0) {
			invoker::invoke<void>(0x702bc4d605522539, p0);
		}
		static void _0x3f52e880aaf6c8ca(bool p0) {
			invoker::invoke<void>(0x3f52e880aaf6c8ca, p0);
		}
		static void _0xf1eea2dda9ffa69d(type::any p0) {
			invoker::invoke<void>(0xf1eea2dda9ffa69d, p0);
		}
		static void _0x1153fa02a659051c() {
			invoker::invoke<void>(0x1153fa02a659051c);
		}
		static void _network_session_hosted(bool p0) {
			invoker::invoke<void>(0xc19f6c8e7865a6ff, p0);
		}
		static void network_add_followers(int* p0, int p1) {
			invoker::invoke<void>(0x236406f60cf216d6, p0, p1);
		}
		static void network_clear_followers() {
			invoker::invoke<void>(0x058f43ec59a8631a);
		}
		static void _network_get_server_time(int* hours, int* minutes, int* seconds) {
			invoker::invoke<void>(0x6d03bfbd643b2a02, hours, minutes, seconds);
		}
		static void _0x600f8cb31c7aab6e(type::any p0) {
			invoker::invoke<void>(0x600f8cb31c7aab6e, p0);
		}
		static bool network_x_affects_gamers(int crewid) {
			return invoker::invoke<bool>(0xe532d6811b3a4d2a, crewid);
		}
		static bool network_find_matched_gamers(int attribute, float p1, float lowerlimit, float upperlimit) {
			return invoker::invoke<bool>(0xf7b2cfde5c9f700d, attribute, p1, lowerlimit, upperlimit);
		}
		static bool network_is_finding_gamers() {
			return invoker::invoke<bool>(0xdddf64c91bfcf0aa);
		}
		static bool _0xf9b83b77929d8863() {
			return invoker::invoke<bool>(0xf9b83b77929d8863);
		}
		static int network_get_num_found_gamers() {
			return invoker::invoke<int>(0xa1b043ee79a916fb);
		}
		static bool network_get_found_gamer(type::any* p0, type::any p1) {
			return invoker::invoke<bool>(0x9dcff2afb68b3476, p0, p1);
		}
		static void network_clear_found_gamers() {
			invoker::invoke<void>(0x6d14ccee1b40381a);
		}
		static bool _0x85a0ef54a500882c(type::any* p0) {
			return invoker::invoke<bool>(0x85a0ef54a500882c, p0);
		}
		static type::any _0x2cc848a861d01493() {
			return invoker::invoke<type::any>(0x2cc848a861d01493);
		}
		static type::any _0x94a8394d150b013a() {
			return invoker::invoke<type::any>(0x94a8394d150b013a);
		}
		static type::any _0x5ae17c6b0134b7f1() {
			return invoker::invoke<type::any>(0x5ae17c6b0134b7f1);
		}
		static bool _0x02a8bec6fd9af660(type::any* p0, type::any p1) {
			return invoker::invoke<bool>(0x02a8bec6fd9af660, p0, p1);
		}
		static void _0x86e0660e4f5c956d() {
			invoker::invoke<void>(0x86e0660e4f5c956d);
		}
		static void network_is_player_animation_drawing_synchronized() {
			invoker::invoke<void>(0xc6f8ab8a4189cf3a);
		}
		static void network_session_cancel_invite() {
			invoker::invoke<void>(0x2fbf47b1b36d36f9);
		}
		static void network_session_force_cancel_invite() {
			invoker::invoke<void>(0xa29177f7703b5644);
		}
		static bool network_has_pending_invite() {
			return invoker::invoke<bool>(0xac8c7b9b88c4a668);
		}
		static type::any _0xc42dd763159f3461() {
			return invoker::invoke<type::any>(0xc42dd763159f3461);
		}
		static type::any _0x62a0296c1bb1ceb3() {
			return invoker::invoke<type::any>(0x62a0296c1bb1ceb3);
		}
		static bool network_session_was_invited() {
			return invoker::invoke<bool>(0x23dfb504655d0ce4);
		}
		static void network_session_get_inviter(int* networkhandle) {
			invoker::invoke<void>(0xe57397b4a3429dd0, networkhandle);
		}
		static type::any _0xd313de83394af134() {
			return invoker::invoke<type::any>(0xd313de83394af134);
		}
		static type::any _0xbdb6f89c729cf388() {
			return invoker::invoke<type::any>(0xbdb6f89c729cf388);
		}
		static void network_suppress_invite(bool toggle) {
			invoker::invoke<void>(0xa0682d67ef1fba3d, toggle);
		}
		static void network_block_invites(bool toggle) {
			invoker::invoke<void>(0x34f9e9049454a7a0, toggle);
		}
		static void _0xcfeb8af24fc1d0bb(bool p0) {
			invoker::invoke<void>(0xcfeb8af24fc1d0bb, p0);
		}
		static void _0xf814fec6a19fd6e0() {
			invoker::invoke<void>(0xf814fec6a19fd6e0);
		}
		static void _network_block_kicked_players(bool p0) {
			invoker::invoke<void>(0x6b07b9ce4d390375, p0);
		}
		static void _0x7ac752103856fb20(bool p0) {
			invoker::invoke<void>(0x7ac752103856fb20, p0);
		}
		static bool _0x74698374c45701d2() {
			return invoker::invoke<bool>(0x74698374c45701d2);
		}
		static void _0x140e6a44870a11ce() {
			invoker::invoke<void>(0x140e6a44870a11ce);
		}
		static void network_session_host_single_player(int p0) {
			invoker::invoke<void>(0xc74c33fca52856d5, p0);
		}
		static void network_session_leave_single_player() {
			invoker::invoke<void>(0x3442775428fd2daa);
		}
		static bool network_is_game_in_progress() {
			return invoker::invoke<bool>(0x10fab35428ccc9d7);
		}
		static bool network_is_session_active() {
			return invoker::invoke<bool>(0xd83c2b94e7508980);
		}
		static bool network_is_in_session() {
			return invoker::invoke<bool>(0xca97246103b63917);
		}
		static bool network_is_session_started() {
			return invoker::invoke<bool>(0x9de624d2fc4b603f);
		}
		static bool network_is_session_busy() {
			return invoker::invoke<bool>(0xf4435d66a8e2905e);
		}
		static bool network_can_session_end() {
			return invoker::invoke<bool>(0x4eebc3694e49c572);
		}
		static void network_session_mark_visible(bool p0) {
			invoker::invoke<void>(0x271cc6ab59ebf9a5, p0);
		}
		static bool network_session_is_visible() {
			return invoker::invoke<bool>(0xba416d68c631496a);
		}
		static void network_session_block_join_requests(bool toggle) {
			invoker::invoke<void>(0xa73667484d7037c3, toggle);
		}
		static void network_session_change_slots(int p0, bool p1) {
			invoker::invoke<void>(0xb4ab419e0d86acae, p0, p1);
		}
		static int _0x53afd64c6758f2f9() {
			return invoker::invoke<int>(0x53afd64c6758f2f9);
		}
		static void network_session_voice_host() {
			invoker::invoke<void>(0x9c1556705f864230);
		}
		static void network_session_voice_leave() {
			invoker::invoke<void>(0x6793e42be02b575d);
		}
		static void network_session_voice_connect_to_player(type::any* p0) {
			invoker::invoke<void>(0xabd5e88b8a2d3db2, p0);
		}
		static void network_set_keep_focuspoint(bool p0, type::any p1) {
			invoker::invoke<void>(0x7f8413b7fc2aa6b9, p0, p1);
		}
		static void _0x5b8ed3db018927b1(type::any p0) {
			invoker::invoke<void>(0x5b8ed3db018927b1, p0);
		}
		static bool _0x855bc38818f6f684() {
			return invoker::invoke<bool>(0x855bc38818f6f684);
		}
		static type::any _0xb5d3453c98456528() {
			return invoker::invoke<type::any>(0xb5d3453c98456528);
		}
		static bool _0xef0912ddf7c4cb4b() {
			return invoker::invoke<bool>(0xef0912ddf7c4cb4b);
		}
		static bool network_send_text_message(const char* message, int* networkhandle) {
			return invoker::invoke<bool>(0x3a214f2ec889b100, message, networkhandle);
		}
		static void network_set_activity_spectator(bool toggle) {
			invoker::invoke<void>(0x75138790b4359a74, toggle);
		}
		static bool network_is_activity_spectator() {
			return invoker::invoke<bool>(0x12103b9e0c9f92fb);
		}
		static void network_set_activity_spectator_max(int maxspectators) {
			invoker::invoke<void>(0x9d277b76d1d12222, maxspectators);
		}
		static int network_get_activity_player_num(bool p0) {
			return invoker::invoke<int>(0x73e2b500410da5a2, p0);
		}
		static bool network_is_activity_spectator_from_handle(int* networkhandle) {
			return invoker::invoke<bool>(0x2763bbaa72a7bcb9, networkhandle);
		}
		static type::any network_host_transition(type::any p0, int playercount, type::any p2, type::hash jobhash, type::any p4, type::any p5) {
			return invoker::invoke<type::any>(0xa60bb5ce242bb254, p0, playercount, p2, jobhash, p4, p5);
		}
		static bool network_do_transition_quickmatch(type::any p0, type::any p1, type::any p2, type::any p3) {
			return invoker::invoke<bool>(0x71fb0ebcd4915d56, p0, p1, p2, p3);
		}
		static bool network_do_transition_quickmatch_async(type::any p0, type::any p1, type::any p2, type::any p3) {
			return invoker::invoke<bool>(0xa091a5e44f0072e5, p0, p1, p2, p3);
		}
		static bool network_do_transition_quickmatch_with_group(type::any p0, type::any p1, type::any p2, type::any p3, type::any* p4, type::any p5) {
			return invoker::invoke<bool>(0x9c4ab58491fdc98a, p0, p1, p2, p3, p4, p5);
		}
		static type::any network_join_group_activity() {
			return invoker::invoke<type::any>(0xa06509a691d12be4);
		}
		static void _0xb13e88e655e5a3bc() {
			invoker::invoke<void>(0xb13e88e655e5a3bc);
		}
		static type::any _0x6512765e3be78c50() {
			return invoker::invoke<type::any>(0x6512765e3be78c50);
		}
		static type::any _0x0dbd5d7e3c5bec3b() {
			return invoker::invoke<type::any>(0x0dbd5d7e3c5bec3b);
		}
		static bool _0x5dc577201723960a() {
			return invoker::invoke<bool>(0x5dc577201723960a);
		}
		static bool _0x5a6aa44ff8e931e6() {
			return invoker::invoke<bool>(0x5a6aa44ff8e931e6);
		}
		static void _0x261e97ad7bcf3d40(bool p0) {
			invoker::invoke<void>(0x261e97ad7bcf3d40, p0);
		}
		static void _0x39917e1b4cb0f911(bool p0) {
			invoker::invoke<void>(0x39917e1b4cb0f911, p0);
		}
		static void network_set_transition_creator_handle(type::any* p0) {
			invoker::invoke<void>(0xef26739bcd9907d5, p0);
		}
		static void network_clear_transition_creator_handle() {
			invoker::invoke<void>(0xfb3272229a82c759);
		}
		static bool network_invite_gamers_to_transition(type::any* p0, type::any p1) {
			return invoker::invoke<bool>(0x4a595c32f77dff76, p0, p1);
		}
		static void network_set_gamer_invited_to_transition(int* networkhandle) {
			invoker::invoke<void>(0xca2c8073411ecdb6, networkhandle);
		}
		static type::any network_leave_transition() {
			return invoker::invoke<type::any>(0xd23a1a815d21db19);
		}
		static type::any network_launch_transition() {
			return invoker::invoke<type::any>(0x2dcf46cb1a4f0884);
		}
		static void _0xa2e9c1ab8a92e8cd(bool p0) {
			invoker::invoke<void>(0xa2e9c1ab8a92e8cd, p0);
		}
		static void network_bail_transition() {
			invoker::invoke<void>(0xeaa572036990cd1b);
		}
		static bool network_do_transition_to_game(bool p0, int maxplayers) {
			return invoker::invoke<bool>(0x3e9bb38102a589b0, p0, maxplayers);
		}
		static bool network_do_transition_to_new_game(bool p0, int maxplayers, bool p2) {
			return invoker::invoke<bool>(0x4665f51efed00034, p0, maxplayers, p2);
		}
		static bool network_do_transition_to_freemode(type::any* p0, type::any p1, bool p2, int players, bool p4) {
			return invoker::invoke<bool>(0x3aad8b2fca1e289f, p0, p1, p2, players, p4);
		}
		static bool network_do_transition_to_new_freemode(int* networkhandle, type::any* p1, int players, bool p3, bool p4, bool p5) {
			return invoker::invoke<bool>(0x9e80a5ba8109f974, networkhandle, p1, players, p3, p4, p5);
		}
		static type::any network_is_transition_to_game() {
			return invoker::invoke<type::any>(0x9d7696d8f4fa6cb7);
		}
		static type::any network_get_transition_members(type::any* p0, type::any p1) {
			return invoker::invoke<type::any>(0x73b000f7fbc55829, p0, p1);
		}
		static void network_apply_transition_parameter(type::any p0, type::any p1) {
			invoker::invoke<void>(0x521638ada1ba0d18, p0, p1);
		}
		static void _0xebefc2e77084f599(type::any p0, const char* p1, bool p2) {
			invoker::invoke<void>(0xebefc2e77084f599, p0, p1, p2);
		}
		static bool network_send_transition_gamer_instruction(int* networkhandle, const char* p1, int p2, int p3, bool p4) {
			return invoker::invoke<bool>(0x31d1d2b858d25e6b, networkhandle, p1, p2, p3, p4);
		}
		static bool network_mark_transition_gamer_as_fully_joined(type::any* p0) {
			return invoker::invoke<bool>(0x5728bb6d63e3ff1d, p0);
		}
		static type::any network_is_transition_host() {
			return invoker::invoke<type::any>(0x0b824797c9bf2159);
		}
		static bool network_is_transition_host_from_handle(int* networkhandle) {
			return invoker::invoke<bool>(0x6b5c83ba3efe6a10, networkhandle);
		}
		static bool network_get_transition_host(int* networkhandle) {
			return invoker::invoke<bool>(0x65042b9774c4435e, networkhandle);
		}
		static bool network_is_in_transition() {
			return invoker::invoke<bool>(0x68049aeff83d8f0a);
		}
		static bool network_is_transition_started() {
			return invoker::invoke<bool>(0x53fa83401d9c07fe);
		}
		static type::any network_is_transition_busy() {
			return invoker::invoke<type::any>(0x520f3282a53d26b7);
		}
		static type::any network_is_transition_matchmaking() {
			return invoker::invoke<type::any>(0x292564c735375edf);
		}
		static type::any _0xc571d0e77d8bbc29() {
			return invoker::invoke<type::any>(0xc571d0e77d8bbc29);
		}
		static void network_open_transition_matchmaking() {
			invoker::invoke<void>(0x2b3a8f7ca3a38fde);
		}
		static void network_close_transition_matchmaking() {
			invoker::invoke<void>(0x43f4dba69710e01e);
		}
		static type::any _0x37a4494483b9f5c9() {
			return invoker::invoke<type::any>(0x37a4494483b9f5c9);
		}
		static void _0x0c978fda19692c2c(bool p0, bool p1) {
			invoker::invoke<void>(0x0c978fda19692c2c, p0, p1);
		}
		static type::any _0xd0a484cb2f829fbe() {
			return invoker::invoke<type::any>(0xd0a484cb2f829fbe);
		}
		static void network_set_transition_activity_id(type::any p0) {
			invoker::invoke<void>(0x30de938b516f0ad2, p0);
		}
		static void network_change_transition_slots(type::any p0, type::any p1) {
			invoker::invoke<void>(0xeeeda5e6d7080987, p0, p1);
		}
		static void _0x973d76aa760a6cb6(bool p0) {
			invoker::invoke<void>(0x973d76aa760a6cb6, p0);
		}
		static bool network_has_player_started_transition(type::player player) {
			return invoker::invoke<bool>(0x9ac9ccbfa8c29795, player);
		}
		static bool network_are_transition_details_valid(type::any p0) {
			return invoker::invoke<bool>(0x2615aa2a695930c1, p0);
		}
		static bool network_join_transition(type::player player) {
			return invoker::invoke<bool>(0x9d060b08cd63321a, player);
		}
		static bool network_has_invited_gamer_to_transition(type::any* p0) {
			return invoker::invoke<bool>(0x7284a47b3540e6cf, p0);
		}
		static bool _0x3f9990bf5f22759c(type::any* p0) {
			return invoker::invoke<bool>(0x3f9990bf5f22759c, p0);
		}
		static bool network_is_activity_session() {
			return invoker::invoke<bool>(0x05095437424397fa);
		}
		static void _network_block_invites_2(bool p0) {
			invoker::invoke<void>(0x4a9fde3a5a6d0437, p0);
		}
		static bool _network_send_presence_invite(int* networkhandle, type::any* p1, type::any p2, type::any p3) {
			return invoker::invoke<bool>(0xc3c7a6afdb244624, networkhandle, p1, p2, p3);
		}
		static bool _network_send_presence_transition_invite(type::any* p0, type::any* p1, type::any p2, type::any p3) {
			return invoker::invoke<bool>(0xc116ff9b4d488291, p0, p1, p2, p3);
		}
		static bool _0x1171a97a3d3981b6(type::any* p0, type::any* p1, type::any p2, type::any p3) {
			return invoker::invoke<bool>(0x1171a97a3d3981b6, p0, p1, p2, p3);
		}
		static type::any _0x742b58f723233ed9(type::any p0) {
			return invoker::invoke<type::any>(0x742b58f723233ed9, p0);
		}
		static int network_get_num_presence_invites() {
			return invoker::invoke<int>(0xcefa968912d0f78d);
		}
		static bool network_accept_presence_invite(type::any p0) {
			return invoker::invoke<bool>(0xfa91550df9318b22, p0);
		}
		static bool network_remove_presence_invite(type::any p0) {
			return invoker::invoke<bool>(0xf0210268db0974b1, p0);
		}
		static type::any network_get_presence_invite_id(type::any p0) {
			return invoker::invoke<type::any>(0xdff09646e12ec386, p0);
		}
		static type::any network_get_presence_invite_inviter(type::any p0) {
			return invoker::invoke<type::any>(0x4962cc4aa2f345b7, p0);
		}
		static bool network_get_presence_invite_handle(type::any p0, type::any* p1) {
			return invoker::invoke<bool>(0x38d5b0febb086f75, p0, p1);
		}
		static type::any network_get_presence_invite_session_id(type::any p0) {
			return invoker::invoke<type::any>(0x26e1cd96b0903d60, p0);
		}
		static type::any _0x24409fc4c55cb22d(type::any p0) {
			return invoker::invoke<type::any>(0x24409fc4c55cb22d, p0);
		}
		static type::any _0xd39b3fff8ffdd5bf(type::any p0) {
			return invoker::invoke<type::any>(0xd39b3fff8ffdd5bf, p0);
		}
		static type::any _0x728c4cc7920cd102(type::any p0) {
			return invoker::invoke<type::any>(0x728c4cc7920cd102, p0);
		}
		static bool _0x3dbf2df0aeb7d289(type::any p0) {
			return invoker::invoke<bool>(0x3dbf2df0aeb7d289, p0);
		}
		static bool _0x8806cebfabd3ce05(type::any p0) {
			return invoker::invoke<bool>(0x8806cebfabd3ce05, p0);
		}
		static bool network_has_follow_invite() {
			return invoker::invoke<bool>(0x76d9b976c4c09fde);
		}
		static type::any network_action_follow_invite() {
			return invoker::invoke<type::any>(0xc88156ebb786f8d5);
		}
		static type::any network_clear_follow_invite() {
			return invoker::invoke<type::any>(0x439bfde3cd0610f6);
		}
		static void _0xebf8284d8cadeb53() {
			invoker::invoke<void>(0xebf8284d8cadeb53);
		}
		static void network_remove_transition_invite(type::any* p0) {
			invoker::invoke<void>(0x7524b431b2e6f7ee, p0);
		}
		static void network_remove_all_transition_invite() {
			invoker::invoke<void>(0x726e0375c7a26368);
		}
		static void _0xf083835b70ba9bfe() {
			invoker::invoke<void>(0xf083835b70ba9bfe);
		}
		static bool network_invite_gamers(type::any* p0, type::any p1, type::any* p2, type::any* p3) {
			return invoker::invoke<bool>(0x9d80cd1d0e6327de, p0, p1, p2, p3);
		}
		static bool network_has_invited_gamer(type::any* p0) {
			return invoker::invoke<bool>(0x4d86cd31e8976ece, p0);
		}
		static bool network_get_currently_selected_gamer_handle_from_invite_menu(type::any* p0) {
			return invoker::invoke<bool>(0x74881e6bcae2327c, p0);
		}
		static bool network_set_currently_selected_gamer_handle_from_invite_menu(type::any* p0) {
			return invoker::invoke<bool>(0x7206f674f2a3b1bb, p0);
		}
		static void _0x66f010a4b031a331(type::any* p0) {
			invoker::invoke<void>(0x66f010a4b031a331, p0);
		}
		static type::any _0x44b37cdcae765aae(type::player p0, bool* p1) {
			return invoker::invoke<type::any>(0x44b37cdcae765aae, p0, p1);
		}
		static void _0x0d77a82dc2d0da59(type::any* p0, type::any* p1) {
			invoker::invoke<void>(0x0d77a82dc2d0da59, p0, p1);
		}
		static bool fillout_pm_player_list(int* networkhandle, type::any p1, type::any p2) {
			return invoker::invoke<bool>(0xcbbd7c4991b64809, networkhandle, p1, p2);
		}
		static bool fillout_pm_player_list_with_names(type::any* p0, type::any* p1, type::any p2, type::any p3) {
			return invoker::invoke<bool>(0x716b6db9d1886106, p0, p1, p2, p3);
		}
		static bool using_network_weapontype(type::any p0) {
			return invoker::invoke<bool>(0xe26ccff8094d8c74, p0);
		}
		static bool _network_check_data_manager_for_handle(type::any* p0) {
			return invoker::invoke<bool>(0x796a87b3b68d1f3d, p0);
		}
		static type::any _0x2fc5650b0271cb57() {
			return invoker::invoke<type::any>(0x2fc5650b0271cb57);
		}
		static type::any _0x01abce5e7cbda196() {
			return invoker::invoke<type::any>(0x01abce5e7cbda196);
		}
		static type::any _0x120364de2845daf8(type::any* p0, type::any p1) {
			return invoker::invoke<type::any>(0x120364de2845daf8, p0, p1);
		}
		static type::any _0xfd8b834a8ba05048() {
			return invoker::invoke<type::any>(0xfd8b834a8ba05048);
		}
		static bool network_is_chatting_in_platform_party(int* networkhandle) {
			return invoker::invoke<bool>(0x8de9945bcc9aec52, networkhandle);
		}
		static bool network_is_in_party() {
			return invoker::invoke<bool>(0x966c2bc2a7fe3f30);
		}
		static bool network_is_party_member(int* networkhandle) {
			return invoker::invoke<bool>(0x676ed266aadd31e0, networkhandle);
		}
		static type::any _0x2bf66d2e7414f686() {
			return invoker::invoke<type::any>(0x2bf66d2e7414f686);
		}
		static type::any _0x14922ed3e38761f0() {
			return invoker::invoke<type::any>(0x14922ed3e38761f0);
		}
		static void _0xfa2888e3833c8e96() {
			invoker::invoke<void>(0xfa2888e3833c8e96);
		}
		static void _0x25d990f8e0e3f13c() {
			invoker::invoke<void>(0x25d990f8e0e3f13c);
		}
		static void _0x77faddcbe3499df7(type::any p0) {
			invoker::invoke<void>(0x77faddcbe3499df7, p0);
		}
		static void _network_set_random_int_seed(int seed) {
			invoker::invoke<void>(0xf1b84178f8674195, seed);
		}
		static int network_get_random_int() {
			return invoker::invoke<int>(0x599e4fa1f87eb5ff);
		}
		static int _network_get_random_int_in_range(int rangestart, int rangeend) {
			return invoker::invoke<int>(0xe30cf56f1efa5f43, rangestart, rangeend);
		}
		static bool network_player_is_cheater() {
			return invoker::invoke<bool>(0x655b91f1495a9090);
		}
		static bool _network_player_is_unk() {
			return invoker::invoke<bool>(0x172f75b6ee2233ba);
		}
		static bool network_player_is_badsport() {
			return invoker::invoke<bool>(0x19d8da0e5a68045a);
		}
		static bool get_player_stat_stars_visibility(type::player player, int p1, type::hash scripthash) {
			return invoker::invoke<bool>(0x46fb3ed415c7641c, player, p1, scripthash);
		}
		static bool bad_sport_player_left_detected(int* networkhandle, int p1, int p2) {
			return invoker::invoke<bool>(0xec5e3af5289dca81, networkhandle, p1, p2);
		}
		static void _0xe66c690248f11150(type::entity p0, type::any p1) {
			invoker::invoke<void>(0xe66c690248f11150, p0, p1);
		}
		static void network_set_this_script_is_network_script(int lobbysize, bool p1, int instanceid) {
			invoker::invoke<void>(0x1ca59e306ecb80a5, lobbysize, p1, instanceid);
		}
		static bool _network_set_this_script_is_network_script_2(int numplayers, bool p1, int instanceid) {
			return invoker::invoke<bool>(0xd1110739eeadb592, numplayers, p1, instanceid);
		}
		static bool network_get_this_script_is_network_script() {
			return invoker::invoke<bool>(0x2910669969e9535e);
		}
		static int _network_get_num_participants_host() {
			return invoker::invoke<int>(0xa6c90fbc38e395ee);
		}
		static int network_get_num_participants() {
			return invoker::invoke<int>(0x18d0456e86604654);
		}
		static int network_get_script_status() {
			return invoker::invoke<int>(0x57d158647a6bfabf);
		}
		static void network_register_host_broadcast_variables(int* vars, int sizeofvars) {
			invoker::invoke<void>(0x3e9b2f01c50df595, vars, sizeofvars);
		}
		static void network_register_player_broadcast_variables(const char* vars, type::scrhandle numvars) {
			invoker::invoke<void>(0x3364aa97340ca215, vars, numvars);
		}
		static void _0x64f62afb081e260d() {
			invoker::invoke<void>(0x64f62afb081e260d);
		}
		static bool _0x5d10b3795f3fc886() {
			return invoker::invoke<bool>(0x5d10b3795f3fc886);
		}
		static int network_get_player_index(type::player player) {
			return invoker::invoke<int>(0x24fb80d107371267, player);
		}
		static int network_get_participant_index(int index) {
			return invoker::invoke<int>(0x1b84df6af2a46938, index);
		}
		static type::player network_get_player_index_from_ped(type::ped ped) {
			return invoker::invoke<type::player>(0x6c0e2e0125610278, ped);
		}
		static int network_get_num_connected_players() {
			return invoker::invoke<int>(0xa4a79dd2d9600654);
		}
		static bool network_is_player_connected(type::player player) {
			return invoker::invoke<bool>(0x93dc1be4e1abe9d1, player);
		}
		static int _0xcf61d4b4702ee9eb() {
			return invoker::invoke<int>(0xcf61d4b4702ee9eb);
		}
		static bool network_is_participant_active(int p0) {
			return invoker::invoke<bool>(0x6ff8ff40b6357d45, p0);
		}
		static bool network_is_player_active(type::player player) {
			return invoker::invoke<bool>(0xb8dfd30d6973e135, player);
		}
		static bool network_is_player_a_participant(type::player playerid) {
			return invoker::invoke<bool>(0x3ca58f6cb7cbd784, playerid);
		}
		static bool network_is_host_of_this_script() {
			return invoker::invoke<bool>(0x83cd99a1e6061ab5);
		}
		static type::any network_get_host_of_this_script() {
			return invoker::invoke<type::any>(0xc7b4d79b01fa7a5c);
		}
		static int network_get_host_of_script(const char* scriptname, int instanceid, int positionhash) {
			return invoker::invoke<int>(0x1d6a14f1f9a736fc, scriptname, instanceid, positionhash);
		}
		static void network_set_mission_finished() {
			invoker::invoke<void>(0x3b3d11cd9ffcdfc9);
		}
		static bool network_is_script_active(const char* scriptname, int instanceid, bool unk, int positionhash) {
			return invoker::invoke<bool>(0x9d40df90fad26098, scriptname, instanceid, unk, positionhash);
		}
		static int network_get_num_script_participants(type::any* p0, type::any p1, type::any p2) {
			return invoker::invoke<int>(0x3658e8cd94fc121a, p0, p1, p2);
		}
		static int _network_get_player_ped_from_index() {
			return invoker::invoke<int>(0x638a3a81733086db);
		}
		static bool _0x1ad5b71586b94820(type::player p0, type::any* p1, type::any p2) {
			return invoker::invoke<bool>(0x1ad5b71586b94820, p0, p1, p2);
		}
		static void _0x2302c0264ea58d31() {
			invoker::invoke<void>(0x2302c0264ea58d31);
		}
		static void _0x741a3d8380319a81() {
			invoker::invoke<void>(0x741a3d8380319a81);
		}
		static type::player participant_id() {
			return invoker::invoke<type::player>(0x90986e8876ce0a83);
		}
		static int participant_id_to_int() {
			return invoker::invoke<int>(0x57a3bddad8e5aa0a);
		}
		static int network_get_destroyer_of_network_id(int netid, type::hash* weaponhash) {
			return invoker::invoke<int>(0x7a1adeef01740a24, netid, weaponhash);
		}
		static bool _network_get_destroyer_of_entity(type::any p0, type::any p1, type::hash* weaponhash) {
			return invoker::invoke<bool>(0x4caca84440fa26f6, p0, p1, weaponhash);
		}
		static type::entity network_get_entity_killer_of_player(type::player player, type::hash* weaponhash) {
			return invoker::invoke<type::entity>(0x42b2daa6b596f5f8, player, weaponhash);
		}
		static void network_resurrect_local_player(float x, float y, float z, float heading, bool unk, bool changetime) {
			invoker::invoke<void>(0xea23c49eaa83acfb, x, y, z, heading, unk, changetime);
		}
		static void network_set_local_player_invincible_time(int time) {
			invoker::invoke<void>(0x2d95c7e2d7e07307, time);
		}
		static bool network_is_local_player_invincible() {
			return invoker::invoke<bool>(0x8a8694b48715b000);
		}
		static void network_disable_invincible_flashing(int player, bool p1) {
			invoker::invoke<void>(0x9dd368bf06983221, player, p1);
		}
		static void _0x524ff0aeff9c3973(type::any p0) {
			invoker::invoke<void>(0x524ff0aeff9c3973, p0);
		}
		static bool _0xb07d3185e11657a5(type::entity p0) {
			return invoker::invoke<bool>(0xb07d3185e11657a5, p0);
		}
		static int network_get_network_id_from_entity(type::entity entity) {
			return invoker::invoke<int>(0xa11700682f3ad45c, entity);
		}
		static type::entity network_get_entity_from_network_id(int netid) {
			return invoker::invoke<type::entity>(0xce4e5d9b0a4ff560, netid);
		}
		static bool network_get_entity_is_networked(type::entity entity) {
			return invoker::invoke<bool>(0xc7827959479dcc78, entity);
		}
		static bool network_get_entity_is_local(type::entity entity) {
			return invoker::invoke<bool>(0x0991549de4d64762, entity);
		}
		static void network_register_entity_as_networked(type::entity entity) {
			invoker::invoke<void>(0x06faacd625d80caa, entity);
		}
		static void network_unregister_networked_entity(type::entity entity) {
			invoker::invoke<void>(0x7368e683bb9038d6, entity);
		}
		static bool network_does_network_id_exist(int netid) {
			return invoker::invoke<bool>(0x38ce16c96bd11344, netid);
		}
		static bool network_does_entity_exist_with_network_id(int netid) {
			return invoker::invoke<bool>(0x18a47d074708fd68, netid);
		}
		static bool network_request_control_of_network_id(int netid) {
			return invoker::invoke<bool>(0xa670b3662faffbd0, netid);
		}
		static bool network_has_control_of_network_id(int netid) {
			return invoker::invoke<bool>(0x4d36070fe0215186, netid);
		}
		static bool network_request_control_of_entity(type::entity entity) {
			return invoker::invoke<bool>(0xb69317bf5e782347, entity);
		}
		static bool network_request_control_of_door(int doorid) {
			return invoker::invoke<bool>(0x870ddfd5a4a796e4, doorid);
		}
		static bool network_has_control_of_entity(type::entity entity) {
			return invoker::invoke<bool>(0x01bf60a500e28887, entity);
		}
		static bool network_has_control_of_pickup(type::pickup pickup) {
			return invoker::invoke<bool>(0x5bc9495f0b3b6fa6, pickup);
		}
		static bool network_has_control_of_door(type::hash doorhash) {
			return invoker::invoke<bool>(0xcb3c68adb06195df, doorhash);
		}
		static bool _network_has_control_of_pavement_stats(type::hash doorhash) {
			return invoker::invoke<bool>(0xc01e93fac20c3346, doorhash);
		}
		static int veh_to_net(type::vehicle vehicle) {
			return invoker::invoke<int>(0xb4c94523f023419c, vehicle);
		}
		static int ped_to_net(type::ped ped) {
			return invoker::invoke<int>(0x0edec3c276198689, ped);
		}
		static int obj_to_net(type::object object) {
			return invoker::invoke<int>(0x99bfdc94a603e541, object);
		}
		static type::vehicle net_to_veh(int nethandle) {
			return invoker::invoke<type::vehicle>(0x367b936610ba360c, nethandle);
		}
		static type::ped net_to_ped(int nethandle) {
			return invoker::invoke<type::ped>(0xbdcd95fc216a8b3e, nethandle);
		}
		static type::object net_to_obj(int nethandle) {
			return invoker::invoke<type::object>(0xd8515f5fea14cb3f, nethandle);
		}
		static type::entity net_to_ent(int nethandle) {
			return invoker::invoke<type::entity>(0xbffeab45a9a9094a, nethandle);
		}
		static void network_get_local_handle(int* networkhandle, int buffersize) {
			invoker::invoke<void>(0xe86051786b66cd8e, networkhandle, buffersize);
		}
		static void network_handle_from_user_id(const char* userid, int* networkhandle, int buffersize) {
			invoker::invoke<void>(0xdcd51dd8f87aec5c, userid, networkhandle, buffersize);
		}
		static void network_handle_from_member_id(const char* memberid, int* networkhandle, int buffersize) {
			invoker::invoke<void>(0xa0fd21bed61e5c4c, memberid, networkhandle, buffersize);
		}
		static void network_handle_from_player(type::player player, int* networkhandle, int buffersize) {
			invoker::invoke<void>(0x388eb2b86c73b6b3, player, networkhandle, buffersize);
		}
		static type::hash _network_hash_from_player_handle(type::player player) {
			return invoker::invoke<type::hash>(0xbc1d768f2f5d6c05, player);
		}
		static type::hash _network_hash_from_gamer_handle(int* networkhandle) {
			return invoker::invoke<type::hash>(0x58575ac3cf2ca8ec, networkhandle);
		}
		static void network_handle_from_friend(type::player friendindex, int* networkhandle, int buffersize) {
			invoker::invoke<void>(0xd45cb817d7e177d2, friendindex, networkhandle, buffersize);
		}
		static bool network_gamertag_from_handle_start(int* networkhandle) {
			return invoker::invoke<bool>(0x9f0c0a981d73fa56, networkhandle);
		}
		static bool network_gamertag_from_handle_pending() {
			return invoker::invoke<bool>(0xb071e27958ef4cf0);
		}
		static bool network_gamertag_from_handle_succeeded() {
			return invoker::invoke<bool>(0xfd00798dba7523dd);
		}
		static const char* network_get_gamertag_from_handle(int* networkhandle) {
			return invoker::invoke<const char*>(0x426141162ebe5cdb, networkhandle);
		}
		static int _0xd66c9e72b3cc4982(type::any* p0, type::any p1) {
			return invoker::invoke<int>(0xd66c9e72b3cc4982, p0, p1);
		}
		static type::any _0x58cc181719256197(type::any p0, type::any p1, type::any p2) {
			return invoker::invoke<type::any>(0x58cc181719256197, p0, p1, p2);
		}
		static bool network_are_handles_the_same(int* nethandle1, int* nethandle2) {
			return invoker::invoke<bool>(0x57dba049e110f217, nethandle1, nethandle2);
		}
		static bool network_is_handle_valid(int* networkhandle, int buffersize) {
			return invoker::invoke<bool>(0x6f79b93b0a8e4133, networkhandle, buffersize);
		}
		static type::player network_get_player_from_gamer_handle(int* networkhandle) {
			return invoker::invoke<type::player>(0xce5f689cf5a0a49d, networkhandle);
		}
		static const char* network_member_id_from_gamer_handle(int* networkhandle) {
			return invoker::invoke<const char*>(0xc82630132081bb6f, networkhandle);
		}
		static bool network_is_gamer_in_my_session(int* networkhandle) {
			return invoker::invoke<bool>(0x0f10b05ddf8d16e9, networkhandle);
		}
		static void network_show_profile_ui(int* networkhandle) {
			invoker::invoke<void>(0x859ed1cea343fca8, networkhandle);
		}
		static const char* network_player_get_name(type::player player) {
			return invoker::invoke<const char*>(0x7718d2e2060837d2, player);
		}
		static const char* network_player_get_userid(type::player player, const char* userid) {
			return invoker::invoke<const char*>(0x4927fc39cd0869a0, player, userid);
		}
		static bool network_player_is_rockstar_dev(type::player player) {
			return invoker::invoke<bool>(0x544abdda3b409b6d, player);
		}
		static bool _network_player_is_cheater(type::player player) {
			return invoker::invoke<bool>(0x565e430db3b05bec, player);
		}
		static bool network_is_inactive_profile(type::any* p0) {
			return invoker::invoke<bool>(0x7e58745504313a2e, p0);
		}
		static int network_get_max_friends() {
			return invoker::invoke<int>(0xafebb0d5d8f687d2);
		}
		static int network_get_friend_count() {
			return invoker::invoke<int>(0x203f1cfd823b27a4);
		}
		static const char* network_get_friend_name(int friendindex) {
			return invoker::invoke<const char*>(0xe11ebbb2a783fe8b, friendindex);
		}
		static const char* _network_get_friend_name_from_index(int friendindex) {
			return invoker::invoke<const char*>(0x4164f227d052e293, friendindex);
		}
		static bool network_is_friend_online(const char* name) {
			return invoker::invoke<bool>(0x425a44533437b64d, name);
		}
		static bool _network_is_friend_handle_online(int* networkhandle) {
			return invoker::invoke<bool>(0x87eb7a3ffcb314db, networkhandle);
		}
		static bool network_is_friend_in_same_title(const char* friendname) {
			return invoker::invoke<bool>(0x2ea9a3bedf3f17b8, friendname);
		}
		static bool network_is_friend_in_multiplayer(const char* friendname) {
			return invoker::invoke<bool>(0x57005c18827f3a28, friendname);
		}
		static bool network_is_friend(int* networkhandle) {
			return invoker::invoke<bool>(0x1a24a179f9b31654, networkhandle);
		}
		static bool network_is_pending_friend(type::any p0) {
			return invoker::invoke<bool>(0x0be73da6984a6e33, p0);
		}
		static bool network_is_adding_friend() {
			return invoker::invoke<bool>(0x6ea101606f6e4d81);
		}
		static bool network_add_friend(int* networkhandle, const char* message) {
			return invoker::invoke<bool>(0x8e02d73914064223, networkhandle, message);
		}
		static bool network_is_friend_index_online(int friendindex) {
			return invoker::invoke<bool>(0xbad8f2a42b844821, friendindex);
		}
		static void _0x1b857666604b1a74(bool p0) {
			invoker::invoke<void>(0x1b857666604b1a74, p0);
		}
		static bool _0x82377b65e943f72d(type::any p0) {
			return invoker::invoke<bool>(0x82377b65e943f72d, p0);
		}
		static bool network_can_set_waypoint() {
			return invoker::invoke<bool>(0xc927ec229934af60);
		}
		static type::any _0xb309ebea797e001f(type::any p0) {
			return invoker::invoke<type::any>(0xb309ebea797e001f, p0);
		}
		static type::any _0x26f07dd83a5f7f98() {
			return invoker::invoke<type::any>(0x26f07dd83a5f7f98);
		}
		static bool network_has_headset() {
			return invoker::invoke<bool>(0xe870f9f1f7b4f1fa);
		}
		static void _0x7d395ea61622e116(bool p0) {
			invoker::invoke<void>(0x7d395ea61622e116, p0);
		}
		static type::any _0xc0d2af00bcc234ca() {
			return invoker::invoke<type::any>(0xc0d2af00bcc234ca);
		}
		static bool network_gamer_has_headset(type::any* p0) {
			return invoker::invoke<bool>(0xf2fd55cb574bcc55, p0);
		}
		static bool network_is_gamer_talking(int* p0) {
			return invoker::invoke<bool>(0x71c33b22606cd88a, p0);
		}
		static bool network_can_communicate_with_gamer(int* player) {
			return invoker::invoke<bool>(0xa150a4f065806b1f, player);
		}
		static bool network_is_gamer_muted_by_me(int* p0) {
			return invoker::invoke<bool>(0xce60de011b6c7978, p0);
		}
		static bool network_am_i_muted_by_gamer(type::any* p0) {
			return invoker::invoke<bool>(0xdf02a2c93f1f26da, p0);
		}
		static bool network_is_gamer_blocked_by_me(type::any* p0) {
			return invoker::invoke<bool>(0xe944c4f5af1b5883, p0);
		}
		static bool network_am_i_blocked_by_gamer(type::any* p0) {
			return invoker::invoke<bool>(0x15337c7c268a27b2, p0);
		}
		static bool _0xb57a49545ba53ce7(type::any* p0) {
			return invoker::invoke<bool>(0xb57a49545ba53ce7, p0);
		}
		static bool _0xcca4318e1ab03f1f(type::any* p0) {
			return invoker::invoke<bool>(0xcca4318e1ab03f1f, p0);
		}
		static bool _0x07dd29d5e22763f1(type::any* p0) {
			return invoker::invoke<bool>(0x07dd29d5e22763f1, p0);
		}
		static bool _0x135f9b7b7add2185(type::any* p0) {
			return invoker::invoke<bool>(0x135f9b7b7add2185, p0);
		}
		static bool network_is_player_talking(type::player player) {
			return invoker::invoke<bool>(0x031e11f3d447647e, player);
		}
		static bool network_player_has_headset(type::player player) {
			return invoker::invoke<bool>(0x3fb99a8b08d18fd6, player);
		}
		static bool network_is_player_muted_by_me(type::player player) {
			return invoker::invoke<bool>(0x8c71288ae68ede39, player);
		}
		static bool network_am_i_muted_by_player(type::player player) {
			return invoker::invoke<bool>(0x9d6981dfc91a8604, player);
		}
		static bool network_is_player_blocked_by_me(type::player player) {
			return invoker::invoke<bool>(0x57af1f8e27483721, player);
		}
		static bool network_am_i_blocked_by_player(type::player player) {
			return invoker::invoke<bool>(0x87f395d957d4353d, player);
		}
		static float network_get_player_loudness(type::any p0) {
			return invoker::invoke<float>(0x21a1684a25c2867f, p0);
		}
		static void network_set_talker_proximity(float p0) {
			invoker::invoke<void>(0xcbf12d65f95ad686, p0);
		}
		static type::any network_get_talker_proximity() {
			return invoker::invoke<type::any>(0x84f0f13120b4e098);
		}
		static void network_set_voice_active(bool toggle) {
			invoker::invoke<void>(0xbabec9e69a91c57b, toggle);
		}
		static void _0xcfeb46dcd7d8d5eb(bool p0) {
			invoker::invoke<void>(0xcfeb46dcd7d8d5eb, p0);
		}
		static void network_override_transition_chat(bool p0) {
			invoker::invoke<void>(0xaf66059a131aa269, p0);
		}
		static void network_set_team_only_chat(bool toggle) {
			invoker::invoke<void>(0xd5b4883ac32f24c3, toggle);
		}
		static void _0x6f697a66ce78674e(int team, bool toggle) {
			invoker::invoke<void>(0x6f697a66ce78674e, team, toggle);
		}
		static void network_set_override_spectator_mode(bool toggle) {
			invoker::invoke<void>(0x70da3bf8dacd3210, toggle);
		}
		static void _0x3c5c1e2c2ff814b1(bool p0) {
			invoker::invoke<void>(0x3c5c1e2c2ff814b1, p0);
		}
		static void _0x9d7afcbf21c51712(bool p0) {
			invoker::invoke<void>(0x9d7afcbf21c51712, p0);
		}
		static void _0xf46a1e03e8755980(bool p0) {
			invoker::invoke<void>(0xf46a1e03e8755980, p0);
		}
		static void _0x6a5d89d7769a40d8(bool p0) {
			invoker::invoke<void>(0x6a5d89d7769a40d8, p0);
		}
		static void network_override_chat_restrictions(type::player player, bool toggle) {
			invoker::invoke<void>(0x3039ae5ad2c9c0c4, player, toggle);
		}
		static void _network_override_send_restrictions(type::player player, bool toggle) {
			invoker::invoke<void>(0x97dd4c5944cc2e6a, player, toggle);
		}
		static void _network_chat_mute(bool p0) {
			invoker::invoke<void>(0x57b192b4d4ad23d5, p0);
		}
		static void network_override_receive_restrictions(type::player player, bool toggle) {
			invoker::invoke<void>(0xddf73e2b1fec5ab4, player, toggle);
		}
		static void _0x0ff2862b61a58af9(bool p0) {
			invoker::invoke<void>(0x0ff2862b61a58af9, p0);
		}
		static void network_set_voice_channel(type::any p0) {
			invoker::invoke<void>(0xef6212c2efef1a23, p0);
		}
		static void network_clear_voice_channel() {
			invoker::invoke<void>(0xe036a705f989e049);
		}
		static void is_network_vehicle_been_damaged_by_any_object(float x, float y, float z) {
			invoker::invoke<void>(0xdbd2056652689917, x, y, z);
		}
		static void _0xf03755696450470c() {
			invoker::invoke<void>(0xf03755696450470c);
		}
		static void _0x5e3aa4ca2b6fb0ee(type::any p0) {
			invoker::invoke<void>(0x5e3aa4ca2b6fb0ee, p0);
		}
		static void _0xca575c391fea25cc(type::any p0) {
			invoker::invoke<void>(0xca575c391fea25cc, p0);
		}
		static void _0xadb57e5b663cca8b(type::player p0, float* p1, float* p2) {
			invoker::invoke<void>(0xadb57e5b663cca8b, p0, p1, p2);
		}
		static bool _network_is_text_chat_active() {
			return invoker::invoke<bool>(0x5fcf4d7069b09026);
		}
		static void shutdown_and_launch_single_player_game() {
			invoker::invoke<void>(0x593850c16a36b692);
		}
		static void network_set_friendly_fire_option(bool toggle) {
			invoker::invoke<void>(0xf808475fa571d823, toggle);
		}
		static void network_set_rich_presence(int p0, type::any p1, type::any p2, type::any p3) {
			invoker::invoke<void>(0x1dccacdcfc569362, p0, p1, p2, p3);
		}
		static void _network_set_rich_presence_2(int p0, const char* gxtlabel) {
			invoker::invoke<void>(0x3e200c2bcf4164eb, p0, gxtlabel);
		}
		static int network_get_timeout_time() {
			return invoker::invoke<int>(0x5ed0356a0ce3a34f);
		}
		static void _network_respawn_coords(type::player player, float x, float y, float z, bool p4, bool p5) {
			invoker::invoke<void>(0x9769f811d1785b03, player, x, y, z, p4, p5);
		}
		static void _network_respawn_player(type::player player, bool p1) {
			invoker::invoke<void>(0xbf22e0f32968e967, player, p1);
		}
		static void _network_remove_stickybomb(type::entity entity) {
			invoker::invoke<void>(0x715135f4b82ac90d, entity);
		}
		static bool _network_player_is_in_clan() {
			return invoker::invoke<bool>(0x579cced0265d4896);
		}
		static bool network_clan_player_is_active(int* networkhandle) {
			return invoker::invoke<bool>(0xb124b57f571d8f18, networkhandle);
		}
		static bool network_clan_player_get_desc(int* clandesc, int buffersize, int* networkhandle) {
			return invoker::invoke<bool>(0xeee6eacbe8874fba, clandesc, buffersize, networkhandle);
		}
		static bool _0x7543bb439f63792b(int* clandesc, int buffersize) {
			return invoker::invoke<bool>(0x7543bb439f63792b, clandesc, buffersize);
		}
		static void _0xf45352426ff3a4f0(int* clandesc, int buffersize, int* networkhandle) {
			invoker::invoke<void>(0xf45352426ff3a4f0, clandesc, buffersize, networkhandle);
		}
		static int _get_num_membership_desc() {
			return invoker::invoke<int>(0x1f471b79acc90bef);
		}
		static bool network_clan_get_membership_desc(int* memberdesc, int p1) {
			return invoker::invoke<bool>(0x48de78af2c8885b8, memberdesc, p1);
		}
		static bool network_clan_download_membership(int* networkhandle) {
			return invoker::invoke<bool>(0xa989044e70010abe, networkhandle);
		}
		static bool network_clan_download_membership_pending(type::any* p0) {
			return invoker::invoke<bool>(0x5b9e023dc6ebedc0, p0);
		}
		static bool _network_is_clan_membership_finished_downloading() {
			return invoker::invoke<bool>(0xb3f64a6a91432477);
		}
		static bool network_clan_remote_memberships_are_in_cache(int* p0) {
			return invoker::invoke<bool>(0xbb6e6fee99d866b2, p0);
		}
		static int network_clan_get_membership_count(int* p0) {
			return invoker::invoke<int>(0xaab11f6c4adbc2c1, p0);
		}
		static bool network_clan_get_membership_valid(int* p0, type::any p1) {
			return invoker::invoke<bool>(0x48a59cf88d43df0e, p0, p1);
		}
		static bool network_clan_get_membership(int* p0, int* clanmembership, int p2) {
			return invoker::invoke<bool>(0xc8bc2011f67b3411, p0, clanmembership, p2);
		}
		static bool network_clan_join(int clandesc) {
			return invoker::invoke<bool>(0x9faaa4f4fc71f87f, clandesc);
		}
		static bool _network_clan_animation(const char* animdict, const char* animname) {
			return invoker::invoke<bool>(0x729e3401f0430686, animdict, animname);
		}
		static bool _0x2b51edbefc301339(int p0, const char* p1) {
			return invoker::invoke<bool>(0x2b51edbefc301339, p0, p1);
		}
		static type::any _0xc32ea7a2f6ca7557() {
			return invoker::invoke<type::any>(0xc32ea7a2f6ca7557);
		}
		static bool _network_get_player_crew_emblem_txd_name(type::player* player, type::any* p1) {
			return invoker::invoke<bool>(0x5835d9cd92e83184, player, p1);
		}
		static bool _0x13518ff1c6b28938(type::any p0) {
			return invoker::invoke<bool>(0x13518ff1c6b28938, p0);
		}
		static bool _0xa134777ff7f33331(type::any p0, type::any* p1) {
			return invoker::invoke<bool>(0xa134777ff7f33331, p0, p1);
		}
		static void _0x113e6e3e50e286b0(type::any p0) {
			invoker::invoke<void>(0x113e6e3e50e286b0, p0);
		}
		static type::any network_get_primary_clan_data_clear() {
			return invoker::invoke<type::any>(0x9aa46badad0e27ed);
		}
		static void network_get_primary_clan_data_cancel() {
			invoker::invoke<void>(0x042e4b70b93e6054);
		}
		static bool network_get_primary_clan_data_start(type::any* p0, type::any p1) {
			return invoker::invoke<bool>(0xce86d8191b762107, p0, p1);
		}
		static type::any network_get_primary_clan_data_pending() {
			return invoker::invoke<type::any>(0xb5074db804e28ce7);
		}
		static type::any network_get_primary_clan_data_success() {
			return invoker::invoke<type::any>(0x5b4f04f19376a0ba);
		}
		static bool network_get_primary_clan_data_new(type::any* p0, type::any* p1) {
			return invoker::invoke<bool>(0xc080ff658b2e41da, p0, p1);
		}
		static void set_network_id_can_migrate(int netid, bool toggle) {
			invoker::invoke<void>(0x299eeb23175895fc, netid, toggle);
		}
		static void set_network_id_exists_on_all_machines(int netid, bool toggle) {
			invoker::invoke<void>(0xe05e81a888fa63c8, netid, toggle);
		}
		static void _set_network_id_sync_to_player(int netid, type::player player, bool toggle) {
			invoker::invoke<void>(0xa8a024587329f36a, netid, player, toggle);
		}
		static void network_set_entity_can_blend(type::entity entity, bool toggle) {
			invoker::invoke<void>(0xd830567d88a1e873, entity, toggle);
		}
		static void _network_set_entity_invisible_to_network(type::entity entity, bool toggle) {
			invoker::invoke<void>(0xf1ca12b18aef5298, entity, toggle);
		}
		static void set_network_id_visible_in_cutscene(int netid, bool p1, bool p2) {
			invoker::invoke<void>(0xa6928482543022b4, netid, p1, p2);
		}
		static void _0xaaa553e7dd28a457(bool p0) {
			invoker::invoke<void>(0xaaa553e7dd28a457, p0);
		}
		static void _0x3fa36981311fa4ff(int netid, bool state) {
			invoker::invoke<void>(0x3fa36981311fa4ff, netid, state);
		}
		static bool _network_can_network_id_be_seen(int netid) {
			return invoker::invoke<bool>(0xa1607996431332df, netid);
		}
		static void set_local_player_visible_in_cutscene(bool p0, bool p1) {
			invoker::invoke<void>(0xd1065d68947e7b6e, p0, p1);
		}
		static void set_local_player_invisible_locally(bool p0) {
			invoker::invoke<void>(0xe5f773c1a1d9d168, p0);
		}
		static void set_local_player_visible_locally(bool p0) {
			invoker::invoke<void>(0x7619364c82d3bf14, p0);
		}
		static void set_player_invisible_locally(type::player player, bool toggle) {
			invoker::invoke<void>(0x12b37d54667db0b8, player, toggle);
		}
		static void set_player_visible_locally(type::player player, bool toggle) {
			invoker::invoke<void>(0xfaa10f1fafb11af2, player, toggle);
		}
		static void fade_out_local_player(bool p0) {
			invoker::invoke<void>(0x416dbd4cd6ed8dd2, p0);
		}
		static void network_fade_out_entity(type::entity entity, bool normal, bool slow) {
			invoker::invoke<void>(0xde564951f95e09ed, entity, normal, slow);
		}
		static void network_fade_in_entity(type::entity entity, bool state) {
			invoker::invoke<void>(0x1f4ed342acefe62d, entity, state);
		}
		static bool _0x631dc5dff4b110e3(type::player index) {
			return invoker::invoke<bool>(0x631dc5dff4b110e3, index);
		}
		static bool _0x422f32cc7e56abad(type::vehicle vehicle) {
			return invoker::invoke<bool>(0x422f32cc7e56abad, vehicle);
		}
		static bool is_player_in_cutscene(type::player player) {
			return invoker::invoke<bool>(0xe73092f4157cd126, player);
		}
		static void set_entity_visible_in_cutscene(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0xe0031d3c8f36ab82, p0, p1, p2);
		}
		static void set_entity_locally_invisible(type::entity entity) {
			invoker::invoke<void>(0xe135a9ff3f5d05d8, entity);
		}
		static void set_entity_locally_visible(type::entity entity) {
			invoker::invoke<void>(0x241e289b5c059edc, entity);
		}
		static bool is_damage_tracker_active_on_network_id(int netid) {
			return invoker::invoke<bool>(0x6e192e33ad436366, netid);
		}
		static void activate_damage_tracker_on_network_id(int netid, bool p1) {
			invoker::invoke<void>(0xd45b1ffccd52ff19, netid, p1);
		}
		static bool is_sphere_visible_to_another_machine(float p0, float p1, float p2, float p3) {
			return invoker::invoke<bool>(0xd82cf8e64c8729d8, p0, p1, p2, p3);
		}
		static bool is_sphere_visible_to_player(type::player player, float x, float y, float z, float scale) {
			return invoker::invoke<bool>(0xdc3a310219e5da62, player, x, y, z, scale);
		}
		static void reserve_network_mission_objects(int amount) {
			invoker::invoke<void>(0x4e5c93bd0c32fbf8, amount);
		}
		static void reserve_network_mission_peds(int amount) {
			invoker::invoke<void>(0xb60feba45333d36f, amount);
		}
		static void reserve_network_mission_vehicles(int amount) {
			invoker::invoke<void>(0x76b02e21ed27a469, amount);
		}
		static bool can_register_mission_objects(int amount) {
			return invoker::invoke<bool>(0x800dd4721a8b008b, amount);
		}
		static bool can_register_mission_peds(int amount) {
			return invoker::invoke<bool>(0xbcbf4fef9fa5d781, amount);
		}
		static bool can_register_mission_vehicles(int amount) {
			return invoker::invoke<bool>(0x7277f1f2e085ee74, amount);
		}
		static bool can_register_mission_entities(int ped_amt, int vehicle_amt, int object_amt, int pickup_amt) {
			return invoker::invoke<bool>(0x69778e7564bade6d, ped_amt, vehicle_amt, object_amt, pickup_amt);
		}
		static int get_num_reserved_mission_objects(bool p0) {
			return invoker::invoke<int>(0xaa81b5f10bc43ac2, p0);
		}
		static int get_num_reserved_mission_peds(bool p0) {
			return invoker::invoke<int>(0x1f13d5ae5cb17e17, p0);
		}
		static int get_num_reserved_mission_vehicles(bool p0) {
			return invoker::invoke<int>(0xcf3a965906452031, p0);
		}
		static int _0x12b6281b6c6706c0(bool p0) {
			return invoker::invoke<int>(0x12b6281b6c6706c0, p0);
		}
		static int _0xcb215c4b56a7fae7(bool p0) {
			return invoker::invoke<int>(0xcb215c4b56a7fae7, p0);
		}
		static int _0x0cd9ab83489430ea(bool p0) {
			return invoker::invoke<int>(0x0cd9ab83489430ea, p0);
		}
		static type::any _0xc7be335216b5ec7c() {
			return invoker::invoke<type::any>(0xc7be335216b5ec7c);
		}
		static type::any _0x0c1f7d49c39d2289() {
			return invoker::invoke<type::any>(0x0c1f7d49c39d2289);
		}
		static type::any _0x0afce529f69b21ff() {
			return invoker::invoke<type::any>(0x0afce529f69b21ff);
		}
		static type::any _0xa72835064dd63e4c() {
			return invoker::invoke<type::any>(0xa72835064dd63e4c);
		}
		static int get_network_time() {
			return invoker::invoke<int>(0x7a5487fe9faa6b48);
		}
		static int _0x89023fbbf9200e9f() {
			return invoker::invoke<int>(0x89023fbbf9200e9f);
		}
		static bool has_network_time_started() {
			return invoker::invoke<bool>(0x46718aceedeafc84);
		}
		static int get_time_offset(int timea, int timeb) {
			return invoker::invoke<int>(0x017008ccdad48503, timea, timeb);
		}
		static bool is_time_less_than(int timea, int timeb) {
			return invoker::invoke<bool>(0xcb2cf5148012c8d0, timea, timeb);
		}
		static bool is_time_more_than(int timea, int timeb) {
			return invoker::invoke<bool>(0xde350f8651e4346c, timea, timeb);
		}
		static bool is_time_equal_to(int timea, int timeb) {
			return invoker::invoke<bool>(0xf5bc95857bd6d512, timea, timeb);
		}
		static int get_time_difference(int timea, int timeb) {
			return invoker::invoke<int>(0xa2c6fc031d46fff0, timea, timeb);
		}
		static const char* get_time_as_string(int time) {
			return invoker::invoke<const char*>(0x9e23b1777a927dad, time);
		}
		static int _get_posix_time() {
			return invoker::invoke<int>(0x9a73240b49945c76);
		}
		static void _get_date_and_time_from_unix_epoch(int unixepoch, type::any* timestructure) {
			invoker::invoke<void>(0xac97af97fa68e5d5, unixepoch, timestructure);
		}
		static void network_set_in_spectator_mode(bool toggle, type::ped playerped) {
			invoker::invoke<void>(0x423de3854bb50894, toggle, playerped);
		}
		static void network_set_in_spectator_mode_extended(bool toggle, type::ped playerped, bool p2) {
			invoker::invoke<void>(0x419594e137637120, toggle, playerped, p2);
		}
		static void _0xfc18db55ae19e046(bool p0) {
			invoker::invoke<void>(0xfc18db55ae19e046, p0);
		}
		static void _0x5c707a667df8b9fa(bool p0, type::any p1) {
			invoker::invoke<void>(0x5c707a667df8b9fa, p0, p1);
		}
		static bool network_is_in_spectator_mode() {
			return invoker::invoke<bool>(0x048746e388762e11);
		}
		static void network_set_in_mp_cutscene(bool p0, bool p1) {
			invoker::invoke<void>(0x9ca5de655269fec4, p0, p1);
		}
		static bool network_is_in_mp_cutscene() {
			return invoker::invoke<bool>(0x6cc27c9fa2040220);
		}
		static bool network_is_player_in_mp_cutscene(type::player player) {
			return invoker::invoke<bool>(0x63f9ee203c3619f2, player);
		}
		static void set_network_vehicle_respot_timer(int netid, int time) {
			invoker::invoke<void>(0xec51713ab6ec36e8, netid, time);
		}
		static void _set_network_object_non_contact(type::object object, bool toggle) {
			invoker::invoke<void>(0x6274c4712850841e, object, toggle);
		}
		static void use_player_colour_instead_of_team_colour(bool toggle) {
			invoker::invoke<void>(0x5ffe9b4144f9712f, toggle);
		}
		static bool _0x21d04d7bc538c146(type::any p0) {
			return invoker::invoke<bool>(0x21d04d7bc538c146, p0);
		}
		static void _0x77758139ec9b66c7(bool p0) {
			invoker::invoke<void>(0x77758139ec9b66c7, p0);
		}
		static int network_create_synchronised_scene(float x, float y, float z, float xrot, float yrot, float zrot, int p6, int p7, int p8, float p9) {
			return invoker::invoke<int>(0x7cd6bc4c2bbdd526, x, y, z, xrot, yrot, zrot, p6, p7, p8, p9);
		}
		static void network_add_ped_to_synchronised_scene(type::ped ped, int netscene, const char* animdict, const char* animname, float speed, float speedmultiplier, int duration, int flag, float playbackrate, type::any p9) {
			invoker::invoke<void>(0x742a637471bcecd9, ped, netscene, animdict, animname, speed, speedmultiplier, duration, flag, playbackrate, p9);
		}
		static void network_add_entity_to_synchronised_scene(type::entity entity, int netscene, const char* animdict, const char* animname, float speed, float speedmulitiplier, int flag) {
			invoker::invoke<void>(0xf2404d68cbc855fa, entity, netscene, animdict, animname, speed, speedmulitiplier, flag);
		}
		static void _network_force_local_use_of_synced_scene_camera(int netscene, const char* animdict, const char* animname) {
			invoker::invoke<void>(0xcf8bd3b0bd6d42d7, netscene, animdict, animname);
		}
		static void network_attach_synchronised_scene_to_entity(int netscene, type::entity entity, int bone) {
			invoker::invoke<void>(0x478dcbd2a98b705a, netscene, entity, bone);
		}
		static void network_start_synchronised_scene(int netscene) {
			invoker::invoke<void>(0x9a1b3fcdb36c8697, netscene);
		}
		static void network_stop_synchronised_scene(int netscene) {
			invoker::invoke<void>(0xc254481a4574cb2f, netscene);
		}
		static int _network_convert_synchronised_scene_to_synchronized_scene(int netscene) {
			return invoker::invoke<int>(0x02c40bf885c567b6, netscene);
		}
		static void _0xc9b43a33d09cada7(type::any p0) {
			invoker::invoke<void>(0xc9b43a33d09cada7, p0);
		}
		static type::any _0xfb1f9381e80fa13f(int p0, type::any* p1) {
			return invoker::invoke<type::any>(0xfb1f9381e80fa13f, p0, p1);
		}
		static bool _0x5a6ffa2433e2f14c(type::player player, float p1, float p2, float p3, float p4, float p5, float p6, float p7, int flags) {
			return invoker::invoke<bool>(0x5a6ffa2433e2f14c, player, p1, p2, p3, p4, p5, p6, p7, flags);
		}
		static bool _0x4ba92a18502bca61(type::player player, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, int flags) {
			return invoker::invoke<bool>(0x4ba92a18502bca61, player, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, flags);
		}
		static type::any _0x3c891a251567dfce(type::any* p0) {
			return invoker::invoke<type::any>(0x3c891a251567dfce, p0);
		}
		static void _0xfb8f2a6f3df08cbe() {
			invoker::invoke<void>(0xfb8f2a6f3df08cbe);
		}
		static void network_get_respawn_result(int randomint, PVector3* coordinates, float* heading) {
			invoker::invoke<void>(0x371ea43692861cf1, randomint, coordinates, heading);
		}
		static type::any _0x6c34f1208b8923fd(type::any p0) {
			return invoker::invoke<type::any>(0x6c34f1208b8923fd, p0);
		}
		static void _0x17e0198b3882c2cb() {
			invoker::invoke<void>(0x17e0198b3882c2cb);
		}
		static void _0xfb680d403909dc70(type::any p0, type::any p1) {
			invoker::invoke<void>(0xfb680d403909dc70, p0, p1);
		}
		static void network_end_tutorial_session() {
			invoker::invoke<void>(0xd0afaff5a51d72f7);
		}
		static bool network_is_in_tutorial_session() {
			return invoker::invoke<bool>(0xada24309fe08dacf);
		}
		static type::any _0xb37e4e6a2388ca7b() {
			return invoker::invoke<type::any>(0xb37e4e6a2388ca7b);
		}
		static type::any _0x35f0b98a8387274d() {
			return invoker::invoke<type::any>(0x35f0b98a8387274d);
		}
		static type::any _0x3b39236746714134(type::any p0) {
			return invoker::invoke<type::any>(0x3b39236746714134, p0);
		}
		static bool _network_is_player_equal_to_index(type::player player, int index) {
			return invoker::invoke<bool>(0x9de986fc9a87c474, player, index);
		}
		static void _0xbbdf066252829606(type::any p0, bool p1) {
			invoker::invoke<void>(0xbbdf066252829606, p0, p1);
		}
		static bool _0x919b3c98ed8292f9(type::any p0) {
			return invoker::invoke<bool>(0x919b3c98ed8292f9, p0);
		}
		static void network_override_clock_time(int hours, int minutes, int seconds) {
			invoker::invoke<void>(0xe679e3e06e363892, hours, minutes, seconds);
		}
		static void network_clear_clock_time_override() {
			invoker::invoke<void>(0xd972df67326f966e);
		}
		static bool network_is_clock_time_overridden() {
			return invoker::invoke<bool>(0xd7c95d322ff57522);
		}
		static type::any network_add_entity_area(float p0, float p1, float p2, float p3, float p4, float p5) {
			return invoker::invoke<type::any>(0x494c8fb299290269, p0, p1, p2, p3, p4, p5);
		}
		static type::any _network_add_entity_angled_area(float p0, float p1, float p2, float p3, float p4, float p5, float p6) {
			return invoker::invoke<type::any>(0x376c6375ba60293a, p0, p1, p2, p3, p4, p5, p6);
		}
		static type::any _0x25b99872d588a101(float p0, float p1, float p2, float p3, float p4, float p5) {
			return invoker::invoke<type::any>(0x25b99872d588a101, p0, p1, p2, p3, p4, p5);
		}
		static bool network_remove_entity_area(type::any p0) {
			return invoker::invoke<bool>(0x93cf869baa0c4874, p0);
		}
		static bool _0xe64a3ca08dfa37a9(type::any p0) {
			return invoker::invoke<bool>(0xe64a3ca08dfa37a9, p0);
		}
		static bool _0x4df7cfff471a7fb1(type::any p0) {
			return invoker::invoke<bool>(0x4df7cfff471a7fb1, p0);
		}
		static bool _0x4a2d4e8bf4265b0f(type::any p0) {
			return invoker::invoke<bool>(0x4a2d4e8bf4265b0f, p0);
		}
		static void _network_set_network_id_dynamic(int netid, bool toggle) {
			invoker::invoke<void>(0x2b1813aba29016c5, netid, toggle);
		}
		static bool _network_request_cloud_background_scripts() {
			return invoker::invoke<bool>(0x924426bffd82e915);
		}
		static bool _has_bg_script_been_downloaded() {
			return invoker::invoke<bool>(0x8132c0eb8b2b3293);
		}
		static void network_request_cloud_tunables() {
			invoker::invoke<void>(0x42fb3b532d526e6c);
		}
		static bool _network_have_tunables_been_downloaded() {
			return invoker::invoke<bool>(0x0467c11ed88b7d28);
		}
		static type::any _0x10bd227a753b0d84() {
			return invoker::invoke<type::any>(0x10bd227a753b0d84);
		}
		static bool network_does_tunable_exist(const char* tunablecontext, const char* tunablename) {
			return invoker::invoke<bool>(0x85e5f8b9b898b20a, tunablecontext, tunablename);
		}
		static bool network_access_tunable_int(const char* tunablecontext, const char* tunablename, int* value) {
			return invoker::invoke<bool>(0x8be1146dfd5d4468, tunablecontext, tunablename, value);
		}
		static bool network_access_tunable_float(const char* tunablecontext, const char* tunablename, float* value) {
			return invoker::invoke<bool>(0xe5608ca7bc163a5f, tunablecontext, tunablename, value);
		}
		static bool network_access_tunable_bool(const char* tunablecontext, const char* tunablename) {
			return invoker::invoke<bool>(0xaa6a47a573abb75a, tunablecontext, tunablename);
		}
		static bool _network_does_tunable_exist_hash(type::hash tunablecontext, type::hash tunablename) {
			return invoker::invoke<bool>(0xe4e53e1419d81127, tunablecontext, tunablename);
		}
		static bool _network_access_tunable_int_hash(type::hash tunablecontext, type::hash tunablename, int* value) {
			return invoker::invoke<bool>(0x40fce03e50e8dbe8, tunablecontext, tunablename, value);
		}
		static bool _network_access_tunable_float_hash(type::hash tunablecontext, type::hash tunablename, float* value) {
			return invoker::invoke<bool>(0x972bc203bbc4c4d5, tunablecontext, tunablename, value);
		}
		static bool _network_access_tunable_bool_hash(type::hash tunablecontext, type::hash tunablename) {
			return invoker::invoke<bool>(0xea16b69d93d71a45, tunablecontext, tunablename);
		}
		static bool _network_access_tunable_bool_hash_fail_val(type::hash tunablecontext, type::hash tunablename, bool defaultvalue) {
			return invoker::invoke<bool>(0xc7420099936ce286, tunablecontext, tunablename, defaultvalue);
		}
		static int _get_tunables_content_modifier_id(type::hash contenthash) {
			return invoker::invoke<int>(0x187382f8a3e0a6c3, contenthash);
		}
		static type::any _0x7db53b37a2f211a0() {
			return invoker::invoke<type::any>(0x7db53b37a2f211a0);
		}
		static void network_reset_body_tracker() {
			invoker::invoke<void>(0x72433699b4e6dd64);
		}
		static type::any _0xd38c4a6d047c019d() {
			return invoker::invoke<type::any>(0xd38c4a6d047c019d);
		}
		static bool _0x2e0bf682cc778d49(type::any p0) {
			return invoker::invoke<bool>(0x2e0bf682cc778d49, p0);
		}
		static bool _0x0ede326d47cd0f3e(type::ped ped, type::player player) {
			return invoker::invoke<bool>(0x0ede326d47cd0f3e, ped, player);
		}
		static type::any network_explode_vehicle(type::vehicle vehicle, bool isaudible, bool isinvisible, type::player player) {
			return invoker::invoke<type::any>(0x301a42153c9ad707, vehicle, isaudible, isinvisible, player);
		}
		static void _0xcd71a4ecab22709e(type::entity entity) {
			invoker::invoke<void>(0xcd71a4ecab22709e, entity);
		}
		static void _0xa7e30de9272b6d49(type::ped ped, float x, float y, float z, float p4) {
			invoker::invoke<void>(0xa7e30de9272b6d49, ped, x, y, z, p4);
		}
		static void _0x407091cf6037118e(int netid) {
			invoker::invoke<void>(0x407091cf6037118e, netid);
		}
		static void network_set_property_id(type::any p0) {
			invoker::invoke<void>(0x1775961c2fbbcb5c, p0);
		}
		static void network_clear_property_id() {
			invoker::invoke<void>(0xc2b82527ca77053e);
		}
		static void _0x367ef5e2f439b4c6(int p0) {
			invoker::invoke<void>(0x367ef5e2f439b4c6, p0);
		}
		static void _0x94538037ee44f5cf(bool p0) {
			invoker::invoke<void>(0x94538037ee44f5cf, p0);
		}
		static void _network_cache_local_player_head_blend_data() {
			invoker::invoke<void>(0xbd0be0bfc927eac1);
		}
		static bool _0x237d5336a9a54108(type::any p0) {
			return invoker::invoke<bool>(0x237d5336a9a54108, p0);
		}
		static bool _network_copy_ped_blend_data(type::ped ped, type::player player) {
			return invoker::invoke<bool>(0x99b72c7abde5c910, ped, player);
		}
		static type::any _0xf2eac213d5ea0623() {
			return invoker::invoke<type::any>(0xf2eac213d5ea0623);
		}
		static type::any _0xea14eef5b7cd2c30() {
			return invoker::invoke<type::any>(0xea14eef5b7cd2c30);
		}
		static void _0xb606e6cc59664972(type::any p0) {
			invoker::invoke<void>(0xb606e6cc59664972, p0);
		}
		static type::any _0x1d4dc17c38feaff0() {
			return invoker::invoke<type::any>(0x1d4dc17c38feaff0);
		}
		static type::any _0x662635855957c411(type::any p0) {
			return invoker::invoke<type::any>(0x662635855957c411, p0);
		}
		static type::any _0xb4271092ca7edf48(type::any p0) {
			return invoker::invoke<type::any>(0xb4271092ca7edf48, p0);
		}
		static type::any _0xca94551b50b4932c(type::any p0) {
			return invoker::invoke<type::any>(0xca94551b50b4932c, p0);
		}
		static type::any _0x2a7776c709904ab0(type::any p0) {
			return invoker::invoke<type::any>(0x2a7776c709904ab0, p0);
		}
		static type::any _0x6f44cbf56d79fac0(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x6f44cbf56d79fac0, p0, p1);
		}
		static void _0x58c21165f6545892(const char* p0, const char* p1) {
			invoker::invoke<void>(0x58c21165f6545892, p0, p1);
		}
		static type::any _0x2eac52b4019e2782() {
			return invoker::invoke<type::any>(0x2eac52b4019e2782);
		}
		static void set_store_enabled(bool toggle) {
			invoker::invoke<void>(0x9641a9ff718e9c5e, toggle);
		}
		static bool _0xa2f952104fc6dd4b(type::any p0) {
			return invoker::invoke<bool>(0xa2f952104fc6dd4b, p0);
		}
		static void _0x72d0706cd6ccdb58() {
			invoker::invoke<void>(0x72d0706cd6ccdb58);
		}
		static type::any _0x722f5d28b61c5ea8(type::any p0) {
			return invoker::invoke<type::any>(0x722f5d28b61c5ea8, p0);
		}
		static type::any _0x883d79c4071e18b3() {
			return invoker::invoke<type::any>(0x883d79c4071e18b3);
		}
		static void _0x265635150fb0d82e() {
			invoker::invoke<void>(0x265635150fb0d82e);
		}
		static void _0x444c4525ece0a4b9() {
			invoker::invoke<void>(0x444c4525ece0a4b9);
		}
		static type::any _0x59328eb08c5ceb2b() {
			return invoker::invoke<type::any>(0x59328eb08c5ceb2b);
		}
		static void _0xfae628f1e9adb239(type::any p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0xfae628f1e9adb239, p0, p1, p2);
		}
		static type::any _0xc64ded7ef0d2fe37(type::any* p0) {
			return invoker::invoke<type::any>(0xc64ded7ef0d2fe37, p0);
		}
		static bool _0x4c61b39930d045da(type::any p0) {
			return invoker::invoke<bool>(0x4c61b39930d045da, p0);
		}
		static type::hash _0x3a3d5568af297cd5(type::hash p1) {
			return invoker::invoke<type::hash>(0x3a3d5568af297cd5, p1);
		}
		static void cloud_check_availability() {
			invoker::invoke<void>(0x4f18196c8d38768d);
		}
		static type::any _0xc7abac5de675ee3b() {
			return invoker::invoke<type::any>(0xc7abac5de675ee3b);
		}
		static type::any network_enable_motion_drugged() {
			return invoker::invoke<type::any>(0x0b0cc10720653f3b);
		}
		static type::any _0x8b0c2964ba471961() {
			return invoker::invoke<type::any>(0x8b0c2964ba471961);
		}
		static type::any _0x88b588b41ff7868e() {
			return invoker::invoke<type::any>(0x88b588b41ff7868e);
		}
		static type::any _0x67fc09bc554a75e5() {
			return invoker::invoke<type::any>(0x67fc09bc554a75e5);
		}
		static void _0x966dd84fb6a46017() {
			invoker::invoke<void>(0x966dd84fb6a46017);
		}
		static bool _0x152d90e4c1b4738a(type::any* p0, type::any* p1) {
			return invoker::invoke<bool>(0x152d90e4c1b4738a, p0, p1);
		}
		static type::any _0x9fedf86898f100e9() {
			return invoker::invoke<type::any>(0x9fedf86898f100e9);
		}
		static type::any _0x5e24341a7f92a74b() {
			return invoker::invoke<type::any>(0x5e24341a7f92a74b);
		}
		static type::any _0x24e4e51fc16305f9() {
			return invoker::invoke<type::any>(0x24e4e51fc16305f9);
		}
		static type::any _0xfbc5e768c7a77a6a() {
			return invoker::invoke<type::any>(0xfbc5e768c7a77a6a);
		}
		static type::any _0xc55a0b40ffb1ed23() {
			return invoker::invoke<type::any>(0xc55a0b40ffb1ed23);
		}
		static void _0x17440aa15d1d3739() {
			invoker::invoke<void>(0x17440aa15d1d3739);
		}
		static bool _0x9bf438815f5d96ea(type::any p0, type::any p1, type::any* p2, type::any p3, type::any p4, type::any p5) {
			return invoker::invoke<bool>(0x9bf438815f5d96ea, p0, p1, p2, p3, p4, p5);
		}
		static bool _0x692d58df40657e8c(type::any p0, type::any p1, type::any p2, type::any* p3, type::any p4, bool p5) {
			return invoker::invoke<bool>(0x692d58df40657e8c, p0, p1, p2, p3, p4, p5);
		}
		static bool _0x158ec424f35ec469(const char* p0, bool p1, const char* contenttype) {
			return invoker::invoke<bool>(0x158ec424f35ec469, p0, p1, contenttype);
		}
		static bool _0xc7397a83f7a2a462(type::any* p0, type::any p1, bool p2, type::any* p3) {
			return invoker::invoke<bool>(0xc7397a83f7a2a462, p0, p1, p2, p3);
		}
		static bool _0x6d4cb481fac835e8(type::any p0, type::any p1, const char* p2, type::any p3) {
			return invoker::invoke<bool>(0x6d4cb481fac835e8, p0, p1, p2, p3);
		}
		static bool _0xd5a4b59980401588(type::any p0, type::any p1, type::any* p2, type::any* p3) {
			return invoker::invoke<bool>(0xd5a4b59980401588, p0, p1, p2, p3);
		}
		static bool _0x3195f8dd0d531052(type::any p0, type::any p1, type::any* p2, type::any* p3) {
			return invoker::invoke<bool>(0x3195f8dd0d531052, p0, p1, p2, p3);
		}
		static bool _0xf9e1ccae8ba4c281(type::any p0, type::any p1, type::any* p2, type::any* p3) {
			return invoker::invoke<bool>(0xf9e1ccae8ba4c281, p0, p1, p2, p3);
		}
		static bool _0x9f6e2821885caee2(type::any p0, type::any p1, type::any p2, type::any* p3, type::any* p4) {
			return invoker::invoke<bool>(0x9f6e2821885caee2, p0, p1, p2, p3, p4);
		}
		static bool _0x678bb03c1a3bd51e(type::any p0, type::any p1, type::any p2, type::any* p3, type::any* p4) {
			return invoker::invoke<bool>(0x678bb03c1a3bd51e, p0, p1, p2, p3, p4);
		}
		static bool set_balance_add_machine(type::any* p0, type::any* p1) {
			return invoker::invoke<bool>(0x815e5e3073da1d67, p0, p1);
		}
		static bool set_balance_add_machines(type::any* p0, type::any p1, type::any* p2) {
			return invoker::invoke<bool>(0xb8322eeb38be7c26, p0, p1, p2);
		}
		static bool _0xa7862bc5ed1dfd7e(type::any p0, type::any p1, type::any* p2, type::any* p3) {
			return invoker::invoke<bool>(0xa7862bc5ed1dfd7e, p0, p1, p2, p3);
		}
		static bool _0x97a770beef227e2b(type::any p0, type::any p1, type::any* p2, type::any* p3) {
			return invoker::invoke<bool>(0x97a770beef227e2b, p0, p1, p2, p3);
		}
		static bool _0x5324a0e3e4ce3570(type::any p0, type::any p1, type::any* p2, type::any* p3) {
			return invoker::invoke<bool>(0x5324a0e3e4ce3570, p0, p1, p2, p3);
		}
		static void _0xe9b99b6853181409() {
			invoker::invoke<void>(0xe9b99b6853181409);
		}
		static type::any _0xd53acdbef24a46e8() {
			return invoker::invoke<type::any>(0xd53acdbef24a46e8);
		}
		static type::any _0x02ada21ea2f6918f() {
			return invoker::invoke<type::any>(0x02ada21ea2f6918f);
		}
		static type::any _0x941e5306bcd7c2c7() {
			return invoker::invoke<type::any>(0x941e5306bcd7c2c7);
		}
		static type::any _0xc87e740d9f3872cc() {
			return invoker::invoke<type::any>(0xc87e740d9f3872cc);
		}
		static type::any _0xedf7f927136c224b() {
			return invoker::invoke<type::any>(0xedf7f927136c224b);
		}
		static type::any _0xe0a6138401bcb837() {
			return invoker::invoke<type::any>(0xe0a6138401bcb837);
		}
		static type::any _0x769951e2455e2eb5() {
			return invoker::invoke<type::any>(0x769951e2455e2eb5);
		}
		static type::any _0x3a17a27d75c74887() {
			return invoker::invoke<type::any>(0x3a17a27d75c74887);
		}
		static void _0xba96394a0eecfa65() {
			invoker::invoke<void>(0xba96394a0eecfa65);
		}
		static const char* get_player_advanced_modifier_privileges(int p0) {
			return invoker::invoke<const char*>(0xcd67ad041a394c9c, p0);
		}
		static bool _0x584770794d758c18(type::any p0, type::any* p1) {
			return invoker::invoke<bool>(0x584770794d758c18, p0, p1);
		}
		static bool _0x8c8d2739ba44af0f(type::any p0) {
			return invoker::invoke<bool>(0x8c8d2739ba44af0f, p0);
		}
		static type::any _0x703f12425eca8bf5(type::any p0) {
			return invoker::invoke<type::any>(0x703f12425eca8bf5, p0);
		}
		static bool _0xaeab987727c5a8a4(type::any p0) {
			return invoker::invoke<bool>(0xaeab987727c5a8a4, p0);
		}
		static int _get_content_category(int p0) {
			return invoker::invoke<int>(0xa7bab11e7c9c6c5a, p0);
		}
		static const char* _get_content_id(type::any p0) {
			return invoker::invoke<const char*>(0x55aa95f481d694d2, p0);
		}
		static const char* _get_root_content_id(type::any p0) {
			return invoker::invoke<const char*>(0xc0173d6bff4e0348, p0);
		}
		static type::any _0xbf09786a7fcab582(type::any p0) {
			return invoker::invoke<type::any>(0xbf09786a7fcab582, p0);
		}
		static int _get_content_description_hash(type::any p0) {
			return invoker::invoke<int>(0x7cf0448787b23758, p0);
		}
		static type::any _0xbaf6babf9e7ccc13(int p0, type::any* p1) {
			return invoker::invoke<type::any>(0xbaf6babf9e7ccc13, p0, p1);
		}
		static void _0xcfd115b373c0df63(type::any p0, type::any* p1) {
			invoker::invoke<void>(0xcfd115b373c0df63, p0, p1);
		}
		static type::any _get_content_file_version(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x37025b27d9b658b1, p0, p1);
		}
		static bool _0x1d610eb0fea716d9(type::any p0) {
			return invoker::invoke<bool>(0x1d610eb0fea716d9, p0);
		}
		static bool _0x7fcc39c46c3c03bd(type::any p0) {
			return invoker::invoke<bool>(0x7fcc39c46c3c03bd, p0);
		}
		static type::any _0x32dd916f3f7c9672(type::any p0) {
			return invoker::invoke<type::any>(0x32dd916f3f7c9672, p0);
		}
		static bool _0x3054f114121c21ea(type::any p0) {
			return invoker::invoke<bool>(0x3054f114121c21ea, p0);
		}
		static bool _0xa9240a96c74cca13(type::any p0) {
			return invoker::invoke<bool>(0xa9240a96c74cca13, p0);
		}
		static type::any _0x1accfba3d8dab2ee(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x1accfba3d8dab2ee, p0, p1);
		}
		static type::any _0x759299c5bb31d2a9(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x759299c5bb31d2a9, p0, p1);
		}
		static type::any _0x87e5c46c187fe0ae(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x87e5c46c187fe0ae, p0, p1);
		}
		static type::any _0x4e548c0d7ae39ff9(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x4e548c0d7ae39ff9, p0, p1);
		}
		static bool _0x70ea8da57840f9be(type::any p0) {
			return invoker::invoke<bool>(0x70ea8da57840f9be, p0);
		}
		static bool _0x993cbe59d350d225(type::any p0) {
			return invoker::invoke<bool>(0x993cbe59d350d225, p0);
		}
		static type::any _0x171df6a0c07fb3dc(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x171df6a0c07fb3dc, p0, p1);
		}
		static type::any _0x7fd2990af016795e(type::any* p0, type::any* p1, type::any p2, type::any p3, type::any p4) {
			return invoker::invoke<type::any>(0x7fd2990af016795e, p0, p1, p2, p3, p4);
		}
		static type::any _0x5e0165278f6339ee(type::any p0) {
			return invoker::invoke<type::any>(0x5e0165278f6339ee, p0);
		}
		static bool _0x2d5dc831176d0114(type::any p0) {
			return invoker::invoke<bool>(0x2d5dc831176d0114, p0);
		}
		static bool _0xebfa8d50addc54c4(type::any p0) {
			return invoker::invoke<bool>(0xebfa8d50addc54c4, p0);
		}
		static bool _0x162c23ca83ed0a62(type::any p0) {
			return invoker::invoke<bool>(0x162c23ca83ed0a62, p0);
		}
		static type::any _0x40f7e66472df3e5c(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x40f7e66472df3e5c, p0, p1);
		}
		static bool _0x5a34cd9c3c5bec44(type::any p0) {
			return invoker::invoke<bool>(0x5a34cd9c3c5bec44, p0);
		}
		static void _0x68103e2247887242() {
			invoker::invoke<void>(0x68103e2247887242);
		}
		static bool _0x1de0f5f50d723caa(type::any* p0, type::any* p1, type::any* p2) {
			return invoker::invoke<bool>(0x1de0f5f50d723caa, p0, p1, p2);
		}
		static bool _0x274a1519dfc1094f(type::any* p0, bool p1, type::any* p2) {
			return invoker::invoke<bool>(0x274a1519dfc1094f, p0, p1, p2);
		}
		static bool _0xd05d1a6c74da3498(type::any* p0, bool p1, type::any* p2) {
			return invoker::invoke<bool>(0xd05d1a6c74da3498, p0, p1, p2);
		}
		static type::any _0x45e816772e93a9db() {
			return invoker::invoke<type::any>(0x45e816772e93a9db);
		}
		static type::any _0x299ef3c576773506() {
			return invoker::invoke<type::any>(0x299ef3c576773506);
		}
		static type::any _0x793ff272d5b365f4() {
			return invoker::invoke<type::any>(0x793ff272d5b365f4);
		}
		static type::any _0x5a0a3d1a186a5508() {
			return invoker::invoke<type::any>(0x5a0a3d1a186a5508);
		}
		static void _0xa1e5e0204a6fcc70() {
			invoker::invoke<void>(0xa1e5e0204a6fcc70);
		}
		static bool _0xb746d20b17f2a229(type::any* p0, type::any* p1) {
			return invoker::invoke<bool>(0xb746d20b17f2a229, p0, p1);
		}
		static type::any _0x63b406d7884bfa95() {
			return invoker::invoke<type::any>(0x63b406d7884bfa95);
		}
		static type::any _0x4d02279c83be69fe() {
			return invoker::invoke<type::any>(0x4d02279c83be69fe);
		}
		static type::any _0x597f8dba9b206fc7() {
			return invoker::invoke<type::any>(0x597f8dba9b206fc7);
		}
		static bool _0x5cae833b0ee0c500(type::any p0) {
			return invoker::invoke<bool>(0x5cae833b0ee0c500, p0);
		}
		static void _0x61a885d3f7cfee9a() {
			invoker::invoke<void>(0x61a885d3f7cfee9a);
		}
		static void _0xf98dde0a8ed09323(bool p0) {
			invoker::invoke<void>(0xf98dde0a8ed09323, p0);
		}
		static void _0xfd75dabc0957bf33(bool p0) {
			invoker::invoke<void>(0xfd75dabc0957bf33, p0);
		}
		static bool _0xf53e48461b71eecb(type::any p0) {
			return invoker::invoke<bool>(0xf53e48461b71eecb, p0);
		}
		static bool _facebook_set_heist_complete(const char* heistname, int cashearned, int xpearned) {
			return invoker::invoke<bool>(0x098ab65b9ed9a9ec, heistname, cashearned, xpearned);
		}
		static bool _facebook_set_create_character_complete() {
			return invoker::invoke<bool>(0xdc48473142545431);
		}
		static bool _facebook_set_milestone_complete(int milestoneid) {
			return invoker::invoke<bool>(0x0ae1f1653b554ab9, milestoneid);
		}
		static bool _facebook_is_sending_data() {
			return invoker::invoke<bool>(0x62b9fec9a11f10ef);
		}
		static bool _facebook_do_unk_check() {
			return invoker::invoke<bool>(0xa75e2b6733da5142);
		}
		static bool _facebook_is_available() {
			return invoker::invoke<bool>(0x43865688ae10f0d7);
		}
		static int texture_download_request(int* playerhandle, const char* filepath, const char* name, bool p3) {
			return invoker::invoke<int>(0x16160da74a8e74a2, playerhandle, filepath, name, p3);
		}
		static type::any _0x0b203b4afde53a4f(type::any* p0, type::any* p1, bool p2) {
			return invoker::invoke<type::any>(0x0b203b4afde53a4f, p0, p1, p2);
		}
		static type::any _0x308f96458b7087cc(type::any* p0, type::any p1, type::any p2, type::any p3, type::any* p4, bool p5) {
			return invoker::invoke<type::any>(0x308f96458b7087cc, p0, p1, p2, p3, p4, p5);
		}
		static void texture_download_release(int p0) {
			invoker::invoke<void>(0x487eb90b98e9fb19, p0);
		}
		static bool texture_download_has_failed(int p0) {
			return invoker::invoke<bool>(0x5776ed562c134687, p0);
		}
		static const char* texture_download_get_name(int p0) {
			return invoker::invoke<const char*>(0x3448505b6e35262d, p0);
		}
		static type::any _0x8bd6c6dea20e82c6(type::any p0) {
			return invoker::invoke<type::any>(0x8bd6c6dea20e82c6, p0);
		}
		static type::any _0x60edd13eb3ac1ff3() {
			return invoker::invoke<type::any>(0x60edd13eb3ac1ff3);
		}
		static bool network_is_cable_connected() {
			return invoker::invoke<bool>(0xeffb25453d8600f9);
		}
		static bool _0x66b59cffd78467af() {
			return invoker::invoke<bool>(0x66b59cffd78467af);
		}
		static bool _0x606e4d3e3cccf3eb() {
			return invoker::invoke<bool>(0x606e4d3e3cccf3eb);
		}
		static bool _is_rockstar_banned() {
			return invoker::invoke<bool>(0x8020a73847e0ca7d);
		}
		static bool _is_socialclub_banned() {
			return invoker::invoke<bool>(0xa0ad7e2af5349f61);
		}
		static bool _can_play_online() {
			return invoker::invoke<bool>(0x5f91d5d0b36aa310);
		}
		static bool _0x422d396f80a96547() {
			return invoker::invoke<bool>(0x422d396f80a96547);
		}
		static bool network_has_ros_privilege(int index) {
			return invoker::invoke<bool>(0xa699957e60d80214, index);
		}
		static bool _0xc22912b1d85f26b1(int p0, int* p1, type::any* p2) {
			return invoker::invoke<bool>(0xc22912b1d85f26b1, p0, p1, p2);
		}
		static bool _0x593570c289a77688() {
			return invoker::invoke<bool>(0x593570c289a77688);
		}
		static type::any _0x91b87c55093de351() {
			return invoker::invoke<type::any>(0x91b87c55093de351);
		}
		static type::any _0x36391f397731595d(type::any p0) {
			return invoker::invoke<type::any>(0x36391f397731595d, p0);
		}
		static type::any _0xdeb2b99a1af1a2a6(type::any p0) {
			return invoker::invoke<type::any>(0xdeb2b99a1af1a2a6, p0);
		}
		static void _0x9465e683b12d3f6b() {
			invoker::invoke<void>(0x9465e683b12d3f6b);
		}
		static void _network_update_player_scars() {
			invoker::invoke<void>(0xb7c7f6ad6424304b);
		}
		static void _0xc505036a35afd01b(bool p0) {
			invoker::invoke<void>(0xc505036a35afd01b, p0);
		}
		static void _0x267c78c60e806b9a(type::any p0, bool p1) {
			invoker::invoke<void>(0x267c78c60e806b9a, p0, p1);
		}
		static void _0x6bff5f84102df80a(type::any p0) {
			invoker::invoke<void>(0x6bff5f84102df80a, p0);
		}
		static void _0x5c497525f803486b() {
			invoker::invoke<void>(0x5c497525f803486b);
		}
		static type::any _0x6fb7bb3607d27fa2() {
			return invoker::invoke<type::any>(0x6fb7bb3607d27fa2);
		}
		static void _0x45a83257ed02d9bc() {
			invoker::invoke<void>(0x45a83257ed02d9bc);
		}
	}

	namespace networkcash {
		static void network_initialize_cash(int p1, int p2) {
			invoker::invoke<void>(0x3da5ecd1a56cba6d, p1, p2);
		}
		static void network_delete_character(int characterindex, bool p1, bool p2) {
			invoker::invoke<void>(0x05a50af38947eb8d, characterindex, p1, p2);
		}
		static void network_clear_character_wallet(type::any p0) {
			invoker::invoke<void>(0xa921ded15fdf28f5, p0);
		}
		static void network_give_player_jobshare_cash(int amount, int* networkhandle) {
			invoker::invoke<void>(0xfb18df9cb95e0105, amount, networkhandle);
		}
		static void network_receive_player_jobshare_cash(int value, int* networkhandle) {
			invoker::invoke<void>(0x56a3b51944c50598, value, networkhandle);
		}
		static bool network_can_share_job_cash() {
			return invoker::invoke<bool>(0x1c2473301b1c66ba);
		}
		static void network_refund_cash(int m_index, const char* sztype, const char* szreason, bool bunk) {
			invoker::invoke<void>(0xf9c812cd7c46e817, m_index, sztype, szreason, bunk);
		}
		static bool network_money_can_bet(type::any p0, bool p1, bool p2) {
			return invoker::invoke<bool>(0x81404f3dc124fe5b, p0, p1, p2);
		}
		static bool network_can_bet(type::any p0) {
			return invoker::invoke<bool>(0x3a54e33660ded67f, p0);
		}
		static void network_earn_from_pickup(int amount) {
			invoker::invoke<void>(0xed1517d3af17c698, amount);
		}
		static void _network_earn_from_gang_pickup(int amount) {
			invoker::invoke<void>(0xa03d4ace0a3284ce, amount);
		}
		static void _network_earn_from_armour_truck(int amount) {
			invoker::invoke<void>(0xf514621e8ea463d0, amount);
		}
		static void network_earn_from_crate_drop(int amount) {
			invoker::invoke<void>(0xb1cc1b9ec3007a2a, amount);
		}
		static void network_earn_from_betting(int amount, const char* p1) {
			invoker::invoke<void>(0x827a5ba1a44aca6d, amount, p1);
		}
		static void network_earn_from_job(int amount, const char* p1) {
			invoker::invoke<void>(0xb2cc4836834e8a98, amount, p1);
		}
		static void _network_earn_from_bend_job(int amount, const char* heisthash) {
			invoker::invoke<void>(0x61326ee6df15b0ca, amount, heisthash);
		}
		static void network_earn_from_challenge_win(type::any p0, type::any* p1, bool p2) {
			invoker::invoke<void>(0x2b171e6b2f64d8df, p0, p1, p2);
		}
		static void network_earn_from_bounty(int amount, int* networkhandle, type::any* p2, type::any p3) {
			invoker::invoke<void>(0x131bb5da15453acf, amount, networkhandle, p2, p3);
		}
		static void network_earn_from_import_export(type::any p0, type::any p1) {
			invoker::invoke<void>(0xf92a014a634442d6, p0, p1);
		}
		static void network_earn_from_holdups(int amount) {
			invoker::invoke<void>(0x45b8154e077d9e4d, amount);
		}
		static void network_earn_from_property(int amount, type::hash propertyname) {
			invoker::invoke<void>(0x849648349d77f5c5, amount, propertyname);
		}
		static void network_earn_from_ai_target_kill(type::any p0, type::any p1) {
			invoker::invoke<void>(0x515b4a22e4d3c6d7, p0, p1);
		}
		static void network_earn_from_not_badsport(int amount) {
			invoker::invoke<void>(0x4337511fa8221d36, amount);
		}
		static void network_earn_from_rockstar(int amount) {
			invoker::invoke<void>(0x02ce1d6ac0fc73ea, amount);
		}
		static void network_earn_from_vehicle(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4, type::any p5, type::any p6, type::any p7) {
			invoker::invoke<void>(0xb539bd8a4c1eecf8, p0, p1, p2, p3, p4, p5, p6, p7);
		}
		static void network_earn_from_personal_vehicle(type::any p0, type::any p1, type::any p2, type::any p3, type::any p4, type::any p5, type::any p6, type::any p7, type::any p8) {
			invoker::invoke<void>(0x3f4d00167e41e0ad, p0, p1, p2, p3, p4, p5, p6, p7, p8);
		}
		static void _network_earn_from_daily_objective(int p0, const char* p1, int p2) {
			invoker::invoke<void>(0x6ea318c91c1a8786, p0, p1, p2);
		}
		static void _network_earn_from_ambient_job(int p0, const char* p1, type::any* p2) {
			invoker::invoke<void>(0xfb6db092fbae29e6, p0, p1, p2);
		}
		static void _network_earn_from_job_bonus(type::any amont, type::any* p1, type::any* p2) {
			invoker::invoke<void>(0x6816fb4416760775, amont, p1, p2);
		}
		static bool network_can_spend_money(type::any p0, bool p1, bool p2, bool p3, type::any p4) {
			return invoker::invoke<bool>(0xab3caa6b422164da, p0, p1, p2, p3, p4);
		}
		static bool _0x7303e27cc6532080(type::any p0, bool p1, bool p2, bool p3, type::any* p4, type::any p5) {
			return invoker::invoke<bool>(0x7303e27cc6532080, p0, p1, p2, p3, p4, p5);
		}
		static void network_buy_item(type::ped player, type::hash item, type::any p2, type::any p3, bool p4, const char* item_name, type::any p6, type::any p7, type::any p8, bool p9) {
			invoker::invoke<void>(0xf0077c797f66a355, player, item, p2, p3, p4, item_name, p6, p7, p8, p9);
		}
		static void network_spent_taxi(int amount, bool p1, bool p2) {
			invoker::invoke<void>(0x17c3a7d31eae39f9, amount, p1, p2);
		}
		static void network_pay_employee_wage(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x5fd5ed82cbbe9989, p0, p1, p2);
		}
		static void network_pay_utility_bill(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0xafe08b35ec0c9eae, p0, p1, p2);
		}
		static void network_pay_match_entry_fee(int value, int* p1, bool p2, bool p3) {
			invoker::invoke<void>(0x9346e14f2af74d46, value, p1, p2, p3);
		}
		static void network_spent_betting(type::any p0, type::any p1, type::any* p2, bool p3, bool p4) {
			invoker::invoke<void>(0x1c436fd11ffa692f, p0, p1, p2, p3, p4);
		}
		static void network_spent_in_stripclub(type::any p0, bool p1, type::any p2, bool p3) {
			invoker::invoke<void>(0xee99784e4467689c, p0, p1, p2, p3);
		}
		static void network_buy_healthcare(int cost, bool p1, bool p2) {
			invoker::invoke<void>(0xd9b067e55253e3dd, cost, p1, p2);
		}
		static void network_buy_airstrike(int cost, bool p1, bool p2) {
			invoker::invoke<void>(0x763b4bd305338f19, cost, p1, p2);
		}
		static void network_buy_heli_strike(int cost, bool p1, bool p2) {
			invoker::invoke<void>(0x81aa4610e3fd3a69, cost, p1, p2);
		}
		static void network_spent_ammo_drop(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0xb162dc95c0a3317b, p0, p1, p2);
		}
		static void network_buy_bounty(int amount, type::player victim, bool p2, bool p3) {
			invoker::invoke<void>(0x7b718e197453f2d9, amount, victim, p2, p3);
		}
		static void network_buy_property(float propertycost, type::hash propertyname, bool p2, bool p3) {
			invoker::invoke<void>(0x650a08a280870af6, propertycost, propertyname, p2, p3);
		}
		static void network_spent_heli_pickup(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x7bf1d73db2eca492, p0, p1, p2);
		}
		static void network_spent_boat_pickup(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x524ee43a37232c00, p0, p1, p2);
		}
		static void network_spent_bull_shark(int amount, bool p1, bool p2) {
			invoker::invoke<void>(0xa6dd8458ce24012c, amount, p1, p2);
		}
		static void network_spent_cash_drop(int amount, bool p1, bool p2) {
			invoker::invoke<void>(0x289016ec778d60e0, amount, p1, p2);
		}
		static void network_spent_hire_mugger(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0xe404bfb981665bf0, p0, p1, p2);
		}
		static void network_spent_robbed_by_mugger(int amount, bool p1, bool p2) {
			invoker::invoke<void>(0x995a65f15f581359, amount, p1, p2);
		}
		static void network_spent_hire_mercenary(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0xe7b80e2bf9d80bd6, p0, p1, p2);
		}
		static void network_spent_buy_wantedlevel(type::any p0, type::any* p1, bool p2, bool p3) {
			invoker::invoke<void>(0xe1b13771a843c4f6, p0, p1, p2, p3);
		}
		static void network_spent_buy_offtheradar(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0xa628a745e2275c5d, p0, p1, p2);
		}
		static void network_spent_buy_reveal_players(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x6e176f1b18bc0637, p0, p1, p2);
		}
		static void network_spent_carwash(type::any p0, type::any p1, type::any p2, bool p3, bool p4) {
			invoker::invoke<void>(0xec03c719db2f4306, p0, p1, p2, p3, p4);
		}
		static void network_spent_cinema(type::any p0, type::any p1, bool p2, bool p3) {
			invoker::invoke<void>(0x6b38ecb05a63a685, p0, p1, p2, p3);
		}
		static void network_spent_telescope(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x7fe61782ad94cc09, p0, p1, p2);
		}
		static void network_spent_holdups(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0xd9b86b9872039763, p0, p1, p2);
		}
		static void network_spent_buy_passive_mode(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x6d3a430d1a809179, p0, p1, p2);
		}
		static void network_spent_prostitutes(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0xb21b89501cfac79e, p0, p1, p2);
		}
		static void network_spent_arrest_bail(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x812f5488b1b2a299, p0, p1, p2);
		}
		static void network_spent_pay_vehicle_insurance_premium(int amount, type::hash vehiclemodel, int* networkhandle, bool notbankrupt, bool hasthemoney) {
			invoker::invoke<void>(0x9ff28d88c766e3e8, amount, vehiclemodel, networkhandle, notbankrupt, hasthemoney);
		}
		static void network_spent_call_player(type::any p0, type::any* p1, bool p2, bool p3) {
			invoker::invoke<void>(0xacde7185b374177c, p0, p1, p2, p3);
		}
		static void network_spent_bounty(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x29b260b84947dfcc, p0, p1, p2);
		}
		static void network_spent_from_rockstar(int bank, bool p1, bool p2) {
			invoker::invoke<void>(0x6a445b64ed7abeb5, bank, p1, p2);
		}
		static const char* process_cash_gift(int* p0, int* p1, const char* p2) {
			return invoker::invoke<const char*>(0x20194d48eaec9a41, p0, p1, p2);
		}
		static void network_spent_player_healthcare(type::any p0, type::any p1, bool p2, bool p3) {
			invoker::invoke<void>(0x7c99101f7fce2ee5, p0, p1, p2, p3);
		}
		static void network_spent_no_cops(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0xd5bb406f4e04019f, p0, p1, p2);
		}
		static void network_spent_request_job(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x8204da7934df3155, p0, p1, p2);
		}
		static void _network_spent_request_heist(type::any p0, bool p1, bool p2) {
			invoker::invoke<void>(0x9d26502bb97bfe62, p0, p1, p2);
		}
		static void network_buy_fairground_ride(int amountspent, type::any p1, bool p2, bool p3) {
			invoker::invoke<void>(0x8a7b3952dd64d2b5, amountspent, p1, p2, p3);
		}
		static bool _0x7c4fccd2e4deb394() {
			return invoker::invoke<bool>(0x7c4fccd2e4deb394);
		}
		static int network_get_vc_bank_balance() {
			return invoker::invoke<int>(0x76ef28da05ea395a);
		}
		static int network_get_vc_wallet_balance(int character) {
			return invoker::invoke<int>(0xa40f9c2623f6a8b5, character);
		}
		static int network_get_vc_balance() {
			return invoker::invoke<int>(0x5cbad97e059e1b94);
		}
		static const char* _network_get_bank_balance_string() {
			return invoker::invoke<const char*>(0xa6fa3979bed01b81);
		}
		static bool _0xdc18531d7019a535(type::any p0, type::any p1) {
			return invoker::invoke<bool>(0xdc18531d7019a535, p0, p1);
		}
		static bool network_can_receive_player_cash(type::any p0, type::any p1, type::any p2, type::any p3) {
			return invoker::invoke<bool>(0x5d17be59d2123284, p0, p1, p2, p3);
		}
		static type::any _0xf70efa14fe091429(type::any p0) {
			return invoker::invoke<type::any>(0xf70efa14fe091429, p0);
		}
		static bool _0xe260e0bb9cd995ac(type::any p0) {
			return invoker::invoke<bool>(0xe260e0bb9cd995ac, p0);
		}
		static bool _0xe154b48b68ef72bc(type::any p0) {
			return invoker::invoke<bool>(0xe154b48b68ef72bc, p0);
		}
		static bool _0x6fcf8ddea146c45b(int p0) {
			return invoker::invoke<bool>(0x6fcf8ddea146c45b, p0);
		}
	}

	namespace dlc1 {
		static int _get_num_decorations(int character) {
			return invoker::invoke<int>(0x278f76c3b0a8f109, character);
		}
		static bool _0xff56381874f82086(int character, int p1, int* outcomponent) {
			return invoker::invoke<bool>(0xff56381874f82086, character, p1, outcomponent);
		}
		static void init_shop_ped_component(int* outcomponent) {
			invoker::invoke<void>(0x1e8c308fd312c036, outcomponent);
		}
		static void init_shop_ped_prop(int* outprop) {
			invoker::invoke<void>(0xeb0a2b758f7b850f, outprop);
		}
		static int _0x50f457823ce6eb5f(int p0, int p1, int p2, int p3) {
			return invoker::invoke<int>(0x50f457823ce6eb5f, p0, p1, p2, p3);
		}
		static int _get_num_props_from_outfit(int character, int p1, int p2, bool p3, int p4, int componentid) {
			return invoker::invoke<int>(0x9bdf59818b1e38c1, character, p1, p2, p3, p4, componentid);
		}
		static void get_shop_ped_query_component(int componentid, int* outcomponent) {
			invoker::invoke<void>(0x249e310b2d920699, componentid, outcomponent);
		}
		static void get_shop_ped_component(type::hash p0, type::any* p1) {
			invoker::invoke<void>(0x74c0e2a57ec66760, p0, p1);
		}
		static void get_shop_ped_query_prop(type::any p0, type::any* p1) {
			invoker::invoke<void>(0xde44a00999b2837d, p0, p1);
		}
		static void _0x5d5caff661ddf6fc(type::any p0, type::any* p1) {
			invoker::invoke<void>(0x5d5caff661ddf6fc, p0, p1);
		}
		static type::hash get_hash_name_for_component(type::entity entity, int componentid, int drawablevariant, int texturevariant) {
			return invoker::invoke<type::hash>(0x0368b3a838070348, entity, componentid, drawablevariant, texturevariant);
		}
		static type::hash get_hash_name_for_prop(type::entity entity, int componentid, int propindex, int proptextureindex) {
			return invoker::invoke<type::hash>(0x5d6160275caec8dd, entity, componentid, propindex, proptextureindex);
		}
		static int _get_variants_for_component_count(type::hash componenthash) {
			return invoker::invoke<int>(0xc17ad0e5752becda, componenthash);
		}
		static void get_variant_component(type::hash componenthash, int componentid, type::any* p2, type::any* p3, type::any* p4) {
			invoker::invoke<void>(0x6e11f282f11863b6, componenthash, componentid, p2, p3, p4);
		}
		static int _get_num_forced_components(type::hash componenthash) {
			return invoker::invoke<int>(0xc6b9db42c04dd8c3, componenthash);
		}
		static type::any _get_forced_component(type::any p0) {
			return invoker::invoke<type::any>(0x017568a8182d98a6, p0);
		}
		static void get_forced_component(type::hash componenthash, int componentid, type::any* p2, type::any* p3, type::any* p4) {
			invoker::invoke<void>(0x6c93ed8c2f74859b, componenthash, componentid, p2, p3, p4);
		}
		static void _0xe1ca84ebf72e691d(type::any p0, type::any p1, type::any* p2, type::any* p3, type::any* p4) {
			invoker::invoke<void>(0xe1ca84ebf72e691d, p0, p1, p2, p3, p4);
		}
		static bool _is_tag_restricted(type::hash componenthash, type::hash restrictiontag, int componentid) {
			return invoker::invoke<bool>(0x341de7ed1d2a1bfd, componenthash, restrictiontag, componentid);
		}
		static int _get_character_outfits_count(int character, bool p1) {
			return invoker::invoke<int>(0xf3fbe2d50a6a8c28, character, p1);
		}
		static void get_shop_ped_query_outfit(type::any p0, type::any* outfit) {
			invoker::invoke<void>(0x6d793f03a631fe56, p0, outfit);
		}
		static void get_shop_ped_outfit(type::any p0, type::any* p1) {
			invoker::invoke<void>(0xb7952076e444979d, p0, p1);
		}
		static type::any get_shop_ped_outfit_locate(type::any p0) {
			return invoker::invoke<type::any>(0x073ca26b079f956e, p0);
		}
		static bool get_shop_ped_outfit_prop_variant(type::any outfitstruct, int slot, type::any* propstruct) {
			return invoker::invoke<bool>(0xa9f9c2e0fde11cbb, outfitstruct, slot, propstruct);
		}
		static bool get_shop_ped_outfit_component_variant(type::any outfitstruct, int slot, type::any* componentstruct) {
			return invoker::invoke<bool>(0x19f2a026edf0013f, outfitstruct, slot, componentstruct);
		}
		static int get_num_dlc_vehicles() {
			return invoker::invoke<int>(0xa7a866d21cd2329b);
		}
		static type::hash get_dlc_vehicle_model(int dlcvehicleindex) {
			return invoker::invoke<type::hash>(0xecc01b7c5763333c, dlcvehicleindex);
		}
		static bool get_dlc_vehicle_data(int dlcvehicleindex, int* outdata) {
			return invoker::invoke<bool>(0x33468edc08e371f6, dlcvehicleindex, outdata);
		}
		static int get_dlc_vehicle_flags(int dlcvehicleindex) {
			return invoker::invoke<int>(0x5549ee11fa22fcf2, dlcvehicleindex);
		}
		static int get_num_dlc_weapons() {
			return invoker::invoke<int>(0xee47635f352da367);
		}
		static bool get_dlc_weapon_data(int dlcweaponindex, int* outdata) {
			return invoker::invoke<bool>(0x79923cd21bece14e, dlcweaponindex, outdata);
		}
		static int get_num_dlc_weapon_components(int dlcweaponindex) {
			return invoker::invoke<int>(0x405425358a7d61fe, dlcweaponindex);
		}
		static bool get_dlc_weapon_component_data(int dlcweaponindex, int dlcweapcompindex, type::any* componentdataptr) {
			return invoker::invoke<bool>(0x6cf598a2957c2bf8, dlcweaponindex, dlcweapcompindex, componentdataptr);
		}
		static bool _is_dlc_data_empty(type::any* dlcdata) {
			return invoker::invoke<bool>(0xd4d7b033c3aa243c, dlcdata);
		}
		static bool is_dlc_vehicle_mod(type::any moddata) {
			return invoker::invoke<bool>(0x0564b9ff9631b82c, moddata);
		}
		static int _0xc098810437312fff(int moddata) {
			return invoker::invoke<int>(0xc098810437312fff, moddata);
		}
	}

	namespace dlc2 {
		static bool is_dlc_present(type::hash dlchash) {
			return invoker::invoke<bool>(0x812595a0644ce1de, dlchash);
		}
		static bool _0xf2e07819ef1a5289() {
			return invoker::invoke<bool>(0xf2e07819ef1a5289);
		}
		static type::any _0x9489659372a81585() {
			return invoker::invoke<type::any>(0x9489659372a81585);
		}
		static type::any _0xa213b11dff526300() {
			return invoker::invoke<type::any>(0xa213b11dff526300);
		}
		static bool _0x8d30f648014a92b5() {
			return invoker::invoke<bool>(0x8d30f648014a92b5);
		}
		static bool get_is_loading_screen_active() {
			return invoker::invoke<bool>(0x10d0a8f259e93ec9);
		}
		static bool _nullify(type::any* variable, type::any unused) {
			return invoker::invoke<bool>(0x46e2b844905bc5f0, variable, unused);
		}
		static void _unload_mp_dlc_maps() {
			invoker::invoke<void>(0xd7c10c4a637992c9);
		}
		static void _load_mp_dlc_maps() {
			invoker::invoke<void>(0x0888c3502dbbeef5);
		}
	}

	namespace system {
		static void wait(int ms) {
			invoker::invoke<void>(0x4ede34fbadd967a6, ms);
		}
		static int start_new_script(const char* scriptname, int stacksize) {
			return invoker::invoke<int>(0xe81651ad79516e48, scriptname, stacksize);
		}
		static int start_new_script_with_args(const char* scriptname, type::any* args, int argcount, int stacksize) {
			return invoker::invoke<int>(0xb8ba7f44df1575e1, scriptname, args, argcount, stacksize);
		}
		static int start_new_script_with_name_hash(type::hash scripthash, int stacksize) {
			return invoker::invoke<int>(0xeb1c67c3a5333a92, scripthash, stacksize);
		}
		static int start_new_script_with_name_hash_and_args(type::hash scripthash, type::any* args, int argcount, int stacksize) {
			return invoker::invoke<int>(0xc4bb298bd441be78, scripthash, args, argcount, stacksize);
		}
		static int timera() {
			return invoker::invoke<int>(0x83666f9fb8febd4b);
		}
		static int timerb() {
			return invoker::invoke<int>(0xc9d9444186b5a374);
		}
		static void settimera(int value) {
			invoker::invoke<void>(0xc1b1e9a034a63a62, value);
		}
		static void settimerb(int value) {
			invoker::invoke<void>(0x5ae11bc36633de4e, value);
		}
		static float timestep() {
			return invoker::invoke<float>(0x0000000050597ee2);
		}
		static float sin(float value) {
			return invoker::invoke<float>(0x0badbfa3b172435f, value);
		}
		static float cos(float value) {
			return invoker::invoke<float>(0xd0ffb162f40a139c, value);
		}
		static float sqrt(float value) {
			return invoker::invoke<float>(0x71d93b57d07f9804, value);
		}
		static float pow(float base, float exponent) {
			return invoker::invoke<float>(0xe3621cc40f31fe2e, base, exponent);
		}
		static float vmag(float x, float y, float z) {
			return invoker::invoke<float>(0x652d2eeef1d3e62c, x, y, z);
		}
		static float vmag2(float x, float y, float z) {
			return invoker::invoke<float>(0xa8ceacb4f35ae058, x, y, z);
		}
		static float vdist(float x1, float y1, float z1, float x2, float y2, float z2) {
			return invoker::invoke<float>(0x2a488c176d52cca5, x1, y1, z1, x2, y2, z2);
		}
		static float vdist2(float x1, float y1, float z1, float x2, float y2, float z2) {
			return invoker::invoke<float>(0xb7a628320eff8e47, x1, y1, z1, x2, y2, z2);
		}
		static int shift_left(int value, int bitshift) {
			return invoker::invoke<int>(0xedd95a39e5544de8, value, bitshift);
		}
		static int shift_right(int value, int bitshift) {
			return invoker::invoke<int>(0x97ef1e5bce9dc075, value, bitshift);
		}
		static int floor(float value) {
			return invoker::invoke<int>(0xf34ee736cf047844, value);
		}
		static int ceil(float value) {
			return invoker::invoke<int>(0x11e019c8f43acc8a, value);
		}
		static int round(float value) {
			return invoker::invoke<int>(0xf2db717a73826179, value);
		}
		static float to_float(int value) {
			return invoker::invoke<float>(0xbbda792448db5a89, value);
		}
	}

	namespace decorator {
		static bool decor_set_time(type::entity entity, const char* propertyname, int timestamp) {
			return invoker::invoke<bool>(0x95aed7b8e39ecaa4, entity, propertyname, timestamp);
		}
		static bool decor_set_bool(type::entity entity, const char* propertyname, bool value) {
			return invoker::invoke<bool>(0x6b1e8e2ed1335b71, entity, propertyname, value);
		}
		static bool _decor_set_float(type::entity entity, const char* propertyname, float value) {
			return invoker::invoke<bool>(0x211ab1dd8d0f363a, entity, propertyname, value);
		}
		static bool decor_set_int(type::entity entity, const char* propertyname, int value) {
			return invoker::invoke<bool>(0x0ce3aa5e1ca19e10, entity, propertyname, value);
		}
		static bool decor_get_bool(type::entity entity, const char* propertyname) {
			return invoker::invoke<bool>(0xdace671663f2f5db, entity, propertyname);
		}
		static float _decor_get_float(type::entity entity, const char* propertyname) {
			return invoker::invoke<float>(0x6524a2f114706f43, entity, propertyname);
		}
		static int decor_get_int(type::entity entity, const char* propertyname) {
			return invoker::invoke<int>(0xa06c969b02a97298, entity, propertyname);
		}
		static bool decor_exist_on(type::entity entity, const char* propertyname) {
			return invoker::invoke<bool>(0x05661b80a8c9165f, entity, propertyname);
		}
		static bool decor_remove(type::entity entity, const char* propertyname) {
			return invoker::invoke<bool>(0x00ee9f297c738720, entity, propertyname);
		}
		static void decor_register(const char* propertyname, int type) {
			invoker::invoke<void>(0x9fd90732f56403ce, propertyname, type);
		}
		static bool decor_is_registered_as_type(const char* propertyname, int type) {
			return invoker::invoke<bool>(0x4f14f9f870d6fbc8, propertyname, type);
		}
		static void decor_register_lock() {
			invoker::invoke<void>(0xa9d14eea259f9248);
		}
		static bool _0x241fca5b1aa14f75() {
			return invoker::invoke<bool>(0x241fca5b1aa14f75);
		}
	}

	namespace socialclub {
		static int _get_total_sc_inbox_ids() {
			return invoker::invoke<int>(0x03a93ff1a2ca0864);
		}
		static type::hash _sc_inbox_message_init(int p0) {
			return invoker::invoke<type::hash>(0xbb8ea16ecbc976c4, p0);
		}
		static bool _is_sc_inbox_valid(int p0) {
			return invoker::invoke<bool>(0x93028f1db42bfd08, p0);
		}
		static bool _sc_inbox_message_pop(int p0) {
			return invoker::invoke<bool>(0x2c015348cf19ca1d, p0);
		}
		static bool sc_inbox_message_get_data_int(int p0, const char* context, int* out) {
			return invoker::invoke<bool>(0xa00efe4082c4056e, p0, context, out);
		}
		static bool _sc_inbox_message_get_data_bool(int p0, const char* p1) {
			return invoker::invoke<bool>(0xffe5c16f402d851d, p0, p1);
		}
		static bool sc_inbox_message_get_data_string(int p0, const char* context, const char* out) {
			return invoker::invoke<bool>(0x7572ef42fc6a9b6d, p0, context, out);
		}
		static bool _sc_inbox_message_push(int p0) {
			return invoker::invoke<bool>(0x9a2c8064b6c1e41a, p0);
		}
		static const char* _sc_inbox_message_get_string(int p0) {
			return invoker::invoke<const char*>(0xf3e31d16cbdcb304, p0);
		}
		static void _0xda024bdbd600f44a(int* networkhandle) {
			invoker::invoke<void>(0xda024bdbd600f44a, networkhandle);
		}
		static void _0xa68d3d229f4f3b06(const char* p0) {
			invoker::invoke<void>(0xa68d3d229f4f3b06, p0);
		}
		static bool sc_inbox_message_get_ugcdata(type::any p0, type::any* p1) {
			return invoker::invoke<bool>(0x69d82604a1a5a254, p0, p1);
		}
		static bool _0x6afd2cd753feef83(const char* playername) {
			return invoker::invoke<bool>(0x6afd2cd753feef83, playername);
		}
		static bool _0x87e0052f08bd64e6(type::any p0, int* p1) {
			return invoker::invoke<bool>(0x87e0052f08bd64e6, p0, p1);
		}
		static void _sc_inbox_get_emails(int offset, int limit) {
			invoker::invoke<void>(0x040addcbafa1018a, offset, limit);
		}
		static type::any _0x16da8172459434aa() {
			return invoker::invoke<type::any>(0x16da8172459434aa);
		}
		static bool _0x4737980e8a283806(int p0, type::any* p1) {
			return invoker::invoke<bool>(0x4737980e8a283806, p0, p1);
		}
		static void _0x44aca259d67651db(type::any* p0, type::any p1) {
			invoker::invoke<void>(0x44aca259d67651db, p0, p1);
		}
		static void sc_email_message_push_gamer_to_recip_list(type::player* player) {
			invoker::invoke<void>(0x2330c12a7a605d16, player);
		}
		static void sc_email_message_clear_recip_list() {
			invoker::invoke<void>(0x55df6db45179236e);
		}
		static void _0x116fb94dc4b79f17(const char* p0) {
			invoker::invoke<void>(0x116fb94dc4b79f17, p0);
		}
		static void _0xbfa0a56a817c6c7d(bool p0) {
			invoker::invoke<void>(0xbfa0a56a817c6c7d, p0);
		}
		static bool _0xbc1cc91205ec8d6e() {
			return invoker::invoke<bool>(0xbc1cc91205ec8d6e);
		}
		static const char* _0xdf649c4e9afdd788() {
			return invoker::invoke<const char*>(0xdf649c4e9afdd788);
		}
		static bool _0x1f1e9682483697c7(type::any p0, type::any p1) {
			return invoker::invoke<bool>(0x1f1e9682483697c7, p0, p1);
		}
		static bool _0x287f1f75d2803595(type::any p0, type::any* p1) {
			return invoker::invoke<bool>(0x287f1f75d2803595, p0, p1);
		}
		static bool _0x487912fd248efddf(type::any p0, float p1) {
			return invoker::invoke<bool>(0x487912fd248efddf, p0, p1);
		}
		static bool _0x8416fe4e4629d7d7(const char* p0) {
			return invoker::invoke<bool>(0x8416fe4e4629d7d7, p0);
		}
		static bool _sc_start_check_string_task(const char* string, int* taskhandle) {
			return invoker::invoke<bool>(0x75632c5ecd7ed843, string, taskhandle);
		}
		static bool _sc_has_check_string_task_completed(int taskhandle) {
			return invoker::invoke<bool>(0x1753344c770358ae, taskhandle);
		}
		static int _sc_get_check_string_status(int taskhandle) {
			return invoker::invoke<int>(0x82e4a58babc15ae7, taskhandle);
		}
		static type::any _0x85535acf97fc0969(type::any p0) {
			return invoker::invoke<type::any>(0x85535acf97fc0969, p0);
		}
		static int _0x930de22f07b1cce3(type::any p0) {
			return invoker::invoke<int>(0x930de22f07b1cce3, p0);
		}
		static bool _0xf6baaaf762e1bf40(const char* p0, int* p1) {
			return invoker::invoke<bool>(0xf6baaaf762e1bf40, p0, p1);
		}
		static bool _0xf22ca0fd74b80e7a(type::any p0) {
			return invoker::invoke<bool>(0xf22ca0fd74b80e7a, p0);
		}
		static type::any _0x9237e334f6e43156(type::any p0) {
			return invoker::invoke<type::any>(0x9237e334f6e43156, p0);
		}
		static type::any _0x700569dba175a77c(type::any p0) {
			return invoker::invoke<type::any>(0x700569dba175a77c, p0);
		}
		static type::any _0x1d4446a62d35b0d0(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x1d4446a62d35b0d0, p0, p1);
		}
		static type::any _0x2e89990ddff670c3(type::any p0, type::any p1) {
			return invoker::invoke<type::any>(0x2e89990ddff670c3, p0, p1);
		}
		static bool _0xd0ee05fe193646ea(type::any* p0, type::any* p1, type::any* p2) {
			return invoker::invoke<bool>(0xd0ee05fe193646ea, p0, p1, p2);
		}
		static bool _0x1989c6e6f67e76a8(type::any* p0, type::any* p1, type::any* p2) {
			return invoker::invoke<bool>(0x1989c6e6f67e76a8, p0, p1, p2);
		}
		static type::any _0x07c61676e5bb52cd(type::any p0) {
			return invoker::invoke<type::any>(0x07c61676e5bb52cd, p0);
		}
		static type::any _0x8147fff6a718e1ad(type::any p0) {
			return invoker::invoke<type::any>(0x8147fff6a718e1ad, p0);
		}
		static bool _0x0f73393bac7e6730(type::any* p0, int* p1) {
			return invoker::invoke<bool>(0x0f73393bac7e6730, p0, p1);
		}
		static type::any _0xd302e99edf0449cf(type::any p0) {
			return invoker::invoke<type::any>(0xd302e99edf0449cf, p0);
		}
		static type::any _0x5c4ebffa98bdb41c(type::any p0) {
			return invoker::invoke<type::any>(0x5c4ebffa98bdb41c, p0);
		}
		static type::any _0xff8f3a92b75ed67a() {
			return invoker::invoke<type::any>(0xff8f3a92b75ed67a);
		}
		static type::any _0x4a7d6e727f941747(type::any* p0) {
			return invoker::invoke<type::any>(0x4a7d6e727f941747, p0);
		}
		static bool _0x8cc469ab4d349b7c(int p0, const char* p1, type::any* p2) {
			return invoker::invoke<bool>(0x8cc469ab4d349b7c, p0, p1, p2);
		}
		static bool _0x699e4a5c8c893a18(int p0, const char* p1, type::any* p2) {
			return invoker::invoke<bool>(0x699e4a5c8c893a18, p0, p1, p2);
		}
		static bool _0x19853b5b17d77bca(type::any p0, type::any* p1) {
			return invoker::invoke<bool>(0x19853b5b17d77bca, p0, p1);
		}
		static bool _0x6bfb12ce158e3dd4(type::any p0) {
			return invoker::invoke<bool>(0x6bfb12ce158e3dd4, p0);
		}
		static bool _0xfe4c1d0d3b9cc17e(type::any p0, type::any p1) {
			return invoker::invoke<bool>(0xfe4c1d0d3b9cc17e, p0, p1);
		}
		static type::any _0xd8122c407663b995() {
			return invoker::invoke<type::any>(0xd8122c407663b995);
		}
		static bool _0x3001bef2feca3680() {
			return invoker::invoke<bool>(0x3001bef2feca3680);
		}
		static bool _0x92da6e70ef249bd1(const char* p0, int* p1) {
			return invoker::invoke<bool>(0x92da6e70ef249bd1, p0, p1);
		}
		static void _0x675721c9f644d161() {
			invoker::invoke<void>(0x675721c9f644d161);
		}
		static const char* _sc_get_nickname() {
			return invoker::invoke<const char*>(0x198d161f458ecc7f);
		}
		static bool _0x225798743970412b(int* p0) {
			return invoker::invoke<bool>(0x225798743970412b, p0);
		}
		static bool _0x418dc16fae452c1c(int p0) {
			return invoker::invoke<bool>(0x418dc16fae452c1c, p0);
		}
	}

	namespace unk {
		static int _return_zero() {
			return invoker::invoke<int>(0xf2ca003f167e21d2);
		}
		static bool _return_zero2() {
			return invoker::invoke<bool>(0xef7d17bc6c85264c);
		}
		static void _0xb0c56bd3d808d863(bool p0) {
			invoker::invoke<void>(0xb0c56bd3d808d863, p0);
		}
		static type::any _0x8aa464d4e0f6accd() {
			return invoker::invoke<type::any>(0x8aa464d4e0f6accd);
		}
		static void _is_in_loading_screen(bool p0) {
			invoker::invoke<void>(0xfc309e94546fcdb5, p0);
		}
		static bool _is_ui_loading_multiplayer() {
			return invoker::invoke<bool>(0xc6dc823253fbb366);
		}
		static void _0xc7e7181c09f33b69(bool p0) {
			invoker::invoke<void>(0xc7e7181c09f33b69, p0);
		}
		static void _0xfa1e0e893d915215(bool p0) {
			invoker::invoke<void>(0xfa1e0e893d915215, p0);
		}
		static int _get_current_language_id() {
			return invoker::invoke<int>(0x2bdd44cc428a7eae);
		}
		static int _get_user_language_id() {
			return invoker::invoke<int>(0xa8ae43aec1a61314);
		}
	}

	namespace unk1 {
		static void _0x48621c9fca3ebd28(int p0) {
			invoker::invoke<void>(0x48621c9fca3ebd28, p0);
		}
		static void _0x81cbae94390f9f89() {
			invoker::invoke<void>(0x81cbae94390f9f89);
		}
		static void _0x13b350b8ad0eee10() {
			invoker::invoke<void>(0x13b350b8ad0eee10);
		}
		static void _0x293220da1b46cebc(float p0, float p1, int p2) {
			invoker::invoke<void>(0x293220da1b46cebc, p0, p1, p2);
		}
		static void _0x208784099002bc30(const char* missionnamelabel, type::any p1) {
			invoker::invoke<void>(0x208784099002bc30, missionnamelabel, p1);
		}
		static void _0xeb2d525b57f42b40() {
			invoker::invoke<void>(0xeb2d525b57f42b40);
		}
		static void _0xf854439efbb3b583() {
			invoker::invoke<void>(0xf854439efbb3b583);
		}
		static void _0xaf66dcee6609b148() {
			invoker::invoke<void>(0xaf66dcee6609b148);
		}
		static void _0x66972397e0757e7a(type::any p0, type::any p1, type::any p2) {
			invoker::invoke<void>(0x66972397e0757e7a, p0, p1, p2);
		}
		static void _start_recording(int mode) {
			invoker::invoke<void>(0xc3ac2fff9612ac81, mode);
		}
		static void _stop_recording_and_save_clip() {
			invoker::invoke<void>(0x071a5197d6afc8b3);
		}
		static void _stop_recording_and_discard_clip() {
			invoker::invoke<void>(0x88bb3507ed41a240);
		}
		static bool _0x644546ec5287471b() {
			return invoker::invoke<bool>(0x644546ec5287471b);
		}
		static bool _is_recording() {
			return invoker::invoke<bool>(0x1897ca71995a90b4);
		}
		static type::any _0xdf4b952f7d381b95() {
			return invoker::invoke<type::any>(0xdf4b952f7d381b95);
		}
		static type::any _0x4282e08174868be3() {
			return invoker::invoke<type::any>(0x4282e08174868be3);
		}
		static bool _0x33d47e85b476abcd(bool* p0) {
			return invoker::invoke<bool>(0x33d47e85b476abcd, p0);
		}
	}

	namespace unk2 {
		static void _0x7e2bd3ef6c205f09(const char* p0, bool p1) {
			invoker::invoke<void>(0x7e2bd3ef6c205f09, p0, p1);
		}
		static bool _is_interior_rendering_disabled() {
			return invoker::invoke<bool>(0x95ab8b5c992c7b58);
		}
		static void _0x5ad3932daeb1e5d3() {
			invoker::invoke<void>(0x5ad3932daeb1e5d3);
		}
		static void _0xe058175f8eafe79a(bool p0) {
			invoker::invoke<void>(0xe058175f8eafe79a, p0);
		}
		static void _reset_editor_values() {
			invoker::invoke<void>(0x3353d13f09307691);
		}
		static void _activate_rockstar_editor() {
			invoker::invoke<void>(0x49da8145672b2725);
		}
	}

	namespace unk3 {
		static int _network_shop_get_price(type::hash itemhash, type::hash categoryhash, int unused) {
			return invoker::invoke<int>(0xc27009422fcca88d, itemhash, categoryhash, unused);
		}
		static type::any _0x3c4487461e9b0dcb() {
			return invoker::invoke<type::any>(0x3c4487461e9b0dcb);
		}
		static type::any _0x2b949a1e6aec8f6a() {
			return invoker::invoke<type::any>(0x2b949a1e6aec8f6a);
		}
		static type::any _0x85f6c9aba1de2bcf() {
			return invoker::invoke<type::any>(0x85f6c9aba1de2bcf);
		}
		static type::any _0x357b152ef96c30b6() {
			return invoker::invoke<type::any>(0x357b152ef96c30b6);
		}
		static bool _0xcf38dafbb49ede5e(type::any* p0) {
			return invoker::invoke<bool>(0xcf38dafbb49ede5e, p0);
		}
		static type::any _0xe3e5a7c64ca2c6ed() {
			return invoker::invoke<type::any>(0xe3e5a7c64ca2c6ed);
		}
		static bool _0x0395cb47b022e62c(type::any* p0) {
			return invoker::invoke<bool>(0x0395cb47b022e62c, p0);
		}
		static bool _network_shop_start_session(type::any p0) {
			return invoker::invoke<bool>(0xa135ac892a58fc07, p0);
		}
		static bool _0x72eb7ba9b69bf6ab() {
			return invoker::invoke<bool>(0x72eb7ba9b69bf6ab);
		}
		static bool _0x170910093218c8b9(type::any* p0) {
			return invoker::invoke<bool>(0x170910093218c8b9, p0);
		}
		static bool _0xc13c38e47ea5df31(type::any* p0) {
			return invoker::invoke<bool>(0xc13c38e47ea5df31, p0);
		}
		static bool _network_shop_get_transactions_enabled_for_character(int mpchar) {
			return invoker::invoke<bool>(0xb24f0944da203d9e, mpchar);
		}
		static int _0x74a0fd0688f1ee45(int p0) {
			return invoker::invoke<int>(0x74a0fd0688f1ee45, p0);
		}
		static bool _network_shop_session_apply_received_data(type::any p0) {
			return invoker::invoke<bool>(0x2f41d51ba3bcd1f1, p0);
		}
		static bool _network_shop_get_transactions_disabled() {
			return invoker::invoke<bool>(0x810e8431c0614bf9);
		}
		static bool _0x35a1b3e1d1315cfa(bool p0, bool p1) {
			return invoker::invoke<bool>(0x35a1b3e1d1315cfa, p0, p1);
		}
		static bool _0x897433d292b44130(type::any* p0, type::any* p1) {
			return invoker::invoke<bool>(0x897433d292b44130, p0, p1);
		}
		static bool _network_shop_basket_start(type::any* p0, int p1, int p2, int p3) {
			return invoker::invoke<bool>(0x279f08b1a4b29b7e, p0, p1, p2, p3);
		}
		static bool _network_shop_basket_end() {
			return invoker::invoke<bool>(0xa65568121df2ea26);
		}
		static bool _network_shop_basket_add_item(type::any* p0, type::any p1) {
			return invoker::invoke<bool>(0xf30980718c8ed876, p0, p1);
		}
		static type::any _network_shop_basket_is_full() {
			return invoker::invoke<type::any>(0x27f76cc6c55ad30e);
		}
		static bool _network_shop_basket_apply_server_data(type::any p0, type::any* p1) {
			return invoker::invoke<bool>(0xe1a0450ed46a7812, p0, p1);
		}
		static bool _network_shop_checkout_start(type::any p0) {
			return invoker::invoke<bool>(0x39be7cea8d9cc8e6, p0);
		}
		static bool _network_shop_begin_service(int* transactionid, type::hash p1, type::hash transactionhash, type::hash transactiontype, int ammount, int mode) {
			return invoker::invoke<bool>(0x3c5fd37b5499582e, transactionid, p1, transactionhash, transactiontype, ammount, mode);
		}
		static bool _network_shop_terminate_service(int transactionid) {
			return invoker::invoke<bool>(0xe2a99a9b524befff, transactionid);
		}
		static bool _0x51f1a8e48c3d2f6d(type::any p0, bool p1, type::any p2) {
			return invoker::invoke<bool>(0x51f1a8e48c3d2f6d, p0, p1, p2);
		}
		static type::any _0x0a6d923dffc9bd89() {
			return invoker::invoke<type::any>(0x0a6d923dffc9bd89);
		}
		static type::any _network_shop_delete_set_telemetry_nonce_seed() {
			return invoker::invoke<type::any>(0x112cef1615a1139f);
		}
		static bool _network_transfer_bank_to_wallet(int charstatint, int amount) {
			return invoker::invoke<bool>(0xd47a2c1ba117471d, charstatint, amount);
		}
		static bool _network_transfer_wallet_to_bank(int charstatint, int amount) {
			return invoker::invoke<bool>(0xc2f7fe5309181c7d, charstatint, amount);
		}
		static type::any _0x23789e777d14ce44() {
			return invoker::invoke<type::any>(0x23789e777d14ce44);
		}
		static type::any _0x350aa5ebc03d3bd2() {
			return invoker::invoke<type::any>(0x350aa5ebc03d3bd2);
		}
		static bool _network_shop_cash_transfer_set_telemetry_nonce_seed() {
			return invoker::invoke<bool>(0x498c1e05ce5f7877);
		}
		static bool _network_shop_set_telemetry_nonce_seed(type::any p0) {
			return invoker::invoke<bool>(0x9507d4271988e1ae, p0);
		}
		static const char* _get_online_version() {
			return invoker::invoke<const char*>(0xfca9373ef340ac0a);
		}
	}

#pragma warning(pop)

}
