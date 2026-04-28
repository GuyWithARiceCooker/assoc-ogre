/**
 * Linux display backend before SDL / Ogre init.
 *
 * Arch **`pacman -S ogre`** is usually **X11 EGL only** (no OGRE_USE_WAYLAND). If SDL opens a
 * **native Wayland** window, Ogre still uses **OgreX11EGLWindow** → `externalWlDisplay` assert.
 *
 * **Default:** on Wayland sessions, set **`SDL_VIDEODRIVER=x11`** (XWayland) unless overridden.
 *
 * **Native Wayland** (Ogre built with **OGRE_USE_WAYLAND=ON**): set before running:
 *   export ASSOC_OGRE_WAYLAND_NATIVE=1
 * then SDL may use Wayland and Ogre's Wayland window path.
 *
 * Overrides: `SDL_VIDEODRIVER` (always wins), `ASSOC_OGRE_WAYLAND_NATIVE=1` (skip forcing x11).
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
    if (std::getenv("ASSOC_OGRE_WAYLAND_NATIVE"))
    {
        return;
    }
    char const* const sessionType{std::getenv("XDG_SESSION_TYPE")};
    bool const waylandSession{sessionType != nullptr && std::strcmp(sessionType, "wayland") == 0};
    bool const hasWlDisp{std::getenv("WAYLAND_DISPLAY") != nullptr};
    if (waylandSession || hasWlDisp)
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
