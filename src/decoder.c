#include "decoder.h"
#include <stdio.h>

/*
 * Decodes a URL-encoded string.
 *
 * @param src
 * @param dst
 * @param dst_size
 */
void url_decode(char *src, char *dst, size_t dst_size)
{
    if (!src || !dst || dst_size == 0)
        return;
    char *d = dst;
    const char *end = dst + dst_size - 1;
    while (*src && d < end)
    {
        if (*src == '%')
        {
            unsigned int val;
            if (sscanf(src + 1, "%2x", &val) == 1)
            {
                *d++ = (char)val;
                src += 3;
            }
            else
            {
                *d++ = *src++;
            }
        }
        else if (*src == '+')
        {
            *d++ = ' ';
            src++;
        }
        else
        {
            *d++ = *src++;
        }
    }
    *d = '\0';
}
