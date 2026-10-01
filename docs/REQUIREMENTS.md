<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Requirement-to-evidence map

Source: JANUS whitepaper v0.1, sections cited below. IDs are bootstrap traceability
labels, not new claims of implementation or a substitute for the original text.
M0 uses a bounded reference model; real enforcement is later work.

| ID | Intended invariant | Source | Minimum M0 scenario | Observed M0 evidence |
| --- | --- | --- | --- | --- |
| J-001 | Worlds and activities have different roles; a world need not be a VM | 3-4 | Native/VM classes share lifecycle without merging their execution claims | PASS (model/review only): test_world_matrix; [M0 evidence](evidence/m0.md) |
| J-002 | Object names/content hashes and manifest requests do not grant access | 5-6 | A known object ID with no grant is denied | PASS (model/review only): test_authority; [M0 evidence](evidence/m0.md) |
| J-003 | Delegation requires explicit authority and cannot amplify rights, scope or delegability | 5.2 | Default non-delegable; explicit delegation and policy attenuation; broader rights/scope rejected | PASS M0.1 (model only): test_delegation_policy, test_delegation_lifetime, test_authority, test_malformed; [evidence](evidence/m0-1.md) |
| J-004 | Current authority outranks saved state | 7.3, App. A | Revoke after checkpoint, restore, reject stale reference | PASS (model/review only): test_restore; [M0 evidence](evidence/m0.md) |
| J-005 | Completed revocation denies new operations; in-flight effects are specified | 5.2 | Operations before/during/after completion; no false claim of erasing copied data | PASS (model/review only): test_authority, test_commit; [M0 evidence](evidence/m0.md) |
| J-006 | Restore creates a new incarnation with revalidated session state | 3.1, 7.3 | Old incarnation events and handles cannot affect the new one | PASS (model/review only): test_restore, test_lengths_replay; [M0 evidence](evidence/m0.md) |
| J-007 | Execution, viewer, foreground and device ownership are separate state | 8-9, App. A | Viewer disconnect/crash leaves running execution and active device ownership intact | PASS (model/review only): test_foreground_device, test_regressions; [M0 evidence](evidence/m0.md) |
| J-008 | Foreground ownership does not enlarge unrelated authority | 9.1 | Foreground grant adds no object/network rights | PASS (model/review only): test_foreground_device; [M0 evidence](evidence/m0.md) |
| J-009 | Resources and delegated service work are bounded | 4.2 | Child budget sum cannot exceed parent; counter/queue overflow rejected | PASS (model/review only): test_limits, test_lengths_replay; [M0 evidence](evidence/m0.md) |
| J-010 | Uncertain device recovery ends in quarantine, not availability | 8.2-8.3 | Reset failure refuses reassignment; model only, no physical reset claim | PASS (model/review only): test_device_matrix, test_foreground_device; [M0 evidence](evidence/m0.md) |
| J-011 | Reconstructible, checkpointable and ephemeral persistence differ | 7.1 | Unsupported checkpoint rejected; suspension is not durable checkpoint completion | PASS (model/review only): test_world_matrix, test_restore; [M0 evidence](evidence/m0.md) |
| J-012 | External effects do not rewind; uncertain outcomes stay explicit | 7.2 | Unknown completion does not cause automatic irreversible retry | PASS (model/review only): test_commit, test_restore; [M0 evidence](evidence/m0.md) |
| J-013 | Recovery control does not depend on guest cooperation | 9.2, 13.3 | Recovery event handled independently of unresponsive foreground world | PASS (model/review only): test_foreground_device, test_limits, test_regressions; [M0 evidence](evidence/m0.md) |
| J-014 | Object identity, revisions, authority and commit boundaries are distinct | 6.1 | Stale base revision conflicts; failed commit preserves committed root | PASS (model/review only): test_commit; [M0 evidence](evidence/m0.md) |
| J-015 | Linux prototype, models and native substrate have different TCB claims | 12-14 | Review identifies every claimed boundary and what the model does not enforce | PASS (model/review only): boundary review / threat-model.md; [M0 evidence](evidence/m0.md) |
| J-016 | Control plane accepts bounded operations, not arbitrary privileged commands | 14.1 | Unknown operation/replayed request rejected; no shell command forwarding | PASS (model/review only): test_actor_matrix, test_lengths_replay, test_malformed; [M0 evidence](evidence/m0.md) |
| J-017 | No required runtime LLM service | 1.3, 12.1 | Dependency/design review distinguishes build assistance from runtime | PASS (model/review only): boundary review / dependency inspection; [M0 evidence](evidence/m0.md) |
| J-018 | Failed restore preserves committed source and does not release active devices | 7, App. A | Inject failure during restore; retain source and actual lease state | PASS (model/review only): test_restore, test_foreground_device; [M0 evidence](evidence/m0.md) |

## Test-oracle discipline

For each implemented scenario, replace NOT RUN with test identifier(s), exact
command/evidence, and observed outcome. Add failure cases for invalid enum/state,
zero and maximum values, exhaustion, overflow, and duplicate/out-of-order events.
Maintain expected transition tables separately from production transition logic.

The whitepaper is not a complete executable specification. When it leaves a
concurrency or error detail open, propose it visibly rather than claim the text
already settled it. M0 may test proposed semantics, labelled EXPERIMENTAL, before
maintainer review. Host isolation, crash-safe disk I/O, DMA revocation and physical
recovery require tests beyond a reference model.

## Later evidence, not M0 completion requirements

M1 needs actual domain identity/state reconciliation and viewer recovery. M2 needs
an enforcing object/grant boundary and interrupted commits. M3 needs a measured
native substrate configuration. M4 needs named device/reset/IOMMU evidence. M5
needs updates, independent backup restoration, and responsiveness/power results.

## M1 hosted evidence — 20 September 2026

The M0 rows above retain their model scope. The following additional evidence
comes from [M1 live tests](evidence/m1.md), not a reinterpretation of M0 passes.

| ID | Additional observed evidence | Boundary |
| --- | --- | --- |
| J-007 | PASS live B–J: guest execution, viewer process and foreground are independent; leave/crash/restart preserve execution | No device ownership or secure-kiosk claim |
| J-009 | PASS unit limits and live L: fixed profiles/messages/client slots, incarnation overflow and fork/FD exhaustion | No host-wide resource isolation or hard real-time guarantee |
| J-012 | PASS injected start/save errors and live H/J; uncertainty never adopts an active execution without a matching record | Experimental host record, no durable authority/anti-rollback |
| J-013 | PASS live G/H: SIGSTOP viewer recovered without guest input, guest survives broker crash | Software owner socket only, no physical secure attention |
| J-015 | PASS boundary review describes Linux/libvirt ambient authority and M0/M1/native distinctions | No native capability enforcement |
| J-016 | PASS authenticated owner socket, strict parser, unknown/oversized/trailing/replay-like input; K proves no UUID/name fallback | Other-UID expected-peer test is unit-level; no generic command/XML/QMP forwarding |

M0 accepted at 7d4a800 on the maintainer's explicit instruction. M1 is READY FOR
REVIEW, not accepted. Physical/nested evidence is not inferred from these results.

## M1 acceptance

The maintainer accepted the bounded M1 results at 8e910332 on 20 September 2026.
See [M1 acceptance](evidence/m1-acceptance.md); historical review evidence above
keeps its original wording. No hosted result becomes native-substrate evidence.
M2 now targets live J-002/J-004/J-006/J-009/J-011/J-014/J-015; those additional
claims require new evidence.

## M2 hosted enforcement — 21 September 2026

See [M2 evidence](evidence/m2.md), [boundary review](evidence/m2-boundary-review.md)
and [native contract](contracts/native-m2.md). These are additional live results,
not a reinterpretation of historical M0 model tests or M1 viewer evidence.

| ID | M2 evidence | Scope |
| --- | --- | --- |
| J-002 | A/B PASS: known ungranted object denied; direct store open EACCES | Service authority plus Linux sandbox, not a native kernel |
| J-003 | Unit explicit unsupported delegation and non-delegable grants | Live owner grants only; M0 delegation semantics retained |
| J-004 | H PASS: revoke, restore old activity metadata, restart, access denied | Activity-only reconstruction; no whole-store anti-rollback |
| J-005 | Unit revoke and max-generation revoke deny future work | Serial quiescent requests; no asynchronous drain claim |
| J-006 | F/G PASS: old handle rejected in new incarnation, fresh handle succeeds | Connection-local authority with current durable incarnation |
| J-009 | L and unit limits PASS: world/grant/handle/object/revision/activity/counter/queue bounds | Reserved owner slot; no hard real-time or host-wide DoS guarantee |
| J-011 | G/H PASS: restart app from activity with current authority | Reconstruction, not exact process checkpoint |
| J-012 | J PASS: lost acknowledgement reconciles complete new revision | No automatic retry or exactly-once operation ID |
| J-014 | C/D/E/I/J PASS: immutable revisions, stale-base conflict, READ-only denial, four crash checkpoints | Real SQLite transactions; no power-loss test |
| J-015 | K and boundary review PASS: service/owner boundary protected and TCB stated | Linux/SQLite/libc/trusted launcher remain trusted |
| J-016 | Malformed owner/world requests rejected; owner secret checked first | Explicit experimental packets, no arbitrary SQL/commands |

M2 results await maintainer review. M3–M5, nested KVM and physical tests are NOT RUN.

## M2 acceptance

M2 ACCEPTED — 21 September 2026, by explicit maintainer review at 61c2830.
[Acceptance](evidence/m2-acceptance.md) bounds these results; prior READY FOR
REVIEW evidence remains historical. M3 capability-substrate work is authorised;
M4–M5 are not. Linux M2 evidence does not establish a native substrate proof.

## M3 capability-substrate evidence — 21 September 2026

M3 READY FOR REVIEW, not accepted. See [evidence](evidence/m3.md),
[boundary review](evidence/m3-boundary-review.md), [contract](contracts/substrate-m3.md)
and [proof/dependency boundary](evidence/m3-dependencies.md).

| ID | Additional M3 result | Scope |
| --- | --- | --- |
| J-002 | B/C PASS: known ID denied; other-world handle denied by actual channel-derived identity | Two static seL4/Microkit world PDs |
| J-003 | Explicit UNSUPPORTED live delegation; owner default non-delegable | M0 delegation semantics unchanged |
| J-004 | H PASS: saved activity after revoke cannot reacquire authority | One in-memory hint record, no durability |
| J-005 | E/F PASS: revoke denies old/new work; regrant does not revive handles | Serial quiescent PPC admission, no asynchronous drain |
| J-006 | G/H PASS: retained token bytes fail after incarnation rotation | Semantic rotation, not actual PD reconstruction |
| J-009 | K PASS: handle/request limits, owner capacity, B progress; passive service charged to caller context | Static budgets and bounded experiment; no hard real-time guarantee |
| J-013 | I/L PASS: trusted parent stops sacrificial faulty child; owner and B continue | Software fault recovery, no physical secure attention |
| J-014 | J PASS: revision conflict preserves current content | Tiny in-memory object, M2 durable experiment retained separately |
| J-015 | TCB/proof comparison recorded; proof inheritance NOT ESTABLISHED | QEMU release non-VTX, no whole-JANUS verification claim |
| J-016 | D/J PASS: bounded register grammar, owner operations denied on world channels | SDF/capDL authority graph reviewed |

M0–M2 acceptance does not cover M3. M4/M5, optional VTX, nested KVM and physical
hardware validation remain NOT RUN. GCC target CONTROL analyzer limitation is
reported explicitly rather than hidden behind passing portable checks.

## Local M3 quota correction, 1 October 2026

See [quota evidence](evidence/m3-quota.md). The PR finding exposed an accounting
gap in the historical M3 results: denied owner-only world requests were uncharged.
The maintainer accepted the correction for publication to PR #3; M3 remains
subject to second review and is not accepted.

| ID | Additional evidence | Boundary |
| --- | --- | --- |
| J-009 | PASS owner_quota: six owner operations, both callers, 64/65 admission boundary, other-world access and owner revoke/rotation; live K passes denied-owner exhaustion under GCC/Clang | Portable matrix and static Microkit World A live exhaustion; no exhaustive scheduling or host-wide DoS claim |
| J-016 | PASS authority/owner_quota and live K: denied owner calls change only caller accounting; malformed calls are uncharged; exhausted replies contain no error data | Channel identity and existing experimental message grammar unchanged |

Environment: this session's Ubuntu 26.04.1, not the historical Debian host.
Changed-file formatting and direct portable analyses pass; whole-M3 formatting
and the Make Clang analyzer target retain tool-version failures. No M3 acceptance,
permanent-substrate or M4/M5 authority follows from this additional evidence.
