/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. Deliberate sacrificial isolation test. */
#include <microkit.h>
#include <stdint.h>
void
init(void)
{
}
void
notified(microkit_channel ch)
{
	if (ch != 0)
		return;
	uint64_t value;
	uintptr_t address = UINT64_C(0x40000000);
	/* Defined machine load experiment: address deliberately lacks a
	 * mapping. Inline assembly avoids a C optimiser deleting an
	 * intentionally bad access. */
	__asm__ __volatile__("movq (%1), %0"
	                     : "=r"(value)
	                     : "r"(address)
	                     : "memory");
	(void)value;
}
