/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. M3 test orchestration, not a public API.
 */
#ifndef JANUS_M3_EXPERIMENT_H
#define JANUS_M3_EXPERIMENT_H
#include <stdbool.h>
#include <microkit.h>
#include "janus/substrate/model.h"
enum {
	TEST_HELLO = 100,
	TEST_PHASE,
	TEST_REPORT,
	TEST_START = 200,
	TEST_QUERY
};
static inline uint64_t
stamp(void)
{
	uint32_t low, high;
	/* x86-64 experiment clock only, not inter-thread synchronisation. */
	__asm__ __volatile__("lfence; rdtsc"
	                     : "=a"(low), "=d"(high)
	                     :
	                     : "memory");
	return ((uint64_t)high << 32) | low;
}
static inline void
exchange(uint64_t *q, uint64_t *r, uint64_t label, uint16_t count)
{
	for (uint8_t i = 0; i < JM_WORDS; i++)
		microkit_mr_set(i, q[i]);
	microkit_msginfo info =
	    microkit_ppcall(0, microkit_msginfo_new(label, count));
	for (uint8_t i = 0; i < JM_WORDS; i++)
		r[i] = microkit_mr_get(i);
	if (microkit_msginfo_get_count(info) != JM_WORDS ||
	    microkit_msginfo_get_label(info))
		r[0] = JM_INVALID;
}
#endif
