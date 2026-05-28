#!/bin/bash
# Build get_abogus executable
# Usage: ./build.sh [target]
#   target: macos-arm64 (default) | macos-x64 | linux | win | all

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

TARGET="${1:-macos-arm64}"

echo "==> Installing dependencies..."
npm install

echo "==> Building for $TARGET..."

case "$TARGET" in
    macos-arm64)
        npm run build:macos-arm64
        ;;
    macos-x64)
        npm run build:macos-x64
        ;;
    linux)
        npm run build:linux
        ;;
    win)
        npm run build:win
        ;;
    all)
        npm run build
        mv dist/* . 2>/dev/null || true
        rmdir dist 2>/dev/null || true
        ;;
    *)
        echo "Unknown target: $TARGET"
        echo "Usage: $0 [macos-arm64|macos-x64|linux|win|all]"
        exit 1
        ;;
esac

# Cleanup
rm -f bundle.js

echo "==> Build complete!"
ls -lh get_abogus* 2>/dev/null || ls -lh dist/
