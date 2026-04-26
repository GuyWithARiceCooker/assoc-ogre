#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${repo_dir}/build"

: "${OGRE_SDK:=${1:-/usr}}"
: "${OGRE_PLUGIN_DIR:=}"

if [[ -z "${CXX:-}" ]] && command -v g++ >/dev/null 2>&1; then
  export CXX=g++
fi

if [[ -z "${OGRE_PLUGIN_DIR}" ]]; then
  for candidate in \
    "${OGRE_SDK}/lib/OGRE" \
    "${OGRE_SDK}/lib64/OGRE" \
    "${OGRE_SDK}/lib/OGRE-14" \
    "${OGRE_SDK}/lib64/OGRE-14"; do
    if [[ -d "${candidate}" ]]; then
      OGRE_PLUGIN_DIR="${candidate}"
      break
    fi
  done
fi

cmake_args=(
  -S "${repo_dir}"
  -B "${build_dir}"
  -DOGRE_SDK="${OGRE_SDK}"
)

if [[ -n "${OGRE_PLUGIN_DIR}" ]]; then
  cmake_args+=(-DOGRE_PLUGIN_DIR="${OGRE_PLUGIN_DIR}")
fi

cmake "${cmake_args[@]}"
cmake --build "${build_dir}" --parallel "${JOBS:-2}"

echo
echo "Kesz. Futtatas:"
echo "  cd \"${build_dir}\" && ./assoc"
