# Porphyrion <img width="45" height="45" alt="hat" src="hat.png" /> <sub>v0.0.1</sub>

Porphyrion is a lightweight HTTP API testing client designed to run inside a Podman container with a minimal memory footprint. It proxies outbound requests through a C backend to eliminate browser CORS restrictions, and provides a dynamic interface for organizing and executing API calls.

---

## Architecture

```
Porphyrion/
├── src/                         # C translation units
│   ├── main.c                   # Server lifecycle: socket, bind, listen, accept
│   ├── router.c                 # Request dispatch and client connection ownership
│   ├── call_api.c               # Outbound HTTP proxy via libcurl
│   ├── http_response.c          # Static file serving and text/JSON response helpers
│   ├── decoder.c                # URL percent-encoding and plus-to-space decoding
│   └── proxy_networking.c       # Localhost-to-host.containers.internal rewriting
│
├── include/                     # Public headers (one per translation unit)
│   ├── router.h
│   ├── call_api.h
│   ├── http_response.h
│   ├── decoder.h
│   └── proxy_networking.h
│
├── data/                        # Persisted request collection (host-mounted volume)
│   └── requests.json
│
├── index.html                   # Single-page frontend (HTML + JS, no framework)
├── hat.png                      # Application icon
├── Makefile                     # Build configuration with dynamic versioning
├── Dockerfile                   # Two-stage image: build on Alpine, run on scratch
└── deploy.sh                    # Build, run, and cleanup automation script
```

### Request Lifecycle

```
Browser (index.html)
        |
        | POST /proxy  (form-encoded: url, method, body, ct, auth)
        v
  router.c :: handle_client()          <-- sole owner of socket lifecycle
        |
        | route_request()
        |
        +---> GET /           --> http_response.c :: serve_file()
        +---> GET /hat.png    --> http_response.c :: serve_file()
        +---> GET /requests.json --> http_response.c :: serve_file() or send_text("[]")
        +---> POST /save       --> write body to data/requests.json
        +---> OPTIONS          --> CORS preflight (204)
        +---> POST /proxy
                |
                | decoder.c :: url_decode()
                | proxy_networking.c :: resolve_url()
                v
          call_api.c :: perform_api_call()
                |
                | libcurl (outbound HTTP)
                v
          Target API Server
                |
                v
          HTTP response forwarded back to browser
```

---

## Features

- Deployment: Zero-OS `scratch` runtime with high-speed incremental builds.
- Dynamic request builder: GET, POST, PUT, DELETE, PATCH.
- HTTP Status Recognition: 2xx, 3xx, 4xx, 500.
- Body types: No Body, Form Encoded (key-value grid), JSON, XML, Text.
- Bearer Token authentication with manual token input.
- Nested folder organization for saved requests using slash-separated paths (e.g. `Services/Auth/Tokens`).
- Request persistence to `data/requests.json` via a host-mounted Podman volume.
- Resident memory (VmRSS) logged to stdout at startup and after each request via `/proc/self/status`.

---

## Build Configuration

```makefile
VERSION = v0.0.1
CFLAGS  = -Wall -Wextra -Wshadow -Wformat=2 -Wformat-security \
           -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
           -Os -Iinclude -DPORPHYRION_VERSION=\"$(VERSION)\"
LDFLAGS = -lcurl -s
```

| Flag | Purpose |
|------|---------|
| `-Os` | Optimize for binary size |
| `-s` | Strip debug symbols from the final executable |
| `-Wshadow` | Detect variables that shadow an outer-scope declaration |
| `-Wformat=2 -Wformat-security` | Catch unsafe printf/snprintf format strings |
| `-fstack-protector-strong` | Insert stack canaries to detect stack smashing |
| `-D_FORTIFY_SOURCE=2` | Runtime bounds checking on standard library calls |


---

## Deployment

### Prerequisites

- [Podman](https://podman.io/docs/installation)

### Build and Run

```bash
./deploy.sh
```

The script:
1. Instantly force-stops and removes any running Porphyrion containers (`podman rm -t 0 -f`).
2. Rebuilds the `localhost/porphyrion:latest` image using a two-stage Dockerfile. Uses `.dockerignore` and BuildKit Cache Mounts (`--mount=type=cache`).
3. Starts the container in detached mode on port `8099`.
4. Mounts `./data` to `/data` to persist saved requests across restarts.

Access the application at **http://localhost:8099**.

### Dockerfile — Two-Stage Build

| Stage | Base | Purpose |
|-------|------|---------|
| builder | `alpine:3.18` | Installs gcc, upx, static library packages. Compiles a compressed, fully static binary. |
| runtime | `scratch` | Zero OS. Contains only the stripped/compressed binary, CA certificates, and static HTML file. |

The binary is statically linked against libcurl, OpenSSL, nghttp2, brotli, zlib,
libidn2, libunistring, and libpsl, and then compressed using UPX. The runtime image has no operating system, no
shell, and no shared libraries. Total image size: **1.94 MB**.

sub-gnome, subatomic build latency: ~2.3s. Output footprint: 1.94 MB.
Resident memory footprint: ~5 MB.

---

## Data Persistence

All saved requests are stored in `./data/requests.json` on the host machine.
This directory is bind-mounted into the container at `/data`, so requests
survive image rebuilds and container restarts.

Because the file is on the host, it can be committed to version control.
Avoid committing Bearer Tokens or credentials; use `.gitignore` accordingly.
