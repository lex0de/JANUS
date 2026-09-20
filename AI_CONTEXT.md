<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# JANUS: current engineering context

## Observed state — 20 September 2026

M0 ACCEPTED by Danyal A. Samak at
7d4a800ab79c6e5934a81cabe8d21cb5a08097fa. Documentation-only acceptance commit
`db4e698cadee038a27bfef19851b79d9ece585bb` pushed to PRIVATE main before branching.
ADR 0001 is accepted only for the experimental reference model: explicit
delegation authority, deny-new-then-drain revocation and stale-incarnation rules.
Its representation is not a stable ABI or enforcement. Historical evidence stays
historical; see docs/evidence/m0-acceptance.md.

M1 hosted implementation on m1-hosted-appliance is READY FOR REVIEW, not accepted.
PR https://github.com/lex0de/JANUS/pull/1 is OPEN against main, unmerged;
implementation 527cf1b and evidence/rollback 75fc276 were pushed.
C17 janusd/janusctl use owner-local authenticated bounded commands, trusted
URI/UUID profiles, libvirt C API, owned virt-viewer children and experimental
execution records. Viewer loss/leave/recovery preserve guest execution. Restart
reconciles exact UUID/epoch and never imports viewer/foreground from a PID file.
See docs/development/HOSTED_M1.md and docs/evidence/m1.md/m1-boundary-review.md.

M0 source/tests unchanged: GCC/Clang each pass 7176 assertions plain and ASan/UBSan.
M1: 223 unit/process assertions per compiler/mode; both static analyzers, formatting
and bootstrap checks pass. Live A–L pass with GCC/Clang plain and ASan/UBSan,
including actual guest boot and VNC connection on isolated headless Wayland.
Initial ACPI and viewer-runtime test failures are retained in evidence.

## Explicit decisions and limits

Maintainer: Danyal A. Samak <dabsamak@tuta.com>, https://www.cryogenix.org.
Approved licensing (20 September 2026, ADR 0002): independent software ISC;
docs/whitepaper CC BY 4.0; upstream terms preserved; name/logo rights retained,
no trademark licence. Inherited instruction-provenance ambiguity remains recorded.
Canonical whitepaper and supplied assets are unchanged.

M1 branch commits/private push/unmerged PR are authorised; merge and M2–M5 are not.
ADR 0003 records experimental hosted choices, not stable product decisions.
No secure kiosk, native capability isolation, durable replay/anti-rollback,
durable object storage, physical/DMA isolation, trusted physical recovery,
native kernel or stable public API. M0 remains serial caller-owned memory and
revocation can stay BUSY. Backend is local monolithic libvirtd only; same-user
administration is trusted. Viewer PID is not a graphics-readiness guarantee.

## Host/repository checkpoint

Origin https://github.com/lex0de/JANUS.git; private=true/main verified via gh.
Existing author identity preserved; no global Git configuration changes.
Debian 13.7, Linux 6.12.107+deb13-amd64, GCC 14.2.0, Clang 19.1.7.
Installed libvirt-dev, virt-viewer and Weston plus 18 dependencies after simulation.
The audit found an unwanted setuid USB helper/active-session policy. On the
maintainer's explicit instruction, removed virt-viewer, both new SPICE client
libraries and the helper; verified executable/helper/policy absent. Seventeen
new packages remain, no upgrades. Live evidence predates this cleanup; new live
runs are BLOCKED pending an approved viewer dependency setup. Exact versions/rollback:
docs/evidence/m1-dependencies.md. Normal libvirt on-demand daemons are recorded.
No desktop/network/firmware/KVM module or custom policy reconfiguration.

One qemu:///session KVM fixture, UUID c4a20229-6664-49ce-96f1-b8b267d66965,
was stopped/undefined after tests. Guest boot probe uses the installed Debian
kernel, no downloaded image. Session domain UUID inventory matches the pre-test
inventory. No existing VM was operated on. Raw logs/generated images stay in
ignored artifacts/; generated builds in out/. Nested L2 boot NOT RUN.

## Next permitted action

Review the unmerged M1 PR and decide the next scope. Do not merge or begin M2.
Commit/PR identities belong in the final handoff and Git history; verify current
Git state before resuming. M1 test success is not maintainer acceptance.
