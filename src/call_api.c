#define _POSIX_C_SOURCE 200809L
#include "call_api.h"
#include "proxy_networking.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#define MAX_RESPONSE_BYTES (512 * 1024)
#define MAX_RESP_HDRS (16 * 1024)
#define RESOLVED_URL_SIZE 2048
#define HEADERS_BUF_SIZE 8192
#define CT_HDR_SIZE 300

typedef struct
{
    char buf[MAX_RESPONSE_BYTES];
    size_t len;
    int truncated;
} RespBuf;

typedef struct
{
    char buf[MAX_RESP_HDRS];
    size_t len;
} RespHdrBuf;

/*
 * Callback for libcurl to write response body into a buffer.
 *
 * @param ptr
 * @param size
 * @param nmemb
 * @param rb
 * @return
 */
static size_t write_to_buf(void *ptr, size_t size, size_t nmemb, RespBuf *rb)
{
    size_t incoming = size * nmemb;
    size_t space = MAX_RESPONSE_BYTES - rb->len;
    if (incoming > space)
    {
        incoming = space;
        rb->truncated = 1;
    }
    if (incoming > 0)
    {
        memcpy(rb->buf + rb->len, ptr, incoming);
        rb->len += incoming;
    }
    return size * nmemb;
}

/*
 * Callback for libcurl to write response headers into buffer.
 *
 * @param ptr
 * @param size
 * @param nmemb
 * @param hb
 * @return
 */
static size_t header_callback(void *ptr, size_t size, size_t nmemb,
                              RespHdrBuf *hb)
{
    size_t incoming = size * nmemb;
    size_t space = MAX_RESP_HDRS - hb->len;
    if (incoming > space)
        incoming = space;
    if (incoming > 0)
    {
        memcpy(hb->buf + hb->len, ptr, incoming);
        hb->len += incoming;
    }
    return size * nmemb;
}

/*
 * Builds a libcurl header list from a URL-encoded headers payload.
 *
 * @param ct
 * @param headers_payload newline-separated header string
 * @return libcurl slist pointer
 */
static struct curl_slist *build_headers(const char *ct,
                                        const char *headers_payload)
{
    struct curl_slist *hdrs = NULL;
    if (ct[0] != '\0')
    {
        char ct_hdr[CT_HDR_SIZE];
        snprintf(ct_hdr, sizeof(ct_hdr), "Content-Type: %s", ct);
        hdrs = curl_slist_append(hdrs, ct_hdr);
    }
    if (headers_payload[0] != '\0')
    {
        static char buf[HEADERS_BUF_SIZE];
        strncpy(buf, headers_payload, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        char *saveptr;
        char *line = strtok_r(buf, "\n", &saveptr);
        while (line)
        {
            hdrs = curl_slist_append(hdrs, line);
            line = strtok_r(NULL, "\n", &saveptr);
        }
    }
    return hdrs;
}

/*
 * Configures libcurl for the given HTTP method, and attaches a body if present.
 *
 * @param curl
 * @param method
 * @param post_body
 */
static void setup_method(CURL *curl, const char *method, const char *post_body)
{
    if (strcmp(method, "POST") == 0)
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(post_body));
    }
    else if (strcmp(method, "GET") != 0)
    {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
        if (post_body[0] != '\0')
        {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                             (long)strlen(post_body));
        }
    }
}

static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
/*
 * Encodes a buffer to Base64 format.
 *
 * @param src
 * @param src_len
 * @param dst
 * @param dst_size
 * @return number of bytes written
 */
static size_t base64_encode(const char *src, size_t src_len, char *dst,
                            size_t dst_size)
{
    size_t o = 0, i = 0;
    while (i < src_len && o + 4 < dst_size)
    {
        unsigned char a = (unsigned char)src[i++];
        unsigned char b = i < src_len ? (unsigned char)src[i++] : 0;
        unsigned char c = i < src_len ? (unsigned char)src[i++] : 0;
        int pad = (i > src_len + 1) ? 2 : (i > src_len) ? 1 : 0;
        dst[o++] = B64[a >> 2];
        dst[o++] = B64[((a & 3) << 4) | (b >> 4)];
        dst[o++] = pad >= 2 ? '=' : B64[((b & 0xF) << 2) | (c >> 6)];
        dst[o++] = pad >= 1 ? '=' : B64[c & 0x3F];
    }
    dst[o] = '\0';
    return o;
}

/*
 * Performs an API call as a proxy via libcurl and returns.
 *
 * @param sock
 * @param url
 * @param method
 * @param post_body
 * @param ct
 * @param headers_payload
 */
void perform_api_call(int sock, const char *url, const char *method,
                      const char *post_body, const char *ct,
                      const char *headers_payload)
{
    static char resolved[RESOLVED_URL_SIZE];
    resolve_url(url, resolved, sizeof(resolved));
    static RespBuf rb;
    rb.len = 0;
    rb.truncated = 0;
    static RespHdrBuf hb;
    hb.len = 0;
    long http_code = 0;
    static char err_body[300];
    const char *resp_data = rb.buf;
    size_t resp_len = 0;
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        const char *msg = "{\"error\":\"curl_easy_init failed\"}";
        char hdr[256];
        snprintf(hdr, sizeof(hdr),
                 "HTTP/1.1 502 Bad Gateway\r\nContent-Type: "
                 "application/json\r\nAccess-Control-Allow-Origin: "
                 "*\r\nContent-Length: %zu\r\n\r\n",
                 strlen(msg));
        send(sock, hdr, strlen(hdr), 0);
        send(sock, msg, strlen(msg), 0);
        return;
    }
    struct curl_slist *hdrs = build_headers(ct, headers_payload);
    curl_easy_setopt(curl, CURLOPT_URL, resolved);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buf);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rb);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &hb);
    if (hdrs)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    setup_method(curl, method, post_body);
    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK)
    {
        snprintf(err_body, sizeof(err_body), "{\"error\":\"Curl Error: %s\"}",
                 curl_easy_strerror(rc));
        resp_data = err_body;
        resp_len = strlen(err_body);
    }
    else
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        resp_data = rb.buf;
        resp_len = rb.len;
    }
    if (hdrs)
        curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    static char b64_hdrs[MAX_RESP_HDRS * 2];
    base64_encode(hb.buf, hb.len, b64_hdrs, sizeof(b64_hdrs));
    static char hdr[MAX_RESP_HDRS * 2 + 1024];
    snprintf(hdr, sizeof(hdr),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/octet-stream\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Expose-Headers: "
             "X-Proxy-Status,X-Resp-Headers,X-Resp-Size\r\n"
             "X-Proxy-Status: %ld\r\n"
             "X-Resp-Headers: %s\r\n"
             "X-Resp-Size: %zu\r\n"
             "Content-Length: %zu\r\n\r\n",
             http_code, b64_hdrs, resp_len, resp_len);
    send(sock, hdr, strlen(hdr), 0);
    send(sock, resp_data, resp_len, 0);
}
