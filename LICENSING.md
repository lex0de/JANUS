<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# JANUS licensing policy

**Maintainer approved, 20 September 2026.** This supersedes the previous
"licensing undecided" status. Decision: [ADR 0002](docs/decisions/0002-licensing.md).
Copyright (c) 2026 Danyal A. Samak <dabsamak@tuta.com> for independently authored
JANUS material. Project: https://www.cryogenix.org.

| Material | Approved policy |
| --- | --- |
| Core JANUS software | ISC |
| JANUS libraries / SDK | ISC |
| JANUS native applications | ISC by default |
| Independently written JANUS drivers | ISC |
| Imported or derived drivers | All applicable original licences retained |
| External components | Their existing licences |
| JANUS documentation | CC BY 4.0 |
| JANUS whitepaper | CC BY 4.0 |
| JANUS name and logo | Rights retained by Danyal A. Samak; no trademark licence |

[LICENSE](LICENSE) is an overview of this mixed structure, not a blanket ISC
licence. [ISC text](LICENSES/ISC.txt) is canonical ISC wording with JANUS's
copyright notice. [CC BY 4.0 reference](LICENSES/CC-BY-4.0.txt) links to the
canonical Creative Commons legal code; its legal terms are not paraphrased here.
Sources checked on 20 September 2026:
[SPDX ISC text](https://spdx.org/licenses/ISC.html),
[ISC licence source](https://www.isc.org/licenses/), and
[CC BY 4.0 legal code](https://creativecommons.org/licenses/by/4.0/legalcode).

## Current scope and provenance

The M0 model in `include/janus/model.h`, `lib/contract/model.c` and
`tests/contract/test_model.c` is newly authored JANUS software, as recorded in
docs/SOURCES.md and the bootstrap evidence. These files carry ISC SPDX markers.
The independently authored Make build, toolchain/helper software in tools/ and
tests/bootstrap/ and tests/toolchain/ are also ISC under this policy. Configuration
authored for this project follows the software default; third-party material does
not become ISC simply by being placed beside it.

Independently authored JANUS prose, contracts, decisions, plans, evidence and
workflow documentation are CC BY 4.0. New/touched documentation is marked where
practical. Instructions adapted from other sources retain any applicable source
terms; see the explicit provenance gap below rather than inferring a grant from
their filename. Canonical licence texts/references retain their own status and
are not relicensed as JANUS software.

The maintainer explicitly grants CC BY 4.0 for the JANUS whitepaper content in:

- `docs/architecture/JANUS_Whitepaper_v0.1.md` (canonical, unchanged bytes);
- `docs/JANUS_Whitepaper_v0.1.md`;
- `docs/JANUS_Whitepaper_v0.1.docx`;
- `docs/JANUS_Whitepaper_v0.1.pdf` and `docs/JANUS_Whitepaper_v0.1-1.pdf`.

This applies to the maintainer's whitepaper content, not an automatic relicensing
of fonts, embedded third-party assets or quoted material in a container. The
supplied documents identify Danyal A. Samak as author. Historical licence-open
wording in immutable whitepaper copies and earlier evidence describes the prior
state and is superseded by this decision. No whitepaper bytes need alteration.

**`docs/JANUS_logo.png` is excluded from CC BY 4.0 and ISC.** The JANUS name and
logo are covered by [TRADEMARKS.md](TRADEMARKS.md). Neither software nor document
licences grant permission to use JANUS branding as an indication of official
origin, endorsement or compatibility. No trademark registration is claimed.

Provenance gap: docs/SOURCES.md records adaptation of an earlier project's
instruction guideline (source digest b884801d390206ee2b0f3dbbb108e0f3ac6ee14da72e093b5cc50233941d1658),
but no upstream licence or exact revision is recorded for that guideline. Do not
infer permission to relicense any inherited expressive text in AGENTS.md or
related adapted guidance. Newly authored JANUS additions follow this policy;
inherited portions require source-rights clarification before redistribution.
No whole-file SPDX grant is added to AGENTS.md for that reason. No upstream
implementation code import was identified in the current model change. Existing
notices remain intact; this task makes no new third-party redistribution claim.

## Permanent contribution and import rule

New independently authored JANUS software should normally be
licensed under the ISC licence.

Imported or derived code retains all applicable upstream copyright
and licence terms. No contributor or automated tool may relicense
third-party material merely by rewriting, translating, or
reformatting it.

Exceptions to the project's default licence require an explicit
licensing decision before the affected code is committed.

AI-generated rewriting does not erase source provenance or licence obligations.
Before importing source, record the upstream project, source URL, exact
revision/version, imported file paths, applicable licence, required notices and
attribution, local modifications, and any redistribution restrictions. Preserve
those terms and notices. Do not copy incompatible donor code into JANUS and
attempt to convert it to ISC through mechanical or AI-assisted rewriting.
Generated-from-upstream, vendored, firmware, binary or other third-party material
is never automatically covered by the JANUS default grants.

## Publication boundary

This decision authorises policy/licence files and the scoped M0.1 changes to be
committed and pushed to the existing PRIVATE lex0de/JANUS repository. It does not
authorise changing visibility, a release, public announcement, third-party
distribution, trademark registration claims, branding permission or third-party
relicensing. These are project workflow limits, not modifications to licence terms.
