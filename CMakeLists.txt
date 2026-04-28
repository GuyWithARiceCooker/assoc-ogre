# =============================================================================
# projekt: assoc-ogre
# =============================================================================
# Build (Linux / Arch példa):
#   cmake -S . -B build
#   cmake --build build
# Opcionális: -DOGRE_SAMPLES_MEDIA=/path/to/ogre/Samples/Media (assoc minta hálókhoz)
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(assoc-ogre LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Felülírható: saját Ogre prefix (üres = CMake keres a CMAKE_PREFIX_PATH-on)
set(OGRE_SDK "" CACHE PATH "Ogre install prefix (optional; empty = auto via find_package)")
# assoc + minta anyagok: Ogre forrás Samples/Media (models/ogrehead.mesh stb.)
set(OGRE_SAMPLES_MEDIA "" CACHE PATH "Path to Ogre Samples/Media (models/, materials/)")

if(OGRE_SDK AND EXISTS "${OGRE_SDK}/CMake/OGREConfig.cmake")
  list(APPEND CMAKE_PREFIX_PATH "${OGRE_SDK}")
  set(OGRE_DIR "${OGRE_SDK}/CMake")
elseif(OGRE_SDK AND EXISTS "${OGRE_SDK}/lib/cmake/OGRE/OGREConfig.cmake")
  list(APPEND CMAKE_PREFIX_PATH "${OGRE_SDK}")
elseif(OGRE_SDK AND EXISTS "${OGRE_SDK}/lib/OGRE/cmake/OGREConfig.cmake")
  list(APPEND CMAKE_PREFIX_PATH "${OGRE_SDK}")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND CMAKE_PREFIX_PATH /usr /usr/local /opt/ogre-sdk)
endif()
list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/sdl2")

find_package(OGRE REQUIRED COMPONENTS Bites CONFIG)

if(NOT OGRE_SAMPLES_MEDIA)
  if(DEFINED ENV{OGRE_SAMPLES_MEDIA} AND NOT "$ENV{OGRE_SAMPLES_MEDIA}" STREQUAL "")
    set(OGRE_SAMPLES_MEDIA "$ENV{OGRE_SAMPLES_MEDIA}")
  endif()
endif()

# Arch / distro csomag: gyakran a teljes mintamédia az OGRE_MEDIA_DIR alatt (models/ogrehead.mesh).
if(NOT OGRE_SAMPLES_MEDIA)
  if(EXISTS "${OGRE_MEDIA_DIR}/models/ogrehead.mesh")
    set(OGRE_SAMPLES_MEDIA "${OGRE_MEDIA_DIR}")
  endif()
endif()

if(NOT OGRE_SAMPLES_MEDIA)
  find_path(OGRE_SAMPLES_MEDIA
    NAMES models/ogrehead.mesh
    PATH_SUFFIXES Samples/Media Share/OGRE/samples/Media
    PATHS
      "${CMAKE_SOURCE_DIR}/../ogre"
      "${CMAKE_SOURCE_DIR}/../ogre-src"
      /opt/ogre-src
      "$ENV{HOME}/src/ogre"
      "$ENV{HOME}/ogre"
  )
endif()

# find_path sikertelen
if(OGRE_SAMPLES_MEDIA AND OGRE_SAMPLES_MEDIA MATCHES "NOTFOUND")
  set(OGRE_SAMPLES_MEDIA "")
endif()

# assoc demóhoz kell teljes Samples/Media (ogrehead.mesh). meadow / bamboo_gap csak Essential-ből él — cmake mindig lefut.
set(OGRE_GENERAL_MEDIA "${OGRE_SAMPLES_MEDIA}")
if(NOT OGRE_GENERAL_MEDIA)
  set(OGRE_GENERAL_MEDIA "${OGRE_MEDIA_DIR}")
endif()

set(ASSOC_CAN_BUILD OFF)
if(OGRE_SAMPLES_MEDIA AND EXISTS "${OGRE_SAMPLES_MEDIA}/models/ogrehead.mesh")
  set(ASSOC_CAN_BUILD ON)
elseif(NOT OGRE_SAMPLES_MEDIA AND EXISTS "${OGRE_MEDIA_DIR}/models/ogrehead.mesh")
  set(ASSOC_CAN_BUILD ON)
  set(OGRE_SAMPLES_MEDIA "${OGRE_MEDIA_DIR}")
endif()

if(NOT ASSOC_CAN_BUILD)
  message(STATUS "assoc kihagyva: nincs models/ogrehead.mesh — állítsd -DOGRE_SAMPLES_MEDIA=... vagy telepítsd az Ogre Samples/Media-t.")
endif()

# configure_file számára (OGREConfig.cmake-ből)
if(NOT DEFINED OGRE_MEDIA_DIR)
  message(FATAL_ERROR "OGRE_MEDIA_DIR nincs az OGRE CMake csomagban (régi Ogre?)")
endif()

# Konfig: plugin + erőforrás útvonalak (abszolút, így a build könyvtárból is indul)
configure_file(
  "${CMAKE_SOURCE_DIR}/cfg/plugins.cfg.in"
  "${CMAKE_BINARY_DIR}/plugins.cfg" @ONLY)
configure_file(
  "${CMAKE_SOURCE_DIR}/cfg/resources.cfg.in"
  "${CMAKE_BINARY_DIR}/resources.cfg" @ONLY)

# Első indításhoz kell (ne a rendszer config dialógus blokkolja a fejlesztést)
file(COPY "${CMAKE_SOURCE_DIR}/cfg/ogre.cfg" DESTINATION "${CMAKE_BINARY_DIR}")

set(_ogre_rpath "${OGRE_LIBRARY_DIRS}")
if(OGRE_PLUGIN_DIR)
  list(APPEND _ogre_rpath "${OGRE_PLUGIN_DIR}")
endif()

if(ASSOC_CAN_BUILD)
  add_executable(assoc
    src/AssocApp.cpp)

  target_link_libraries(assoc PRIVATE OgreBites)
  target_include_directories(assoc PRIVATE ${OGRE_INCLUDE_DIRS})

  set_target_properties(assoc PROPERTIES
    BUILD_RPATH "${_ogre_rpath}"
    BUILD_WITH_INSTALL_RPATH FALSE)
endif()

add_executable(meadow
  src/MeadowApp.cpp)

target_link_libraries(meadow PRIVATE OgreBites)
target_include_directories(meadow PRIVATE ${OGRE_INCLUDE_DIRS})

set_target_properties(meadow PROPERTIES
  BUILD_RPATH "${_ogre_rpath}"
  BUILD_WITH_INSTALL_RPATH FALSE)

add_executable(bamboo_gap
  src/BambooGapApp.cpp)

target_link_libraries(bamboo_gap PRIVATE OgreBites)
target_include_directories(bamboo_gap PRIVATE ${OGRE_INCLUDE_DIRS})

set_target_properties(bamboo_gap PROPERTIES
  BUILD_RPATH "${_ogre_rpath}"
  BUILD_WITH_INSTALL_RPATH FALSE)
