# JANUS bootstrap and first engineering task

Current checkpoint: B0 complete; M0/M1 accepted on 20 September 2026; M2 accepted
on 21 September 2026. M3 capability-substrate work is explicitly authorised. The original B0/M0
execution plan below is retained; actual evidence is in `docs/evidence/`.
This is not evidence of completed work. B0 is tooling setup, not a renamed
whitepaper milestone. M0-M5 retain the whitepaper's section 14 definitions.

## B0: establish a recoverable development baseline

1. Inspect ancestor/repository instructions, working directory, Git boundaries,
   status, identity, remotes, local edits, and the files in this bundle.
2. Read the whitepaper, `AGENTS.md`, `AI_CONTEXT.md`, `docs/REQUIREMENTS.md`, and
   decision status. Reconcile conflicts in favour of explicit user decisions.
3. Run `make check`; then `tools/host-preflight.sh`. Save private outputs only
   inside `artifacts/`. Record exact commands, exit codes, tools and environment.
4. Review `make deps-plan`. Use `tools/install-deps.sh --apply` only for justified
   missing packages, through the normal sudo/doas and agent approval path.
   Run `make toolchain`. Investigate compiler/sanitizer failures rather than
   weakening checks to get green output.
5. Initialise a standalone JANUS Git root on main after checking parents. Use
   the supplied author identity locally only when no suitable identity exists.
   No global configuration or parent-tree changes.
6. Prepare the bounded M0 work plan. The private GitHub repository may be created
   once initial contents have been reviewed; actual push includes only reviewed
   source/docs/tests. Follow `docs/development/GITHUB.md`.

Exit: actual host/toolchain recorded, local work preserved, setup changes listed,
private repository verified when reachable, and an explicit scope for M0.
Remote/auth trouble may block repository publication without blocking local M0.
No host reboot, live guest, networking change or firmware operation is required.

## M0: contracts and executable reference models

Deliver the whitepaper's contracts, not a kernel or a GUI demo. Separate three
kinds of output: candidate specification; executable reference model; enforcement
that has NOT yet been implemented. Never name an in-memory model a security test
of operating-system isolation.

### Specifications to draft

Create focused `docs/contracts/` documents for world lifecycle and incarnation,
object identity/revisions/transactions, grant issue/attenuation/revocation/restore,
foreground and console events, and device handover/quarantine. State preconditions,
postconditions, invalid transitions, concurrency boundary, error results, bounded
resources, and persistence behaviour. Mark formats and APIs EXPERIMENTAL.

The Appendix A YAML is design notation. Do not announce YAML, JSON, TOML, a numeric
handle scheme, or a C struct layout as the final manifest/IPC/disk contract.
Choose the smallest test-only representation and document what it does not prove.

Draft a threat model tied to the hosted and eventual native trust boundaries.
Show what the host, broker, storage, presentation and recovery services can do.
Do not reproduce the whitepaper as a second long narrative.

### Executable work

Implement a small portable C model library and table-driven tests. Create source
subdirectories only when code needs them, for example `include/janus/`,
`lib/contract/`, and `tests/contract/`. Namespace new interfaces with `janus_`.
No network daemon, actual libvirt control, database dependency, custom cryptography,
GUI toolkit, full driver framework, or fake syscall layer is needed for M0.

Test the required distinctions rather than only the happy path. Every exported
transition must reject invalid enum values and invalid state combinations without
partial mutation. Cover out-of-range lengths, exhausted counters and allocations
where applicable. Use fixed regression seeds and retain failing fuzz inputs.

Minimum scenarios are in `docs/REQUIREMENTS.md`. Include an independent table of
expected outcomes derived from the specification; tests that merely re-execute
the implementation's own decision function are not an independent oracle.

Add meaningful `make test` and `make test-sanitize` targets when the model exists.
Run GCC and Clang. Document each exact invocation and failure. `make check` remains
bootstrap hygiene; do not redefine it as evidence of M0 runtime enforcement.

### Evidence and review

Update requirement rows with actual test identifiers and evidence paths. Record
what is modelled versus enforced. Draft ADRs for choices needing acceptance;
private experiments may proceed without inventing a release licence. Prepare a
small M1 plan showing the viewer/execution distinction and owner-recovery path.

Use the boundary-review and evidence skills where available. Inspect staged
content, commit the bounded M0 work under this prompt's authorisation, and push
only to the verified private remote. A failed push is not a failed local test.

**Stop here:** report M0 READY FOR REVIEW if the candidate contracts and tests are
complete, otherwise state the remaining failures. Human review is part of M0's
exit evidence. Do not call M0 accepted or begin M1 without approval.

## Later stages: direction only, not bootstrap authority

| Stage | Whitepaper deliverable | Gate before starting |
| --- | --- | --- |
| M1 | Hosted guest activity with select/run/recover/leave | Approved M0; specific lab topology and resources |
| M2 | Native tool, object grants and activity reconstruction | Reviewed enforcing boundaries and persistence plan |
| M3 | Narrow contracts on a capability substrate | Substrate comparison/ADR and actual supported toolchain |
| M4 | One assignable device class with reset/quarantine | Named hardware, isolation topology and recovery authorisation |
| M5 | Daily-use alpha, updates and tested backup | Release/licence policy, restore exercise and measurements |

Review gates are not an excuse to stop early on independently testable authorised
work. They prevent an agent from promoting a prototype choice into the product.
