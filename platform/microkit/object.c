/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. */
#include "experiment.h"
static struct jm_state objects;
static uint64_t boot_mask, phase[2], argument[2], report_world, report_phase,
    report_errors, token_a, work_begin, work_end;
uintptr_t private_page;
void
init(void)
{
	jm_init(&objects);
	*(uint64_t *)private_page = UINT64_C(0x123456789abcdef0);
}
void
notified(microkit_channel ch)
{
	(void)ch;
}
microkit_msginfo protected(microkit_channel ch, microkit_msginfo info)
{
	uint64_t q[JM_WORDS], r[JM_WORDS] = {0};
	for (uint8_t i = 0; i < JM_WORDS; i++)
		q[i] = microkit_mr_get(i);
	size_t count = (size_t)microkit_msginfo_get_count(info);
	uint64_t label = microkit_msginfo_get_label(info);
	unsigned int caller = ch <= 2 ? ch : 3;
	if (!label && count == JM_WORDS && q[0] == JM_VERSION && !q[7] &&
	    q[1] >= 100) {
		r[0] = JM_INVALID;
		if (caller == 0 && q[1] == TEST_START && q[2] >= 1 &&
		    q[2] <= 2 && q[4] >= 1 && q[4] <= 14 && !q[3] && !q[6]) {
			unsigned int w = (unsigned int)q[2] - 1;
			phase[w] = q[4];
			argument[w] = q[5];
			report_world = 0;
			report_phase = 0;
			report_errors = 0;
			microkit_notify((microkit_channel)q[2]);
			r[0] = JM_OK;
		} else if (caller == 0 && q[1] == TEST_QUERY && !q[2] &&
		           !q[3] && !q[4] && !q[5] && !q[6]) {
			r[0] = JM_OK;
			r[1] = boot_mask;
			r[2] = report_phase;
			r[3] = report_errors;
			r[4] = token_a;
			r[5] = work_begin;
			r[6] = work_end;
			r[7] = report_world;
		} else if (caller >= 1 && caller <= 2) {
			unsigned int w = caller - 1;
			if (q[1] == TEST_HELLO && !q[2] && !q[3] && !q[4] &&
			    !q[5] && !q[6]) {
				boot_mask |= UINT64_C(1) << w;
				r[0] = JM_OK;
				microkit_notify(0);
			} else if (q[1] == TEST_PHASE && !q[2] && !q[3] &&
			           !q[4] && !q[5] && !q[6]) {
				r[0] = JM_OK;
				r[1] = phase[w];
				r[2] = argument[w];
			} else if (q[1] == TEST_REPORT && !q[2] && !q[3] &&
			           !q[6] && q[4] <= 1000) {
				report_world = caller;
				report_phase = phase[w];
				report_errors = q[4];
				if (caller == 1)
					token_a = q[5];
				r[0] = JM_OK;
				microkit_notify(0);
			}
		}
	} else {
		jm_dispatch(&objects, caller, label, count, q, r);
		if (r[0] == JM_OK && q[1] == JM_WORK && caller != 0) {
			work_begin = stamp();
			work_end = 0;
			microkit_notify(3);
			/* Bounded service work, executed on this PPC caller's
			 * context. */
			uint64_t sum = 1;
			for (uint64_t i = 0; i < UINT64_C(10000000); i++) {
				sum = sum * UINT64_C(6364136223846793005) + i;
				__asm__ __volatile__("" : "+r"(sum));
			}
			work_end = stamp();
			r[2] = sum;
		}
	}
	for (uint8_t i = 0; i < JM_WORDS; i++)
		microkit_mr_set(i, r[i]);
	return microkit_msginfo_new(0, JM_WORDS);
}
