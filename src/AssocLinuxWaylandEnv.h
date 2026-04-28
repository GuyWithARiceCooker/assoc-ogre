/**
 * Linux display backend before SDL / Ogre init.
 *
 * Arch **`pacman -S ogre`** matches **SDL video driver x11** (XWayland on Wayland desktops). Native SDL Wayland
 * + distro Ogre leads to **externalWlDisplay** assert.
 *
 * **Default:** **`SDL_VIDEODRIVER=x11`** on all Linux runs unless already set.
 *
 * **Native Wayland Ogre** (`OGRE_USE_WAYLAND=ON`): run with **`ASSOC_OGRE_WAYLAND_NATIVE=1`** so SDL can pick Wayland.
 *
 * Overrides: `SDL_VIDEODRIVER` always wins if set before main().
 */
#pragma once

#if defined(__linux__)

#include <cstdlib>

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
    ::setenv("SDL_VIDEODRIVER", "x11", 1);
}
} // namespace AssocLinuxEnv

#else

namespace AssocLinuxEnv
{
inline void applyLinuxDisplayEnvForOgre() {}
} // namespace AssocLinuxEnv

#endif
