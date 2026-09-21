<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M3 channel-bound object experiment

EXPERIMENTAL. This is an in-memory, static Microkit experiment, not a stable ABI,
portable wire format, disk format, complete kernel or replacement for M0–M2.
A JANUS world uses a preallocated PD slot; a PD does not define JANUS lifecycle.
See [ADR 0006](../decisions/0006-m3-capability-substrate.md).

## Authority and admission

The service derives caller 0/1/2 from Microkit's protected-call channel:
CONTROL/World A/World B respectively. Unknown channels deny. Payload identity is
never accepted. Public object IDs 1 and 2 confer no access. Only CONTROL may
GRANT, REVOKE, ROTATE, SAVE, RESTORE or INSPECT. A world sending these operations
on its own channel receives DENIED. Its CSpace contains no owner-badged endpoint.

Each world/object grant has READ=1 and/or WRITE=2, a bounded generation, and
implicit **false** delegation permission in this limited implementation. The
explicit delegation-request bit 4 returns UNSUPPORTED, as does world DELEGATE.
No live delegation occurs; accepted M0 delegation semantics remain applicable to
future implementation. Unknown rights bits are INVALID.

ACQUIRE consults the current grant and allocates one of four world-local handle
slots. Tokens are globally increasing 64-bit integers within this boot, never
wrapped or reused. UINT64_MAX is a terminal allocation limit. Tokens are not
secret bearer capabilities: lookup is confined to the channel's world, then
incarnation and grant generation are checked. Knowing another world's token
cannot change that lookup. Regrant always advances generation; exhausted
generations refuse regrant. REVOKE still removes rights at the generation limit.

Every admitted object request executes serially inside one protected procedure;
there is no outstanding split transaction. Revocation occurs at this quiescent
boundary: earlier work finishes, later admission checks current authority.
No claim of asynchronous drain, erasure of disclosed content or forced
interruption of already admitted work is made. The synthetic WORK test is bounded
and changes no object; it holds the passive service until it returns.

ROTATE increments incarnation and clears that world's handles/request accounting.
It does not restart its PD or clear application memory. Tests deliberately retain
old token bytes. SAVE retains only world, object ID and revision hint. RESTORE
rotates incarnation and returns hints, never grants. Old activity state therefore
cannot reinstate revoked authority. All state is lost on whole-system reboot.

## Explicit message fields

Each request is label zero, exactly eight 64-bit Microkit message words. This is
an architecture-specific register protocol, not serialization of C structs.
There are no pointers, padding, strings, arbitrary memory transfers or raw seL4
calls in JANUS components. Version is 1. Unused fields must be zero.

| Word | Meaning |
| --- | --- |
| 0 | Version |
| 1 | Operation |
| 2 | Public object ID for ACQUIRE; owner target `(world << 32) | object`; world 1/2 for ROTATE/RESTORE |
| 3 | Session token for GET/PUT |
| 4 | Expected revision for PUT/SAVE |
| 5 | Complete eight-byte integer content for PUT |
| 6 | Requested grant rights for GRANT |
| 7 | Reserved, zero; cannot convey a claimed world identity |

Operations: ACQUIRE=1, GET=2, PUT=3, DELEGATE=4, WORK=5; GRANT=32,
REVOKE=33, ROTATE=34, SAVE=35, RESTORE=36, INSPECT=37.
Responses have label zero/eight words: status, revision, content, token (or
INSPECT rights), incarnation, generation, requests, used handle slots. RESTORE
returns revision hint in word 1 and public object ID in word 2. Status values:
OK=0, INVALID=1, DENIED=2, STALE=3, LIMIT=4, CONFLICT=5, UNSUPPORTED=6.
Only documented fields are nonzero. Conflict may disclose current revision to a
WRITE-authorised caller, never content. Other errors return zero remaining words.

PUT requires WRITE and matching base revision; success replaces the complete
integer and increments revision. Stale-base conflict preserves the current value.
Revision UINT32_MAX refuses further writes. This demonstrates an in-memory
revision boundary, not immutable durable revision history or crash-safe storage.

Malformed version/count/label/reserved/operation/field values change no state.
Well-shaped world requests, including denied requests, charge one of 64 admissions
per incarnation. Valid owner operations and test reports do not consume world
quota. No heap, queues, descendants, dynamic capabilities or unbounded content.
There are two objects, two worlds, four grants, eight handle slots and one activity.
One service PPC executes at a time; kernel scheduling accounts for waiting callers.

## Test orchestration and resources

The live adapter additionally implements bounded test-only HELLO/PHASE/REPORT
(100–102) for channel-bound worlds and START/QUERY (200–201) for CONTROL. These
coordinate assertions; reports are not authority and cannot mutate grants,
incarnations or objects. A malicious world could lie about its own test result;
the test program and host verifier are evidence tools, not a production attestation
mechanism. Separate generated-capability and host contract checks challenge them.

The service is passive, priority 250. A PPC donates its caller's scheduling context.
CONTROL has its own context and receives a service-work-start notification. A
bounded workload demonstrates CONTROL's timestamp between service begin/end;
it is not a worst-case response-time proof. Per-world quotas preserve owner
inspection/revocation after logical exhaustion. A malicious looping caller is
still bounded by the kernel context; exhaustive scheduling interference is untested.

Only CONTROL maps COM1 I/O ports for release serial evidence. Only OBJECT maps the
extra private page at 0x40000000. ATTACKER is CONTROL's sacrificial child and has no
object channel. Its intentional load faults; CONTROL verifies child/address/fault
type, stops that child and withholds fault reply. It never resumes the faulting
instruction. Subsequent B reads and owner inspection check containment.
