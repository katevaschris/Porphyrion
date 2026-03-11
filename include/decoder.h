#ifndef DECODER_H
#define DECODER_H

/*
 * Converts url-encoded strings back to chars.
 *
 * @param src url-encoded source string
 * @param dst decoded destination buffer
 */
void url_decode(char *src, char *dst);

#endif
