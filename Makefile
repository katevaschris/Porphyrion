VERSION = v0.3
CC      = gcc

CFLAGS  = -std=c11 -Os -Iinclude -DPORPHYRION_VERSION=\"$(VERSION)\"
CFLAGS += -Wcast-align -Wcast-qual -Wconversion -Wdouble-promotion \
          -Wduplicated-branches -Wduplicated-cond -Werror -Wextra \
          -Wformat=2 -Wformat-security -Wformat-signedness \
          -Wjump-misses-init -Wlogical-op -Wall -Wmissing-prototypes \
          -Wnested-externs -Wnull-dereference -Wold-style-definition \
          -Wpedantic -Wpointer-arith -Wredundant-decls -Wshadow \
          -Wsign-conversion -Wstack-usage=8192 -Wstrict-overflow=2 \
          -Wstrict-prototypes -Wundef -Wwrite-strings
CFLAGS += -D_FORTIFY_SOURCE=2 -fstack-protector-strong

LDFLAGS = -lcurl -s

TARGET     = porter
SRC        = src/call_api.c \
             src/decoder.c \
             src/http_parser.c \
             src/http_response.c \
             src/main.c \
             src/proxy_networking.c \
             src/router.c

IMAGE_NAME = porphyrion
PORT       = 8099
OBJ_DIR   ?= obj
OBJ        = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRC))

.PHONY: all clean docker-build docker-clean docker-run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

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
