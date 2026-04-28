/**
 * Force SDL x11 video driver for Arch/pacman Ogre (X11 EGL path). SDL may ignore
 * SDL_VIDEODRIVER env when WAYLAND_DISPLAY is set — SDL_HINT_VIDEO_DRIVER + OVERRIDE fixes it.
 */
#include "AssocLinuxWaylandEnv.h"

#if defined(__linux__)

#include <SDL.h>
#include <cstdlib>

namespace AssocLinuxEnv
{
void applyLinuxDisplayEnvForOgre()
{
    if (std::getenv("SDL_VIDEODRIVER"))
    {
        return;
    }
    if (std::getenv("ASSOC_OGRE_WAYLAND_NATIVE"))
    {
        return;
    }
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
