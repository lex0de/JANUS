/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. Contract-derived independent
 * expectations. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "janus/substrate/model.h"
static unsigned int checks;
#define CHECK(x)                                                               \
	do {                                                                   \
		checks++;                                                      \
		if (!(x)) {                                                    \
			fprintf(stderr, "line %d: %s\n", __LINE__, #x);        \
			exit(1);                                               \
		}                                                              \
	} while (0)
static void
call(struct jm_state *s, unsigned int caller, uint64_t op, uint64_t id,
     uint64_t token, uint64_t rev, uint64_t value, uint64_t rights,
     uint64_t expected, uint64_t *r)
{
	uint64_t q[JM_WORDS] = {1, op, id, token, rev, value, rights, 0};
	jm_dispatch(s, caller, 0, 8, q, r);
	CHECK(r[0] == expected);
	if (expected != JM_OK && expected != JM_CONFLICT)
		for (size_t i = 1; i < 8; i++)
			CHECK(r[i] == 0);
}
static void
malformed(void)
{
	/* Each row is independently invalid under the written eight-word
	 * grammar. */
	const uint64_t rows[][JM_WORDS] = {
	    {0, 1, 1},
	    {2, 1, 1},
	    {1, 0},
	    {1, 255},
	    {1, 1, 0},
	    {1, 1, 3},
	    {1, 1, 1, 1},
	    {1, 1, 1, 0, 1},
	    {1, 1, 1, 0, 0, 1},
	    {1, 1, 1, 0, 0, 0, 1},
	    {1, 1, 1, 0, 0, 0, 0, 1},
	    {1, 2, 0, 0},
	    {1, 2, 1, 1},
	    {1, 2, 0, 1, 1},
	    {1, 3, 0, 1, 0},
	    {1, 3, 0, 1, UINT64_MAX},
	    {1, 3, 0, 1, 1, 0, 1},
	    {1, 32, 0},
	    {1, 32, UINT64_C(0x100000001), 0, 0, 0, 0},
	    {1, 32, UINT64_C(0x100000001), 0, 0, 0, 8},
	    {1, 32, UINT64_C(0x300000001), 0, 0, 0, 1},
	    {1, 34, 3},
	    {1, 34, 1, 0, 1},
	    {1, 35, UINT64_C(0x100000001), 0, 0},
	    {1, 4, 1},
	    {1, 5, 0, 1}};
	struct jm_state s, before;
	uint64_t r[8];
	jm_init(&s);
	for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
		for (unsigned int caller = 0; caller < 3; caller++) {
			before = s;
			jm_dispatch(&s, caller, 0, 8, rows[i], r);
			CHECK(r[0] == JM_INVALID);
			CHECK(memcmp(&before, &s, sizeof(s)) == 0);
		}
	}
	uint64_t q[8] = {1, 1, 1};
	for (size_t count = 0; count < 130; count++) {
		if (count == 8)
			continue;
		before = s;
		jm_dispatch(&s, 1, 0, count, q, r);
		CHECK(r[0] == JM_INVALID);
		CHECK(memcmp(&before, &s, sizeof(s)) == 0);
	}
	before = s;
	jm_dispatch(&s, 1, 1, 8, q, r);
	CHECK(r[0] == JM_INVALID);
	CHECK(memcmp(&before, &s, sizeof(s)) == 0);
	jm_dispatch(&s, 3, 0, 8, q, r);
	CHECK(r[0] == JM_DENIED);
	CHECK(memcmp(&before, &s, sizeof(s)) == 0);
	/* Bounded high-bit/reserved-word mutation oracle: always invalid, no
	 * state change. */
	for (uint64_t i = 1; i <= 4096; i++) {
		q[7] = i * UINT64_C(0x9e3779b97f4a7c15);
		jm_dispatch(&s, 1, 0, 8, q, r);
		CHECK(r[0] == JM_INVALID);
		CHECK(memcmp(&before, &s, sizeof(s)) == 0);
	}
}
static void
authority(void)
{
	struct jm_state s;
	uint64_t r[8], a, b, newer;
	const uint64_t target = UINT64_C(0x100000001);
	jm_init(&s);
	const uint64_t controls[][8] = {
	    {1, 32, UINT64_C(0x100000001), 0, 0, 0, 3},
	    {1, 33, UINT64_C(0x100000001)},
	    {1, 34, 1},
	    {1, 35, UINT64_C(0x100000001), 0, 1},
	    {1, 36, 1},
	    {1, 37, UINT64_C(0x100000001)}};
	for (size_t i = 0; i < sizeof(controls) / sizeof(controls[0]); i++) {
		struct jm_state unchanged = s;
		for (unsigned int caller = 1; caller <= 2; caller++) {
			jm_dispatch(&s, caller, 0, 8, controls[i], r);
			CHECK(r[0] == JM_DENIED);
			CHECK(memcmp(&s, &unchanged, sizeof(s)) == 0);
		}
	}
	call(&s, 1, JM_ACQUIRE, 1, 0, 0, 0, 0, JM_DENIED, r);
	call(&s, 0, JM_GRANT, target, 0, 0, 0, 1, JM_OK, r);
	call(&s, 1, JM_ACQUIRE, 1, 0, 0, 0, 0, JM_OK, r);
	a = r[3];
	CHECK(a != 0);
	call(&s, 1, JM_GET, 0, a, 0, 0, 0, JM_OK, r);
	CHECK(r[1] == 1 && r[2] == 0xaaaa);
	call(&s, 2, JM_ACQUIRE, 1, 0, 0, 0, 0, JM_DENIED, r);
	call(&s, 2, JM_GET, 0, a, 0, 0, 0, JM_STALE, r);
	call(&s, 1, JM_PUT, 0, a, 1, 0xcccc, 0, JM_DENIED, r);
	call(&s, 1, JM_GRANT, target, 0, 0, 0, 3, JM_DENIED, r);
	call(&s, 1, JM_REVOKE, target, 0, 0, 0, 0, JM_DENIED, r);
	call(&s, 1, JM_ROTATE, 1, 0, 0, 0, 0, JM_DENIED, r);
	call(&s, 1, JM_DELEGATE, 0, 0, 0, 0, 0, JM_UNSUPPORTED, r);
	struct jm_state before = s;
	call(&s, 0, JM_GRANT, target, 0, 0, 0, 5, JM_UNSUPPORTED, r);
	CHECK(memcmp(&s, &before, sizeof(s)) == 0);
	call(&s, 0, JM_GRANT, target, 0, 0, 0, 3, JM_OK, r);
	call(&s, 1, JM_GET, 0, a, 0, 0, 0, JM_STALE, r);
	call(&s, 1, JM_ACQUIRE, 1, 0, 0, 0, 0, JM_OK, r);
	b = r[3];
	CHECK(b != a);
	call(&s, 1, JM_PUT, 0, b, 1, 0xcccc, 0, JM_OK, r);
	CHECK(r[1] == 2);
	call(&s, 1, JM_PUT, 0, b, 1, 0xdddd, 0, JM_CONFLICT, r);
	CHECK(r[1] == 2 && r[2] == 0);
	call(&s, 1, JM_GET, 0, b, 0, 0, 0, JM_OK, r);
	CHECK(r[1] == 2 && r[2] == 0xcccc);
	call(&s, 0, JM_SAVE, target, 0, 2, 0, 0, JM_OK, r);
	call(&s, 0, JM_REVOKE, target, 0, 0, 0, 0, JM_OK, r);
	call(&s, 1, JM_GET, 0, b, 0, 0, 0, JM_STALE, r);
	call(&s, 0, JM_RESTORE, 1, 0, 0, 0, 0, JM_OK, r);
	CHECK(r[4] == 2 && r[2] == 1);
	call(&s, 1, JM_GET, 0, b, 0, 0, 0, JM_STALE, r);
	call(&s, 1, JM_ACQUIRE, 1, 0, 0, 0, 0, JM_DENIED, r);
	call(&s, 0, JM_GRANT, target, 0, 0, 0, 2, JM_OK, r);
	call(&s, 1, JM_ACQUIRE, 1, 0, 0, 0, 0, JM_OK, r);
	newer = r[3];
	CHECK(newer != a && newer != b);
	call(&s, 1, JM_GET, 0, newer, 0, 0, 0, JM_DENIED, r);
	call(&s, 1, JM_PUT, 0, newer, 2, 0xeeee, 0, JM_OK, r);
	call(&s, 0, JM_ROTATE, 1, 0, 0, 0, 0, JM_OK, r);
	call(&s, 1, JM_GET, 0, newer, 0, 0, 0, JM_STALE, r);
}
static void
bounds(void)
{
	struct jm_state s;
	uint64_t r[8], token = 0;
	const uint64_t target = UINT64_C(0x100000001);
	jm_init(&s);
	call(&s, 0, JM_GRANT, target, 0, 0, 0, 3, JM_OK, r);
	for (size_t i = 0; i < 4; i++) {
		call(&s, 1, JM_ACQUIRE, 1, 0, 0, 0, 0, JM_OK, r);
		CHECK(r[3] > token);
		token = r[3];
	}
	call(&s, 1, JM_ACQUIRE, 1, 0, 0, 0, 0, JM_LIMIT, r);
	for (size_t i = 5; i < 64; i++)
		call(&s, 1, JM_GET, 0, token, 0, 0, 0, JM_OK, r);
	call(&s, 1, JM_GET, 0, token, 0, 0, 0, JM_LIMIT, r);
	call(&s, 0, JM_INSPECT, target, 0, 0, 0, 0, JM_OK, r);
	CHECK(r[6] == 64 && r[7] == 4);
	call(&s, 0, JM_REVOKE, target, 0, 0, 0, 0, JM_OK, r);
	call(&s, 0, JM_ROTATE, 1, 0, 0, 0, 0, JM_OK, r);
	call(&s, 0, JM_GRANT, UINT64_C(0x200000002), 0, 0, 0, 1, JM_OK, r);
	call(&s, 2, JM_ACQUIRE, 2, 0, 0, 0, 0, JM_OK, r);
	call(&s, 2, JM_GET, 0, r[3], 0, 0, 0, JM_OK, r);
	CHECK(r[2] == 0xbbbb);
	s.worlds[0].incarnation = UINT32_MAX;
	call(&s, 0, JM_ROTATE, 1, 0, 0, 0, 0, JM_LIMIT, r);
	s.grants[0][0].generation = UINT32_MAX;
	call(&s, 0, JM_GRANT, target, 0, 0, 0, 3, JM_LIMIT, r);
	call(&s, 0, JM_REVOKE, target, 0, 0, 0, 0, JM_OK, r);
	CHECK(s.grants[0][0].rights == 0);
	jm_init(&s);
	call(&s, 0, JM_GRANT, target, 0, 0, 0, 3, JM_OK, r);
	s.next_token = UINT64_MAX;
	call(&s, 1, JM_ACQUIRE, 1, 0, 0, 0, 0, JM_LIMIT, r);
	s.next_token = 1;
	call(&s, 1, JM_ACQUIRE, 1, 0, 0, 0, 0, JM_OK, r);
	token = r[3];
	s.objects[0].revision = UINT32_MAX;
	call(&s, 1, JM_PUT, 0, token, UINT32_MAX, 99, 0, JM_LIMIT, r);
	CHECK(s.objects[0].content == 0xaaaa);
}
int
main(void)
{
	malformed();
	authority();
	bounds();
	printf("PASS M3 portable: %u checks\n", checks);
	return 0;
}
