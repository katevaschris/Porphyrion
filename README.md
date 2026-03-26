# Porphyrion <img width="45" height="45" alt="hat" src="resources/hat.png" /> <sub>v0.3</sub>

API client that runs as a single static binary inside a scratch container. A zero-allocation C backend proxies requests via libcurl to bypass CORS, serves the frontend, and persists data to a JSON file.

## What it does

- **HTTP methods**: GET, POST, PUT, DELETE, PATCH
- **Body types**: Form Encoded (key-value grid), JSON, XML, Text, GraphQL (split-pane Query + Variables), TOON (`text/toon`)
- **Authentication**: Bearer Token, Basic Auth, API Key (header/query param), and Hierarchical Auth scoped via Folder-level security contexts.
- **Environment Management**: Dynamic workspace configurations for base URLs and tokens
- **Settings Control**: Global CORS bypass toggling
- **cURL export**: Copy the current request as a ready-to-paste curl command
- **Animated hats**

## Architecture

```text
Browser ──POST /proxy──▸ router.c ──▸ call_api.c ──▸ libcurl ──▸ Target API
                              │
                              ├── GET /              → web/index.html
                              ├── GET /css/style.css → web/css/style.css
                              ├── GET /js/app.js     → web/js/app.js
                              ├── POST /save         → write data/requests.json
                              └── GET /requests.json → read saved data
```

## Project structure

```text
src/
  main.c               socket, bind, listen, accept loop
  router.c             scalable array-driven routing & parameter parsing
  call_api.c           libcurl proxy with response header capture
  http_parser.c        zero-allocation HTTP/1.x parser for HttpRequest structs
  http_response.c      static file serving, strict payload boundary guards (413/400)
  decoder.c            URL percent-decode
  proxy_networking.c   localhost → host.containers.internal rewrite
include/               one header per translation unit
web/
  index.html           modular single-page frontend
  css/style.css        all styles, dark/light theme
  js/app.js            vanilla logic, GraphQL editors, hierarchical auth resolution
resources/
  hat.png              application icon
data/
  requests.json        persisted collections (host-mounted volume)
```

## Build

Requires [Podman](https://podman.io/docs/installation) or Docker.

```bash
./deploy.sh
```

Opens at **http://localhost:8099**.

The deploy script:
1. Stops any running Porphyrion container
2. Builds via two-stage Dockerfile (alpine:3.18 → scratch)
3. Starts on port 8099 with `./data` mounted for persistence

Incremental build: ~2s. Image size: 1.99 MB. Runtime RSS: ~5 MB.

## Compiler flags

```makefile
-std=c11 -Os -Wcast-align -Wcast-qual -Wconversion -Wdouble-promotion
-Wduplicated-branches -Wduplicated-cond -Werror -Wextra -Wformat=2
-Wformat-security -Wformat-signedness -Wjump-misses-init -Wlogical-op
-Wall -Wmissing-prototypes -Wnested-externs -Wnull-dereference
-Wold-style-definition -Wpedantic -Wpointer-arith -Wredundant-decls
-Wshadow -Wsign-conversion -Wstack-usage=8192 -Wstrict-overflow=2
-Wstrict-prototypes -Wundef -Wwrite-strings -fstack-protector-strong
-D_FORTIFY_SOURCE=2
```

## Data

All saved requests are stored in `./data/requests.json` on the host machine. This directory is bind-mounted into the container at `/data`, so requests survive image rebuilds and container restarts.

Because the file is on the host, it can be committed to version control. Avoid committing Bearer Tokens or credentials; use `.gitignore` accordingly.
