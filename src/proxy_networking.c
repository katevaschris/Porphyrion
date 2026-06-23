#include "proxy_networking.h"
#include <assert.h>
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
    assert(src != NULL);
    assert(dst != NULL);
    assert(size > 0);
    if (strncmp(src, "http://localhost", 16) == 0 ||
        strncmp(src, "http://127.0.0.1", 16) == 0)
    {
        (void)snprintf(dst, size, "http://host.containers.internal%s",
                       src + 16);
    }
    else
    {
        (void)snprintf(dst, size, "%s", src);
    }
}
