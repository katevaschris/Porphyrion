#include "http_response.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * Serves a static file directly to the socket.
 *
 * @param sock
 * @param path
 * @param ct
 */
void serve_file(int sock, const char *path, const char *ct)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        const char *nf = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(sock, nf, strlen(nf), 0);
        return;
    }
    struct stat st;
    if (fstat(fd, &st) < 0)
    {
        close(fd);
        const char *err =
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        send(sock, err, strlen(err), 0);
        return;
    }
    char hdr[256];
    snprintf(
        hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lld\r\n\r\n",
        ct, (long long)st.st_size);
    send(sock, hdr, strlen(hdr), 0);
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        send(sock, buf, (size_t)n, 0);
    }
    close(fd);
}

/*
 * Sends a JSON text response.
 *
 * @param sock
 * @param status
 * @param body
 */
void send_text(int sock, int status, const char *body)
{
    char hdr[256];
    snprintf(hdr, sizeof(hdr),
             "HTTP/1.1 %d OK\r\nContent-Type: application/json\r\n"
             "Access-Control-Allow-Origin: *\r\nContent-Length: %zu\r\n\r\n",
             status, strlen(body));
    send(sock, hdr, strlen(hdr), 0);
    send(sock, body, strlen(body), 0);
}
