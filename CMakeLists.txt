# =============================================================================
# projekt: assoc-ogre
# =============================================================================
# Egyetlen cél: `assoc` futtatható, Ogre3D 14 (Bites) + RTShader demó.
# A jelenet asset-mentes: ManualObject víz + moduláris napelemes katamarán.
# Build:
#   cmake -S . -B build -DOGRE_SDK=/usr
#   cmake --build build
# Futtatás: a `build/` könyvtárból, mert ott vannak a generált .cfg fájlok.
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(assoc-ogre LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Telepitett Ogre 14 SDK. Archon tipikusan /usr, sajat forditasnal egy prefix.
set(OGRE_SDK "/usr" CACHE PATH "Ogre3D install prefix")
set(OGRE_PLUGIN_DIR "${OGRE_SDK}/lib/OGRE" CACHE PATH "Ogre3D plugin directory")

list(APPEND CMAKE_PREFIX_PATH "${OGRE_SDK}" "/usr/local/opt/sdl2")

find_package(OGRE REQUIRED COMPONENTS Bites CONFIG
  PATHS "${OGRE_SDK}"
  PATH_SUFFIXES CMake lib/cmake/OGRE lib64/cmake/OGRE share/OGRE/cmake)

# Konfig: plugin + eroforras utvonalak (abszolut, igy a build konyvtarbol is indul)
configure_file(
  "${CMAKE_SOURCE_DIR}/cfg/plugins.cfg.in"
  "${CMAKE_BINARY_DIR}/plugins.cfg" @ONLY)
configure_file(
  "${CMAKE_SOURCE_DIR}/cfg/resources.cfg.in"
  "${CMAKE_BINARY_DIR}/resources.cfg" @ONLY)

# Elso inditashoz kell (ne a rendszer config dialogus blokkolja a fejlesztest)
file(COPY "${CMAKE_SOURCE_DIR}/cfg/ogre.cfg" DESTINATION "${CMAKE_BINARY_DIR}")

add_executable(assoc
  src/AssocApp.cpp)

target_link_libraries(assoc PRIVATE OgreBites)
target_include_directories(assoc PRIVATE ${OGRE_INCLUDE_DIR})

# futasidoben: shared library-k es cfg a mellett
set_target_properties(assoc PROPERTIES
  BUILD_RPATH "${OGRE_SDK}/lib;${OGRE_PLUGIN_DIR}"
  BUILD_WITH_INSTALL_RPATH FALSE)

# nem installaljuk, csak a build konyvtarat hasznaljuk
