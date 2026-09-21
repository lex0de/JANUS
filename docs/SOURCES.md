# Bootstrap sources and provenance

## Requested source material

The whitepaper is copied byte-for-byte to
`docs/architecture/JANUS_Whitepaper_v0.1.md`.

- Title: JANUS: A Personal Operating System of Isolated Worlds.
- Version/date: 0.1, 20 September 2026.
- Author: Danyal A. Samak <dabsamak@tuta.com>.
- SHA-256: `9736f4acfc70940a15a91924e157b1e7ab547ee17302913268bd7fd7e0ba45b7`.

The user-supplied AGENTS.md was used as the engineering/workflow guideline.
Its original heading described a different System V project. It is deliberately
not installed verbatim or bundled as an active instruction file: its source-gate,
machine, SSH and architecture details are unrelated to JANUS.

- Source-file SHA-256: `b884801d390206ee2b0f3dbbb108e0f3ac6ee14da72e093b5cc50233941d1658`.
- Retained: evidence discipline, bounded changes, host preservation, provenance,
  explicit review gates, C/style, failure handling and honest validation.
- Updated by current user request: C++ is permitted; Debian hosts the native
  hosted prototypes; nested VM experiments are possible after verification;
  one new JANUS GitHub repository is requested; new author identity applies.
- Not inherited: illumos/System V architecture, Mercuron directory/gate rules,
  machine names/addresses, old SSH-key policy, or an automatically chosen licence.

The new requirement IDs, workflow defaults, scripts and file organisation are
bootstrap proposals/implementation aids. They are not text claimed to have been
specified in the whitepaper. No logo artwork was available for this bundle.

## External tool references

Checked 20 September 2026. These references support tool configuration or style,
not JANUS architectural approval. Recheck installed versions before execution.
The original whitepaper's references are preserved as supplied, not all re-audited
as part of this bootstrap task.

[AGENTS] OpenAI, Custom instructions with AGENTS.md.
https://developers.openai.com/codex/guides/agents-md

[SKILLS] OpenAI, Agent skills / Build skills.
https://developers.openai.com/codex/skills

[CONFIG] OpenAI, Configuration reference.
https://developers.openai.com/codex/config-reference

[STYLE] OpenBSD, style(9).
https://man.openbsd.org/style.9

[GH] GitHub CLI, gh repo create.
https://cli.github.com/manual/gh_repo_create

[NESTED] Linux kernel, Running nested guests with KVM.
https://docs.kernel.org/virt/kvm/x86/running-nested-guests.html

[DEBIAN-CLANG] Debian 13 package records for the default Clang compiler and
compiler-rt development/runtime support. The package manifest explicitly includes
libclang-rt-dev because the installer does not install recommendations.
https://packages.debian.org/trixie/clang
https://packages.debian.org/trixie/libclang-rt-dev

For every later source import, add a separate provenance record with exact
upstream revision, original file paths, licence/notices, modifications, build-only
versus shipped usage, and validation. A URL alone is not a pinned dependency.

## M0 additions, 20 September 2026

Candidate contracts and C model/tests were newly written in this authorised
session, derived from the retained whitepaper and candidate rules; no upstream
implementation code was copied or imported. No release licence selected.

The actual working directory also contains supplied PDF/DOCX whitepaper copies,
a duplicate Markdown copy, and docs/JANUS_logo.png. These pre-existing assets
are preserved unchanged; the earlier bundle's artwork-unavailable note describes
its delivery state, not the current checkout. No artwork was generated in M0.
PDF/DOCX author metadata identifies Danyal A. Samak; private inclusion is not a
public redistribution licence. The canonical design source remains the retained
architecture Markdown and its digest above.

## Guideline rights clarification — 21 September 2026

Danyal A. Samak identifies the earlier supplied guideline as his own Mercuron /
System V project AGENTS.md and confirms rights in the original independently
authored wording. He expressly permits JANUS to use, adapt, modify and
redistribute that owned material under CC BY 4.0 from 21 September 2026.
Copyright (c) 2026 Danyal A. Samak <dabsamak@tuta.com>.

Source-file SHA-256 remains
`b884801d390206ee2b0f3dbbb108e0f3ac6ee14da72e093b5cc50233941d1658`.
This is a rights-holder clarification, not a guessed upstream licence or a new
source revision. The ambiguity is resolved for his material only. Third-party
quotations/code/licence texts and other independent rights are excluded and keep
their applicable terms. Historical evidence records the then-unresolved state.
See [ADR 0005](decisions/0005-guideline-provenance-publication.md) and LICENSING.md.
