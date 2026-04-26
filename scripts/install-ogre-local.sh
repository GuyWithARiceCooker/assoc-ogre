#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
deps_dir="${repo_dir}/.deps"
src_dir="${deps_dir}/ogre-src"
build_dir="${deps_dir}/ogre-build"
prefix="${OGRE_SDK:-${deps_dir}/ogre-14}"
version="${OGRE_VERSION:-v14.3.4}"

mkdir -p "${deps_dir}"

if [[ ! -d "${src_dir}/.git" ]]; then
  git clone --depth 1 --branch "${version}" https://github.com/OGRECave/ogre.git "${src_dir}"
else
  git -C "${src_dir}" fetch --depth 1 origin "refs/tags/${version}:refs/tags/${version}" || true
  git -C "${src_dir}" checkout "${version}"
fi

if [[ -z "${CC:-}" ]] && command -v gcc >/dev/null 2>&1; then
  export CC=gcc
fi
if [[ -z "${CXX:-}" ]] && command -v g++ >/dev/null 2>&1; then
  export CXX=g++
fi

cmake -S "${src_dir}" -B "${build_dir}" -G "${CMAKE_GENERATOR:-Ninja}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${prefix}" \
  -DOGRE_BUILD_SAMPLES=OFF \
  -DOGRE_BUILD_TESTS=OFF \
  -DOGRE_BUILD_TOOLS=OFF \
  -DOGRE_INSTALL_SAMPLES=OFF \
  -DOGRE_INSTALL_TOOLS=OFF \
  -DOGRE_BUILD_COMPONENT_BITES=ON \
  -DOGRE_BUILD_RENDERSYSTEM_GL3PLUS=ON \
  -DOGRE_BUILD_RENDERSYSTEM_GL=OFF \
  -DOGRE_BUILD_RENDERSYSTEM_GLES2=OFF \
  -DOGRE_BUILD_PLUGIN_BSP=OFF \
  -DOGRE_BUILD_PLUGIN_PCZ=OFF \
  -DOGRE_BUILD_PLUGIN_PFX=OFF \
  -DOGRE_BUILD_PLUGIN_DOT_SCENE=OFF \
  -DOGRE_BUILD_PLUGIN_OCTREE=ON \
  -DOGRE_BUILD_CODEC_STBI=ON \
  -DOGRE_CONFIG_ENABLE_ZIP=ON

cmake --build "${build_dir}" --target install --parallel "${JOBS:-2}"

echo
echo "Ogre kesz: ${prefix}"
echo "Build:"
echo "  OGRE_PLUGIN_DIR=\"${prefix}/lib/OGRE\" ./scripts/build-arch.sh \"${prefix}\""
