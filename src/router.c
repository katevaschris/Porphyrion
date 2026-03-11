#include "router.h"
#include "http_response.h"
#include "call_api.h"
#include "decoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <strings.h>

#define BUFFER_SIZE 65536

/*
 * Reads the full http request from the socket into buf.
 *
 * @param sock        client socket fd
 * @param buf         buffer to read into
 * @param max_size    max buffer size
 * @param hdr_end_out points to the end of headers (\r\n\r\n)
 * @return total bytes read, or <= 0 on error
 */
static int read_full_request(int sock, char *buf, size_t max_size, char **hdr_end_out) {
    int n = read(sock, buf, max_size - 1);
    if (n <= 0) return n;
    buf[n] = '\0';

    char *hdr_end = strstr(buf, "\r\n\r\n");
    if (hdr_end) {
        int cl = 0;
        char *p = buf;
        while (p && p < hdr_end) {
            if (strncasecmp(p, "Content-Length:", 15) == 0) { cl = atoi(p + 15); break; }
            p = strstr(p, "\r\n");
            if (p) p += 2;
        }
        if (cl > 0) {
            int hdr_len = (hdr_end + 4) - buf;
            while (n < hdr_len + cl && (size_t)n < max_size - 1) {
                int r = read(sock, buf + n, max_size - 1 - n);
                if (r <= 0) break;
                n += r;
                buf[n] = '\0';
            }
        }
        *hdr_end_out = strstr(buf, "\r\n\r\n");
    } else {
        *hdr_end_out = NULL;
    }
    return n;
}

/*
 * Parses a url-encoded proxy request body and triggers the api call.
 *
 * @param sock    client socket fd
 * @param buf     the full request buffer
 * @param hdr_end pointer to the end of headers
 */
static void handle_proxy_route(int sock, char *hdr_end) {
    char *body_start = hdr_end ? (hdr_end + 4) : NULL;
    if (!body_start || strlen(body_start) == 0) {
        send_text(sock, 400, "{\"error\":\"Empty body\"}");
        return;
    }

    char url[2048] = {0}, method[16] = {0}, post_body[16384] = {0};
    char ct[256] = {0}, auth[8192] = {0};

    char *tmp = strdup(body_start);
    char *tok = strtok(tmp, "&");
    while (tok) {
        if      (strncmp(tok, "url=",    4) == 0) url_decode(tok + 4,    url);
        else if (strncmp(tok, "method=", 7) == 0) url_decode(tok + 7,    method);
        else if (strncmp(tok, "body=",   5) == 0) url_decode(tok + 5,    post_body);
        else if (strncmp(tok, "ct=",     3) == 0) url_decode(tok + 3,    ct);
        else if (strncmp(tok, "auth=",   5) == 0) url_decode(tok + 5,    auth);
        tok = strtok(NULL, "&");
    }
    free(tmp);

    if (strlen(url) == 0) {
        send_text(sock, 400, "{\"error\":\"Missing proxy URL\"}");
        return;
    }

    perform_api_call(sock, url, method, post_body, ct, auth);
}

/*
 * Saves the json to a file.
 *
 * @param sock    client socket fd
 * @param buf     full request buffer
 * @param n       total bytes in buffer
 * @param hdr_end pointer to end of headers
 */
static void handle_save_route(int sock, char *buf, int n, char *hdr_end) {
    char *body_start = hdr_end ? (hdr_end + 4) : NULL;
    if (body_start) {
        int len = n - (body_start - buf);
        FILE *f = fopen("data/requests.json", "w");
        if (f) { fwrite(body_start, 1, len, f); fclose(f); }
    }
    send_text(sock, 200, "{\"status\":\"saved\"}");
}

/*
 * Routes the request to a handler or static file.
 *
 * @param sock    client socket fd
 * @param buf     full request buffer
 * @param n       total bytes read
 * @param hdr_end pointer to end of headers
 */
static void route_request(int sock, char *buf, int n, char *hdr_end) {
    if (strncmp(buf, "GET / ", 6) == 0 || strncmp(buf, "GET /index.html", 15) == 0) {
        serve_file(sock, "index.html", "text/html");

    } else if (strncmp(buf, "GET /hat.png", 12) == 0) {
        serve_file(sock, "hat.png", "image/png");

    } else if (strncmp(buf, "OPTIONS ", 8) == 0) {
        const char *pre =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n\r\n";
        send(sock, pre, strlen(pre), 0);

    } else if (strncmp(buf, "GET /requests.json", 18) == 0) {
        if (access("data/requests.json", F_OK) == 0) {
            serve_file(sock, "data/requests.json", "application/json");
        } else {
            send_text(sock, 200, "[]");
        }

    } else if (strncmp(buf, "POST /save", 10) == 0) {
        handle_save_route(sock, buf, n, hdr_end);

    } else if (strncmp(buf, "POST /proxy", 11) == 0) {
        handle_proxy_route(sock, hdr_end);

    } else {
        const char *nf = "HTTP/1.1 404 Not Found\r\n\r\n";
        send(sock, nf, strlen(nf), 0);
    }
}

/*
 * Entry point for client connections.
 * Reads, routes, and terminates the socket (always the master).
 *
 * @param sock client socket fd
 */
void handle_client(int sock) {
    char *buf = malloc(BUFFER_SIZE);
    if (!buf) { close(sock); return; }

    char *hdr_end = NULL;
    int n = read_full_request(sock, buf, BUFFER_SIZE, &hdr_end);

    if (n > 0) {
        route_request(sock, buf, n, hdr_end);
    }
    free(buf);
    close(sock);
}
