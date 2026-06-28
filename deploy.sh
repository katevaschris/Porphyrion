#!/bin/bash
set -eu

IMAGE_NAME="porphyrion"
PORT=8099

if command -v podman >/dev/null 2>&1; then
    ENGINE="podman"
elif command -v docker >/dev/null 2>&1; then
    ENGINE="docker"
else
    echo "Error: neither podman nor docker is installed." >&2
    exit 1
fi

START_TIME=$(date +%s)
"$ENGINE" rm -f "$IMAGE_NAME" >/dev/null 2>&1 || true

echo "Building image with $ENGINE..."
"$ENGINE" build -t "$IMAGE_NAME" .

DURATION=$(( $(date +%s) - START_TIME ))
SIZE=$("$ENGINE" images --format '{{.Size}}' "$IMAGE_NAME" 2>/dev/null | head -n 1 || true)
VERSION=$(grep '^VERSION =' Makefile | cut -d '=' -f 2 | xargs)

echo "------------------------------------"
echo "  Porphyrion $VERSION ($ENGINE)"
echo "  Build time: ${DURATION}s"
echo "  Image size: ${SIZE}"
echo "  Running on http://localhost:$PORT"
echo "------------------------------------"

"$ENGINE" run --name "$IMAGE_NAME" --rm -d -p "$PORT:$PORT" -v "$(pwd)/data:/data" -v "$(pwd)/web:/web:ro" "$IMAGE_NAME"
