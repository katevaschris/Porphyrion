#include "decoder.h"
#include <stddef.h>

/*
 * Converts a hex digit to its value.
 *
 * @param c
 * @return
 */
static int hex_val(unsigned char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    return -1;
}

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
    {
        return;
    }
    char *d = dst;
    const char *end = dst + dst_size - 1;
    while (*src != '\0' && d < end)
    {
        if (src[0] == '%')
        {
            int hi = hex_val((unsigned char)src[1]);
            int lo = (hi >= 0) ? hex_val((unsigned char)src[2]) : -1;
            if (lo >= 0)
            {
                *d++ = (char)((hi << 4) | lo);
                src += 3;
                continue;
            }
        }
        *d++ = (char)(*src == '+' ? ' ' : *src);
        src++;
    }
    *d = '\0';
}
