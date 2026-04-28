/**
 * Linux display backend before SDL / Ogre init.
 *
 * Default: do **not** force X11 — use **native Wayland** when SDL chooses it; requires OGRE
 * built with **OGRE_USE_WAYLAND=ON** (not the typical Arch distro package).
 *
 * Arch `ogre` package is often X11-EGL only → needs **XWayland** workaround:
 *   ASSOC_OGRE_USE_XWAYLAND=1
 * or:
 *   SDL_VIDEODRIVER=x11
 */
#pragma once

#if defined(__linux__)

#include <cstdlib>
#include <cstring>

namespace AssocLinuxEnv
{
inline void applyLinuxDisplayEnvForOgre()
{
    if (std::getenv("SDL_VIDEODRIVER"))
    {
        return;
    }
    char const* useXw{std::getenv("ASSOC_OGRE_USE_XWAYLAND")};
    if (useXw != nullptr && useXw[0] != '\0' && std::strcmp(useXw, "0") != 0)
    {
        ::setenv("SDL_VIDEODRIVER", "x11", 1);
    }
}
} // namespace AssocLinuxEnv

#else

namespace AssocLinuxEnv
{
inline void applyLinuxDisplayEnvForOgre() {}
} // namespace AssocLinuxEnv

#endif
