<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# JANUS: current engineering context

## Observed state — 20 September 2026

M0 ACCEPTED by the maintainer on 20 September 2026 at
7d4a800ab79c6e5934a81cabe8d21cb5a08097fa; see docs/evidence/m0-acceptance.md. M0.1 fixes the maintainer's
blocking finding that grant delegation was implicit. Grants now record an
explicit checked delegable policy, default zero; children may only retain or
reduce delegation authority. Representation remains EXPERIMENTAL.

GCC and Clang each pass 7176 assertions plain and under ASan/UBSan with leak
detection. Static analyzers, clang-format and make check pass. Evidence and
whole affected authority review: docs/evidence/m0-1.md and
m0-1-boundary-review.md. Canonical whitepaper and supplied copies/logo unchanged.

The model is trusted caller-owned memory, not enforcing capabilities or OS
isolation. Two worlds, two objects, eight lifetime grant/operation slots; serial
calls. An uncooperative world can keep revocation BUSY. No real backend/kernel.

## Explicit decisions and scope

Maintainer Danyal A. Samak <dabsamak@tuta.com>, https://www.cryogenix.org,
approved licensing on 20 September 2026: independently authored JANUS software
ISC; documentation/whitepaper CC BY 4.0; upstream terms preserved; name/logo
rights retained, no trademark licence. ADR 0002 is approved; ADR 0001 is accepted for the M0 model only. LICENSING.md records inherited instruction-provenance ambiguity.

The current task authorises M0 acceptance on PRIVATE main, then M1 work on
m1-hosted-appliance and an unmerged PR. No visibility change, release or public announcement.
Native substrate, stable ABI/wire/disk formats, hardware/device and key/recovery
policy remain open. M1 is authorised; M2–M5 are not. No required runtime LLM.

## Host/repository checkpoint

Started clean on main at 37a0ae7c8b6ba072e96c9fc51cd09658b48e28dd.
Origin https://github.com/lex0de/JANUS.git; API verified private=true/main.
The commit containing this checkpoint is M0.1; final handoff records commit and
push verification. Existing author identity preserved; no global Git changes.
Debian 13.7, GCC 14.2.0, Clang 19.1.7. No host/package/VM/configuration changes
in this task. Earlier B0 setup remains documented in docs/evidence/m0.md.
Raw logs stay in ignored artifacts/. Actual nested boot/hardware enforcement
remain NOT RUN; indicators alone do not verify them.

## Next permitted action

Push the documentation-only M0 acceptance commit on main, then branch
m1-hosted-appliance for the explicitly authorised hosted C17 broker/client.
Use one disposable UUID-bound guest and isolated presentation, no existing guest
or host policy changes. Stop at M1 READY FOR REVIEW or precise partial/blockers.
Do not merge the M1 PR or start M2. Existing M0 limitations remain unchanged.
