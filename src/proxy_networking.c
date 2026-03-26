#include "proxy_networking.h"
#include <stdio.h>
#include <string.h>

/*
 * Rewrites localhost to host.containers.internal for pod.
 *
 * @param src
 * @param dst
 * @param size
 */
void resolve_url(const char *src, char *dst, size_t size)
{
    if (strncmp(src, "http://localhost", 16) == 0 ||
        strncmp(src, "http://127.0.0.1", 16) == 0)
    {
        snprintf(dst, size, "http://host.containers.internal%s", src + 16);
    }
    else
    {
        snprintf(dst, size, "%s", src);
    }
}
