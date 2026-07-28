#!/usr/bin/env bash
#
# gh-deploy.sh — build the WebAssembly game and publish it to GitHub Pages.
#
# Compiles snake.c to wasm, verifies the output, then force-pushes docs/ to the
# `gh-pages` branch as a single orphan commit (the branch is a build artifact,
# not history worth keeping). Pages then serves it at:
#
#   https://<your-user>.github.io/classic-snake/
#
# The repo is created on GitHub if it does not exist yet, and Pages is pointed
# at the gh-pages branch — both via the `gh` CLI, so a first deploy needs no
# clicking around in Settings.
#
# Usage:
#   ./gh-deploy.sh                  # dry run: build and verify, push nothing
#   ./gh-deploy.sh --push           # build, verify and publish
#   ./gh-deploy.sh --push --private # create the repo private (default: public)
#
# Overridable via env:
#   GH_BRANCH   branch Pages serves from        (default: gh-pages)
#   GH_REPO     repo name to create/push to     (default: classic-snake)
#   GH_REMOTE   full remote URL, skips creation (default: derived from gh)
#
# Requires: emsdk on PATH (source ~/emsdk/emsdk_env.sh), gh, git.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$REPO_ROOT"

GH_BRANCH="${GH_BRANCH:-gh-pages}"
GH_REPO="${GH_REPO:-classic-snake}"
DOCS_DIR="$REPO_ROOT/docs"
PUSH=0
VISIBILITY="--public"

if [[ -t 1 ]]; then
  BOLD=$'\033[1m'; DIM=$'\033[2m'; RED=$'\033[31m'; YEL=$'\033[33m'; RESET=$'\033[0m'
else
  BOLD=""; DIM=""; RED=""; YEL=""; RESET=""
fi
step() { printf '%s==>%s %s\n' "$BOLD" "$RESET" "$*"; }
info() { printf '    %s%s%s\n' "$DIM" "$*" "$RESET"; }
warn() { printf '%swarning:%s %s\n' "$YEL" "$RESET" "$*"; }
die()  { printf '%serror:%s %s\n' "$RED" "$RESET" "$*" >&2; exit 1; }

while [[ $# -gt 0 ]]; do
  case "$1" in
    --push)     PUSH=1; shift ;;
    --private)  VISIBILITY="--private"; shift ;;
    --branch)   GH_BRANCH="${2:?--branch needs a value}"; shift 2 ;;
    -h|--help)  sed -n '3,25p' "${BASH_SOURCE[0]}" | sed 's|^# \{0,1\}||'; exit 0 ;;
    *)          die "unknown option: $1 (try --help)" ;;
  esac
done

# --- preconditions ----------------------------------------------------------

command -v git >/dev/null || die "git not found on PATH"
git rev-parse --is-inside-work-tree >/dev/null 2>&1 || die "not a git repository"

command -v emcc >/dev/null \
  || die "emcc not found — run: source ~/emsdk/emsdk_env.sh"

if (( PUSH )); then
  command -v gh >/dev/null || die "gh not found on PATH"
  gh auth status >/dev/null 2>&1 \
    || die "not logged in to GitHub — run: gh auth login"

  # Publishing a tree that does not match a commit makes the deployed site
  # impossible to trace back to a revision.
  [[ -z "$(git status --porcelain)" ]] \
    || die "working tree is dirty — commit or stash first (deploys must be traceable)"
fi

# --- build ------------------------------------------------------------------

step "Building WebAssembly bundle"
make clean-web >/dev/null 2>&1 || true
make web || die "wasm build failed — not deploying"

[[ -f "$DOCS_DIR/index.html" ]] || die "build produced no docs/index.html"

# The build is only useful if the wasm really got inlined; a --shell-file typo
# yields a valid-looking HTML file with no game in it.
#
# Every grep here needs -a. With -sSINGLE_FILE, emscripten 6 embeds the wasm
# as a raw binary string (not base64), so the file contains NUL bytes and grep
# otherwise decides it is binary and silently reports no matches.
#
# Two embedding styles are accepted: binaryDecode() from emscripten 6, and the
# base64 data URI older versions emit.
grep -aq -e 'binaryDecode(' -e 'data:application/octet-stream;base64' \
     "$DOCS_DIR/index.html" \
  || die "docs/index.html has no inlined wasm — check -sSINGLE_FILE"
grep -aq 'curses_push_key' "$DOCS_DIR/index.html" \
  || die "docs/index.html is missing the input export — check -sEXPORTED_FUNCTIONS"
grep -aq 'renderScreen' "$DOCS_DIR/index.html" \
  || die "docs/index.html is missing the renderer — check --shell-file"

# The whole point of SINGLE_FILE is that nothing else has to be served.
if compgen -G "$DOCS_DIR/*.wasm" >/dev/null; then
  die "a separate .wasm was emitted — SINGLE_FILE did not take effect"
fi

# Raw binary embedded in the page is only safe if the file is still valid
# UTF-8, since Pages serves it as text/html;charset=utf-8 and any invalid
# sequence would be replaced with U+FFFD, corrupting the module.
python3 -c "open('$DOCS_DIR/index.html','rb').read().decode('utf-8')" 2>/dev/null \
  || die "docs/index.html is not valid UTF-8 — serving it would corrupt the wasm"

SIZE="$(du -h "$DOCS_DIR/index.html" | cut -f1)"
info "docs/index.html — $SIZE, self-contained"

# Pages runs Jekyll by default, which silently drops files starting with '_'.
touch "$DOCS_DIR/.nojekyll"

# --- dry run ----------------------------------------------------------------

if (( ! PUSH )); then
  echo
  step "Dry run — built and verified, nothing pushed"
  info "preview locally:  make web-serve   (then open http://localhost:8000)"
  info "publish for real: $0 --push"
  exit 0
fi

# --- make sure the repo exists ----------------------------------------------

GH_USER="$(gh api user --jq .login)"
[[ -n "$GH_USER" ]] || die "could not determine your GitHub username"

if [[ -n "${GH_REMOTE:-}" ]]; then
  REMOTE_URL="$GH_REMOTE"
elif git remote get-url origin >/dev/null 2>&1; then
  REMOTE_URL="$(git remote get-url origin)"
  info "using existing origin: $REMOTE_URL"
else
  step "Creating $VISIBILITY repo ${GH_USER}/${GH_REPO}"
  gh repo create "$GH_REPO" $VISIBILITY \
     --source=. --remote=origin \
     --description="Terminal Snake in C + ncurses, also compiled to WebAssembly" \
     || die "gh repo create failed"
  REMOTE_URL="$(git remote get-url origin)"
fi

# Push the source branch too, so gh-pages is traceable to a real commit.
CURRENT_BRANCH="$(git rev-parse --abbrev-ref HEAD)"
step "Pushing $CURRENT_BRANCH to origin"
git push -u origin "$CURRENT_BRANCH"

# --- publish docs/ to gh-pages ----------------------------------------------

SOURCE_SHA="$(git rev-parse --short HEAD)"

step "Publishing docs/ to origin/$GH_BRANCH"
# Build a throwaway single-commit repo inside docs/ and force-push it, rather
# than carrying the generated bundle in main's history.
trap 'rm -rf "$DOCS_DIR/.git"' EXIT

rm -rf "$DOCS_DIR/.git"
git -C "$DOCS_DIR" init -q -b "$GH_BRANCH"
git -C "$DOCS_DIR" add -A
git -C "$DOCS_DIR" \
    -c user.name="$(git config user.name)" \
    -c user.email="$(git config user.email)" \
    commit -q -m "Deploy wasm build from ${SOURCE_SHA}"
git -C "$DOCS_DIR" push -q --force "$REMOTE_URL" "$GH_BRANCH" \
  || die "push to $GH_BRANCH failed"
rm -rf "$DOCS_DIR/.git"

# --- point Pages at the branch ----------------------------------------------

step "Configuring GitHub Pages"
if gh api "repos/${GH_USER}/${GH_REPO}/pages" >/dev/null 2>&1; then
  gh api -X PUT "repos/${GH_USER}/${GH_REPO}/pages" \
     -f "source[branch]=${GH_BRANCH}" -f "source[path]=/" >/dev/null \
     && info "Pages already enabled, source set to ${GH_BRANCH}" \
     || warn "could not update Pages config — set it in Settings → Pages"
else
  gh api -X POST "repos/${GH_USER}/${GH_REPO}/pages" \
     -f "source[branch]=${GH_BRANCH}" -f "source[path]=/" >/dev/null \
     && info "Pages enabled on ${GH_BRANCH}" \
     || warn "could not enable Pages — set it in Settings → Pages (branch: ${GH_BRANCH}, folder: /)"
fi

LIVE_URL="https://${GH_USER}.github.io/${GH_REPO}/"

# The README ships with a USERNAME placeholder in the play link; nag until it
# is replaced, otherwise the badge on the front page points nowhere.
if grep -q 'USERNAME.github.io' "$REPO_ROOT/README.md" 2>/dev/null; then
  warn "README.md still has the USERNAME placeholder in its play link"
  info "fix it with:  sed -i 's|USERNAME|${GH_USER}|g' README.md && git commit -am 'Set Pages URL'"
fi

step "Done"
info "source commit: ${SOURCE_SHA}"
info "live shortly:  ${LIVE_URL}"
info "first deploy can take a minute or two to go live"
