/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. */
#include <limits.h>
#include "janus/substrate/model.h"
static void
reset_world(struct jm_world *w, uint32_t incarnation)
{
	w->incarnation = incarnation;
	w->requests = 0;
	w->used = 0;
	for (size_t i = 0; i < JM_HANDLES; i++) {
		w->handles[i].token = 0;
		w->handles[i].object = 0;
		w->handles[i].incarnation = 0;
		w->handles[i].generation = 0;
	}
}
void
jm_init(struct jm_state *s)
{
	s->activity_world = 0;
	s->activity_object = 0;
	s->activity_revision = 0;
	s->next_token = 1;
	for (size_t i = 0; i < 2; i++) {
		s->objects[i].revision = 1;
		s->objects[i].content = i ? 0xbbbb : 0xaaaa;
		reset_world(&s->worlds[i], 1);
		for (size_t j = 0; j < 2; j++) {
			s->grants[i][j].generation = 1;
			s->grants[i][j].rights = 0;
		}
	}
}
static int
shape(const uint64_t *q)
{
	if (q[0] != JM_VERSION || q[7])
		return 0;
	switch (q[1]) {
	case JM_ACQUIRE:
		return q[2] >= 1 && q[2] <= 2 && !q[3] && !q[4] && !q[5] &&
		       !q[6];
	case JM_GET:
		return !q[2] && q[3] && !q[4] && !q[5] && !q[6];
	case JM_PUT:
		return !q[2] && q[3] && q[4] && q[4] <= UINT32_MAX && !q[6];
	case JM_DELEGATE:
	case JM_WORK:
		return !q[2] && !q[3] && !q[4] && !q[5] && !q[6];
	case JM_GRANT:
	case JM_REVOKE:
	case JM_SAVE:
	case JM_INSPECT:
		if ((q[2] >> 32) < 1 || (q[2] >> 32) > 2 ||
		    (uint32_t)q[2] < 1 || (uint32_t)q[2] > 2 || q[3] || q[5])
			return 0;
		if (q[1] == JM_GRANT)
			return q[6] >= 1 && q[6] <= 7 && !q[4];
		if (q[1] == JM_SAVE)
			return q[4] >= 1 && q[4] <= UINT32_MAX && !q[6];
		return !q[4] && !q[6];
	case JM_ROTATE:
	case JM_RESTORE:
		return q[2] >= 1 && q[2] <= 2 && !q[3] && !q[4] && !q[5] &&
		       !q[6];
	default:
		return 0;
	}
}
static unsigned int
owner(struct jm_state *s, const uint64_t *q, uint64_t *r)
{
	unsigned int op = (unsigned int)q[1];
	if (op < JM_GRANT)
		return JM_DENIED;
	if (op == JM_ROTATE || op == JM_RESTORE) {
		unsigned int w = (unsigned int)q[2] - 1;
		if (op == JM_RESTORE && s->activity_world != w + 1)
			return JM_DENIED;
		if (s->worlds[w].incarnation == UINT32_MAX)
			return JM_LIMIT;
		uint32_t next = s->worlds[w].incarnation + 1;
		reset_world(&s->worlds[w], next);
		r[4] = next;
		if (op == JM_RESTORE) {
			r[1] = s->activity_revision;
			r[2] = s->activity_object;
		}
		return JM_OK;
	}
	unsigned int w = (unsigned int)(q[2] >> 32) - 1,
	             o = (unsigned int)(uint32_t)q[2] - 1;
	struct jm_grant *g = &s->grants[w][o];
	if (op == JM_INSPECT) {
		r[1] = s->objects[o].revision;
		r[2] = s->objects[o].content;
		r[3] = g->rights;
		r[4] = s->worlds[w].incarnation;
		r[5] = g->generation;
		r[6] = s->worlds[w].requests;
		r[7] = s->worlds[w].used;
		return JM_OK;
	}
	if (op == JM_SAVE) {
		if (q[4] != s->objects[o].revision)
			return JM_CONFLICT;
		s->activity_world = w + 1;
		s->activity_object = o + 1;
		s->activity_revision = q[4];
		return JM_OK;
	}
	if (op == JM_GRANT) {
		if (q[6] & 4u)
			return JM_UNSUPPORTED;
		if (g->generation == UINT32_MAX)
			return JM_LIMIT;
		g->generation++;
		g->rights = (uint32_t)q[6];
		return JM_OK;
	}
	if (op == JM_REVOKE) {
		g->rights = 0;
		if (g->generation < UINT32_MAX)
			g->generation++;
		return JM_OK;
	}
	return JM_INVALID;
}
static unsigned int
world(struct jm_state *s, unsigned int caller, const uint64_t *q, uint64_t *r)
{
	unsigned int w = caller - 1;
	struct jm_world *world = &s->worlds[w];
	if (world->requests >= JM_QUOTA)
		return JM_LIMIT;
	world->requests++;
	if (q[1] >= JM_GRANT)
		return JM_DENIED;
	if (q[1] == JM_DELEGATE)
		return JM_UNSUPPORTED;
	if (q[1] == JM_WORK)
		return JM_OK;
	if (q[1] == JM_ACQUIRE) {
		unsigned int o = (unsigned int)q[2] - 1;
		struct jm_grant *g = &s->grants[w][o];
		if (!g->rights)
			return JM_DENIED;
		if (world->used == JM_HANDLES || s->next_token == UINT64_MAX)
			return JM_LIMIT;
		struct jm_handle h = {.token = s->next_token++,
		                      .object = o,
		                      .incarnation = world->incarnation,
		                      .generation = g->generation};
		world->handles[world->used++] = h;
		r[3] = h.token;
		r[4] = h.incarnation;
		r[5] = h.generation;
		return JM_OK;
	}
	struct jm_handle *h = NULL;
	for (size_t i = 0; i < world->used; i++)
		if (world->handles[i].token == q[3]) {
			h = &world->handles[i];
			break;
		}
	if (!h || h->incarnation != world->incarnation)
		return JM_STALE;
	struct jm_grant *g = &s->grants[w][h->object];
	if (h->generation != g->generation)
		return JM_STALE;
	if (!g->rights)
		return JM_DENIED;
	struct jm_object *o = &s->objects[h->object];
	if (q[1] == JM_GET) {
		if (!(g->rights & JM_READ))
			return JM_DENIED;
		r[1] = o->revision;
		r[2] = o->content;
		return JM_OK;
	}
	if (!(g->rights & JM_WRITE))
		return JM_DENIED;
	if (q[4] != o->revision) {
		r[1] = o->revision;
		return JM_CONFLICT;
	}
	if (o->revision == UINT32_MAX)
		return JM_LIMIT;
	o->content = q[5];
	o->revision++;
	r[1] = o->revision;
	return JM_OK;
}
void
jm_dispatch(struct jm_state *s, unsigned int caller, uint64_t label,
            size_t count, const uint64_t q[JM_WORDS], uint64_t r[JM_WORDS])
{
	for (size_t i = 0; i < JM_WORDS; i++)
		r[i] = 0;
	if (caller > 2) {
		r[0] = JM_DENIED;
		return;
	}
	if (label || count != JM_WORDS || !shape(q)) {
		r[0] = JM_INVALID;
		return;
	}
	r[0] = caller == 0 ? owner(s, q, r) : world(s, caller, q, r);
}
