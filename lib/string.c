// Copyright (C) 2006 Microchip Technology Inc. and its subsidiaries
//
// SPDX-License-Identifier: MIT

#include "string.h"
#include "common.h"

void *memcpy(void *dst, const void *src, int cnt)
{
	char *d;
	const char *s;
	struct chunk {
		unsigned long val[2];
	};

	const struct chunk *csrc = (const struct chunk *) src;
	struct chunk *cdst = (struct chunk *)dst;

	if (((unsigned long)src & 0xf) == 0 && ((unsigned long)dst & 0xf) == 0) {
		while (cnt >= sizeof(struct chunk)) {
			*cdst++ = *csrc++;
			cnt -= sizeof(struct chunk);
		}
	}

	d = (char *) cdst;
	s = (const char *) csrc;

	while (cnt--)
		*d++ = *s++;

	return dst;
}

void *memset(void *dst, int val, int cnt)
{
	char *d = (char *)dst;

	while (cnt--)
		*d++ = (char)val;

	return dst;
}

int memcmp(const void *dst, const void *src, unsigned int cnt)
{
	const char *d = (const char *)dst;
	const char *s = (const char *)src;
	int r = 0;

	while (cnt-- && (r = *d++ - *s++) == 0) ;

	return r;
}

unsigned int strlen(const char *str)
{
	int i = 0;

	while (str[i++] != '\0') ;

	return i - 1;
}

char *strcpy(char *dst, const char *src)
{
	char *bak = dst;

	while ((*dst++ = *src++) != '\0') ;

	return bak;
}

char *strncpy(char *dst, const char *src, size_t n)
{
	if (n != 0) {
		char *d = dst;
		const char *s = src;
		do {
			if ((*d++ = *s++) == 0) {
				/* NUL pad the remaining n-1 bytes */
				while (--n != 0)
					*d++ = 0;
				break;
			}
		} while (--n != 0);
	}
	return (dst);
}

char *strcat(char *dst, const char *src)
{
	char *p = dst;

	while (*dst != '\0')
		dst++;

	while ((*dst++ = *src++) != '\0') ;

	return p;
}

int strcmp(const char *p1, const char *p2)
{
	unsigned char c1, c2;

	while (1) {
		c1 = *p1++;
		c2 = *p2++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}

	return 0;
}

int strncmp(const char *p1, const char *p2, unsigned int cnt)
{
	unsigned char c1, c2;

	while (cnt--) {
		c1 = *p1++;
		c2 = *p2++;

		if (c1 != c2)
			return c1 < c2 ? -1 : 1;

		if (!c1)
			break;
	}

	return 0;
}

char *strchr(const char *s, int c)
{
	for (; *s != (char) c; ++s)
		if (*s == '\0')
			return NULL;

	return (char *)s;
}

/* NOTE: This is the simple-minded O(len(s1) * len(s2)) worst-case approach. */

char *strstr(const char *s1, const char *s2)
{
	register const char *s = s1;
	register const char *p = s2;

	do {
		if (!*p) {
			return (char *) s1;;
		}
		if (*p == *s) {
			++p;
			++s;
		} else {
			p = s2;
			if (!*s) {
				return NULL;
			}
			s = ++s1;
		}
	} while (1);
}

void *memchr(void *src, int val, unsigned int cnt)
{
	char *p = NULL;
	char *s = (char *)src;

	while (cnt) {
		if (*s == val) {
			p = s;
			break;
		}
		s++;
		cnt--;
	}

	return p;
}

void *memmove(void *dst, const void *src, unsigned int cnt)
{
	char *p, *s;

	if (dst <= src) {
		p = (char *)dst;
		s = (char *)src;
		while (cnt--)
			*p++ = *s++;
		}
	else {
		p = (char *)dst + cnt;
		s = (char *)src + cnt;
		while (cnt--)
			*--p = *--s;
		}

	return dst;
}

int atoi(char* str)
{
    // Initialize result
    int res = 0;

    // Iterate through all characters
    // of input string and update result
    // take ASCII character of corresponding digit and
    // subtract the code from '0' to get numerical
    // value and multiply res by 10 to shuffle
    // digits left to update running total
    for (int i = 0; str[i] != '\0'; ++i)
        res = res * 10 + str[i] - '0';

    // return result.
    return res;
}

int atoh(char* str)
{
    // Initialize result
    int res = 0;

    // Iterate through all characters
    // of input string and update result
    // take ASCII character of corresponding digit and
    // subtract the code from '0' to get numerical
    // value and multiply res by 10 to shuffle
    // digits left to update running total
    for (int i = 0; str[i] != '\0'; ++i)
        res = res * 16 + str[i] - '0';

    // return result.
    return res;
}
