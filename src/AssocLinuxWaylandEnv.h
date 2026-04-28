/**
 * Linux display backend before SDL / Ogre init.
 * See AssocLinuxWaylandEnv.cpp — SDL hint overrides Wayland preference on distro Ogre.
 */
#pragma once

namespace AssocLinuxEnv
{
void applyLinuxDisplayEnvForOgre();
}
