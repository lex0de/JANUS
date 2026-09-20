<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M0 maintainer acceptance

Date: 20 September 2026.
Accepted source: 7d4a800ab79c6e5934a81cabe8d21cb5a08097fa.
Authority: explicit maintainer instruction in the subsequent M1 start request.

Danyal A. Samak accepts the reviewed world/object/authority/foreground/device
contracts as the M0 experimental baseline, including explicit delegation policy,
deny-new-then-drain revocation and restore/incarnation/stale-authority semantics.
ADR 0001 is accepted specifically as the reference-model decision.

This is not approval of stable APIs, ABI, wire/disk layouts or enforcement claims.
Known limitations remain: no OS capability enforcement, durable replay protection,
anti-rollback, actual concurrent revocation, physical/DMA device isolation,
durable object storage, trusted physical recovery input or native JANUS kernel.
Previous evidence retains its original review-time status; this record supplies
acceptance, not retrospective test results or independent verification.

The same instruction authorises M1 hosted appliance implementation on a separate
branch and an unmerged PR after this documentation-only commit is pushed to
PRIVATE main. M2–M5 remain outside scope. No runtime code changes in this commit.
