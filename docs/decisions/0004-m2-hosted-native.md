<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# ADR 0004: M2 hosted object and native-world boundary

Status: EXPERIMENTAL within explicit maintainer M2 authority; not maintainer
acceptance of the result, a final disk/IPC format or M3 substrate selection.

Use the SQLite C API as the bounded object/grant/activity backing. Fixed prepared
SQL only; foreign_keys ON, journal_mode DELETE, synchronous FULL, trusted_schema
OFF, bounded pages/content/counts, immutable revision rows. Serialize whole writes
inside one BEGIN IMMEDIATE/COMMIT; stale base returns CONFLICT. Owner revocation
occurs only between completed requests (quiescent points), not a fake live drain.
No arbitrary live delegation; explicit delegation bit defaults off and attempts
to enable/use live delegation return UNSUPPORTED. M0 semantics remain accepted.

Objects get 128 random bits from getrandom, encoded as 16 bytes/32 lowercase hex.
IDs are public names, never bearer authority. Durable worlds are owner-created
numeric identities paired with the fixed note application identity. Owner-issued
grants are durable, recipient-bound and checked on each admission. Activity
records reference object/world/revision only; they cannot overwrite grants.

Each launch gets a private SOCK_SEQPACKET socketpair from the service through an
authenticated owner launcher; the service binds its end to world/incarnation.
Native messages cannot claim a world identity or reconnect by a name. Session
handles use fresh 128-bit randomness, are held only in that connection's bounded
server table and refer to current grants. Incarnation advances durably at launch;
crash/restart does not preserve live handles. Whole-store rollback is unsolved.

The trusted launcher closes owner/store/unrelated descriptors before executing a
static native note binary. PR_SET_NO_NEW_PRIVS and Landlock ABI >=6 restrict
filesystem access before untrusted logic; no home/repository/store/credential tree
is allowed. A narrow x86-64 seccomp allowlist complements Landlock: no socket or
connect creation, ptrace/process_vm access, ioctl, fork or filesystem metadata
mutation. Only the inherited object endpoint carries JANUS authority. Bounded
standard I/O may report application results; it carries no owner/store authority.
Fail closed on unsupported kernel/architecture. Do not weaken host policy.

The service and trusted launcher use PR_SET_DUMPABLE=0. Owner requests require
SO_PEERCRED plus a 256-bit random secret in a 0600 owner-runtime file under 0700;
not argv/environment/logs or world inheritance. Constant-work comparison rejects
wrong credentials before command execution. A world endpoint never accepts owner
operations, even if given owner-shaped bytes. M1's UID-only socket is unreachable
from the sandbox because creation/connection and descriptor recovery are denied;
this must be tested, not inferred from filesystem mediation alone.

Limits: four worlds/sessions, sixteen objects, sixteen revisions per object,
eight grants and eight lifetime handles per session, 1024-byte object content,
one internal transaction at a time, bounded activity fields and requests. Owner
poll capacity is separate from world slots and processed first. No world can
hold an open multi-request storage transaction. Storage failures never fake success.
After lost commit acknowledgement, reconcile revision/content; never auto-retry.

Alternatives: custom filesystem/journal adds crash-consistency code; a database
framework or RPC stack is unnecessary. Same-UID directory modes alone do not
confine native applications. Namespaces/user IDs or a native capability kernel
would expand this experiment; Landlock plus a minimal syscall surface is directly
testable without privilege changes. SQLite is backing machinery, not JANUS's
future filesystem. The host kernel, SQLite and trusted services remain in the TCB.

Process-crash tests at before-begin, staged, before-commit and after-commit/before-
reply are required. These do not establish sudden-power-loss safety, anti-rollback,
secure deletion or whole-system confinement. Same-owner processes outside the
sandbox and host administrator are trusted; availability and side channels remain.
Revisit on protocol expansion, delegation, concurrent transactions, new application
runtime or kernel changes. Rollback: stop owned processes, retain ignored test DBs,
remove only newly installed development package after rollback simulation.

Primary references (consulted 21 September 2026; actual installed headers govern):
https://docs.kernel.org/userspace-api/landlock.html (ABI 6 subset; newer features
are not assumed), https://www.sqlite.org/lang_transaction.html and
https://www.sqlite.org/pragma.html#pragma_synchronous. No upstream implementation
source imported. Evidence will pin installed package versions/licences and results.
