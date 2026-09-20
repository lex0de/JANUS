# JANUS

A proposed personal operating system of isolated worlds, explicit authority,
persistent activities, and controlled hardware ownership.

**Danyal A. Samak** <dabsamak@tuta.com>
https://www.cryogenix.org

## Current state

**M0 READY FOR REVIEW**: candidate contracts, threat model and a bounded C17
reference model with executable tests. No JANUS kernel, secure capability
runtime, or working guest appliance is shipped here. Start with the
[whitepaper](docs/architecture/JANUS_Whitepaper_v0.1.md) and
[bootstrap plan](BOOTSTRAP_PLAN.md). A world is not necessarily a VM; an activity
is not automatically a security domain. LLMs are engineering tools, not required
runtime services.

## Start development

Launch Codex locally from this repository and paste `CODEX_START.md`.
`AGENTS.md` supplies durable rules; `AI_CONTEXT.md` records current status.
See `docs/development/CODEX.md` for config, skills and context handling.

```sh
make check           # bootstrap files, syntax and configuration only
make preflight       # non-elevated local host inventory
make deps-plan       # Debian 13 apt simulation, no installation
make toolchain       # GCC/Clang C17/C++17 compiler and sanitizer probes
make test CC=gcc     # M0 model tests; repeat with CC=clang
make test-sanitize CC=gcc # ASan/UBSan; repeat with CC=clang
```

Before editing a freshly extracted bundle, run `sha256sum -c BOOTSTRAP.SHA256`.
This is a delivery-integrity manifest, not a build dependency or a signature.
It will intentionally become stale when source files change; do not regenerate
it as a substitute for reviewing changes. See
[bundle validation](docs/evidence/bundle-validation.md) for checks performed
on the supplied files, separate from work on the development host.

Read a helper before running it. Only `tools/install-deps.sh --apply` performs
package changes, using sudo/doas if not already root. No helper creates GitHub
repositories, changes libvirt domains, configures networking, enables nesting,
or overwrites host configuration. Raw logs go into ignored `artifacts/`.

The first Codex task is B0 setup and M0 candidate contracts/model tests, followed
by review. It is not authority to replace Debian, choose a kernel, or deploy a
kiosk on the host desktop. Details: [host](docs/development/HOST.md),
[VM lab](docs/development/VM_LAB.md), [GitHub](docs/development/GITHUB.md),
and [coding](docs/development/CODING.md).

## Source and status

The whitepaper is retained unmodified. `docs/SOURCES.md` identifies the source
materials and external tool documentation used for this bootstrap. Supplied PDF, DOCX and PNG assets are preserved without alteration; no new
artwork was generated. The canonical design source is the architecture Markdown.

Release licensing is undecided. See `LICENSING.md`. Private repository creation
is the conservative bootstrap default, not a public open-source release.

See [candidate contracts](docs/contracts/README.md), [M0 evidence](docs/evidence/m0.md)
and [boundary review](docs/evidence/m0-boundary-review.md). These tests establish
model semantics only, not enforced isolation or durable storage. M1 awaits owner
approval. Release licensing remains undecided.
