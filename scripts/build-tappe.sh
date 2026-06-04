#!/usr/bin/env bash
# build-tappe.sh
# Compila tutte le tappe tappa-* trovate nel repository.
# SFML e FreeType vengono scaricati da CMake tramite FetchContent.
#
# Uso: ./scripts/build-tappe.sh [--build-root <dir>] [--config <Debug|Release>]
#
# Opzioni:
#   --build-root <dir>  Directory in cui creare le cartelle di build (default: build/tappe)
#   --config <cfg>      Configurazione CMake (default: Release)

set -euo pipefail

BUILD_ROOT="build/tappe"
SOURCE_ROOT="build/tappe-source"
CONFIG="Release"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-root) BUILD_ROOT="$2"; shift 2 ;;
        --config)     CONFIG="$2";     shift 2 ;;
        *) echo "Opzione sconosciuta: $1" >&2; exit 1 ;;
    esac
done

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_ROOT_ABS="$REPO_ROOT/$BUILD_ROOT"
SOURCE_ROOT_ABS="$REPO_ROOT/$SOURCE_ROOT"

mkdir -p "$BUILD_ROOT_ABS"
mkdir -p "$SOURCE_ROOT_ABS"

TAGS=$(git -C "$REPO_ROOT" tag --list "tappa-*" --sort=version:refname)

if [[ -z "$TAGS" ]]; then
    echo "Errore: nessun tag tappa-* trovato nel repository." >&2
    exit 1
fi

SUCCESS=0
FAILURE=0

for TAG in $TAGS; do
    echo ""
    echo "==> Build $TAG"

    TAG_SOURCE="$SOURCE_ROOT_ABS/$TAG"
    TAG_BUILD="$BUILD_ROOT_ABS/$TAG"

    rm -rf "$TAG_SOURCE"
    mkdir -p "$TAG_SOURCE"

    ARCHIVE="$SOURCE_ROOT_ABS/$TAG.tar"
    git -C "$REPO_ROOT" archive --format=tar --output="$ARCHIVE" "$TAG"
    tar -xf "$ARCHIVE" -C "$TAG_SOURCE"
    rm -f "$ARCHIVE"

    if [[ ! -f "$TAG_SOURCE/CMakeLists.txt" ]]; then
        echo "  Avviso: CMakeLists.txt non trovato per $TAG, salto."
        continue
    fi

    if cmake -S "$TAG_SOURCE" -B "$TAG_BUILD" \
        -DCMAKE_BUILD_TYPE="$CONFIG" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.21 2>&1; then
        if cmake --build "$TAG_BUILD" --config "$CONFIG" 2>&1; then
            echo "  OK: $TAG compilata con successo."
            SUCCESS=$((SUCCESS + 1))
        else
            echo "  ERRORE: build fallita per $TAG." >&2
            FAILURE=$((FAILURE + 1))
        fi
    else
        echo "  ERRORE: configure fallita per $TAG." >&2
        FAILURE=$((FAILURE + 1))
    fi
done

echo ""
echo "Risultato: $SUCCESS successi, $FAILURE errori."
[[ $FAILURE -eq 0 ]]
