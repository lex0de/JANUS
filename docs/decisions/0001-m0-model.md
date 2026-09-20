<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# ADR 0001: bounded contract experiment

Status: EXPERIMENTAL / PROPOSED, not maintainer accepted.

The user approved C17 hosted tests, optional restrained C++17, Make, standalone
main and a private personal GitHub repository for B0/M0. These approvals do not
approve the semantics below as product contracts.

Propose a caller-owned, single-threaded, allocation-free C state machine with
two worlds, two objects, eight lifetime grant slots and eight lifetime operation
slots. A trusted test driver labels owner, backend and world calls; integers are
references inside this model, never unforgeable capabilities. Each successful
command is one serialisation point; failed commands leave all state unchanged.
The caller must serialise access. No locks, callbacks, blocking or timeouts occur.

M0.1, 20 September 2026: maintainer review found implicit delegability inconsistent
with whitepaper §5.2. A checked boolean now records explicit delegation authority
on each grant. Zero is non-delegable by default; OWNER must request one to permit
delegation. A permitted parent may grant either value to a child; a non-delegable
parent cannot create children. Invalid values reject atomically. Existing object,
rights, incarnation, ancestor and revocation checks still apply. This required
correction does not approve the representation as a stable ABI or storage format.
An extensible policy framework is deferred; one attenuable bit meets this bounded
experiment's need. See docs/evidence/m0-1.md for validation and review.

Propose deny-new revocation followed by draining admitted work; descendants are
checked through their ancestor chain. Restore never restores grants or external
operation outcomes. It changes incarnation only after validation and refuses an
active device or unfinished operation. Object writes compare a base revision at
completion. Uncertain external completion is terminal until external evidence
exists; this model supplies no automatic retry/reconciliation claim.

Alternatives: dynamically allocated reusable handle tables; optimistic concurrent
implementation; durable journal; cancelling already admitted work. These expand
lifetime, synchronisation or persistence obligations before the contracts are
reviewed. Fixed lifetime slots deliberately demonstrate fail-closed exhaustion;
they are not a proposed usable service lifetime policy.

Consequences: deterministic tests and bounded ancestor walks; no real concurrent
execution, garbage collection, durable bytes, cryptography or enforcing boundary.
Changing semantics requires changing contracts and independent expected tables.
Rollback: remove this experiment; no host format or user data depends on it.
Evidence: docs/evidence/m0.md when executed. Revisit after maintainer M0 review,
before any real broker, reusable handles, persistence or concurrency work.
