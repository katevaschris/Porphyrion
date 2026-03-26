#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

/*
 * Sends 404 if not found.
 *
 * @param sock
 * @param path
 * @param ct
 */
void serve_file(int sock, const char *path, const char *ct);

/*
 * Sends text or json back to client.
 * Adds cors and content-length.
 *
 * @param sock
 * @param status
 * @param body
 */
void send_text(int sock, int status, const char *body);

#endif
