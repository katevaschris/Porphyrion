VERSION = v0.0.1
CC = gcc
CFLAGS  = -Wall -Wextra -Wshadow -Wformat=2 -Wformat-security \
           -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
           -Os -Iinclude -DPORPHYRION_VERSION=\"$(VERSION)\"
LDFLAGS = -lcurl -s

TARGET = porter
SRC = src/main.c src/decoder.c src/proxy_networking.c src/http_response.c src/call_api.c src/router.c
IMAGE_NAME = porphyrion
PORT = 8099

OBJ_DIR ?= obj
OBJ = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)

docker-run:
	podman build --target builder -t $(IMAGE_NAME)-builder .
	podman build -t $(IMAGE_NAME) .
	podman run --rm -it -p $(PORT):$(PORT) $(IMAGE_NAME)
	podman image prune -f

docker-build:
	podman build --target builder -t $(IMAGE_NAME)-builder .
	podman build -t $(IMAGE_NAME) .

docker-clean:
	podman rmi $(IMAGE_NAME) || true
	podman image prune -f

sync-version:
	@echo "Syncing version $(VERSION)..."
	@sed -i '' 's/<span class="hdr-version">.*<\/span>/<span class="hdr-version">$(VERSION)<\/span>/' index.html
	@sed -i '' 's/<sub>.*<\/sub>/<sub>$(VERSION)<\/sub>/' README.md
