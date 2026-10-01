/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. Independent live expectations. */
#include "experiment.h"
static uint64_t old_handle, failures;
static void
expect(uint64_t *q, uint64_t expected, uint64_t *r)
{
	exchange(q, r, 0, JM_WORDS);
	if (r[0] != expected)
		failures++;
	if (expected != JM_OK && expected != JM_CONFLICT)
		for (size_t i = 1; i < JM_WORDS; i++)
			if (r[i])
				failures++;
}
static uint64_t
acquire(uint64_t id)
{
	uint64_t q[JM_WORDS] = {JM_VERSION, JM_ACQUIRE, id}, r[JM_WORDS];
	expect(q, JM_OK, r);
	return r[3];
}
static void
get(uint64_t handle, uint64_t expected, uint64_t content)
{
	uint64_t q[JM_WORDS] = {JM_VERSION, JM_GET, 0, handle}, r[JM_WORDS];
	expect(q, expected, r);
	if (expected == JM_OK && r[2] != content)
		failures++;
}
void
init(void)
{
	uint64_t q[JM_WORDS] = {JM_VERSION, TEST_HELLO}, r[JM_WORDS];
	expect(q, JM_OK, r);
}
void
notified(microkit_channel ch)
{
	if (ch != 0)
		return;
	uint64_t q[JM_WORDS] = {JM_VERSION, TEST_PHASE}, r[JM_WORDS];
	failures = 0;
	expect(q, JM_OK, r);
	uint64_t phase = r[1], arg = r[2];
	switch (phase) {
	case 1:
		old_handle = acquire(1);
		get(old_handle, JM_OK, 0xaaaa);
		break;
	case 2:
		q[1] = JM_ACQUIRE;
		q[2] = 1;
		expect(q, JM_DENIED, r);
		get(arg, JM_STALE, 0);
		q[7] = 1;
		expect(q, JM_INVALID,
		       r); /* Claimed identity in reserved word. */
		old_handle = acquire(2);
		get(old_handle, JM_OK, 0xbbbb);
		break;
	case 3:
		q[1] = JM_GRANT;
		q[2] = (UINT64_C(1) << 32) | 1;
		q[6] = 3;
		expect(q, JM_DENIED, r);
		q[1] = JM_REVOKE;
		q[6] = 0;
		expect(q, JM_DENIED, r);
		break;
	case 4:
	case 7:
		get(old_handle, JM_STALE, 0);
		q[1] = JM_ACQUIRE;
		q[2] = 1;
		expect(q, JM_DENIED, r);
		break;
	case 5:
	case 6:
		get(old_handle, JM_STALE, 0);
		old_handle = acquire(1);
		get(old_handle, JM_OK, 0xaaaa);
		break;
	case 8:
		old_handle = acquire(1);
		q[1] = JM_PUT;
		q[3] = old_handle;
		q[4] = 1;
		q[5] = 0xcccc;
		expect(q, JM_DENIED, r);
		q[1] = JM_GET;
		q[4] = 0;
		q[5] = 0;
		q[7] = 1;
		expect(q, JM_INVALID, r);
		q[7] = 0;
		exchange(q, r, 0, 7);
		if (r[0] != JM_INVALID)
			failures++;
		exchange(q, r, 0, 9);
		if (r[0] != JM_INVALID)
			failures++;
		exchange(q, r, 1, JM_WORDS);
		if (r[0] != JM_INVALID)
			failures++;
		q[1] = 255;
		expect(q, JM_INVALID, r);
		q[1] = JM_DELEGATE;
		q[3] = 0;
		expect(q, JM_UNSUPPORTED, r);
		get(old_handle, JM_OK, 0xaaaa);
		break;
	case 9:
		old_handle = acquire(1);
		q[1] = JM_PUT;
		q[3] = old_handle;
		q[4] = 1;
		q[5] = 0xcccc;
		expect(q, JM_OK, r);
		if (r[1] != 2)
			failures++;
		q[5] = 0xdddd;
		expect(q, JM_CONFLICT, r);
		if (r[2])
			failures++;
		get(old_handle, JM_OK, 0xcccc);
		break;
	case 10: {
		const uint64_t controls[][JM_WORDS] = {
		    {1, 32, UINT64_C(0x100000001), 0, 0, 0, 3},
		    {1, 33, UINT64_C(0x100000001)},
		    {1, 34, 1},
		    {1, 35, UINT64_C(0x100000001), 0, 2},
		    {1, 36, 1},
		    {1, 37, UINT64_C(0x100000001)}};
		for (size_t i = 0; i < JM_HANDLES; i++)
			old_handle = acquire(1);
		q[1] = JM_ACQUIRE;
		q[2] = 1;
		expect(q, JM_LIMIT, r);
		/* Four handles plus the failed fifth acquisition consumed five
		 * admissions. Denied owner calls must consume the rest. */
		for (size_t i = JM_HANDLES + 1; i < JM_QUOTA; i++) {
			for (size_t j = 0; j < JM_WORDS; j++)
				q[j] = controls[i % 6][j];
			expect(q, JM_DENIED, r);
		}
		for (size_t i = 0; i < 6; i++) {
			for (size_t j = 0; j < JM_WORDS; j++)
				q[j] = controls[i][j];
			expect(q, JM_LIMIT, r);
		}
		get(old_handle, JM_LIMIT, 0);
		break;
	}
	case 11:
	case 14:
		get(old_handle, JM_OK, 0xbbbb);
		break;
	case 12:
		q[1] = JM_WORK;
		expect(q, JM_OK, r);
		break;
	default:
		failures++;
		break;
	}
	for (size_t i = 0; i < JM_WORDS; i++)
		q[i] = 0;
	q[0] = JM_VERSION;
	q[1] = TEST_REPORT;
	q[4] = failures;
	q[5] = old_handle;
	exchange(q, r, 0, JM_WORDS);
}
