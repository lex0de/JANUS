# Candidate authority and operation contract

EXPERIMENTAL. J-002–J-006, J-009, J-012, J-016.

OWNER issues a grant to a RUNNING world for one object and nonzero READ/WRITE
rights. It is bound to the recipient's current incarnation. WORLD may delegate
its live grant to the other RUNNING world only for that exact object, with a
nonempty subset of rights. Delegation creates a child record and cannot remove
its ancestry. All ancestors must remain active and incarnation-valid at BEGIN.
Finite append-only grant slots prevent aliasing/reuse and bound descendant work.

BEGIN requires a RUNNING caller, current grant, matching object, and one requested
READ or WRITE right. It consumes one lifetime operation slot and one outstanding
budget unit. Length is 1..4096; it describes synthetic work, not an actual buffer.
WRITE records the expected object revision; READ requires no base match.

OWNER REVOKE_BEGIN changes ACTIVE to REVOKING; immediately deny new operations
and delegation through this grant or descendants. Already admitted operations
may finish under their captured authority. REVOKE_COMPLETE is BUSY until all
pending operations in that subtree finish, then marks REVOKED. Repeating either
transition is STATE. No promise of erasing already disclosed data is made.

WORLD FINISH must own the operation and have its original incarnation. DONE on
WRITE compares expected base and publishes the next revision atomically; a
conflict leaves the operation pending for explicit CANCEL. CANCEL consumes no
object revision. UNKNOWN records terminal uncertainty without changing the local
object; external effects may already exist and are not retried. DONE/CANCEL/
UNKNOWN all release outstanding budget. Duplicate FINISH is STATE. Revocation
completion is permitted after any terminal outcome, including UNKNOWN.

Terminal operation IDs cannot be reused. No automatic retry is available. An
explicit new owner/world request can represent another external action, so this
model does not promise exactly-once external effects or durable replay defence.
