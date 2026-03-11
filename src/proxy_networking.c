#include "proxy_networking.h"
#include <string.h>
#include <stdio.h>

/* 
 * Rewrites localhost to host.containers.internal.
 * Podman talks to host machine.
 *
 * @param src original url
 * @param dst resolved url buffer
 */
void resolve_url(const char *src, char *dst) {
    if (strncmp(src, "http://localhost", 16) == 0 ||
        strncmp(src, "http://127.0.0.1", 16) == 0) {
        sprintf(dst, "http://host.containers.internal%s", src + 16);
    } else {
        strcpy(dst, src);
    }
}
