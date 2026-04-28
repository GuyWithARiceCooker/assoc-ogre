/**
 * Pacman Ogre: X11 EGL path only — must not pass Wayland surfaces (externalWlDisplay assert).
 *
 * Force x11: unset WAYLAND_*, hints, env, SDL_VideoQuit + SDL_VideoInit("x11") before Ogre.
 * Fallback: SDL_Init(SDL_INIT_VIDEO). Native Wayland Ogre: ASSOC_OGRE_WAYLAND_NATIVE=1.
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

    SDL_VideoQuit();

    int rc{SDL_VideoInit("x11")};
    if (rc != 0)
    {
        std::fprintf(stderr, "assoc-ogre: SDL_VideoInit(\"x11\") failed: %s\n", SDL_GetError());
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            std::fprintf(stderr, "assoc-ogre: SDL_Init(SDL_INIT_VIDEO) fallback failed: %s\n", SDL_GetError());
        }
    }

    char const* dr{SDL_GetCurrentVideoDriver()};
    if (dr && std::strcmp(dr, "x11") != 0)
    {
        std::fprintf(stderr,
            "assoc-ogre: SDL video driver is '%s' (pacman ogre needs x11). Run from repo: ./scripts/run-meadow-x11.sh "
            "or: pacman -S sdl2 (remove sdl2-compat if possible).\n",
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
