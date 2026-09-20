<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# JANUS: current engineering context

## Observed state — 20 September 2026

M0 READY FOR SECOND REVIEW, not maintainer accepted. M0.1 fixes the maintainer's
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
rights retained, no trademark licence. ADR 0002 is approved, unlike experimental
model ADR 0001. LICENSING.md records inherited instruction-provenance ambiguity.

The current task authorises the bounded correction/licensing commit and push to
existing PRIVATE origin/main. No visibility change, release or public announcement.
Native substrate, stable ABI/wire/disk formats, hardware/device and key/recovery
policy remain open. No M1 authority. No required runtime LLM.

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

Complete the authorised reviewed private commit/push, then stop at
M0 READY FOR SECOND REVIEW. Maintainer review is required before any M1 work.
See docs/plans/m0-1.md, docs/REQUIREMENTS.md and docs/decisions/README.md.
