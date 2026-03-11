#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

/*
 * Serves a file to the client.
 * Sends 404 if it's not found.
 *
 * @param sock client socket fd
 * @param path path to the file
 * @param ct   Content-Type header
 */
void serve_file(int sock, const char *path, const char *ct);

/*
 * Sends text or json back to client.
 * Adds cors headers and content-length automatically.
 *
 * @param sock   client socket fd
 * @param status http status code
 * @param body   text buffer to send
 */
void send_text(int sock, int status, const char *body);

#endif
