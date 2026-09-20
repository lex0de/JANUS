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
