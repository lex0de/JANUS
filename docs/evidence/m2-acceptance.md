<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M2 maintainer acceptance

**M2 ACCEPTED — 21 September 2026.** Danyal A. Samak explicitly accepted the
bounded Linux-hosted native-world/object experiment reviewed at
`61c2830e417f1a0106799cfb839523a631744b52`.

PR #2 was PRIVATE, OPEN/unmerged, base main, head m2-native-experience at that
exact SHA. Main remained `5cd4620da5872df4703e84911e81e00a324ca36b`. Its three
reviewed commits and 26 changed paths matched the local history. Normal merge
commit: `195169b9c12845cfd26eb8560b951601e0f66f93`. No squash or force-push.

Acceptance covers object-ID/authority separation, current READ/WRITE grants,
tested Linux backing-store bypass denial, stale session/incarnation rejection,
current authority on activity reconstruction, revocation surviving old activity
state, stale-base conflict, complete old/new SQLite revisions after process
interruption, explicit commit/ack uncertainty, owner-path exclusion and bounded
resources. ADR 0004 is accepted for this experiment only.

It does not establish a native JANUS kernel, whole-system Linux confinement,
anti-rollback, sudden-power-loss proof, secure deletion, physical recovery,
DMA/device isolation, secure kiosk, live delegation or stable IPC/storage formats.
The existing evidence's READY FOR REVIEW wording remains historical and unchanged.
Accepted [evidence](m2.md) and [boundary review](m2-boundary-review.md) retain their
limitations, including static-note sanitizer and single-agent-review coverage.

M3 is now authorised as a Microkit 2.3.1 capability-substrate experiment, not a
permanent kernel/framework decision. M4 and M5 remain out of scope. No new live
M1/M2 validation is needed for this documentation-only acceptance change.
