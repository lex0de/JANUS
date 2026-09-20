<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Contributing to JANUS

Work in small, reviewable changes tied to the current milestone and requirement
IDs. Read the [coding conventions](docs/development/CODING.md),
[requirements](docs/REQUIREMENTS.md), [decisions](docs/decisions/README.md) and
applicable repository instructions before changing a boundary. Architectural
approval and passing tests are distinct; do not advance a milestone yourself.

The current development host is Debian 13. Inspect installed dependencies before
installation, simulate package changes, and keep builds/tests unprivileged.
Do not change host policies or existing guests to make a test pass. Follow the
[host runbook](docs/development/HOST.md) and
[VM laboratory rules](docs/development/VM_LAB.md). Removed M1 viewer/USB-helper
packages must not be reinstalled incidentally.

```
make preflight     # non-elevated host inventory
make deps-plan     # inspect the bootstrap dependency simulation
make toolchain     # compiler probes, not implementation evidence
make check
```

Run GCC and Clang, warnings-as-errors, relevant static analysis, sanitizers and
format validation for changed C code. Ordinary unit tests and explicit live tests
must remain separate. Preserve exact commands, failures and exit statuses in
ignored artifacts/; publish only sanitised evidence in docs/evidence/. Record
shared-assumption risks and limitations in a separate boundary review.

Source, tests and documentation belong in commits; credentials, raw host logs,
images and generated binaries do not. Review staged paths/content and whitespace
before each commit/push. See the [GitHub workflow](docs/development/GITHUB.md).
Current maintainer authority overrides historical bootstrap scope, but does not
permit unrelated publication, host changes or automatic milestone acceptance.

New independently authored software normally uses ISC; new documentation uses
CC BY 4.0. Preserve upstream provenance and licence terms, including after
rewriting. Exceptions require an explicit decision. See [LICENSING.md](LICENSING.md).

For an original extracted bootstrap bundle, `sha256sum -c BOOTSTRAP.SHA256`
checks delivery integrity, not current source validity. That historical manifest
becomes stale as development proceeds; do not regenerate it to conceal changes.
The optional agent setup is documented in docs/development/CODEX.md. It is a
development aid, not a JANUS runtime requirement.
