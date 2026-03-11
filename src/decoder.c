#include "decoder.h"
#include <stdio.h>

/* 
 * Converts url-encoded strings back to chars.
 * Handles %xx and plus signs.
 *
 * @param src url-encoded source
 * @param dst decoded destination buffer
 */
void url_decode(char *src, char *dst) {
    while (*src) {
        if (*src == '%') {
            int val;
            if (sscanf(src + 1, "%2x", &val) == 1) { *dst++ = (char)val; src += 3; }
            else { *dst++ = *src++; }
        } else if (*src == '+') {
            *dst++ = ' '; src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}
