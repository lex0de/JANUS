/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. Experimental M3 register protocol. */
#ifndef JANUS_SUBSTRATE_MODEL_H
#define JANUS_SUBSTRATE_MODEL_H
#include <stddef.h>
#include <stdint.h>
#define JM_WORDS 8u
#define JM_HANDLES 4u
#define JM_QUOTA 64u
#define JM_READ 1u
#define JM_WRITE 2u
#define JM_VERSION 1u
/* All messages have eight explicitly assigned 64-bit words, not struct bytes.
 */
enum jm_op {
	JM_ACQUIRE = 1,
	JM_GET,
	JM_PUT,
	JM_DELEGATE,
	JM_WORK,
	JM_GRANT = 32,
	JM_REVOKE,
	JM_ROTATE,
	JM_SAVE,
	JM_RESTORE,
	JM_INSPECT
};
enum jm_status {
	JM_OK,
	JM_INVALID,
	JM_DENIED,
	JM_STALE,
	JM_LIMIT,
	JM_CONFLICT,
	JM_UNSUPPORTED
};
struct jm_object {
	uint64_t revision, content;
};
struct jm_grant {
	uint32_t rights, generation;
};
struct jm_handle {
	uint64_t token;
	uint32_t object, incarnation, generation;
};
struct jm_world {
	uint32_t incarnation, requests, used;
	struct jm_handle handles[JM_HANDLES];
};
struct jm_state {
	struct jm_object objects[2];
	struct jm_grant grants[2][2];
	struct jm_world worlds[2];
	uint64_t next_token;
	uint32_t activity_world, activity_object;
	uint64_t activity_revision;
};
/* Caller 0 is CONTROL, 1/2 are channel-derived worlds; others are denied.
 * Caller owns state; single serial dispatcher; no retained message pointers.
 * Reply words: status, revision, content, token, incarnation, generation,
 * requests, used. Denied/conflicting writes never disclose content. */
void jm_init(struct jm_state *);
void jm_dispatch(struct jm_state *, unsigned int, uint64_t, size_t,
                 const uint64_t[JM_WORDS], uint64_t[JM_WORDS]);
#endif
