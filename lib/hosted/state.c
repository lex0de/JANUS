/* SPDX-License-Identifier: ISC */
/* JANUS serial hosted state; Copyright (c) 2026 Danyal A. Samak. */
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "janus/hosted/hosted.h"

int
jh_refresh_viewer(struct jh_world *w, const struct jh_adapters *a)
{
	if (w->viewer > 0) {
		int alive = a->alive(a->arg, w->viewer);
		if (alive <= 0) {
			w->viewer = 0;
			w->foreground = 0;
			return alive;
		}
	}
	return 0;
}

static int
store(struct jh_world *w, const struct jh_adapters *a, struct jh_record *r)
{
	if (a->save(a->arg, &w->profile, r)) {
		w->record.phase = JH_BAD;
		return -1;
	}
	w->record = *r;
	return 0;
}

const char *
jh_handle(struct jh_world *w, enum jh_command command,
          const struct jh_adapters *a)
{
	struct jh_record next;
	pid_t child;
	if ((unsigned int)command > JH_RECOVER)
		return "invalid";
	(void)jh_refresh_viewer(w, a);
	if (command == JH_LEAVE || command == JH_RECOVER) {
		w->foreground = 0;
		if (w->viewer > 0) {
			if (a->stop(a->arg, w->viewer))
				return "viewer-error";
			w->viewer = 0;
		}
	}
	w->observed.execution = JH_UNKNOWN;
	w->observed.epoch[0] = '\0';
	if (a->backend(a->arg, &w->profile, 0, &w->observed) ||
	    (unsigned int)w->observed.execution > JH_PAUSED) {
		w->observed.execution = JH_UNKNOWN;
		return "backend-error";
	}
	if (w->record.phase == JH_BAD)
		return "reconciliation-required";
	if (w->observed.execution == JH_INACTIVE) {
		if (w->viewer > 0) {
			w->foreground = 0;
			if (a->stop(a->arg, w->viewer))
				return "viewer-error";
			w->viewer = 0;
		}
		if (w->record.phase != JH_STOPPED) {
			next = w->record;
			next.phase = JH_STOPPED;
			next.epoch[0] = '\0';
			if (store(w, a, &next))
				return "record-error";
		}
	} else if (w->record.phase != JH_STARTED || !w->observed.epoch[0] ||
	           strcmp(w->record.epoch, w->observed.epoch)) {
		/* Old presentation cannot stand for an unidentified execution.
		 */
		w->foreground = 0;
		if (w->viewer > 0 && a->stop(a->arg, w->viewer) == 0)
			w->viewer = 0;
		return "reconciliation-required";
	}
	if (command != JH_RUN)
		return "ok";
	if (w->observed.execution == JH_PAUSED)
		return "paused";
	if (w->observed.execution == JH_INACTIVE) {
		if (w->record.incarnation == UINT64_MAX)
			return "limit";
		next = w->record;
		next.incarnation++;
		next.phase = JH_STARTING;
		if (store(w, a, &next))
			return "record-error";
		/* Adapter rechecks UUID/inactive state before creating
		 * execution. */
		if (a->backend(a->arg, &w->profile, 1, &w->observed) ||
		    w->observed.execution != JH_ACTIVE ||
		    !w->observed.epoch[0]) {
			w->observed.execution = JH_UNKNOWN;
			return "start-uncertain";
		}
		next.phase = JH_STARTED;
		memcpy(next.epoch, w->observed.epoch, sizeof(next.epoch));
		if (store(w, a, &next))
			return "record-error";
	}
	/* A failed cleanup cannot be promoted back into presentation ownership.
	 */
	if (w->viewer > 0 && !w->foreground) {
		if (a->stop(a->arg, w->viewer))
			return "viewer-error";
		w->viewer = 0;
	}
	if (!w->viewer) {
		child = 0;
		if (a->spawn(a->arg, &w->profile, &child) || child <= 0) {
			if (child > 0)
				w->viewer = child;
			return "viewer-error";
		}
		w->viewer = child;
	}
	w->foreground = 1;
	return "ok";
}

int
jh_reply(char *buf, size_t n, const struct jh_world *w, const char *result)
{
	static const char *states[] = {"inactive", "active", "paused",
	                               "unknown"};
	if ((unsigned int)w->observed.execution > JH_UNKNOWN)
		return -1;
	int k =
	    snprintf(buf, n,
	             "%s profile=%s uuid=%s execution=%s incarnation=%" PRIu64
	             " viewer=%ld foreground=%d\n",
	             result, w->profile.id, w->profile.uuid,
	             states[w->observed.execution], w->record.incarnation,
	             (long)w->viewer, w->foreground);
	return k < 0 || (size_t)k >= n ? -1 : k;
}
