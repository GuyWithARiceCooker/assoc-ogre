/**
 * Pacman Ogre uses X11 EGL (OgreX11EGLWindow). SDL must not open a native Wayland surface —
 * miscParams then contain externalWlDisplay and Ogre asserts.
 *
 * SDL hints/env alone are not enough on some setups (Wayland + sdl2-compat): unset
 * WAYLAND_DISPLAY / WAYLAND_SOCKET before any SDL init so the x11 driver is chosen (XWayland).
 *
 * Native Wayland Ogre build: ASSOC_OGRE_WAYLAND_NATIVE=1 (skip all of this).
 */
#include "AssocLinuxWaylandEnv.h"

#if defined(__linux__)

#include <SDL.h>
#include <cstdlib>

namespace AssocLinuxEnv
{
void applyLinuxDisplayEnvForOgre()
{
    if (std::getenv("ASSOC_OGRE_WAYLAND_NATIVE"))
    {
        return;
    }
    if (std::getenv("SDL_VIDEODRIVER") || std::getenv("SDL_VIDEO_DRIVER"))
    {
        return;
    }

    ::unsetenv("WAYLAND_DISPLAY");
    ::unsetenv("WAYLAND_SOCKET");

    SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER, "x11", SDL_HINT_OVERRIDE);
    ::setenv("SDL_VIDEODRIVER", "x11", 1);
}
} // namespace AssocLinuxEnv

#else

namespace AssocLinuxEnv
{
void applyLinuxDisplayEnvForOgre() {}
} // namespace AssocLinuxEnv

#endif
