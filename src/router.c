#include "router.h"
#include "call_api.h"
#include "decoder.h"
#include "http_parser.h"
#include "http_response.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 65536
#define MAX_BODY_SIZE 65536
#define URL_SIZE 2048
#define METHOD_SIZE 16
#define BODY_SIZE 16384
#define CT_SIZE 256
#define AUTH_SIZE 8192

static const struct
{
    const char *route;
    const char *fs_path;
    const char *ct;
} STATIC_FILES[] = {
    {"/", "web/index.html", "text/html"},
    {"/index.html", "web/index.html", "text/html"},
    {"/css/style.css", "web/css/style.css", "text/css"},
    {"/js/app.js", "web/js/app.js", "text/javascript"},
    {"/resources/hat.png", "resources/hat.png", "image/png"},
    {"/requests.json", "data/requests.json", "application/json"},
};
#define STATIC_FILE_COUNT (int)(sizeof(STATIC_FILES) / sizeof(STATIC_FILES[0]))

/*
 * Reads the full HTTP request from the socket into buf.
 * Handles reading chunks if a Content-Length is present.
 *
 * @param sock
 * @param buf
 * @param buf_size
 * @param req
 * @return
 */
static int read_full_request(int sock, char *buf, int buf_size,
                             HttpRequest *req)
{
    int n = (int)read(sock, buf, (size_t)buf_size - 1);
    if (n <= 0)
    {
        return n;
    }
    buf[n] = '\0';
    HttpParseResult pr = http_parse_request(buf, (size_t)n, req);
    if (pr == HTTP_PARSE_ERROR)
    {
        return -1;
    }
    if (req->content_length > MAX_BODY_SIZE)
    {
        return -2;
    }
    if (pr == HTTP_PARSE_OK && req->content_length > 0)
    {
        int hdr_end_offset = (int)(req->body - buf);
        int total_expected = hdr_end_offset + (int)req->content_length;
        if (total_expected > buf_size - 1)
        {
            return -2;
        }
        int read_limit = 64;
        while (n < total_expected && read_limit-- > 0)
        {
            int r = (int)read(sock, buf + n, (size_t)(buf_size - 1 - n));
            if (r <= 0)
            {
                break;
            }
            n += r;
            buf[n] = '\0';
        }
        http_parse_request(buf, (size_t)n, req);
    }
    return n;
}

/*
 * Sends a 413 Payload Too Large response.
 *
 * @param sock
 */
static void send_413(int sock)
{
    const char *resp = "HTTP/1.1 413 Payload Too Large\r\n"
                       "Content-Type: application/json\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Content-Length: 27\r\n\r\n"
                       "{\"error\":\"Payload too large\"}";
    send(sock, resp, strlen(resp), 0);
}

/*
 * Sends a 400 Bad Request.
 *
 * @param sock
 * @param msg
 */
static void send_400(int sock, const char *msg)
{
    char body[128];
    snprintf(body, sizeof(body), "{\"error\":\"%s\"}", msg);
    send_text(sock, 400, body);
}

/*
 * Parses a URL-encoded proxy request body and triggers the call.
 *
 * @param sock
 * @param req
 */
static void handle_proxy_route(int sock, const HttpRequest *req)
{
    if (!req->body || req->body_len == 0)
    {
        send_400(sock, "Empty body");
        return;
    }
    static char url[URL_SIZE], method[METHOD_SIZE],
                post_body[BODY_SIZE], ct[CT_SIZE],
                headers[AUTH_SIZE];
    url[0] = method[0] = post_body[0] = ct[0] = headers[0] = '\0';
    static char body_copy[BODY_SIZE];
    size_t copy_len =
        req->body_len < BODY_SIZE - 1 ? req->body_len : BODY_SIZE - 1;
    memcpy(body_copy, req->body, copy_len);
    body_copy[copy_len] = '\0';
    char *p = body_copy, *end = body_copy + copy_len;
    while (p < end)
    {
        char *amp = memchr(p, '&', (size_t)(end - p));
        char *seg_end = amp ? amp : end;
        *seg_end = '\0';
        char *eq = strchr(p, '=');
        if (eq)
        {
            *eq = '\0';
            const char *key = p;
            char *val = eq + 1;
            if (strcmp(key, "url") == 0)
            {
                url_decode(val, url, sizeof(url));
            }
            else if (strcmp(key, "method") == 0)
            {
                url_decode(val, method, sizeof(method));
            }
            else if (strcmp(key, "body") == 0)
            {
                url_decode(val, post_body, sizeof(post_body));
            }
            else if (strcmp(key, "ct") == 0)
            {
                url_decode(val, ct, sizeof(ct));
            }
            else if (strcmp(key, "headers") == 0)
            {
                url_decode(val, headers, sizeof(headers));
            }
        }
        if (amp)
        {
            p = amp + 1;
        }
        else
        {
            break;
        }
    }
    if (url[0] == '\0')
    {
        send_400(sock, "Missing proxy URL");
        return;
    }
    if (method[0] == '\0')
    {
        method[0] = 'G';
        method[1] = 'E';
        method[2] = 'T';
        method[3] = '\0';
    }
    perform_api_call(sock, url, method, post_body, ct, headers);
}

/*
 * Saves the JSON payload from the request body to disk.
 *
 * @param sock
 * @param req
 */
static void handle_save_route(int sock, const HttpRequest *req)
{
    if (req->body && req->body_len > 0)
    {
        FILE *f = fopen("data/requests.json", "w");
        if (f)
        {
            fwrite(req->body, 1, req->body_len, f);
            fclose(f);
        }
    }
    send_text(sock, 200, "{\"status\":\"saved\"}");
}

/*
 * Responds to HTTP requests.
 *
 * @param sock
 */
static void handle_options(int sock)
{
    const char *pre = "HTTP/1.1 204 No Content\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                      "Access-Control-Allow-Headers: Content-Type\r\n\r\n";
    send(sock, pre, strlen(pre), 0);
}

/*
 * Routes the parsed request to a specific handler or static file.
 *
 * @param sock
 * @param req
 */
static void route_request(int sock, const HttpRequest *req)
{
    if (strcmp(req->method, "OPTIONS") == 0)
    {
        handle_options(sock);
        return;
    }
    if (strcmp(req->method, "POST") == 0)
    {
        if (strcmp(req->path, "/proxy") == 0)
        {
            handle_proxy_route(sock, req);
            return;
        }
        if (strcmp(req->path, "/save") == 0)
        {
            handle_save_route(sock, req);
            return;
        }
    }
    if (strcmp(req->method, "GET") == 0)
    {
        for (int i = 0; i < STATIC_FILE_COUNT; i++)
        {
            if (strcmp(req->path, STATIC_FILES[i].route) == 0)
            {
                if (strcmp(STATIC_FILES[i].fs_path, "data/requests.json") ==
                        0 &&
                    access(STATIC_FILES[i].fs_path, F_OK) != 0)
                {
                    send_text(sock, 200, "[]");
                }
                else
                {
                    serve_file(sock, STATIC_FILES[i].fs_path,
                               STATIC_FILES[i].ct);
                }
                return;
            }
        }
    }
    const char *nf = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    send(sock, nf, strlen(nf), 0);
}

/*
 * Reads the request, routes it, and terminates the socket.
 *
 * @param sock
 */
void handle_client(int sock)
{
    static char buf[BUFFER_SIZE];
    HttpRequest req;
    int n = read_full_request(sock, buf, BUFFER_SIZE, &req);
    if (n == -2)
    {
        send_413(sock);
    }
    else if (n > 0)
    {
        route_request(sock, &req);
    }
    close(sock);
}
