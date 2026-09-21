/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. Trusted, bounded experiment controller.
 */
#include "experiment.h"
static uint64_t stage, expected_world, work_control;
static int failed, fault_seen;
static void
putc_serial(char c)
{
	for (unsigned int i = 0; i < 1000000; i++) {
		if (microkit_x86_ioport_read_8(0, 0x3fd) & 0x20) {
			microkit_x86_ioport_write_8(0, 0x3f8, (uint8_t)c);
			return;
		}
	}
}
static void
say(const char *s)
{
	while (*s)
		putc_serial(*s++);
	putc_serial('\n');
}
static void
number(uint64_t n)
{
	const char hex[] = "0123456789abcdef";
	for (unsigned int i = 0; i < 16; i++)
		putc_serial(hex[(n >> ((15 - i) * 4)) & 15]);
	putc_serial('\n');
}
static void
bad(void)
{
	failed = 1;
	say("M3 FAIL stage");
	number(stage);
}
static void
owner(uint64_t op, uint64_t target, uint64_t rev, uint64_t rights, uint64_t *r)
{
	uint64_t q[JM_WORDS] = {JM_VERSION, op, target, 0, rev, 0, rights};
	for (size_t i = 0; i < JM_WORDS; i++)
		r[i] = 0;
	if (failed)
		return;
	exchange(q, r, 0, JM_WORDS);
	if (r[0] != JM_OK)
		bad();
}
static void
start(uint64_t w, uint64_t next, uint64_t arg)
{
	uint64_t q[JM_WORDS] = {JM_VERSION, TEST_START, w, 0, next, arg},
	         r[JM_WORDS];
	if (failed)
		return;
	stage = next;
	expected_world = w;
	exchange(q, r, 0, JM_WORDS);
	if (r[0] != JM_OK)
		bad();
}
static void
inspect(uint64_t target, uint64_t rev, uint64_t content)
{
	uint64_t r[JM_WORDS];
	owner(JM_INSPECT, target, 0, 0, r);
	if (r[1] != rev || r[2] != content)
		bad();
}
void
init(void)
{
	microkit_x86_ioport_write_8(0, 0x3fb, 0x80);
	microkit_x86_ioport_write_8(0, 0x3f8, 1);
	microkit_x86_ioport_write_8(0, 0x3f9, 0);
	microkit_x86_ioport_write_8(0, 0x3fb, 3);
	microkit_x86_ioport_write_8(0, 0x3f9, 0);
	say("M3 BOOT CONTROL release");
}
void
notified(microkit_channel ch)
{
	uint64_t r[JM_WORDS], q[JM_WORDS] = {JM_VERSION, TEST_QUERY};
	const uint64_t a = (UINT64_C(1) << 32) | 1, b = (UINT64_C(2) << 32) | 2;
	if (failed)
		return;
	if (ch == 1) {
		work_control = stamp();
		return;
	}
	if (ch != 0)
		return;
	exchange(q, r, 0, JM_WORDS);
	if (r[0] != JM_OK) {
		bad();
		return;
	}
	if (stage == 0) {
		if (r[1] != 3)
			return;
		say("M3 BOOT OBJECT WORLD_A WORLD_B");
		say("M3 PASS A");
		owner(JM_GRANT, a, 0, 3, r);
		owner(JM_GRANT, b, 0, 1, r);
		start(1, 1, 0);
		return;
	}
	if (r[2] != stage || r[7] != expected_world)
		return;
	if (r[3]) {
		number(r[3]);
		bad();
		return;
	}
	switch (stage) {
	case 1:
		start(2, 2, r[4]);
		break;
	case 2:
		say("M3 PASS B");
		say("M3 PASS C");
		start(1, 3, 0);
		break;
	case 3:
		say("M3 PASS D");
		owner(JM_REVOKE, a, 0, 0, r);
		start(1, 4, 0);
		break;
	case 4:
		say("M3 PASS E");
		owner(JM_GRANT, a, 0, 3, r);
		start(1, 5, 0);
		break;
	case 5:
		say("M3 PASS F");
		owner(JM_ROTATE, 1, 0, 0, r);
		start(1, 6, 0);
		break;
	case 6:
		say("M3 PASS G");
		owner(JM_SAVE, a, 1, 0, r);
		owner(JM_REVOKE, a, 0, 0, r);
		owner(JM_RESTORE, 1, 0, 0, r);
		start(1, 7, 0);
		break;
	case 7:
		say("M3 PASS H");
		owner(JM_GRANT, a, 0, 1, r);
		owner(JM_ROTATE, 1, 0, 0, r);
		start(1, 8, 0);
		break;
	case 8:
		inspect(a, 1, 0xaaaa);
		owner(JM_GRANT, a, 0, 3, r);
		owner(JM_ROTATE, 1, 0, 0, r);
		start(1, 9, 0);
		break;
	case 9:
		inspect(a, 2, 0xcccc);
		say("M3 PASS J");
		owner(JM_ROTATE, 1, 0, 0, r);
		start(1, 10, 0);
		break;
	case 10:
		owner(JM_INSPECT, a, 0, 0, r);
		if (r[6] != JM_QUOTA || r[7] != JM_HANDLES)
			bad();
		owner(JM_REVOKE, a, 0, 0, r);
		owner(JM_GRANT, a, 0, 3, r);
		start(2, 11, 0);
		break;
	case 11:
		say("M3 PASS K");
		owner(JM_ROTATE, 1, 0, 0, r);
		work_control = 0;
		start(1, 12, 0);
		break;
	case 12:
		say("M3 SC timestamps begin/control/end");
		number(r[5]);
		number(work_control);
		number(r[6]);
		if (!(r[5] < work_control && work_control < r[6])) {
			bad();
			return;
		}
		say("M3 PASS caller-sc control-independent");
		stage = 13;
		microkit_notify(2);
		break;
	case 14:
		if (!fault_seen) {
			bad();
			return;
		}
		inspect(a, 2, 0xcccc);
		inspect(b, 1, 0xbbbb);
		if (!failed) {
			say("M3 PASS I");
			say("M3 PASS L");
			say("M3 COMPLETE PASS");
		}
		break;
	default:
		bad();
		break;
	}
}
seL4_Bool
fault(microkit_child child, microkit_msginfo info, microkit_msginfo *reply)
{
	(void)reply;
	if (child != 1 || stage != 13 ||
	    microkit_msginfo_get_label(info) != seL4_Fault_VMFault ||
	    microkit_msginfo_get_count(info) != seL4_VMFault_Length ||
	    microkit_mr_get(seL4_VMFault_PrefetchFault) != 0 ||
	    microkit_mr_get(seL4_VMFault_Addr) != UINT64_C(0x40000000)) {
		bad();
		return false;
	}
	say("M3 FAULT child=1 unmapped=0x40000000");
	microkit_pd_stop(child);
	fault_seen = 1;
	start(2, 14, 0);
	return false;
}
