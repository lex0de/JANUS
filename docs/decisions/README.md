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
semantics — EXPERIMENTAL / PROPOSED, not maintainer accepted.

M0.1 corrects implicit delegation authority following maintainer review; ADR 0001
remains EXPERIMENTAL. M0 awaits second review, not acceptance.
