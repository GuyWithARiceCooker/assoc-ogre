/**
 * Linux + distro Ogre (OGRE_USE_WAYLAND=OFF): SDL Wayland → externalWlDisplay assert.
 * Call once from main() before SDL/Ogre init. Override: ASSOC_OGRE_WAYLAND_NATIVE=1 or SDL_VIDEODRIVER.
 */
#pragma once

#if defined(__linux__)

#include <cstdlib>
#include <cstring>

namespace AssocLinuxEnv
{
inline void preferX11ForDistroOgreOnWayland()
{
    if (std::getenv("ASSOC_OGRE_WAYLAND_NATIVE"))
    {
        return;
    }
    if (std::getenv("SDL_VIDEODRIVER"))
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
inline void preferX11ForDistroOgreOnWayland() {}
} // namespace AssocLinuxEnv

#endif
