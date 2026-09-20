<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# JANUS

A proposed personal operating system of isolated worlds, explicit authority,
persistent activities, and controlled hardware ownership.

**Danyal A. Samak** <dabsamak@tuta.com>
https://www.cryogenix.org

## Current state

**M0 ACCEPTED; M1 READY FOR REVIEW** on `m1-hosted-appliance`.
The experimental C17 hosted broker/client distinguish guest execution, viewer
and presentation ownership, with live libvirt and isolated Wayland tests.
The tested viewer packages were subsequently removed at the maintainer's request
to undo unwanted USB-access policy; live reruns need an approved dependency setup.
No native JANUS kernel or secure capability runtime is shipped here. Start with
the
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
make test-sanitize CC=gcc # M0 ASan/UBSan; repeat with CC=clang
make hosted CC=gcc  # M1 janusd/janusctl
make test-hosted CC=gcc # M1 unit/process tests; repeat with CC=clang
```

Before editing a freshly extracted bundle, run `sha256sum -c BOOTSTRAP.SHA256`.
This is a delivery-integrity manifest, not a build dependency or a signature.
It will intentionally become stale when source files change; do not regenerate
it as a substitute for reviewing changes. See
[bundle validation](docs/evidence/bundle-validation.md) for checks performed
on the supplied files, separate from work on the development host.

Read a helper before running it. Only `tools/install-deps.sh --apply` performs
package changes, using sudo/doas if not already root. Bootstrap helpers do not
change guests or host configuration. M1 live testing
is a separate explicit target that creates and tears down one disposable domain;
read the [hosted runbook](docs/development/HOSTED_M1.md) before invoking it.
Raw logs go into ignored `artifacts/`.

B0/M0 are complete and M0 is maintainer accepted. M1 was explicitly authorised
on a separate branch; it is awaiting review. This is not authority to replace
Debian, select a native kernel, begin M2 or deploy a kiosk on the host desktop.
Details: [host](docs/development/HOST.md),
[VM lab](docs/development/VM_LAB.md), [GitHub](docs/development/GITHUB.md),
and [coding](docs/development/CODING.md).

## Source and status

The whitepaper is retained unmodified. `docs/SOURCES.md` identifies the source
materials and external tool documentation used for this bootstrap. Supplied PDF, DOCX and PNG assets are preserved without alteration; no new
artwork was generated. The canonical design source is the architecture Markdown.

JANUS uses mixed licensing: independently authored software is ISC; documentation
and the whitepaper are CC BY 4.0; upstream terms are preserved for imported or
derived material. Name/logo rights are retained, including `docs/JANUS_logo.png`.
See [licensing scope](LICENSING.md), [licence overview](LICENSE), and
[branding policy](TRADEMARKS.md). The repository remains PRIVATE; this decision
does not authorise a public release.

See [candidate contracts](docs/contracts/README.md), [M0 evidence](docs/evidence/m0.md)
and [boundary review](docs/evidence/m0-boundary-review.md). These tests establish
model semantics only, not enforced isolation or durable storage. See
[M0 acceptance](docs/evidence/m0-acceptance.md), [M1 live evidence](docs/evidence/m1.md)
and [M1 boundary review](docs/evidence/m1-boundary-review.md).
See [M0.1 evidence](docs/evidence/m0-1.md) for the maintainer-requested
delegation correction and approved licensing decision.
