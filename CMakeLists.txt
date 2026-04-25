# =============================================================================
# projekt: assoc-ogre
# =============================================================================
# Egyetlen cél: `assoc` futtatható, Ogre3D 14 (Bites) + RTShader; 3D szöveg a jelenetben (font quads).
# Build lépések (röviden, ha minden a gépen van):
#   cmake -S . -B build -DCMAKE_PREFIX_PATH="/Users/tomi/ogre-sdk;/usr/local/opt/sdl2"
#   cmake --build build
# Futtatás: a `build/` könyvtárból (ott generálódnak a .cfg fájlok), vagy ./run-mac.sh
#
# Távoli ASUS: ne másold át a teljes Ogre-SDK-t feleslegesen; a forráshoz
#   scripts/asus-ubuntu-sync.sh  (a távoli gépen külön Ogre / prefix szükséges).
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(assoc-ogre LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Telepített Ogre 14 SDK (a gépen a build-release már létrehozta a prefixet)
set(OGRE_SDK "/Users/tomi/ogre-sdk" CACHE PATH "Ogre3D install prefix")
# Minta-média (hálók, .material): a forrásfa Samples/Media, nem kerül a „Release” Ogre-telepítésbe
set(OGRE_SAMPLES_MEDIA "/Users/tomi/ogresrc/Samples/Media" CACHE PATH "OGRE sample meshes/materials path")

if(NOT EXISTS "${OGRE_SDK}/CMake/OGREConfig.cmake")
  message(FATAL_ERROR "Nem találom az Ogre SDK-t: ${OGRE_SDK}/CMake/OGREConfig.cmake. Előbb fordítsd/telepítsd az Ogre3D-t (pl. v14.3.2).")
endif()

list(APPEND CMAKE_PREFIX_PATH "${OGRE_SDK}" "/usr/local/opt/sdl2")
set(OGRE_DIR "${OGRE_SDK}/CMake")

find_package(OGRE REQUIRED COMPONENTS Bites CONFIG)

# Konfig: plugin + erőforrás útvonalak (abszolút, így a build könyvtárból is indul)
configure_file(
  "${CMAKE_SOURCE_DIR}/cfg/plugins.cfg.in"
  "${CMAKE_BINARY_DIR}/plugins.cfg" @ONLY)
configure_file(
  "${CMAKE_SOURCE_DIR}/cfg/resources.cfg.in"
  "${CMAKE_BINARY_DIR}/resources.cfg" @ONLY)

# Első indításhoz kell (ne a rendszer config dialógus blokkolja a fejlesztést)
file(COPY "${CMAKE_SOURCE_DIR}/cfg/ogre.cfg" DESTINATION "${CMAKE_BINARY_DIR}")

add_executable(assoc
  src/AssocApp.cpp)

target_link_libraries(assoc PRIVATE OgreBites)
target_include_directories(assoc PRIVATE ${OGRE_INCLUDE_DIR})

# futásidőben: dylib-ek és cfg a mellett
set_target_properties(assoc PROPERTIES
  BUILD_RPATH "${OGRE_SDK}/lib;${OGRE_SDK}/lib/OGRE"
  BUILD_WITH_INSTALL_RPATH FALSE)

# nem installáljuk, csak a build könyvtárat használjuk
