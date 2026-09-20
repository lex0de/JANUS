<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# ADR 0002: JANUS licensing and branding

Status: **MAINTAINER APPROVED — 20 September 2026**.
Decision source: Danyal A. Samak's explicit M0.1 task instruction, section
"Maintainer-approved JANUS licensing policy". This is a real project decision,
not the agent's proposal or acceptance of the M0 model.

## Decision

ISC is the default for independently authored JANUS core software, libraries,
SDK, native applications and drivers. Imported/derived drivers and other external
components retain all applicable upstream copyright and licence terms.
Documentation and the JANUS whitepaper are CC BY 4.0. Rights in the JANUS name
and logo remain with Danyal A. Samak; neither licence grants branding permission
as an indication of official origin, endorsement or compatibility. No trademark
registration is asserted. `docs/JANUS_logo.png` is excluded from the documentation
grant. See LICENSING.md for scope and provenance exceptions, and TRADEMARKS.md.

Copyright holder: Danyal A. Samak <dabsamak@tuta.com>, 2026.
Project: https://www.cryogenix.org

LICENSE is a mixed-licence overview, not an ISC blanket grant. LICENSES/ISC.txt
contains canonical ISC terms with the maintainer's copyright notice.
LICENSES/CC-BY-4.0.txt identifies the canonical Creative Commons legal code.
The whitepaper remains byte-for-byte unchanged; this decision and LICENSING.md
establish its CC BY 4.0 status despite historical licensing-open wording in it.

## Alternatives and consequences

Proprietary licensing would retain tighter distribution control but impede open
reuse. Stronger copyleft could require covered derivative software to preserve
sharing terms, with different integration obligations. The maintainer selected
ISC for permissive software reuse and CC BY 4.0 for attributed document reuse.
Keeping licensing undecided is superseded by this explicit decision.

This does not remove third-party obligations or supply missing provenance.
Per-file SPDX markers are applied only where independent authorship is supported.
The inherited instruction-guideline licence gap is recorded in LICENSING.md;
the maintainer's grant cannot relicense rights held by others. No imported code
has been mechanically or AI-rewritten to make it ISC.

Repository visibility remains PRIVATE. This task authorises a private commit
and push, not a public announcement, release, third-party distribution, branding
grant or visibility change. Copyright licensing and publication approval are
separate decisions; these workflow limits are not additional licence terms.

## Revisit and rollback

Revisit before a licence exception, new third-party import, public distribution,
or any future branding permission. Record the source/revision/notices and
compatibility implications before committing affected code. A new maintainer
decision may govern future work but cannot simply revoke rights already granted
under applicable existing licences. Rollback of repository files is not a legal
revocation mechanism. M0 acceptance and all M1 engineering gates remain separate.
