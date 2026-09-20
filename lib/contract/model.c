/* JANUS - Danyal A. Samak <dabsamak@tuta.com>
 * lib/contract/model.c; EXPERIMENTAL model, see LICENSING.md. */
#include <string.h>

#include "janus/model.h"

static size_t
pending(const struct janus_model *m, size_t world)
{
	size_t i, n = 0;
	for (i = 0; i < m->noperations; i++) {
		if (m->operations[i].world == world &&
		    m->operations[i].outcome == JANUS_PENDING)
			n++;
	}
	return n;
}

static int
valid(const struct janus_model *m)
{
	size_t i, slot;
	if (m->ngrants > JANUS_SLOTS || m->noperations > JANUS_SLOTS ||
	    (m->foreground != JANUS_NONE && m->foreground >= JANUS_WORLDS) ||
	    (unsigned int)m->device > JANUS_QUARANTINED)
		return 0;
	for (i = 0; i < JANUS_WORLDS; i++) {
		const struct janus_world *w = &m->worlds[i];
		if ((unsigned int)w->class > JANUS_VM ||
		    (unsigned int)w->persistence > JANUS_EPHEMERAL ||
		    (unsigned int)w->state > JANUS_FAULTED ||
		    w->incarnation == 0 || w->budget > JANUS_WORK ||
		    w->viewer > 1 ||
		    (w->viewer && w->state != JANUS_RUNNING &&
		     w->state != JANUS_QUIESCING) ||
		    (w->checkpoint && w->persistence != JANUS_CHECKPOINTABLE))
			return 0;
	}
	if (m->worlds[0].budget + m->worlds[1].budget > JANUS_WORK ||
	    (m->foreground != JANUS_NONE &&
	     (!m->worlds[m->foreground].viewer ||
	      m->worlds[m->foreground].state != JANUS_RUNNING)))
		return 0;
	if (m->device == JANUS_AVAILABLE) {
		if (m->device_owner != JANUS_NONE || m->device_incarnation != 0)
			return 0;
	} else if (m->device_owner >= JANUS_WORLDS ||
	           m->device_incarnation !=
	               m->worlds[m->device_owner].incarnation)
		return 0;
	for (i = 0; i < JANUS_OBJECTS; i++) {
		if (m->revision[i] == 0)
			return 0;
	}
	for (i = 0; i < m->ngrants; i++) {
		const struct janus_grant *g = &m->grants[i];
		if (g->world >= JANUS_WORLDS || g->object >= JANUS_OBJECTS ||
		    !g->incarnation ||
		    g->incarnation > m->worlds[g->world].incarnation ||
		    !g->rights || (g->rights & ~3u) ||
		    (unsigned int)g->state > JANUS_REVOKED ||
		    (g->parent != JANUS_NONE && g->parent >= i))
			return 0;
		if (g->parent != JANUS_NONE &&
		    (g->object != m->grants[g->parent].object ||
		     (g->rights & ~m->grants[g->parent].rights)))
			return 0;
	}
	for (i = 0; i < m->noperations; i++) {
		const struct janus_operation *o = &m->operations[i];
		if (o->grant >= m->ngrants || o->world >= JANUS_WORLDS ||
		    o->object >= JANUS_OBJECTS ||
		    (unsigned int)o->outcome > JANUS_UNKNOWN ||
		    (o->right != JANUS_READ && o->right != JANUS_WRITE) ||
		    o->world != m->grants[o->grant].world ||
		    o->object != m->grants[o->grant].object ||
		    o->incarnation != m->grants[o->grant].incarnation ||
		    !(o->right & m->grants[o->grant].rights) ||
		    (o->outcome == JANUS_PENDING &&
		     o->incarnation != m->worlds[o->world].incarnation))
			return 0;
		if (o->outcome == JANUS_PENDING) {
			if (m->worlds[o->world].state == JANUS_DECLARED ||
			    m->worlds[o->world].state == JANUS_SUSPENDED ||
			    m->worlds[o->world].state == JANUS_STOPPED)
				return 0;
			for (slot = o->grant; slot != JANUS_NONE;
			     slot = m->grants[slot].parent) {
				if (m->grants[slot].state == JANUS_REVOKED)
					return 0;
			}
		}
	}
	for (i = 0; i < JANUS_WORLDS; i++) {
		if (pending(m, i) > m->worlds[i].budget)
			return 0;
	}
	return 1;
}

enum janus_result
janus_init(struct janus_model *m, enum janus_class c0,
           enum janus_persistence p0, enum janus_class c1,
           enum janus_persistence p1)
{
	struct janus_model n;
	size_t i;
	if (m == NULL || (unsigned int)c0 > JANUS_VM ||
	    (unsigned int)c1 > JANUS_VM || (unsigned int)p0 > JANUS_EPHEMERAL ||
	    (unsigned int)p1 > JANUS_EPHEMERAL)
		return JANUS_INVALID;
	memset(&n, 0, sizeof(n));
	n.worlds[0].class = c0;
	n.worlds[1].class = c1;
	n.worlds[0].persistence = p0;
	n.worlds[1].persistence = p1;
	for (i = 0; i < JANUS_WORLDS; i++)
		n.worlds[i].incarnation = 1;
	for (i = 0; i < JANUS_OBJECTS; i++)
		n.revision[i] = 1;
	n.foreground = n.device_owner = JANUS_NONE;
	memcpy(m, &n, sizeof(n));
	return JANUS_OK;
}

static int
live(const struct janus_model *m, size_t slot)
{
	while (slot != JANUS_NONE) {
		const struct janus_grant *g = &m->grants[slot];
		if (g->state != JANUS_ACTIVE ||
		    g->incarnation != m->worlds[g->world].incarnation)
			return 0;
		slot = g->parent;
	}
	return 1;
}

static int
descendant(const struct janus_model *m, size_t slot, size_t ancestor)
{
	while (slot != JANUS_NONE) {
		if (slot == ancestor)
			return 1;
		slot = m->grants[slot].parent;
	}
	return 0;
}

static enum janus_actor
actor_for(enum janus_command command, size_t world)
{
	switch (command) {
	case JANUS_SUSPEND:
	case JANUS_STOP:
	case JANUS_FAULT:
	case JANUS_VIEW_OPEN:
	case JANUS_VIEW_LOST:
	case JANUS_DRAIN:
	case JANUS_ASSIGN:
	case JANUS_RESET:
	case JANUS_RESET_OK:
	case JANUS_RESET_FAIL:
		return JANUS_BACKEND;
	case JANUS_DELEGATE:
	case JANUS_BEGIN:
	case JANUS_FINISH:
		return world == 0 ? JANUS_WORLD0 : JANUS_WORLD1;
	default:
		return JANUS_OWNER;
	}
}

static enum janus_result
authority(struct janus_model *m, const struct janus_request *r)
{
	struct janus_grant *g;
	struct janus_operation *o;
	size_t i, recipient = r->world, parent = JANUS_NONE;
	if (r->command == JANUS_ISSUE || r->command == JANUS_DELEGATE) {
		if (!r->rights || (r->rights & ~3u))
			return JANUS_INVALID;
		if (r->command == JANUS_DELEGATE) {
			if (r->slot >= m->ngrants)
				return JANUS_DENIED;
			g = &m->grants[r->slot];
			if (g->world != r->world || !live(m, r->slot) ||
			    g->object != r->object || (r->rights & ~g->rights))
				return JANUS_DENIED;
			recipient = r->target;
			parent = r->slot;
		}
		if (m->worlds[r->world].state != JANUS_RUNNING ||
		    m->worlds[recipient].state != JANUS_RUNNING)
			return JANUS_STATE;
		if (m->ngrants == JANUS_SLOTS)
			return JANUS_LIMIT;
		g = &m->grants[m->ngrants++];
		g->world = recipient;
		g->object = r->object;
		g->parent = parent;
		g->incarnation = m->worlds[recipient].incarnation;
		g->rights = r->rights;
		g->state = JANUS_ACTIVE;
		return JANUS_OK;
	}
	if (r->command == JANUS_FINISH) {
		if (r->slot >= m->noperations)
			return JANUS_INVALID;
		o = &m->operations[r->slot];
		if (o->world != r->world || o->incarnation != r->incarnation)
			return JANUS_DENIED;
		if (o->outcome != JANUS_PENDING || r->outcome == JANUS_PENDING)
			return JANUS_STATE;
		if (r->outcome == JANUS_DONE && o->right == JANUS_WRITE) {
			if (o->base != m->revision[o->object])
				return JANUS_CONFLICT;
			if (m->revision[o->object] == UINT64_MAX)
				return JANUS_LIMIT;
			if (r->fail)
				return JANUS_IO;
			m->revision[o->object]++;
		}
		o->outcome = r->outcome;
		return JANUS_OK;
	}
	if (r->slot >= m->ngrants)
		return JANUS_DENIED;
	g = &m->grants[r->slot];
	if (g->world != r->world)
		return JANUS_DENIED;
	if (r->command == JANUS_REVOKE_BEGIN) {
		if (g->state != JANUS_ACTIVE)
			return JANUS_STATE;
		g->state = JANUS_REVOKING;
		return JANUS_OK;
	}
	if (r->command == JANUS_REVOKE_COMPLETE) {
		if (g->state != JANUS_REVOKING)
			return JANUS_STATE;
		for (i = 0; i < m->noperations; i++) {
			if (m->operations[i].outcome == JANUS_PENDING &&
			    descendant(m, m->operations[i].grant, r->slot))
				return JANUS_BUSY;
		}
		g->state = JANUS_REVOKED;
		return JANUS_OK;
	}
	if (r->rights != JANUS_READ && r->rights != JANUS_WRITE)
		return JANUS_INVALID;
	if (!r->amount || r->amount > 4096)
		return JANUS_INVALID;
	if (!live(m, r->slot) || g->object != r->object ||
	    !(g->rights & r->rights))
		return JANUS_DENIED;
	if (m->worlds[r->world].state != JANUS_RUNNING)
		return JANUS_STATE;
	if (m->noperations == JANUS_SLOTS ||
	    pending(m, r->world) >= m->worlds[r->world].budget)
		return JANUS_LIMIT;
	o = &m->operations[m->noperations++];
	o->grant = r->slot;
	o->world = r->world;
	o->object = r->object;
	o->incarnation = r->incarnation;
	o->base = r->base;
	o->right = r->rights;
	o->outcome = JANUS_PENDING;
	return JANUS_OK;
}

static enum janus_result
device(struct janus_model *m, const struct janus_request *r)
{
	if (r->command == JANUS_RESERVE) {
		if (m->device != JANUS_AVAILABLE ||
		    m->worlds[r->world].state != JANUS_RUNNING)
			return JANUS_STATE;
		m->device = JANUS_RESERVED;
		m->device_owner = r->world;
		m->device_incarnation = r->incarnation;
		return JANUS_OK;
	}
	if (m->device == JANUS_AVAILABLE)
		return JANUS_STATE;
	if (m->device_owner != r->world ||
	    m->device_incarnation != r->incarnation)
		return JANUS_STALE;
	switch (r->command) {
	case JANUS_DRAIN:
		if (m->device != JANUS_RESERVED)
			return JANUS_STATE;
		m->device = JANUS_D_QUIESCING;
		break;
	case JANUS_ASSIGN:
		if (m->device != JANUS_D_QUIESCING ||
		    m->worlds[r->world].state != JANUS_RUNNING)
			return JANUS_STATE;
		m->device = JANUS_ASSIGNED;
		break;
	case JANUS_WITHDRAW:
		if (m->device != JANUS_RESERVED &&
		    m->device != JANUS_D_QUIESCING &&
		    m->device != JANUS_ASSIGNED)
			return JANUS_STATE;
		m->device = JANUS_D_REVOKING;
		break;
	case JANUS_RESET:
		if (m->device != JANUS_D_REVOKING)
			return JANUS_STATE;
		m->device = JANUS_RESETTING;
		break;
	case JANUS_RESET_OK:
		if (m->device != JANUS_RESETTING)
			return JANUS_STATE;
		m->device = JANUS_AVAILABLE;
		m->device_owner = JANUS_NONE;
		m->device_incarnation = 0;
		break;
	case JANUS_RESET_FAIL:
		m->device = JANUS_QUARANTINED;
		break;
	default:
		return JANUS_INVALID;
	}
	return JANUS_OK;
}

static enum janus_result
transition(struct janus_model *m, const struct janus_request *r)
{
	struct janus_world *w = &m->worlds[r->world];
	if (r->command >= JANUS_ISSUE && r->command <= JANUS_FINISH)
		return authority(m, r);
	if (r->command >= JANUS_RESERVE)
		return device(m, r);
	switch (r->command) {
	case JANUS_START:
		if (w->state != JANUS_DECLARED)
			return JANUS_STATE;
		w->state = JANUS_RUNNING;
		break;
	case JANUS_QUIESCE:
		if (w->state != JANUS_RUNNING)
			return JANUS_STATE;
		w->state = JANUS_QUIESCING;
		if (m->foreground == r->world)
			m->foreground = JANUS_NONE;
		break;
	case JANUS_SUSPEND:
		if (w->state != JANUS_QUIESCING)
			return JANUS_STATE;
		if (pending(m, r->world))
			return JANUS_BUSY;
		w->state = JANUS_SUSPENDED;
		w->viewer = 0;
		break;
	case JANUS_STOP:
		if (w->state != JANUS_RUNNING && w->state != JANUS_QUIESCING &&
		    w->state != JANUS_FAULTED)
			return JANUS_STATE;
		if (pending(m, r->world))
			return JANUS_BUSY;
		w->state = JANUS_STOPPED;
		w->viewer = 0;
		if (m->foreground == r->world)
			m->foreground = JANUS_NONE;
		break;
	case JANUS_FAULT:
		if (w->state != JANUS_RUNNING && w->state != JANUS_QUIESCING &&
		    w->state != JANUS_SUSPENDED)
			return JANUS_STATE;
		w->state = JANUS_FAULTED;
		w->viewer = 0;
		if (m->foreground == r->world)
			m->foreground = JANUS_NONE;
		break;
	case JANUS_CHECKPOINT:
		if (w->state != JANUS_SUSPENDED ||
		    w->persistence != JANUS_CHECKPOINTABLE)
			return JANUS_STATE;
		if (w->checkpoint == UINT64_MAX)
			return JANUS_LIMIT;
		w->checkpoint++;
		break;
	case JANUS_RESTORE:
		if ((w->state != JANUS_SUSPENDED && w->state != JANUS_STOPPED &&
		     w->state != JANUS_FAULTED) ||
		    w->persistence == JANUS_EPHEMERAL ||
		    (w->persistence == JANUS_CHECKPOINTABLE && !w->checkpoint))
			return JANUS_STATE;
		if (pending(m, r->world) || m->device_owner == r->world)
			return JANUS_BUSY;
		if (w->incarnation == UINT64_MAX)
			return JANUS_LIMIT;
		if (r->fail)
			return JANUS_IO;
		w->incarnation++;
		w->state = JANUS_RUNNING;
		w->viewer = 0;
		break;
	case JANUS_BUDGET:
		if (r->amount > JANUS_WORK ||
		    r->amount > JANUS_WORK - m->worlds[1 - r->world].budget)
			return JANUS_LIMIT;
		if (r->amount < pending(m, r->world))
			return JANUS_BUSY;
		w->budget = r->amount;
		break;
	case JANUS_VIEW_OPEN:
		if (w->state != JANUS_RUNNING || w->viewer)
			return JANUS_STATE;
		w->viewer = 1;
		break;
	case JANUS_VIEW_LOST:
		if (!w->viewer)
			return JANUS_STATE;
		w->viewer = 0;
		if (m->foreground == r->world)
			m->foreground = JANUS_NONE;
		break;
	case JANUS_FOCUS:
		if (w->state != JANUS_RUNNING || !w->viewer)
			return JANUS_STATE;
		m->foreground = r->world;
		break;
	case JANUS_RECOVER:
		m->foreground = JANUS_NONE;
		break;
	default:
		return JANUS_INVALID;
	}
	return JANUS_OK;
}

enum janus_result
janus_apply(struct janus_model *m, const struct janus_request *r)
{
	struct janus_model n;
	enum janus_result result;
	if (m == NULL || r == NULL || !valid(m) ||
	    (unsigned int)r->command >= JANUS_COMMAND_COUNT ||
	    (unsigned int)r->actor > JANUS_WORLD1 ||
	    (unsigned int)r->outcome > JANUS_UNKNOWN || r->fail > 1 ||
	    r->world >= JANUS_WORLDS || r->target >= JANUS_WORLDS ||
	    r->object >= JANUS_OBJECTS || r->slot >= JANUS_SLOTS)
		return JANUS_INVALID;
	if (r->actor != actor_for(r->command, r->world))
		return JANUS_DENIED;
	if (!r->sequence || r->sequence <= m->sequence[r->actor] ||
	    (r->command != JANUS_RECOVER &&
	     r->incarnation != m->worlds[r->world].incarnation))
		return JANUS_STALE;
	memcpy(&n, m, sizeof(n));
	result = transition(&n, r);
	if (result != JANUS_OK)
		return result;
	n.sequence[r->actor] = r->sequence;
	if (!valid(&n))
		return JANUS_INVALID;
	memcpy(m, &n, sizeof(n));
	return JANUS_OK;
}
