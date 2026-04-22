#pragma once
#include <stdint.h>
#include <stddef.h>

/* ---- Standard C-library replacements for freestanding kernel ---- */

static inline void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    for (size_t i = 0; i < n; i++) p[i] = (uint8_t)c;
    return s;
}

static inline void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* sr = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) d[i] = sr[i];
    return dest;
}

static inline size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static inline int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) { a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

/* Integer to decimal string */
static inline void itoa(int value, char* buf) {
    char tmp[20];
    int i = 0;
    int neg = 0;
    if (value < 0) { neg = 1; value = -value; }
    if (value == 0) { tmp[i++] = '0'; }
    while (value > 0) { tmp[i++] = '0' + (value % 10); value /= 10; }
    if (neg) tmp[i++] = '-';
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}
