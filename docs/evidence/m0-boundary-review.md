# M0 boundary review

Date: 20 September 2026. Scope: include/janus/model.h, lib/contract/model.c,
tests/contract/test_model.c and docs/contracts/. Reviewed before initial commit
using janus-boundary-review. Same agent authored and reviewed this change; no
independent human acceptance or formal verification is claimed.

The review started from the candidate contracts, then traced janus_init and
janus_apply and every command family. Tests use separately written expected
transition tables, not the implementation's dispatch or validation helpers.

## Findings addressed

1. Medium, model input validation: `valid()` accepted a pending operation whose
   grant had already reached REVOKED, or whose world was STOPPED. A malformed
   fixture could therefore contradict completed-revocation/stop invariants while
   an unrelated command succeeded. Added ancestor-state and execution-state
   validation; `test_malformed` fields 18/19 reject without mutation. This was a
   model state-validation defect, not an observed OS authority exploit.
2. Low, setup validation: installed ShellCheck 0.10.0 rejected ambiguous empty
   CDPATH assignments and considered trap-only cleanup unreachable. Quoted empty
   values explicitly and invoked cleanup on normal completion before disarming
   the EXIT trap. No warning was suppressed. Final `make check` and all eight
   toolchain probes pass; failed intermediate logs remain retained.

## Boundary trace

| Boundary | Review result / limitation |
| --- | --- |
| Caller identity | Per-command actor matrix; test driver supplies identity, no authentication |
| Scope and delegation | Object equality, rights subsets and bounded parent chains; names alone denied |
| Grant lifetime | Append-only slots, ancestor validation, new incarnation denies old grants |
| Revocation | Deny new work at BEGIN; completion waits for descendants' admitted work; copied data unaffected |
| Restore | Validate before publish; source/root/leases preserved on error; no authority rollback |
| Commit | Base revision comparison and counter checks before publication; injected failure atomic in memory only |
| Replay and callbacks | Successful per-actor sequence and incarnation checks; no durable replay journal |
| Foreground/viewer | No execution/device release inferred; recovery uses no world slot |
| Device | Owner/incarnation checks, explicit reset evidence event, quarantine refuses reuse |
| Memory/arithmetic | Fixed arrays, checked indices before lookup, bounded parent walk, no allocation/I/O/copies of external buffers |
| Concurrency/cancellation | Serial caller required; explicit terminal cancel/unknown, no actual threaded/DMA race proof |

No remaining confirmed implementation defect found within this bounded review.
Questions for maintainer: whether drain semantics are desired; how an enforcing
backend cancels/reconciles work for a dead world; reusable-handle lifetime policy;
anti-rollback and trusted input authentication. In M0 only the world role finishes
admitted operations, so an uncooperative world can keep revocation BUSY forever.
Owner foreground recovery remains available; backend cancellation requires a
future explicit contract, not a fake completion. Quarantine has no recovery API.

The fixed tables and single agent can share mistaken assumptions. Human M0 review
is still required. Model state is trusted caller memory, not a hostile-input
serialization format; struct bytes must never be used as persistent/IPC state.
