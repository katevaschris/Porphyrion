#include "call_api.h"
#include "proxy_networking.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <curl/curl.h>

// Dynamic string for the curl response.
struct string { char *ptr; size_t len; };

/*
 * Inits the dynamic string.
 * Sets length to 0 and mallocs 1 byte.
 *
 * @param s pointer to the struct
 */
static void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(1);
    if (!s->ptr) { fprintf(stderr, "malloc failed\n"); exit(EXIT_FAILURE); }
    s->ptr[0] = '\0';
}

/*
 * Curl callback to write data into our string.
 * Resizes the buffer.
 *
 * @param ptr   incoming data chunk
 * @param size  unit size
 * @param nmemb number of units
 * @param s     pointer to our string struct
 * @return processed size
 */
static size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
    size_t new_len = s->len + size * nmemb;
    char *new_ptr = realloc(s->ptr, new_len + 1);
    if (!new_ptr) { fprintf(stderr, "realloc failed\n"); exit(EXIT_FAILURE); }
    s->ptr = new_ptr;
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    return size * nmemb;
}

/*
 * Builds the http headers list for curl.
 * Adds content, auth headers etc.
 *
 * @param ct   Content-Type string
 * @param auth Authorization string
 * @return pointer to the curl slist
 */
static struct curl_slist *build_headers(const char *ct, const char *auth) {
    struct curl_slist *hdrs = NULL;
    if (strlen(ct) > 0) {
        char ct_hdr[300];
        snprintf(ct_hdr, sizeof(ct_hdr), "Content-Type: %s", ct);
        hdrs = curl_slist_append(hdrs, ct_hdr);
    }
    if (strlen(auth) > 0) {
        char auth_hdr[8500];
        snprintf(auth_hdr, sizeof(auth_hdr), "Authorization: %s", auth);
        hdrs = curl_slist_append(hdrs, auth_hdr);
    }
    return hdrs;
}

/*
 * Sets up the curl handle with method and body.
 * Handles requests.
 *
 * @param curl      curl handle
 * @param method    http method name
 * @param post_body payload to send
 */
static void setup_method(CURL *curl, const char *method, const char *post_body) {
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(post_body));
    } else if (strcmp(method, "GET") != 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
        if (strlen(post_body) > 0) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(post_body));
        }
    }
}

/*
 * Hits the api via curl and proxies data back.
 * Sends the response to the socket.
 *
 * @param sock      client socket fd
 * @param url       target api url
 * @param method    http method
 * @param post_body the payload
 * @param ct        Content-Type header
 * @param auth      Auth header
 */
void perform_api_call(int sock, const char *url, const char *method,
                      const char *post_body, const char *ct, const char *auth) {
    char resolved[2048] = {0};
    resolve_url(url, resolved);

    CURL *curl = curl_easy_init();
    struct string resp;
    init_string(&resp);
    long http_code = 0;

    if (curl) {
        struct curl_slist *hdrs = build_headers(ct, auth);
        curl_easy_setopt(curl, CURLOPT_URL, resolved);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
        if (hdrs) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
        setup_method(curl, method, post_body);

        CURLcode rc = curl_easy_perform(curl);
        if (rc != CURLE_OK) {
            char err[300];
            snprintf(err, sizeof(err), "{\"error\":\"Curl Error: %s\"}", curl_easy_strerror(rc));
            free(resp.ptr);
            resp.ptr = strdup(err);
            resp.len = strlen(err);
        } else {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        }
        if (hdrs) curl_slist_free_all(hdrs);
        curl_easy_cleanup(curl);
    }

    char hdr[512];
    snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Expose-Headers: X-Proxy-Status\r\n"
        "X-Proxy-Status: %ld\r\n"
        "Content-Length: %zu\r\n\r\n", http_code, resp.len);
    send(sock, hdr, strlen(hdr), 0);
    send(sock, resp.ptr, resp.len, 0);
    free(resp.ptr);
}
