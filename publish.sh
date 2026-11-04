#!/usr/bin/env bash
set -euo pipefail

OWNER="NoRancidIntentions"
REPO="ising-model-simulation"
VISIBILITY="${1:-private}"

case "$VISIBILITY" in
  private) VISIBILITY_FLAG="--private" ;;
  public)  VISIBILITY_FLAG="--public" ;;
  *) echo "Usage: ./publish.sh [private|public]" >&2; exit 2 ;;
esac

command -v gh >/dev/null || { echo "GitHub CLI (gh) is required." >&2; exit 1; }
gh auth status >/dev/null || { echo "Run 'gh auth login' first." >&2; exit 1; }
git rev-parse --is-inside-work-tree >/dev/null 2>&1 || git init -b main
if ! git config user.name >/dev/null || ! git config user.email >/dev/null; then
  echo "Set your Git commit identity first:" >&2
  echo "  git config --global user.name \"Your Name\"" >&2
  echo "  git config --global user.email \"your-github-email\"" >&2
  exit 1
fi
git add -A
if ! git rev-parse --verify HEAD >/dev/null 2>&1; then
  git commit -m "Initial project commit"
fi
gh repo create "${OWNER}/${REPO}" "$VISIBILITY_FLAG" --source=. --remote=origin --push
