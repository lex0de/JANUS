<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M2 hosted native boundary — EXPERIMENTAL

This contract supplements accepted M0 semantics with a bounded Linux-hosted
implementation. It is not a native JANUS ABI, stable storage format or M3 choice.
See [ADR 0004](../decisions/0004-m2-hosted-native.md) and
[the runbook](../development/NATIVE_M2.md).

## Identity and authority

The owner creates world identities 1–4, each bound to the fixed `janus-note`
application. Each admitted launch advances its durable positive incarnation.
Launch failure may consume an incarnation; numbers never wrap or get reused by
normal operations. Service restart drops every live endpoint/handle. The owner
must launch a fresh session; no world can reconnect by claiming an identity.

Objects have random 128-bit identities, encoded as 16 octets (32 lowercase hex
at the CLI). Identity is public. Only a current OWNER-issued grant to the bound
world admits ACQUIRE. Grants distinguish READ and WRITE, live/revoked state and
generation; delegation is explicitly false. Live DELEGATE and owner attempts to
set delegability return UNSUPPORTED. Accepted M0 delegation semantics are unchanged.

ACQUIRE returns a random 128-bit handle in the connection's private server table.
GET/PUT look up only that table, check the current world incarnation, then recheck
the current grant and its generation. Regrant invalidates old handles. Unknown or
other-session handles return STALE; revoked grants return DENIED. Random collision
probability across sessions is not a mathematical non-reuse proof; within-session
collisions fail without inserting a handle. Whole-store rollback is not prevented.

Revocation runs at quiescent request boundaries in the single-threaded service.
A request already admitted has completed/failed before a revoke is processed;
future admissions are denied. There is no asynchronous draining transaction or
REVOKE_COMPLETE claim. Generation exhaustion prevents regrant, but never prevents
withdrawal of authority. Revocation cannot erase content already disclosed.

## Revisions and reconstruction

One basic test/document type exists. Committed revisions are immutable, starting
at 1. PUT supplies a handle, expected base revision and complete bounded content.
The service serializes admission and BEGIN IMMEDIATE, checks the expected base,
inserts a revision, updates current, and commits. A stale base returns CONFLICT
with current revision, without disclosing content to WRITE-only callers. No world
can hold an open transaction across requests. Empty content is valid.

SQLite foreign keys are enabled, DELETE journalling and FULL synchronous mode
are verified, all SQL is fixed/prepared, and all values are bound. Storage errors
fail the operation; uncertain commit/transport outcomes are reported UNCERTAIN.
No automatic retry occurs. Reopen and inspect complete revision/content to reconcile
an ambiguous result; there is no durable exactly-once operation ID. Process-crash
checks are not sudden-power-loss tests or proof of filesystem/device durability.

Activity records contain identity, world, object and a revision hint. Reconstruction
starts a new process/session and reads through current authority. It does not
restore process memory or old handles. The note app selects current content, not
a rollback of the hinted revision. Restoring only old activity records cannot
restore grants. Rolling back the whole database/host is outside this guarantee.

## Experimental packet encoding

AF_UNIX SOCK_SEQPACKET provides one bounded packet per message. Integers use
unsigned big-endian encoding; 64-bit revision/argument values are additionally
bounded by INT64_MAX for SQLite. No raw C layout is transmitted.

| Offset | Width | Field |
| --- | --- | --- |
| 0 | 4 | ASCII JAN2 |
| 4 | 1 | Version 1 |
| 5 | 1 | Operation |
| 6 | 1 | Status, zero in requests |
| 7 | 1 | Reserved, must be zero |
| 8 | 4 | Content length, 0–1024 |
| 12 | 4 | Rights: READ=1, WRITE=2, DELEGABLE=4 (unsupported live) |
| 16 | 8 | Revision |
| 24 | 8 | Owner world argument / response incarnation |
| 32 | 16 | Object/activity ID |
| 48 | 16 | Session handle |
| 64 | length | Content |

Exact lengths and operation-specific unused zero fields are mandatory. World
operations are ACQUIRE=1, GET=2, PUT=3, DELEGATE=4, SESSION=5. Owner operations
are CREATE=32, GRANT=33, REVOKE=34, WORLD=35, SAVE=36, LAUNCH=37, INSPECT=38,
STOP=39. Requests cannot supply a world identity on the world endpoint. Invalid
encoding or ancillary descriptors close that endpoint; semantic malformed requests
return INVALID without storage mutation. Closing the endpoint invalidates its
handles. A malformed packet is never a partially admitted operation.

Owner packets prepend a separate 32-byte secret. SO_PEERCRED must also match the
service UID. The secret is generated with getrandom, rotated on service startup,
held in owner.key (0600) under a 0700 runtime directory, never logged or passed to
a world. Wrong/missing credentials are denied before command parsing. LAUNCH alone
returns one endpoint via SCM_RIGHTS to the trusted launcher. No owner command is
accepted through a world endpoint. Replies carry no credential.

## Ownership, scheduling and bounds

The service owns its SQLite connection, four session slots and all handle tables;
there are no threads, shared globals, callback races or locks between workers.
The optional store checkpoint callback exists for deterministic process-crash
experiments; production never sets it and no packet can enable it.

| Resource | Bound / failure |
| --- | --- |
| Objects | 16; CREATE returns LIMIT |
| Content / request objects | 1024 bytes / one object, no list API |
| Revisions | 16 per object; PUT returns LIMIT |
| Worlds / live endpoints | 4; duplicate live world returns BUSY |
| Grant records | 8 per world, including revoked entries; LIMIT |
| Session handles | 8 lifetime per connection; LIMIT |
| Activities | 8, fixed ID/world/object/revision fields; LIMIT |
| Transactions | One synchronous service transaction; no retained client work |
| Packet | 1088 bytes; owner packet adds 32 credential bytes |
| World socket buffers | SO_SNDBUF/SO_RCVBUF requested 4096 each (Linux accounting may double) |
| SQLite file | max_page_count=2048; no user SQL or unbounded result lists |
| Native process | 64 MiB address space, 3 s CPU, 16 FDs, no core, no fork |
| Native report / lifetime | 16 KiB write-only report pipe; ~6 s launcher loop, bounded SIGKILL reap attempt |

Owner polling has a reserved slot and priority over one packet per world per
iteration; STOP withdraws an endpoint even at world/handle quota. A failed reply
closes its session rather than retaining an unbounded response queue. Owner
clients may occupy a bounded one-second receive interval. Host-owner processes
outside the sandbox are trusted; this is not hard real-time recovery, host-wide
DoS prevention, or a guarantee about uninterruptible kernel I/O.
