<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M0.1 affected authority boundary review

20 September 2026; base 37a0ae7c8b6ba072e96c9fc51cd09658b48e28dd.
Scope: janus_init/janus_apply, valid/live/authority/actor_for, grant and operation
callers, restore/revocation interactions, and independent test expectations.
Workflow: janus-boundary-review. Same agent implemented and reviewed this change;
this is not independent human approval or formal verification.

## Blocking finding supplied by the maintainer

The original `authority()` permitted any live grant recipient to delegate within
object/right bounds without a delegation policy, contrary to whitepaper §5.2.
A recipient of an otherwise READ-only grant could create another live grant
without OWNER ever approving delegation. This is a maintainer-review finding in
the M0 contract/model, not an independently discovered OS vulnerability.

Corrected with `delegable`, a checked unsigned boolean on request and grant.
Default zero denies delegation; only explicit OWNER issue supplies root delegation
authority. DELEGATE requires parent policy one and may record zero or one on its
child. No existing-grant mutation API changes this flag. Root/child records retain
the requested policy. Invalid requests and impossible stored ancestry reject
without publishing the candidate copy or advancing request sequence.

## Whole affected boundary trace

| Boundary | Review and regression evidence |
| --- | --- |
| Identity / entry points | actor_for still restricts ISSUE/revoke to OWNER and DELEGATE/BEGIN/FINISH to the matching world; actor matrix remains exercised |
| Defaults / authority | Zero-initialised ISSUE is non-delegable; supplying policy one to BEGIN cannot upgrade its grant; test_delegation_policy |
| Policy attenuation | Independent four-row parent/child oracle covers 0→0/1 denial and 1→0/1 success; child zero refuses further delegation |
| Malformed inputs / records | Request policy 2 and UINT_MAX rejected for every command; stored invalid values and children beneath non-delegable parents rejected, byte-for-byte preservation |
| Object / rights | Parent ownership, same object and nonempty rights subset still required; positive delegability does not bypass attenuation; test_authority and test_delegation_lifetime |
| Lifetime / ancestors | Parent indices strictly precede child; no cycles or reusable slots; live walks all ancestors, checking incarnation and revocation |
| Restore | Current caller incarnation does not refresh saved grants; stale root/ancestor delegation denied; old request incarnation remains STALE |
| Revocation | REVOKING immediately blocks new delegation throughout subtree; REVOKED still blocks it; admitted operations drain under captured authority before completion |
| Operation commit | Delegability is not required to use READ/WRITE; FINISH retains ownership/incarnation checks, revision conflicts, explicit cancel/unknown and non-retry semantics |
| Atomicity / resources | All validation precedes publication from a local copy; rejected grants consume no slot or sequence; fixed capacities and overflow checks unchanged |
| Recovery / device / presentation | No new coupling: viewer loss does not stop execution/release devices; active device blocks restore; recovery remains outside world work budget |

No remaining confirmed defect found in this bounded review after the maintainer
correction. The new policy is not a wire/disk schema or stable ABI. Caller labels
and memory remain trusted, so editing an in-process root record can forge model
authority. That limitation is unchanged, not solved by adding a boolean.

The existing serialisation requirement, append-only slot exhaustion, indefinite
BUSY revocation for an uncooperative world, lack of real backend cancellation,
absence of durable replay protection and lack of OS/DMA enforcement remain.
No real threaded race, device reset or trusted input mechanism was implemented.
The full old suite remains included; expected tables do not call implementation
decision helpers. Shared-author assumption risk remains for second human review.
