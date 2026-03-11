#include "http_response.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>

/*
 * Serves a file to the client.
 * Sends 404 if it's not found.
 *
 * @param sock client socket fd
 * @param path path to the file
 * @param ct   Content-Type header
 */
void serve_file(int sock, const char *path, const char *ct) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        const char *nf = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(sock, nf, strlen(nf), 0);
        return;
    }
    struct stat st;
    fstat(fd, &st);
    char hdr[256];
    snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lld\r\n\r\n",
        ct, (long long)st.st_size);
    send(sock, hdr, strlen(hdr), 0);
    char buf[4096];
    int n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) send(sock, buf, n, 0);
    close(fd);
}

/*
 * Sends text or json back to client.
 * Adds cors headers and content-length automatically.
 *
 * @param sock   client socket fd
 * @param status http status code
 * @param body   text to send
 */
void send_text(int sock, int status, const char *body) {
    char hdr[256];
    snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 %d OK\r\nContent-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\nContent-Length: %zu\r\n\r\n",
        status, strlen(body));
    send(sock, hdr, strlen(hdr), 0);
    send(sock, body, strlen(body), 0);
}
