<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M1 maintainer acceptance

**M1 ACCEPTED — 20 September 2026**

Authority: explicit maintainer instruction following review of
8e910332faafae7b412931d31d3510ef53fe4461, PR #1. Before merging, verified PRIVATE
lex0de/JANUS; PR open/unmerged, base main, head m1-hosted-appliance, exact SHA;
three expected commits and 24 expected changed paths; main at M0 acceptance
db4e698cadee038a27bfef19851b79d9ece585bb. No unexpected changes or local edits.
`gh pr merge 1 --repo lex0de/JANUS --merge --match-head-commit
8e910332faafae7b412931d31d3510ef53fe4461` succeeded. Normal merge commit:
0817912a81c409d3f53203bff99fa7cfcabbce2a. Reviewed commits remain intact.

Acceptance covers the bounded Linux/libvirt experiment: UUID identity, bounded
owner operations, execution separate from viewer state, guest survival after
viewer loss/leave, re-entry into the same execution, reconciled broker restart,
new incarnation after stop/start, guest-independent software viewer recovery,
and accurate separation of hosted TCB from native JANUS enforcement.

It does not establish secure kiosk, native kernel/capability enforcement, trusted
physical recovery, device/DMA isolation, durable authority, anti-rollback, stable
IPC/profile/storage formats, nested KVM or power-loss-safe JANUS storage.
ADR 0003 is accepted only within that experimental scope.

Historical M1 evidence retains its original READY FOR REVIEW status and remains
valid. The tested viewer/SPICE/USB-helper packages were deliberately removed
afterward; live reruns remain BLOCKED pending separately approved dependency
setup. Nothing was reinstalled for acceptance. No release/tag/visibility change.

The same instruction authorises human-facing documentation cleanup on main and
M2 on m2-native-experience after that cleanup is pushed. M3–M5 are not authorised.
