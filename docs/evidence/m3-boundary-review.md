<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M3 authority boundary review

21 September 2026. Bounded review of new substrate model, Microkit adapters/SDF,
independent expectations, build/verification/live harness and ADR 0006. This uses
the JANUS boundary-review workflow. It is neither maintainer acceptance nor an
independent human audit. One agent wrote implementation and tests: shared-assumption
risk remains despite separate table/script and generated-capability oracles.

## Entry points and authority graph

Expected rules were taken from accepted M0/M2 and the maintainer's M3 request:
object names do not grant access, authority is channel-bound, restoration consults
current grants, owner authority is absent from worlds, and exhaustion/fault must
not disable unaffected worlds/control. The review then traced `protected()` into
`jm_dispatch()`, the controller/world callers, SDF and generated capDL graph.

- SDF gives A/B one PPC each. Generated send caps target ep_object with badges
  0x8000000000000001/2; CONTROL uses 0x8000000000000000. Worlds have write and
  grant-reply only, no receive/grant right on that endpoint. They cannot mint the
  owner badge from message data. No world-to-world or world-to-CONTROL endpoint.
  Runtime infrastructure fault endpoints remain present and are part of the TCB.
- Service caller comes from the delivered channel, not request words. All six
  owner operations are tested over both world identities and deny without state
  change. Claimed identity in reserved payload fails validation. Bad label/count,
  invalid operation and reserved/unused words are rejected before admission.
- `world()` searches only that world's four handles; global token monotonicity,
  incarnation and grant generation prevent cross-world/stale aliasing. Exhausted
  token/incarnation/generation/revision counters do not wrap. Revocation at maximum
  generation still clears rights, and subsequent regrant cannot revive that epoch.
- Delegation is explicitly unsupported; the owner cannot silently enable it.
  READ and WRITE are independently checked. A WRITE-only conflict returns revision
  metadata but no content. Failed writes leave object content/revision unchanged.
- Activity contains only hints. Restore rotates incarnation/clears handles and
  does not assign grants. Live revoked-activity replay denies acquisition.
- Serial synchronous requests make revocation quiescent. A long WORK PPC is
  admitted work and finishes before a queued owner revoke; no false asynchronous
  revocation completion is exposed. Previously returned content is not erased.

## Isolation, resources and recovery

Generated frame sets for A, B and service are disjoint. No world receives the
service's private frame or frame-minting/untyped authority. Low fault-address pages
are absent. The sacrificial child has no object channel. CONTROL verifies fault
label, length, data access and exact address, stops only that child, withholds the
fault reply and then checks B plus the current object state. The live observed
fault is substrate enforcement, not a synthetic C error. It is QEMU evidence,
not physical memory/DMA/device-isolation evidence.

The passive service, active CONTROL and each world have explicit contexts and
fixed budgets. Live service begin/CONTROL/end ordering exercises scheduling-context
donation and independent recovery scheduling. Four handles and 64 requests bound
world admissions; valid owner requests bypass this quota. Malformed traffic still
consumes kernel execution time; all possible hostile scheduling patterns have not
been measured. No hard real-time/host-wide DoS claim. Test REPORT is quota-exempt,
but only changes bounded test metadata, not authority, and still uses caller time.

Only CONTROL holds the bounded emulated serial port authority. World/service data
contains no host secrets. No raw seL4 syscall, heap, filesystem, network, inherited
host descriptors, device passthrough or dynamic process loader is added. SDF and
SDK-generated capabilities are trusted build inputs; the signed SDK gate checks
archive, key, signature and exact installed inputs before executing the builder.
The QEMU harness owns a child handle, bounds time/output and reaps only that child.

## Findings and disposition

1. **Corrected, low: error reply inconsistency.** Initial revision-exhaustion PUT
   returned a nonzero revision field despite the documented zero-data LIMIT reply.
   Independent host expectation failed. Assignment moved into CONFLICT/success;
   exhaustion checks now pass. No content or extra authority was exposed.
2. **Corrected, build portability:** strict C17 rejects upstream GNU `asm`; target
   uses documented GNU C17 without disabling warnings. Missing standard include
   and array-parameter declaration mismatch were fixed. Clang aggregate clearing
   emitted unresolved memset; explicit bounded field reset removed the dependency
   rather than adding libc. GCC/Clang target builds now pass.
3. **Retained, analysis limitation:** GCC `-fanalyzer` reports uninitialised return
   data in SDK `microkit_x86_ioport_read_8()` on its kernel-error path, after
   `microkit_internal_crash()`. This also occurs at -O2. The SDK crash helper writes
   a low error-code address and is not declared noreturn; the analyzer continues
   past it. Generated mappings leave page zero absent, so this configuration faults
   to the monitor rather than returning that value. The normal SDF-authorised
   serial access passed live under both compilers. No SDK patch, warning suppression,
   stub or fake PASS was used. GCC target CONTROL analysis is FAIL / coverage
   limitation, not a clean analyzer result. Other target PD and portable analyses
   pass; Clang CONTROL analysis passes. This is not claimed as a newly discovered
   OS vulnerability and should be revisited if SDK error handling/mappings change.
4. **Residual scope limits:** static preallocated PD slots; semantic incarnation
   rotation, not real process reconstruction; no live delegation, durable storage,
   physical recovery or power/rollback guarantees. A malicious world can falsify
   its own test REPORT; these programs are test oracles, not production attestation.
   Exact proof inheritance is NOT ESTABLISHED for this MCS artifact.

No unresolved JANUS authority bug was found in this bounded review. That is not
proof of absence. The retained SDK analyzer diagnostic and single-agent oracle risk
are explicit review inputs. M0–M2 code is unchanged. M3 awaits maintainer review;
M4/M5 and a permanent substrate choice remain unauthorised.

## Quota correction review, 1 October 2026

The PR review identified a confirmed Medium accounting bug missed by the original
review: owner-only requests over world channels returned DENIED before consuming
quota. The original full-state-equality test encoded that incorrect assumption.
Historical review text above is retained; it does not describe the corrected
accounting. See [correction evidence](m3-quota.md) for exact failures and outcomes.

The bounded patch charges well-shaped world requests before owner-operation
denial. Shape validation still precedes mutation, and caller identity still comes
from the channel. Counter comparison precedes increment, preventing wrap or more
than 64 admissions. Only the caller's accounting may change on these denials;
owner policy, object content, handles and other-world state remain unchanged.
After exhaustion, valid world operations return LIMIT with zero error data.
CONTROL's independent owner path remains usable without world-quota checks.

Host rows test all six owner operations and both world identities through the
64/65 boundary, zeroed responses, other-world reads and revocation/rotation.
The corrected oracle fails on the original code. Actual GCC/Clang target runs
exercise denied-request exhaustion and independent CONTROL/B progress; the host
verifier requires a new quota marker. No new authority path, allocation, pointer
ownership, asynchronous revocation or persistence behavior is added.

No additional authority defect was found in this patch review. This remains a
single-agent review, not independent human acceptance or proof. Whole-M3 Clang 21
formatting fails on an unchanged baseline line; changed-file checks pass. The Make
Clang analyzer command fails on an unused option; direct portable analysis passes.
Historical target CONTROL analyzer limits and proof boundaries remain. The
maintainer accepted the correction for publication to PR #3, with final review
required at its new head. **M3 READY FOR SECOND REVIEW**, not accepted. No
implementation/test/verifier code changed after the accepted live and sanitizer
runs. The final host/analysis/changed-file-format/bootstrap reruns pass; Makefile
portability debt, formatter drift and single-agent shared assumptions remain.
