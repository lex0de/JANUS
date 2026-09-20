# JANUS: current engineering context

## Observed state — 20 September 2026

M0 READY FOR REVIEW, not maintainer accepted. Candidate contracts/threat model,
EXPERIMENTAL ADR 0001, portable C17 reference model, independent expected tables,
and real Make tests exist. GCC and Clang each pass 6920 assertions plain and
under ASan/UBSan with leak detection. Static analysis and bootstrap checks pass.
See docs/evidence/m0.md and docs/evidence/m0-boundary-review.md for failures,
exact commands and limitations. Whitepaper remains byte-for-byte unchanged.

The model is caller-owned memory with trusted actor labels, not enforcing
capabilities or OS isolation. Two worlds, two synthetic objects, eight lifetime
grant/operation slots; serial calls only. No kernel, guest manager or kiosk.

## Explicit approvals and unresolved choices

User authorised B0/M0, standalone main, private personal GitHub repository,
scoped commits/pushes, C17/optional restrained C++17 and Make. Existing configured
Git identity was preserved; maintainer is Danyal A. Samak <dabsamak@tuta.com>,
https://www.cryogenix.org. No global Git configuration changed.

Native substrate, stable ABI/wire/disk formats, hardware/device policy,
cryptography/key recovery, public licensing/distribution remain undecided.
ADR 0001 semantics remain experimental proposals. JANUS names the whole proposed
OS. Worlds need not be VMs; activities do not merge authority. No runtime LLM.

## Host and repository checkpoint

Debian 13.7 x86-64 host inspected outside the restricted sandbox; no virtualisation
detected by systemd-detect-virt. KVM accessible and nesting indicator enabled;
actual L2 boot NOT RUN. Existing libvirt workloads untouched. Raw inventory in
ignored artifacts/. No host configuration, network, firmware or guest changes.
Eight reviewed development packages installed; versions and rollback in evidence.
Sandbox LeakSanitizer failed under ptrace; host probes/model sanitizer runs pass.

Standalone Git main initialised. Authenticated GitHub personal account lex0de
verified; lex0de/JANUS returned 404. Reviewed initial commit/publication pending.
Supplied PDF/DOCX/PNG assets exist and are preserved; no new artwork generated.

## Next permitted action

Finish reviewed private publication, then stop for maintainer M0 review.
The proposal in docs/plans/m1-candidate.md is not authority to implement M1.
Read docs/architecture/JANUS_Whitepaper_v0.1.md for architecture, docs/REQUIREMENTS.md
for evidence IDs, and docs/decisions/README.md for decisions. No native substrate,
guest management or device assignment until the owner approves the next scope.
