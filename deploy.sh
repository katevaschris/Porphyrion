#!/bin/bash
set -e

IMAGE_NAME="porphyrion"
PORT=8099

START_TIME=$(date +%s)
podman rm -t 0 -f "$IMAGE_NAME" > /dev/null 2>&1 || true

echo "→ Building image..."
podman build -t "$IMAGE_NAME" .

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))
SIZE=$(podman images --format "{{.Size}}" "$IMAGE_NAME" | head -n 1)

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
VERSION=$(grep "^VERSION =" Makefile | cut -d "=" -f 2 | xargs)
echo "  Porphyrion $VERSION"
echo "  Build Time: ${DURATION}s"
echo "  Image Size: ${SIZE}"
echo "  Running on http://localhost:$PORT"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

podman run --name "$IMAGE_NAME" --rm -d -p "$PORT:$PORT" -v "$(pwd)/data:/data" "$IMAGE_NAME"


