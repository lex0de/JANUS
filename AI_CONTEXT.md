<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# JANUS: current engineering context

## Observed state — 21 September 2026

M0 ACCEPTED by Danyal A. Samak at
7d4a800ab79c6e5934a81cabe8d21cb5a08097fa. Documentation-only acceptance commit
`db4e698cadee038a27bfef19851b79d9ece585bb` pushed to PRIVATE main before branching.
ADR 0001 is accepted only for the experimental reference model: explicit
delegation authority, deny-new-then-drain revocation and stale-incarnation rules.
Its representation is not a stable ABI or enforcement. Historical evidence stays
historical; see docs/evidence/m0-acceptance.md.

M1 ACCEPTED by the maintainer — 20 September 2026, reviewed at
8e910332faafae7b412931d31d3510ef53fe4461. PR #1 merged with normal merge
0817912a81c409d3f53203bff99fa7cfcabbce2a after exact head/commit/path checks.
See docs/evidence/m1-acceptance.md. Acceptance retains experimental limitations.
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
no trademark licence. Inherited guideline rights clarified by the rights holder on 21 September 2026;
CC BY 4.0 applies to his material, preserving third-party exclusions (ADR 0005).
Canonical whitepaper and supplied assets are unchanged.

M1 merge/acceptance and human-facing README cleanup on main are authorised.
M2 is accepted at 61c2830 and merged by 195169b. M3 Microkit 2.3.1 non-VT-x
x86-64 experiment is authorised on m3-capability-substrate; M4–M5 are not. ADR 0003 is accepted for M1 only, not stable product decisions.
No secure kiosk, native capability isolation, durable replay/anti-rollback,
power-loss-tested object storage, physical/DMA isolation, trusted physical recovery,
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

## M2 checkpoint

M2 ACCEPTED — 21 September 2026 by explicit maintainer review at
61c2830e417f1a0106799cfb839523a631744b52. PR #2 was verified exact, then merged
normally as 195169b9c12845cfd26eb8560b951601e0f66f93. Main before merge was
5cd4620, with no unexpected commits. See docs/evidence/m2-acceptance.md.
New C17 object service/owner tools, inherited socketpair sessions, static note app,
Landlock ABI 6 plus seccomp, SQLite current grants/revisions and reconstruction.
Knowing an object ID gives no authority; current grants are checked per admission.
Owner credential plus peer UID protects M2 controls; sandbox cannot connect to M1.
Quiescent revocation only; live delegation explicitly unsupported/default-deny.
See docs/contracts/native-m2.md and ADR 0004, all representations EXPERIMENTAL.

GCC/Clang plain and ASan/UBSan: M0 7176, M1 223, M2 4320 assertions per mode;
M2 live A–L PASS in all four runs, including actual Landlock bypass denial and
four real storage-process crash stages (71 additional assertions per live run).
Static note is not ASan-instrumented; trusted service/tools/store are. No claims
of sudden-power-loss safety, secure deletion, anti-rollback or whole-system
confinement. Boundary review retains single-agent shared-assumption risk.

Only new M2 host package: libsqlite3-dev 3.46.1-7+deb13u2, 3480 kB installed,
no upgrades; removal-only rollback simulated. No removed viewer package restored.
No M2 VM/display/network/device/configuration changes; nested KVM NOT RUN.
Live owned children are stopped; private DBs/keys/logs remain ignored in artifacts/.
M1 merge 0817912; acceptance 153734a; human README 1906f99 and whitespace fix
5cd4620 were pushed on main before M2. See docs/evidence/m2.md and dependency record.

## M3 checkpoint

M3 READY FOR REVIEW, not accepted. Branch m3-capability-substrate is based on
2b271799; implementation 0a4bd5f78ff963b2af960fcf03f74c9ff3cdc3da. Microkit
2.3.1 official SDK signature and pinned fingerprint verified;
non-VTX x86_64_generic/release boots under QEMU TCG with GCC and Clang PD builds.
Two world PDs use distinct channel badges; owner authority stays in CONTROL.
Live A–L PASS, including private-page fault containment, stale/revoked handles,
activity revalidation, quota exhaustion and caller-context service work with
independent CONTROL progress. Incarnation rotation is semantic, not a PD restart.
See docs/evidence/m3.md, m3-boundary-review.md, m3-dependencies.md and ADR 0006.

Host M0/M1/M2 tests unchanged: 7176/223/4320 assertions per GCC/Clang plain and
ASan/UBSan run. Final M3 portable tests: 8907 checks in all four modes. Portable
static analyses and all Clang target analyses pass. GCC target CONTROL analyzer
reports an SDK error-path uninitialised-result diagnostic after its intentional
low-page crash; retained FAIL/coverage limitation, not suppressed. Other GCC PD
analyses pass. Exact kernel proof inheritance NOT ESTABLISHED (MCS/config limits).
No host package/policy changes; SDK/build/raw logs ignored; owned QEMU children
reaped. Optional VTX/nested KVM/physical tests NOT RUN. No M4/M5 or persistence port.

## Next permitted action

Finish scoped M3 publication/evidence checkpoint and obtain maintainer review.
Do not merge M3, select a permanent substrate, begin M4/M5, or reinstall removed
viewer packages. Static composition, live delegation and actual PD reconstruction
remain open. Single-agent test/review shared-assumption risk remains.

Repository PUBLIC by explicit maintainer instruction, verified after history audit.
Owned inherited guideline CC BY 4.0 clarification is recorded in ADR 0005,
LICENSING.md and docs/SOURCES.md, preserving the original source SHA-256.
No release/tag or third-party relicensing occurred.
