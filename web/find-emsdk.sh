# Sourced, never executed. Puts emcc on PATH by activating an emsdk install
# from the usual locations, so `make web` and ./gh-deploy.sh both work in a
# fresh shell without the caller remembering to source emsdk_env.sh.
#
#   . web/find-emsdk.sh
#
# Does nothing if emcc is already available. Silent on success; the caller
# reports the failure, because what to do about it differs per caller.
#
# Set EMSDK to point at an install somewhere unusual.

if ! command -v emcc >/dev/null 2>&1; then
  for _emsdk_dir in "${EMSDK:-}" "$HOME/emsdk" /opt/emsdk /usr/local/emsdk; do
    [ -n "$_emsdk_dir" ] || continue
    [ -f "$_emsdk_dir/emsdk_env.sh" ] || continue

    # emsdk_env.sh reads unset variables and can return non-zero even when it
    # worked, so a caller running under `set -eu` would die on the source.
    # Save the shell flags, relax them, then put them back exactly as found.
    _emsdk_flags="$-"
    set +eu
    # shellcheck disable=SC1090,SC1091
    . "$_emsdk_dir/emsdk_env.sh" >/dev/null 2>&1
    case "$_emsdk_flags" in *e*) set -e ;; esac
    case "$_emsdk_flags" in *u*) set -u ;; esac

    if command -v emcc >/dev/null 2>&1; then
      EMSDK_FOUND_IN="$_emsdk_dir"
      export EMSDK_FOUND_IN
      break
    fi
  done
  unset _emsdk_dir _emsdk_flags
fi
