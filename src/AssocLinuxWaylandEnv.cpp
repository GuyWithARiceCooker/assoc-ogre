/**
 * Pacman Ogre: X11 EGL only. SDL must use X11/XWayland, not native Wayland (externalWlDisplay assert).
 *
 * sdl2-compat + retained WAYLAND_* can still pick Wayland — we unset those, set both env names,
 * hint override, and SDL_Init(SDL_INIT_VIDEO) before Ogre so the subsystem locks to x11.
 *
 * Escape hatch: ASSOC_OGRE_WAYLAND_NATIVE=1 (own OGRE_USE_WAYLAND=ON build).
 */
#include "AssocLinuxWaylandEnv.h"

#if defined(__linux__)

#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace AssocLinuxEnv
{
void applyLinuxDisplayEnvForOgre()
{
    if (std::getenv("ASSOC_OGRE_WAYLAND_NATIVE"))
    {
        return;
    }

    ::unsetenv("WAYLAND_DISPLAY");
    ::unsetenv("WAYLAND_SOCKET");

    SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER, "x11", SDL_HINT_OVERRIDE);
    ::setenv("SDL_VIDEODRIVER", "x11", 1);
    ::setenv("SDL_VIDEO_DRIVER", "x11", 1);

    if (SDL_WasInit(SDL_INIT_VIDEO))
    {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::fprintf(stderr, "assoc-ogre: SDL_Init(SDL_INIT_VIDEO) failed: %s\n", SDL_GetError());
        return;
    }

    char const* dr{SDL_GetCurrentVideoDriver()};
    if (dr && std::strcmp(dr, "x11") != 0)
    {
        std::fprintf(
            stderr,
            "assoc-ogre: SDL video driver is '%s' (need x11 for pacman ogre). Try: pacman -S sdl2 && pacman -R sdl2-compat\n",
            dr);
    }
}
} // namespace AssocLinuxEnv

#else

namespace AssocLinuxEnv
{
void applyLinuxDisplayEnvForOgre() {}
} // namespace AssocLinuxEnv

#endif
