#ifndef DECODER_H
#define DECODER_H

#include <stddef.h>

/*
 * Decodes a URL percent-encoded string into dst.
 * Writes at most dst_size-1 bytes and always NUL-terminates.
 *
 * @param src
 * @param dst
 * @param dst_size
 */
void url_decode(char *src, char *dst, size_t dst_size);

#endif
