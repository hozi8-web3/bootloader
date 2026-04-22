/* Provide actual symbol definitions for memset/memcpy that the
 * linker may need (GCC can emit implicit calls to these for 
 * struct copies and array initialization). */

#include <stdint.h>
#include <stddef.h>

void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    for (size_t i = 0; i < n; i++) p[i] = (uint8_t)c;
    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* sr = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) d[i] = sr[i];
    return dest;
}
