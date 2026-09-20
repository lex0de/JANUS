<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Candidate authority and operation contract

EXPERIMENTAL. J-002–J-006, J-009, J-012, J-016.

OWNER issues a grant to a RUNNING world for one object and nonzero READ/WRITE
rights and an explicit `delegable` policy: 0 (default) forbids delegation;
1 permits it. Delegation authority is itself authority and must be explicitly
granted and attenuable. The record is bound to the recipient's current incarnation.
WORLD may delegate its live grant only when the parent permits delegation, for
that exact object with a nonempty subset of rights, to a RUNNING recipient.
A delegable parent may produce a child with policy 0 or 1. A non-delegable parent
returns DENIED for either requested child policy, even for READ-only delegation.
The owner must explicitly issue any new delegation authority; neither a right
to READ/WRITE nor knowledge of a grant/object index supplies it.

Request or stored policy values above 1 are INVALID and leave all state unchanged.
Stored child records under non-delegable parents are also INVALID. The unsigned
integer represents a checked boolean only in this EXPERIMENTAL model; its layout
is not a stable ABI, IPC format or persistent format. All request commands check
the policy's range; only ISSUE and DELEGATE record it.

Delegation creates a child record and cannot remove
its ancestry. All ancestors must remain active and incarnation-valid at BEGIN.
Finite append-only grant slots prevent aliasing/reuse and bound descendant work.

M0.1 addresses the maintainer's blocking review finding against whitepaper §5.2:
M0 implicitly allowed any live grant to be delegated. This is a correction to
the reference contract/model, not an independently discovered OS vulnerability.

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
