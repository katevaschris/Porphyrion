#ifndef CALL_API_H
#define CALL_API_H

/*
 * Ηits the api via curl and proxies data back.
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
                      const char *headers_payload);

#endif
