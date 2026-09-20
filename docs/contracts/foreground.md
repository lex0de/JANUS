# Candidate foreground and console contract

EXPERIMENTAL. J-007, J-008, J-013.

BACKEND VIEW_OPEN requires RUNNING and a closed viewer. VIEW_LOST requires an
open viewer and closes it; it clears foreground but does not stop execution or
alter a device lease. Duplicate loss/open is STATE. No backend shutdown is
inferred from a viewer event. OWNER FOCUS requires RUNNING and an open viewer;
it sets one global foreground world and grants no object/network access.

OWNER RECOVER clears foreground independently of the world's execution state,
budget, viewer and responsiveness. It needs no world work slot. It does not
release devices or terminate execution. It is idempotent with a fresh request
sequence. A world cannot label its own request OWNER in a future implementation:
authentication and trusted host-owned input/display are external assumptions,
not features implemented by this C library.
