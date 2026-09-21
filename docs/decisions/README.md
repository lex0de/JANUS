<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Decision register

An ADR is a record, not permission. Accepted records cite the maintainer's actual
approval and scope. Drafts and experimental defaults must remain labelled.

| Topic | Status | Basis / next action |
| --- | --- | --- |
| JANUS is the entire OS | USER DIRECTED | Current project request and whitepaper |
| C/C++, OpenBSD-inspired engineering | USER DIRECTED | Current bootstrap request |
| Debian 13 development host; scoped sudo/doas | USER DIRECTED | Current bootstrap request; inventory recorded in docs/evidence/m0.md |
| New GitHub repository | USER DIRECTED | Current request; private lex0de/JANUS created and implementation commit verified |
| Private visibility, main branch, M0-only start | USER APPROVED FOR B0/M0 | Explicit approval in 20 September 2026 task |
| C17/C++17 hosted tests, small Make build | EXPERIMENTAL DEFAULT | Not a native kernel/toolchain decision |
| Native kernel/framework | OPEN | Compare existing substrates at M3 |
| Persistence and authority wire/disk formats | OPEN | M0 test formats do not settle them |
| Physical target and device assignment | OPEN | Inventory and explicit test authority needed |
| Recovery and cryptographic/key policy | OPEN | Threat model and maintainer review |
| Software/documentation licensing and branding | MAINTAINER APPROVED — 20 September 2026 | [ADR 0002](0002-licensing.md): ISC / CC BY 4.0 / branding excluded |
| Public visibility, release and announcement | NOT AUTHORISED | Repository remains PRIVATE |

Create numbered ADRs only when work reaches the decision. Use
`docs/templates/ADR.md`. Record alternatives, evidence, consequences, rollback,
approval source and revisit conditions. Do not create a dozen empty ADRs to
simulate progress or silently edit the whitepaper to hide a design change.

[ADR 0001](0001-m0-model.md): bounded M0 representation and serial transition
semantics — ACCEPTED for M0 on 20 September 2026; representation EXPERIMENTAL.

M0.1 corrects implicit delegation authority following maintainer review; ADR 0001
is accepted as the M0 reference-model decision; layouts remain EXPERIMENTAL.
M1 hosted appliance was subsequently accepted; M2 is explicitly authorised.
M3 is now explicitly authorised as a substrate experiment; M4–M5 are not.

[ADR 0003](0003-m1-hosted.md): bounded Linux-hosted M1 broker, UUID profiles,
viewer lifecycle and experimental execution records. ACCEPTED for the bounded M1
experiment; representations remain EXPERIMENTAL, with no stable format or
native-substrate approval. Evidence: [M1](../evidence/m1.md).

M1 ACCEPTED — 20 September 2026, at reviewed head 8e910332; PR #1 merged by
normal merge 0817912. [Acceptance](../evidence/m1-acceptance.md) defines the
bounded scope of accepted ADR 0003. M2 native experience is now explicitly
authorised on a separate branch; M3–M5 are not. Earlier entries describe their
original decision-time scope.

[ADR 0004](0004-m2-hosted-native.md): M2 SQLite object backing, current grants,
private inherited sessions, Landlock/seccomp and owner authentication. EXPERIMENTAL
representation, ACCEPTED for bounded M2 on 21 September 2026 at 61c2830.
See [acceptance](../evidence/m2-acceptance.md). Not a final IPC/storage design.

M3 Microkit 2.3.1 non-VT-x x86-64 experiment explicitly authorised; permanent
substrate selection remains OPEN. The maintainer also explicitly requested public
repository visibility; publication provenance clarification is pending separately.
