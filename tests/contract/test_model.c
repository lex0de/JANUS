/* SPDX-License-Identifier: ISC */
/* JANUS - Danyal A. Samak <dabsamak@tuta.com>
 * tests/contract/test_model.c; specification oracle, see LICENSING.md. */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "janus/model.h"

static unsigned int checks;

#define CHECK(x)                                                               \
	do {                                                                   \
		checks++;                                                      \
		if (!(x)) {                                                    \
			fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__,          \
			        __LINE__, #x);                                 \
			exit(1);                                               \
		}                                                              \
	} while (0)

struct step {
	enum janus_command command;
	enum janus_actor actor;
	size_t world, target, slot, object, amount;
	unsigned int rights, delegable;
	enum janus_outcome outcome;
	uint64_t base;
	unsigned int fail;
	enum janus_result expected;
};

static void
fresh(struct janus_model *m)
{
	CHECK(janus_init(m, JANUS_NATIVE, JANUS_CHECKPOINTABLE, JANUS_VM,
	                 JANUS_RECONSTRUCTIBLE) == JANUS_OK);
}

static void
expect(struct janus_model *m, struct janus_request *r,
       enum janus_result expected)
{
	struct janus_model before;
	enum janus_result result;
	memcpy(&before, m, sizeof(before));
	result = janus_apply(m, r);
	if (result != expected)
		fprintf(stderr, "command %d: got %d expected %d\n", r->command,
		        result, expected);
	CHECK(result == expected);
	if (result != JANUS_OK)
		CHECK(memcmp(&before, m, sizeof(before)) == 0);
}

static void
steps(struct janus_model *m, const struct step *table, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++) {
		const struct step *s = &table[i];
		struct janus_request r = {
		    .command = s->command,
		    .actor = s->actor,
		    .world = s->world,
		    .target = s->target,
		    .slot = s->slot,
		    .object = s->object,
		    .amount = s->amount,
		    .rights = s->rights,
		    .delegable = s->delegable,
		    .outcome = s->outcome,
		    .base = s->base,
		    .fail = s->fail,
		    .incarnation = m->worlds[s->world].incarnation,
		    .sequence = m->sequence[s->actor] + 1};
		expect(m, &r, s->expected);
	}
}

#define RUN(m, t) steps((m), (t), sizeof(t) / sizeof((t)[0]))
#define S(c, a, w, e)                                                          \
	{.command = JANUS_##c,                                                 \
	 .actor = JANUS_##a,                                                   \
	 .world = w,                                                           \
	 .expected = JANUS_##e}
#define G(c, a, w, t, s, o, r, d, e)                                           \
	{.command = JANUS_##c,                                                 \
	 .actor = JANUS_##a,                                                   \
	 .world = w,                                                           \
	 .target = t,                                                          \
	 .slot = s,                                                            \
	 .object = o,                                                          \
	 .rights = r,                                                          \
	 .delegable = d,                                                       \
	 .expected = JANUS_##e}
#define B(w, s, o, r, baseval, e)                                              \
	{.command = JANUS_BEGIN,                                               \
	 .actor = (w) == 0 ? JANUS_WORLD0 : JANUS_WORLD1,                      \
	 .world = w,                                                           \
	 .slot = s,                                                            \
	 .object = o,                                                          \
	 .rights = r,                                                          \
	 .amount = 1,                                                          \
	 .base = baseval,                                                      \
	 .expected = JANUS_##e}
#define F(w, s, result, fault, e)                                              \
	{.command = JANUS_FINISH,                                              \
	 .actor = (w) == 0 ? JANUS_WORLD0 : JANUS_WORLD1,                      \
	 .world = w,                                                           \
	 .slot = s,                                                            \
	 .outcome = JANUS_##result,                                            \
	 .fail = fault,                                                        \
	 .expected = JANUS_##e}
#define Q(w, n, e)                                                             \
	{.command = JANUS_BUDGET,                                              \
	 .actor = JANUS_OWNER,                                                 \
	 .world = w,                                                           \
	 .amount = n,                                                          \
	 .expected = JANUS_##e}

static void
running(struct janus_model *m)
{
	static const struct step setup[] = {S(START, OWNER, 0, OK),
	                                    S(START, OWNER, 1, OK), Q(0, 6, OK),
	                                    Q(1, 6, OK)};
	fresh(m);
	RUN(m, setup);
}

static void
test_authority(void)
{
	struct janus_model m;
	static const struct step table[] = {
	    B(0, 0, 0, JANUS_READ, 0, DENIED),
	    G(ISSUE, WORLD0, 0, 0, 0, 0, 3, 0, DENIED),
	    G(ISSUE, OWNER, 0, 0, 0, 0, 0, 0, INVALID),
	    G(ISSUE, OWNER, 0, 0, 0, 0, 4, 0, INVALID),
	    G(ISSUE, OWNER, 0, 0, 0, 0, 3, 1, OK),
	    G(DELEGATE, WORLD0, 0, 1, 0, 1, 1, 0, DENIED),
	    G(DELEGATE, WORLD1, 1, 0, 0, 0, 1, 0, DENIED),
	    G(DELEGATE, WORLD0, 0, 1, 0, 0, 1, 1, OK),
	    G(DELEGATE, WORLD1, 1, 0, 1, 0, 3, 0, DENIED),
	    G(DELEGATE, WORLD1, 1, 0, 1, 0, 1, 0, OK),
	    B(0, 0, 1, JANUS_READ, 0, DENIED),
	    B(1, 1, 0, JANUS_WRITE, 1, DENIED),
	    B(1, 1, 0, JANUS_READ, 0, OK),
	    S(REVOKE_BEGIN, OWNER, 0, OK),
	    B(0, 2, 0, JANUS_READ, 0, DENIED),
	    B(1, 1, 0, JANUS_READ, 0, DENIED),
	    G(DELEGATE, WORLD1, 1, 0, 1, 0, 1, 0, DENIED),
	    S(REVOKE_COMPLETE, OWNER, 0, BUSY),
	    F(0, 0, DONE, 0, DENIED),
	    F(1, 0, DONE, 0, OK),
	    S(REVOKE_COMPLETE, OWNER, 0, OK),
	    S(REVOKE_COMPLETE, OWNER, 0, STATE),
	    S(REVOKE_BEGIN, OWNER, 0, STATE),
	    B(0, 0, 0, JANUS_READ, 0, DENIED),
	    F(1, 0, DONE, 0, STATE)};
	running(&m);
	RUN(&m, table);
	CHECK(m.revision[0] == 1 && m.grants[0].state == JANUS_REVOKED);
	puts("PASS authority: J-002 J-003 J-005; descendant drain and denial");
}

static void
test_delegation_policy(void)
{
	static const struct {
		unsigned int parent, child;
		enum janus_result expected;
	} policy[] = {{0, 0, JANUS_DENIED},
	              {0, 1, JANUS_DENIED},
	              {1, 0, JANUS_OK},
	              {1, 1, JANUS_OK}};
	size_t i;
	for (i = 0; i < sizeof(policy) / sizeof(policy[0]); i++) {
		struct janus_model m;
		struct janus_request r = {.command = JANUS_ISSUE,
		                          .actor = JANUS_OWNER,
		                          .incarnation = 1,
		                          .sequence = 100,
		                          .rights = JANUS_READ,
		                          .delegable = policy[i].parent};
		running(&m);
		expect(&m, &r, JANUS_OK);
		CHECK(m.grants[0].delegable == policy[i].parent);
		r.command = JANUS_DELEGATE;
		r.actor = JANUS_WORLD0;
		r.target = 1;
		r.delegable = policy[i].child;
		expect(&m, &r, policy[i].expected);
		if (policy[i].expected != JANUS_OK)
			continue;
		CHECK(m.grants[1].delegable == policy[i].child);
		CHECK(m.grants[1].rights == JANUS_READ &&
		      m.grants[1].parent == 0);
		r.world = 1;
		r.actor = JANUS_WORLD1;
		r.target = 0;
		r.slot = 1;
		r.delegable = 1;
		expect(&m, &r, policy[i].child ? JANUS_OK : JANUS_DENIED);
		/* Even a non-delegable child cannot exist below such a parent.
		 */
		m.grants[0].delegable = 0;
		r.command = JANUS_RECOVER;
		r.actor = JANUS_OWNER;
		r.sequence = 101;
		expect(&m, &r, JANUS_INVALID);
	}
	{
		struct janus_model m;
		struct janus_request r = {.command = JANUS_ISSUE,
		                          .actor = JANUS_OWNER,
		                          .incarnation = 1,
		                          .sequence = 100,
		                          .rights = JANUS_READ};
		running(&m);
		expect(&m, &r, JANUS_OK);
		CHECK(m.grants[0].delegable == 0);
		r.command = JANUS_BEGIN;
		r.actor = JANUS_WORLD0;
		r.amount = 1;
		r.delegable = 1;
		expect(&m, &r, JANUS_OK);
		CHECK(m.grants[0].delegable == 0);
		r.command = JANUS_DELEGATE;
		r.target = 1;
		r.sequence++;
		expect(&m, &r, JANUS_DENIED);
	}
	puts("PASS delegation policy: J-003; explicit authority, default deny, "
	     "attenuation");
}

static void
test_delegation_lifetime(void)
{
	struct janus_model m;
	static const struct step revocation[] = {
	    G(ISSUE, OWNER, 0, 0, 0, 0, 1, 1, OK),
	    G(DELEGATE, WORLD0, 0, 1, 0, 0, 3, 0, DENIED),
	    G(DELEGATE, WORLD0, 0, 1, 0, 0, 1, 1, OK),
	    S(REVOKE_BEGIN, OWNER, 0, OK),
	    G(DELEGATE, WORLD0, 0, 1, 0, 0, 1, 0, DENIED),
	    G(DELEGATE, WORLD1, 1, 0, 1, 0, 1, 1, DENIED),
	    S(REVOKE_COMPLETE, OWNER, 0, OK),
	    G(DELEGATE, WORLD0, 0, 1, 0, 0, 1, 1, DENIED),
	    G(DELEGATE, WORLD1, 1, 0, 1, 0, 1, 0, DENIED)};
	static const struct step restore[] = {
	    G(ISSUE, OWNER, 1, 0, 0, 0, 1, 1, OK),
	    G(DELEGATE, WORLD1, 1, 0, 0, 0, 1, 1, OK),
	    S(STOP, BACKEND, 1, OK),
	    S(RESTORE, OWNER, 1, OK),
	    G(DELEGATE, WORLD1, 1, 0, 0, 0, 1, 0, DENIED),
	    G(DELEGATE, WORLD0, 0, 1, 1, 0, 1, 1, DENIED)};
	running(&m);
	RUN(&m, revocation);
	running(&m);
	RUN(&m, restore);
	puts("PASS delegation lifetime: J-003 J-004 J-005 J-006");
}

static void
test_commit(void)
{
	struct janus_model m;
	static const struct step table[] = {
	    G(ISSUE, OWNER, 0, 0, 0, 0, 3, 0, OK),
	    B(0, 0, 0, JANUS_WRITE, 1, OK),
	    B(0, 0, 0, JANUS_WRITE, 1, OK),
	    F(0, 0, DONE, 1, IO),
	    S(REVOKE_BEGIN, OWNER, 0, OK),
	    F(0, 0, DONE, 0, OK),
	    F(0, 1, DONE, 0, CONFLICT),
	    S(REVOKE_COMPLETE, OWNER, 0, BUSY),
	    F(0, 1, CANCEL, 0, OK),
	    S(REVOKE_COMPLETE, OWNER, 0, OK),
	    G(ISSUE, OWNER, 0, 0, 0, 0, 3, 0, OK),
	    B(0, 1, 0, JANUS_WRITE, 2, OK),
	    F(0, 2, UNKNOWN, 0, OK),
	    F(0, 2, DONE, 0, STATE)};
	running(&m);
	RUN(&m, table);
	CHECK(m.revision[0] == 2 && m.revision[1] == 1);
	CHECK(m.operations[2].outcome == JANUS_UNKNOWN);
	puts("PASS commit: J-005 J-012 J-014; conflict, IO, uncertainty");
}

static void
test_restore(void)
{
	struct janus_model m;
	struct janus_request stale = {.command = JANUS_VIEW_OPEN,
	                              .actor = JANUS_BACKEND,
	                              .incarnation = 1,
	                              .sequence = 100};
	static const struct step table[] = {
	    G(ISSUE, OWNER, 0, 0, 0, 0, 3, 0, OK),
	    B(0, 0, 0, JANUS_WRITE, 1, OK),
	    F(0, 0, UNKNOWN, 0, OK),
	    S(QUIESCE, OWNER, 0, OK),
	    S(SUSPEND, BACKEND, 0, OK),
	    S(RESTORE, OWNER, 0, STATE),
	    S(CHECKPOINT, OWNER, 0, OK),
	    S(REVOKE_BEGIN, OWNER, 0, OK),
	    S(REVOKE_COMPLETE, OWNER, 0, OK),
	    {.command = JANUS_RESTORE,
	     .actor = JANUS_OWNER,
	     .fail = 1,
	     .expected = JANUS_IO},
	    S(RESTORE, OWNER, 0, OK),
	    B(0, 0, 0, JANUS_READ, 0, DENIED),
	    G(ISSUE, OWNER, 0, 0, 0, 0, 1, 0, OK),
	    B(0, 1, 0, JANUS_READ, 0, OK),
	    F(0, 1, DONE, 0, OK),
	    S(QUIESCE, OWNER, 1, OK),
	    S(SUSPEND, BACKEND, 1, OK),
	    S(CHECKPOINT, OWNER, 1, STATE),
	    S(RESTORE, OWNER, 1, OK)};
	running(&m);
	RUN(&m, table);
	CHECK(m.worlds[0].incarnation == 2 && m.worlds[1].incarnation == 2);
	CHECK(m.worlds[0].checkpoint == 1 && m.revision[0] == 1);
	CHECK(m.operations[0].outcome == JANUS_UNKNOWN);
	expect(&m, &stale, JANUS_STALE);
	/* An unrevoked saved grant is stale too, including its descendants. */
	m.worlds[0].state = JANUS_SUSPENDED;
	{
		static const struct step again[] = {S(RESTORE, OWNER, 0, OK),
		                                    B(0, 1, 0, 1, 0, DENIED)};
		RUN(&m, again);
	}
	puts("PASS restore: J-004 J-006 J-011 J-012 J-018");
}

static void
test_foreground_device(void)
{
	struct janus_model m;
	static const struct step table[] = {
	    S(VIEW_OPEN, BACKEND, 0, OK), S(FOCUS, OWNER, 0, OK),
	    B(0, 0, 0, 1, 0, DENIED),     S(RESERVE, OWNER, 0, OK),
	    S(DRAIN, BACKEND, 0, OK),     S(ASSIGN, BACKEND, 0, OK),
	    S(VIEW_LOST, BACKEND, 0, OK), S(VIEW_LOST, BACKEND, 0, STATE)};
	static const struct step recovery[] = {
	    S(VIEW_OPEN, BACKEND, 0, OK),  S(FOCUS, OWNER, 0, OK),
	    S(RECOVER, WORLD0, 0, DENIED), S(RECOVER, OWNER, 0, OK),
	    S(QUIESCE, OWNER, 0, OK),      S(SUSPEND, BACKEND, 0, OK),
	    S(CHECKPOINT, OWNER, 0, OK),   S(RESTORE, OWNER, 0, BUSY),
	    S(WITHDRAW, OWNER, 0, OK),     S(RESET, BACKEND, 0, OK),
	    S(RESET_FAIL, BACKEND, 0, OK), S(RESET_OK, BACKEND, 0, STATE),
	    S(RESERVE, OWNER, 1, STATE),   S(RESTORE, OWNER, 0, BUSY)};
	running(&m);
	RUN(&m, table);
	CHECK(m.worlds[0].state == JANUS_RUNNING && !m.worlds[0].viewer);
	CHECK(m.device == JANUS_ASSIGNED && m.device_owner == 0);
	CHECK(m.foreground == JANUS_NONE);
	RUN(&m, recovery);
	CHECK(m.device == JANUS_QUARANTINED && m.device_owner == 0);
	CHECK(m.worlds[0].checkpoint == 1 && m.worlds[0].incarnation == 1);
	puts("PASS foreground/device: J-007 J-008 J-010 J-013 J-018");
}

static void
test_limits(void)
{
	struct janus_model m;
	struct janus_request r = {.actor = JANUS_OWNER, .incarnation = 1};
	size_t i;
	static const struct step budget[] = {
	    Q(0, SIZE_MAX, LIMIT), Q(0, 7, LIMIT), Q(1, 0, OK), Q(0, 12, OK),
	    G(ISSUE, OWNER, 0, 0, 0, 0, 3, 0, OK)};
	running(&m);
	RUN(&m, budget);
	r.command = JANUS_ISSUE;
	r.rights = 1;
	for (i = 1; i <= JANUS_SLOTS; i++) {
		r.sequence = m.sequence[JANUS_OWNER] + 1;
		expect(&m, &r, i < JANUS_SLOTS ? JANUS_OK : JANUS_LIMIT);
	}
	r.actor = JANUS_WORLD0;
	r.command = JANUS_BEGIN;
	r.amount = 4096;
	for (i = 0; i <= JANUS_SLOTS; i++) {
		r.sequence = m.sequence[JANUS_WORLD0] + 1;
		expect(&m, &r, i < JANUS_SLOTS ? JANUS_OK : JANUS_LIMIT);
	}
	{
		static const struct step exhausted[] = {
		    Q(0, 0, BUSY),
		    S(QUIESCE, OWNER, 0, OK),
		    S(SUSPEND, BACKEND, 0, BUSY),
		    S(STOP, BACKEND, 0, BUSY),
		    S(RECOVER, OWNER, 0, OK),
		    F(0, 0, CANCEL, 0, OK)};
		RUN(&m, exhausted);
	}
	/* Lifetime operation slots stay exhausted even after cancellation. */
	m.worlds[0].state = JANUS_RUNNING;
	r.sequence = m.sequence[JANUS_WORLD0] + 1;
	expect(&m, &r, JANUS_LIMIT);
	running(&m);
	RUN(&m, budget);
	m.revision[0] = UINT64_MAX;
	r.base = UINT64_MAX;
	r.rights = JANUS_WRITE;
	r.sequence = 1;
	expect(&m, &r, JANUS_OK);
	r.command = JANUS_FINISH;
	r.outcome = JANUS_DONE;
	r.sequence++;
	expect(&m, &r, JANUS_LIMIT);
	fresh(&m);
	m.worlds[0].state = JANUS_SUSPENDED;
	m.worlds[0].checkpoint = UINT64_MAX;
	r = (struct janus_request){.command = JANUS_CHECKPOINT,
	                           .actor = JANUS_OWNER,
	                           .incarnation = 1,
	                           .sequence = 1};
	expect(&m, &r, JANUS_LIMIT);
	m.worlds[0].incarnation = UINT64_MAX;
	r.incarnation = UINT64_MAX;
	r.command = JANUS_RESTORE;
	expect(&m, &r, JANUS_LIMIT);
	fresh(&m);
	r.command = JANUS_START;
	r.incarnation = 1;
	r.sequence = UINT64_MAX;
	expect(&m, &r, JANUS_OK);
	r.command = JANUS_RECOVER;
	expect(&m, &r, JANUS_STALE);
	r.sequence = 0;
	expect(&m, &r, JANUS_STALE);
	puts("PASS limits: J-009 J-013 J-016; budgets, slots, counters");
}

static void
test_world_matrix(void)
{
	/* Rows: declared/running/quiescing/suspended/stopped/faulted.
	 * Columns: start/quiesce/suspend/stop/fault/checkpoint/restore.
	 * Independently transcribed from docs/contracts/world.md. */
	static const int next[6][7] = {
	    {1, -1, -1, -1, -1, -1, -1}, {-1, 2, -1, 4, 5, -1, -1},
	    {-1, -1, 3, 4, 5, -1, -1},   {-1, -1, -1, -1, 5, 3, 1},
	    {-1, -1, -1, -1, -1, -1, 1}, {-1, -1, -1, 4, -1, -1, 1}};
	static const enum janus_actor actors[] = {
	    JANUS_OWNER,   JANUS_OWNER, JANUS_BACKEND, JANUS_BACKEND,
	    JANUS_BACKEND, JANUS_OWNER, JANUS_OWNER};
	unsigned int state, command, kind, persistence;
	for (kind = 0; kind < 2; kind++) {
		for (persistence = 0; persistence < 3; persistence++) {
			for (state = 0; state < 6; state++) {
				for (command = 0; command < 7; command++) {
					struct janus_model m;
					struct janus_request r = {
					    .command =
					        (enum janus_command)command,
					    .actor = actors[command],
					    .sequence = 1,
					    .incarnation = 1};
					int want = next[state][command];
					fresh(&m);
					m.worlds[0].class =
					    (enum janus_class)kind;
					m.worlds[0].persistence =
					    (enum janus_persistence)persistence;
					m.worlds[0].state =
					    (enum janus_world_state)state;
					m.worlds[0].checkpoint =
					    persistence == 1 ? 1 : 0;
					if ((command == 5 &&
					     persistence != 1) ||
					    (command == 6 && persistence == 2))
						want = -1;
					expect(&m, &r,
					       want < 0 ? JANUS_STATE
					                : JANUS_OK);
					if (want >= 0)
						CHECK((int)m.worlds[0].state ==
						      want);
				}
			}
		}
	}
	puts("PASS world matrix: J-001 J-006 J-011; 252 transitions");
}

static void
test_device_matrix(void)
{
	/* Columns reserve/drain/assign/withdraw/reset/ok/fail. */
	static const int next[7][7] = {
	    {1, -1, -1, -1, -1, -1, -1}, {-1, 2, -1, 4, -1, -1, 6},
	    {-1, -1, 3, 4, -1, -1, 6},   {-1, -1, -1, 4, -1, -1, 6},
	    {-1, -1, -1, -1, 5, -1, 6},  {-1, -1, -1, -1, -1, 0, 6},
	    {-1, -1, -1, -1, -1, -1, 6}};
	unsigned int state, event;
	for (state = 0; state < 7; state++) {
		for (event = 0; event < 7; event++) {
			struct janus_model m;
			struct janus_request r = {
			    .command =
			        (enum janus_command)(JANUS_RESERVE + event),
			    .actor = event == 0 || event == 3 ? JANUS_OWNER
			                                      : JANUS_BACKEND,
			    .incarnation = 1,
			    .sequence = 50};
			running(&m);
			m.device = (enum janus_device_state)state;
			m.device_owner = state ? 0 : JANUS_NONE;
			m.device_incarnation = state ? 1 : 0;
			expect(&m, &r,
			       next[state][event] < 0 ? JANUS_STATE : JANUS_OK);
			if (next[state][event] >= 0)
				CHECK((int)m.device == next[state][event]);
		}
	}
	puts("PASS device matrix: J-010; 49 transitions");
}

static void
test_malformed(void)
{
	struct janus_model m, before;
	struct janus_request r;
	unsigned int command, field;
	fresh(&m);
	memcpy(&before, &m, sizeof(m));
	CHECK(janus_init(NULL, 0, 0, 0, 0) == JANUS_INVALID);
	CHECK(janus_init(&m, (enum janus_class) - 1, 0, 0, 0) == JANUS_INVALID);
	CHECK(janus_init(&m, 0, (enum janus_persistence) - 1, 0, 0) ==
	      JANUS_INVALID);
	CHECK(janus_init(&m, 0, 0, (enum janus_class)2, 0) == JANUS_INVALID);
	CHECK(janus_init(&m, 0, 0, 0, (enum janus_persistence)3) ==
	      JANUS_INVALID);
	CHECK(memcmp(&m, &before, sizeof(m)) == 0);
	CHECK(janus_apply(&m, NULL) == JANUS_INVALID);
	r = (struct janus_request){0};
	CHECK(janus_apply(NULL, &r) == JANUS_INVALID);
	for (command = 0; command < JANUS_COMMAND_COUNT; command++) {
		for (field = 0; field < 13; field++) {
			fresh(&m);
			r = (struct janus_request){
			    .command = (enum janus_command)command,
			    .incarnation = 1,
			    .sequence = 1};
			switch (field) {
			case 0:
				r.actor = (enum janus_actor) - 1;
				break;
			case 1:
				r.command = (enum janus_command) - 1;
				break;
			case 2:
				r.command = JANUS_COMMAND_COUNT;
				break;
			case 3:
				r.outcome = (enum janus_outcome) - 1;
				break;
			case 4:
				r.fail = 2;
				break;
			case 5:
				r.world = SIZE_MAX;
				break;
			case 6:
				r.target = JANUS_WORLDS;
				break;
			case 7:
				r.object = JANUS_OBJECTS;
				break;
			case 8:
				r.slot = JANUS_SLOTS;
				break;
			case 9:
				m.worlds[0].state =
				    (enum janus_world_state) - 1;
				break;
			case 10:
				m.device = (enum janus_device_state) - 1;
				break;
			case 11:
				r.delegable = 2;
				break;
			case 12:
				r.delegable = UINT_MAX;
				break;
			}
			expect(&m, &r, JANUS_INVALID);
		}
	}
	for (field = 0; field < 22; field++) {
		static const struct step grant[] = {
		    G(ISSUE, OWNER, 0, 0, 0, 0, 3, 0, OK),
		    B(0, 0, 0, 1, 0, OK)};
		running(&m);
		RUN(&m, grant);
		r = (struct janus_request){.command = JANUS_RECOVER,
		                           .sequence = 100,
		                           .incarnation = 1};
		switch (field) {
		case 0:
			m.worlds[0].class = (enum janus_class) - 1;
			break;
		case 1:
			m.worlds[0].persistence = (enum janus_persistence) - 1;
			break;
		case 2:
			m.worlds[0].viewer = 2;
			break;
		case 3:
			m.worlds[0].budget = SIZE_MAX;
			break;
		case 4:
			m.worlds[0].incarnation = 0;
			break;
		case 5:
			m.foreground = 0;
			break;
		case 6:
			m.device_owner = 0;
			break;
		case 7:
			m.revision[0] = 0;
			break;
		case 8:
			m.ngrants = SIZE_MAX;
			break;
		case 9:
			m.noperations = SIZE_MAX;
			break;
		case 10:
			m.grants[0].parent = 0;
			break;
		case 11:
			m.grants[0].state = (enum janus_grant_state) - 1;
			break;
		case 12:
			m.grants[0].rights = 4;
			break;
		case 13:
			m.grants[0].world = SIZE_MAX;
			break;
		case 14:
			m.operations[0].grant = SIZE_MAX;
			break;
		case 15:
			m.operations[0].outcome = (enum janus_outcome) - 1;
			break;
		case 16:
			m.operations[0].incarnation = 0;
			break;
		case 17:
			m.operations[0].right = 3;
			break;
		case 18:
			m.grants[0].state = JANUS_REVOKED;
			break;
		case 19:
			m.worlds[0].state = JANUS_STOPPED;
			break;
		case 20:
			m.grants[0].delegable = 2;
			break;
		case 21:
			m.grants[0].delegable = UINT_MAX;
			break;
		}
		expect(&m, &r, JANUS_INVALID);
	}
	puts("PASS malformed: enum/range/state rejection without mutation");
}

static void
test_actor_matrix(void)
{
	static const enum janus_actor allowed[JANUS_COMMAND_COUNT] = {
	    JANUS_OWNER,   JANUS_OWNER,  JANUS_BACKEND, JANUS_BACKEND,
	    JANUS_BACKEND, JANUS_OWNER,  JANUS_OWNER,   JANUS_OWNER,
	    JANUS_OWNER,   JANUS_WORLD0, JANUS_OWNER,   JANUS_OWNER,
	    JANUS_WORLD0,  JANUS_WORLD0, JANUS_BACKEND, JANUS_BACKEND,
	    JANUS_OWNER,   JANUS_OWNER,  JANUS_OWNER,   JANUS_BACKEND,
	    JANUS_BACKEND, JANUS_OWNER,  JANUS_BACKEND, JANUS_BACKEND,
	    JANUS_BACKEND};
	unsigned int command, actor;
	for (command = 0; command < JANUS_COMMAND_COUNT; command++) {
		for (actor = 0; actor < 4; actor++) {
			struct janus_model m;
			struct janus_request r = {
			    .command = (enum janus_command)command,
			    .actor = (enum janus_actor)actor,
			    .incarnation = 1,
			    .sequence = 100};
			if (r.actor == allowed[command])
				continue;
			running(&m);
			expect(&m, &r, JANUS_DENIED);
		}
	}
	puts("PASS actor matrix: every command rejects other caller roles");
}

static void
test_regressions(void)
{
	struct janus_model m;
	static const struct step table[] = {
	    Q(0, 0, OK),
	    G(ISSUE, OWNER, 0, 0, 0, 0, 3, 0, OK),
	    B(0, 0, 0, 1, 0, LIMIT),
	    Q(0, 1, OK),
	    B(0, 0, 0, 1, 0, OK),
	    B(0, 0, 0, 1, 0, LIMIT),
	    S(FAULT, BACKEND, 0, OK),
	    S(RECOVER, OWNER, 0, OK),
	    S(RESTORE, OWNER, 0, STATE),
	    F(0, 0, CANCEL, 0, OK),
	    S(STOP, BACKEND, 0, OK)};
	static const struct step delegate_restore[] = {
	    G(ISSUE, OWNER, 1, 0, 0, 0, 1, 1, OK),
	    G(DELEGATE, WORLD1, 1, 0, 0, 0, 1, 0, OK),
	    S(STOP, BACKEND, 1, OK),
	    S(RESTORE, OWNER, 1, OK),
	    B(0, 1, 0, 1, 0, DENIED),
	    S(RESERVE, OWNER, 0, OK),
	    S(DRAIN, BACKEND, 0, OK),
	    S(ASSIGN, BACKEND, 0, OK),
	    S(STOP, BACKEND, 0, OK)};
	static const struct step release[] = {
	    S(WITHDRAW, OWNER, 1, STALE), S(WITHDRAW, OWNER, 0, OK),
	    S(RESET, BACKEND, 0, OK), S(RESET_OK, BACKEND, 0, OK),
	    S(RESERVE, OWNER, 1, OK)};
	running(&m);
	RUN(&m, table);
	running(&m);
	RUN(&m, delegate_restore);
	CHECK(m.device == JANUS_ASSIGNED && m.worlds[0].state == JANUS_STOPPED);
	RUN(&m, release);
	CHECK(m.device_owner == 1 && m.device_incarnation == 2);
	puts("PASS regressions: zero budget, ancestor restore, stop retains "
	     "lease");
}

static void
test_seeded_invalid(void)
{
	/* Fixed regression seed; inputs always contain a specified invalid
	 * field. This is malformed-input stress, not coverage-guided fuzzing.
	 */
	uint32_t seed = 0x4a414e55u;
	unsigned int i;
	struct janus_model m;
	running(&m);
	for (i = 0; i < 2000; i++) {
		struct janus_request r = {.sequence = 100, .incarnation = 1};
		seed = seed * 1664525u + 1013904223u;
		r.command = (enum janus_command)(seed % JANUS_COMMAND_COUNT);
		r.actor = (enum janus_actor)((seed >> 8) % 4);
		r.world = (seed >> 10) % 2;
		switch ((seed >> 12) % 4) {
		case 0:
			r.slot = JANUS_SLOTS + (size_t)seed;
			break;
		case 1:
			r.object = JANUS_OBJECTS + (size_t)seed;
			break;
		case 2:
			r.world = JANUS_WORLDS + (size_t)seed;
			break;
		case 3:
			r.fail = 2;
			break;
		}
		expect(&m, &r, JANUS_INVALID);
	}
	puts("PASS seeded invalid inputs: seed 0x4a414e55, 2000 requests");
}

static void
test_lengths_replay(void)
{
	struct janus_model m;
	struct janus_request r;
	static const size_t lengths[] = {0, 4097, SIZE_MAX, 1, 4096};
	size_t i;
	static const struct step grant[] = {
	    G(ISSUE, OWNER, 0, 0, 0, 0, 1, 0, OK)};
	running(&m);
	RUN(&m, grant);
	for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
		r = (struct janus_request){.command = JANUS_BEGIN,
		                           .actor = JANUS_WORLD0,
		                           .rights = 1,
		                           .amount = lengths[i],
		                           .incarnation = 1,
		                           .sequence =
		                               m.sequence[JANUS_WORLD0] + 1};
		expect(&m, &r, i < 3 ? JANUS_INVALID : JANUS_OK);
	}
	expect(&m, &r, JANUS_STALE);
	r.sequence++;
	r.incarnation = 0;
	expect(&m, &r, JANUS_STALE);
	puts("PASS lengths/replay: J-009 J-016");
}

int
main(void)
{
	test_world_matrix();
	test_device_matrix();
	test_authority();
	test_delegation_policy();
	test_delegation_lifetime();
	test_commit();
	test_restore();
	test_foreground_device();
	test_limits();
	test_malformed();
	test_lengths_replay();
	test_actor_matrix();
	test_regressions();
	test_seeded_invalid();
	printf("PASS: %u assertions; reference model only\n", checks);
	return 0;
}
