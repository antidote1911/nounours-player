#!/usr/bin/env bash
set -euo pipefail

BOLD='\033[1m'; GREEN='\033[0;32m'; RED='\033[0;31m'; CYAN='\033[0;36m'; RESET='\033[0m'
info()    { echo -e "${CYAN}ℹ ${RESET}$*"; }
success() { echo -e "${GREEN}✔ ${RESET}$*"; }
error()   { echo -e "${RED}✖ ${RESET}$*" >&2; }

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT/build"
CORES=$(nproc 2>/dev/null || echo 2)

echo -e "\n${BOLD}── Configuration CMake ──────────────────────────────────────${RESET}"
info "Source : $ROOT/src"
info "Build  : $BUILD_DIR"
cmake -S "$ROOT/src" -B "$BUILD_DIR" \
    -DNOUNOURS_LANG_PATH="$BUILD_DIR" \
    || { error "Configuration échouée."; exit 1; }
success "Configuration OK."

echo -e "\n${BOLD}── Compilation (${CORES} cœurs) ─────────────────────────────────${RESET}"
cmake --build "$BUILD_DIR" -j"$CORES" || { error "Compilation échouée."; exit 1; }
success "Compilation OK."

BINARY="$BUILD_DIR/nounours-player"
if [[ ! -x "$BINARY" ]]; then
    error "Binaire introuvable : $BINARY"
    exit 1
fi

echo -e "\n${BOLD}── Lancement ────────────────────────────────────────────────${RESET}"
info "Exécution : $BINARY"
exec "$BINARY"
