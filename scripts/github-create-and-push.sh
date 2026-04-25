#!/usr/bin/env bash
# GitHub: `gh` bejelentkezés után üres távoli repó + `origin` + push. Ha már van `origin`, csak push.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

: "${REPO_NAME:=assoc-ogre}" # env: távoli repó neve (alap: assoc-ogre)
# Ha szervezet alá kell: export GITHUB_ORG=GuyWithARiceCooker  → gh repo create ORG/REPO
: "${GITHUB_ORG:=}" # üres: saját fiók

if ! command -v gh &>/dev/null; then
  echo "Nincs \`gh\` (brew install gh)." >&2
  exit 1
fi
if ! gh auth status &>/dev/null; then
  echo "Nincs GitHub bejelentkezés. Futtass egy helyi terminálban: gh auth login" >&2
  echo "PAT-vel: echo \"\$GITHUB_TOKEN\" | gh auth login --with-token" >&2
  exit 1
fi

if git remote get-url origin &>/dev/null; then
  echo "Van már origin: $(git remote get-url origin) — push main."
  git push -u origin main
  exit 0
fi

if [[ -n "$GITHUB_ORG" ]]; then
  REMOTE_ID="${GITHUB_ORG}/${REPO_NAME}"
else
  REMOTE_ID="${REPO_NAME}"
fi

gh repo create "$REMOTE_ID" --public --source="$ROOT" --remote=origin --push

echo "Kész: távoli repó \`${REMOTE_ID}\` és push a main-ra."
echo "Cím: https://github.com/${REMOTE_ID}"
