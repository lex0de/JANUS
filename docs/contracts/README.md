# EXPERIMENTAL M0 contracts

These M0 experimental baseline rules were accepted by the maintainer on
20 September 2026 at 7d4a800; see ../evidence/m0-acceptance.md. They interpret
whitepaper sections 3–9 and Appendix A. They are
not an ABI, wire format, disk format or enforced security boundary. ADR 0001
records the accepted M0 reference-model details; layouts remain experimental.

The test driver owns all model memory and supplies actor identity. OWNER models
authenticated grant/recovery control, BACKEND models trusted lifecycle/device
reports, and WORLD0/WORLD1 model distinct recipients. No real authentication is
implemented. A hostile process sharing this address space can overwrite it.

All commands validate state, enum/range values, actor, incarnation and monotonic
per-actor request sequence before changing a copied candidate state. Only a
successful result publishes it and advances sequence. A rejected request is
retryable with corrected input; a successful request cannot be replayed. Sequence
zero is invalid; exhaustion never wraps. Unknown operations are rejected.
Null pointers return INVALID. Valid, non-overlapping caller-owned pointers are
required for the duration of a synchronous call. Nothing retains pointers.

Results: OK, INVALID (malformed), DENIED (authority), STALE (incarnation or replay),
STATE (unsupported transition), LIMIT (bounded capacity/counter), BUSY (unfinished
work/device), CONFLICT (object base changed), IO (injected failure). Rejections
preserve every byte of model state, including committed roots and counters.

Commands are serial, nonblocking and allocation-free; there are no asynchronous
callbacks, locks or cancellation races. Tests enumerate event interleavings.
Thread-safety, CPU/memory accounting, real service queues and crash-safe I/O are
not implemented. Two worlds share a budget of twelve work units; four additional
units are conceptually reserved for recovery. Recovery never consumes world work
slots. Grant/operation slots are lifetime bounded at eight and never recycled.

World names/classes/manifests convey no authority. Activity grouping is outside
this minimal model and never merges grants. Objects have stable indices and
independent revision counters; numeric indices are test references only.

## Hosted enforcement experiments

[M2 native contract](native-m2.md) separately defines the experimental enforced
Linux object/session/activity boundary. It does not turn the M0 model above into
an OS security boundary. See ADR 0004 and the M2 evidence for scope and limits.

[M3 substrate contract](substrate-m3.md): experimental channel-bound in-memory
objects, incarnation/activity checks and passive service budgets on Microkit.
M3 is READY FOR REVIEW, not accepted; this does not replace M0–M2 contracts.
