/* JANUS - Danyal A. Samak <dabsamak@tuta.com>
 * include/janus/model.h; EXPERIMENTAL model, see LICENSING.md. */
#ifndef JANUS_MODEL_H
#define JANUS_MODEL_H

#include <stddef.h>
#include <stdint.h>

#define JANUS_WORLDS 2
#define JANUS_OBJECTS 2
#define JANUS_SLOTS 8
#define JANUS_NONE JANUS_SLOTS
#define JANUS_WORK 12
#define JANUS_READ 1u
#define JANUS_WRITE 2u

enum janus_result {
	JANUS_OK,
	JANUS_INVALID,
	JANUS_DENIED,
	JANUS_STALE,
	JANUS_STATE,
	JANUS_LIMIT,
	JANUS_BUSY,
	JANUS_CONFLICT,
	JANUS_IO
};
enum janus_class { JANUS_NATIVE, JANUS_VM };
enum janus_persistence {
	JANUS_RECONSTRUCTIBLE,
	JANUS_CHECKPOINTABLE,
	JANUS_EPHEMERAL
};
enum janus_actor { JANUS_OWNER, JANUS_BACKEND, JANUS_WORLD0, JANUS_WORLD1 };
enum janus_world_state {
	JANUS_DECLARED,
	JANUS_RUNNING,
	JANUS_QUIESCING,
	JANUS_SUSPENDED,
	JANUS_STOPPED,
	JANUS_FAULTED
};
enum janus_grant_state { JANUS_ACTIVE, JANUS_REVOKING, JANUS_REVOKED };
enum janus_device_state {
	JANUS_AVAILABLE,
	JANUS_RESERVED,
	JANUS_D_QUIESCING,
	JANUS_ASSIGNED,
	JANUS_D_REVOKING,
	JANUS_RESETTING,
	JANUS_QUARANTINED
};
enum janus_outcome { JANUS_PENDING, JANUS_DONE, JANUS_CANCEL, JANUS_UNKNOWN };
enum janus_command {
	JANUS_START,
	JANUS_QUIESCE,
	JANUS_SUSPEND,
	JANUS_STOP,
	JANUS_FAULT,
	JANUS_CHECKPOINT,
	JANUS_RESTORE,
	JANUS_BUDGET,
	JANUS_ISSUE,
	JANUS_DELEGATE,
	JANUS_REVOKE_BEGIN,
	JANUS_REVOKE_COMPLETE,
	JANUS_BEGIN,
	JANUS_FINISH,
	JANUS_VIEW_OPEN,
	JANUS_VIEW_LOST,
	JANUS_FOCUS,
	JANUS_RECOVER,
	JANUS_RESERVE,
	JANUS_DRAIN,
	JANUS_ASSIGN,
	JANUS_WITHDRAW,
	JANUS_RESET,
	JANUS_RESET_OK,
	JANUS_RESET_FAIL,
	JANUS_COMMAND_COUNT
};

struct janus_world {
	enum janus_class class;
	enum janus_persistence persistence;
	enum janus_world_state state;
	uint64_t incarnation, checkpoint;
	size_t budget;
	unsigned int viewer;
};
struct janus_grant {
	size_t world, object, parent;
	uint64_t incarnation;
	unsigned int rights;
	enum janus_grant_state state;
};
struct janus_operation {
	size_t grant, world, object;
	uint64_t incarnation, base;
	unsigned int right;
	enum janus_outcome outcome;
};
struct janus_model {
	struct janus_world worlds[JANUS_WORLDS];
	struct janus_grant grants[JANUS_SLOTS];
	struct janus_operation operations[JANUS_SLOTS];
	size_t ngrants, noperations, foreground, device_owner;
	uint64_t revision[JANUS_OBJECTS], sequence[4], device_incarnation;
	enum janus_device_state device;
};
struct janus_request {
	enum janus_command command;
	enum janus_actor actor;
	size_t world, target, object, slot, amount;
	uint64_t incarnation, sequence, base;
	unsigned int rights, fail;
	enum janus_outcome outcome;
};

#ifdef __cplusplus
extern "C" {
#endif
/* Caller-owned, non-overlapping storage; calls must be serialised. */
enum janus_result janus_init(struct janus_model *, enum janus_class,
                             enum janus_persistence, enum janus_class,
                             enum janus_persistence);
enum janus_result janus_apply(struct janus_model *,
                              const struct janus_request *);
#ifdef __cplusplus
}
#endif
#endif
