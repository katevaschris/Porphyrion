#ifndef PROXY_NETWORKING_H
#define PROXY_NETWORKING_H

/*
 * Rewrites localhost to host.containers.internal.
 * Podman talks to host machine.
 *
 * @param src original url string
 * @param dst resolved url buffer
 */
void resolve_url(const char *src, char *dst);

#endif
