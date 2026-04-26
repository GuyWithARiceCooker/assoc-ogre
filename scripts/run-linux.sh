#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${repo_dir}/build"

: "${OGRE_SDK:=${1:-${repo_dir}/.deps/ogre-14}}"

if [[ ! -x "${build_dir}/assoc" ]]; then
  echo "Nincs build/assoc. Elobb futtasd:" >&2
  echo "  ./scripts/build-arch.sh \"${OGRE_SDK}\"" >&2
  exit 1
fi

export LD_LIBRARY_PATH="${OGRE_SDK}/lib:${LD_LIBRARY_PATH:-}"
cd "${build_dir}"
exec ./assoc "$@"
