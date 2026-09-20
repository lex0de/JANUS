/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "janus/native/native.h"
int
jn_number(const char *s, uint64_t *v)
{
	if (!*s || strspn(s, "0123456789") != strlen(s))
		return -1;
	char *end;
	errno = 0;
	uintmax_t n = strtoumax(s, &end, 10);
	if (errno || *end || n > INT64_MAX)
		return -1;
	*v = (uint64_t)n;
	return 0;
}
void
jn_print(const struct jn_msg *r, int status)
{
	static const char *names[] = {
	    "OK", "INVALID", "DENIED",      "CONFLICT", "LIMIT",
	    "IO", "STALE",   "UNSUPPORTED", "BUSY",     "UNCERTAIN"};
	char id[33], handle[33];
	jn_hex(r->id, id);
	jn_hex(r->handle, handle);
	printf("status=%s op=%u revision=%" PRIu64 " incarnation=%" PRIu64
	       " id=%s handle=%s content=",
	       status >= 0 && status <= JN_UNCERTAIN ? names[status]
	                                             : "INVALID",
	       r->op, r->revision, r->arg, id, handle);
	for (size_t i = 0; i < r->length; i++)
		printf("%02x", r->data[i]);
	putchar('\n');
}
