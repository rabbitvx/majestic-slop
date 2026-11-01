#pragma once

namespace build_profile {
#if defined(HELLO_DEBUG_BUILD)
    inline constexpr bool debug = true;
    inline constexpr bool production = false;
#else
    inline constexpr bool debug = false;
    inline constexpr bool production = true;
#endif
}
