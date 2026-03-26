#ifndef PROXY_NETWORKING_H
#define PROXY_NETWORKING_H

#include <stddef.h>

/*
 * Rewrites localhost / 127.0.0.1 to host.containers.internal.
 * Pod uses this to reach the host machine.
 *
 * @param src
 * @param dst
 * @param size
 */
void resolve_url(const char *src, char *dst, size_t size);

#endif
