#ifndef CALL_API_H
#define CALL_API_H

/*
 * Ηits the api via curl and proxies data back.
 *
 * @param sock      client socket fd
 * @param url       target api url
 * @param method    http method
 * @param post_body the payload
 * @param ct        Content-Type header
 * @param auth      Auth header
 */
void perform_api_call(int sock, const char *url, const char *method,
                      const char *post_body, const char *ct, const char *auth);

#endif
