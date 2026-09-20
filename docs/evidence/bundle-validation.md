# Bootstrap bundle validation

Date: 20 September 2026. These results describe the supplied bootstrap files,
not the user's workstation or an implemented JANUS system.

## Environment

The assistant's Linux container reported Debian GNU/Linux 13 (trixie), with
DEBIAN_VERSION_FULL 13.3. It is not the user's Debian development host.

- Python 3.13.5; GNU Make 4.4.1.
- GCC/G++ 14.2.0, Debian package identity 14.2.0-19.
- Clang/Clang++ 17.0.0 from the container's Swift LLVM installation.
- Shell syntax checked with the container's /bin/sh.

The compiler versions above are observations, not pinned JANUS requirements.
In particular, these checks do not validate a Debian-packaged Clang 19 runtime.

## Commands and observed outcomes

Commands ran from the supplied JANUS root, before packaging:

```sh
make check
./tools/probe-toolchain.sh
```

Both commands returned exit status 0. No user-host operation was performed.

| Check | Outcome | Coverage |
| --- | --- | --- |
| Required files, UTF-8/newlines, root instruction size | PASS | Bootstrap consistency, not architecture correctness |
| Project TOML parse and intended local safety settings | PASS | Python TOML validation, not live Codex acceptance |
| Three skill frontmatters and README relative links | PASS | Structural checks, not a live skill invocation |
| Retained whitepaper SHA-256 | PASS | Exact source-copy comparison |
| POSIX-shell syntax, three helpers | PASS | `sh -n`, not full semantic analysis |
| Helper argument contracts | PASS | Three test methods, nine subcases: help, unknown option, extra operand |
| GCC C17, plain | PASS | Compile and execute the small allocation fixture |
| GCC C17, ASan/UBSan | PASS | Compile and execute with non-recovering diagnostics |
| Clang C17, plain | PASS | Compile and execute the small allocation fixture |
| Clang C17, ASan/UBSan | PASS | Compile and execute with non-recovering diagnostics |
| G++ C++17, plain | PASS | Compile and execute the small RAII fixture |
| G++ C++17, ASan/UBSan | PASS | Compile and execute with non-recovering diagnostics |
| Clang++ C++17, plain | PASS | Compile and execute the small RAII fixture |
| Clang++ C++17, ASan/UBSan | PASS | Compile and execute with non-recovering diagnostics |
| ShellCheck | NOT RUN | Tool unavailable in the container |
| clang-format configuration acceptance | NOT RUN | Tool unavailable in the container |
| Live Codex project config/skill loading | NOT RUN | No live Codex session used for these checks |
| Host inventory and apt simulate/apply on the user's host | NOT RUN | Must be performed locally by Codex |
| Installer apply path and full apt dependency resolution | NOT RUN | No real package transaction was performed |
| GitHub identity, repo creation and push | NOT RUN | Requires the user's authenticated local gh |
| Nested KVM / libvirt guest boot / IOMMU / device reset | NOT RUN | No virtualisation or hardware test performed |
| JANUS M0 contract implementation and tests | NOT RUN | Initial Codex work, not included as an implemented model |
| Native kernel / OS isolation / formal verification | NOT RUN | Outside bootstrap-bundle scope |

The helper argument tests do not exhaustively prove absence of side effects;
they test their documented argument paths and verify the helper source is
unchanged. Compiler fixtures only test toolchain execution and sanitizer startup.
They do not test world, grant, object, device or foreground contracts.

## Failed supporting operation

An attempt to obtain optional ShellCheck and clang-format validation tools into
a separate assistant-container directory failed because package-index DNS
resolution was unavailable. The installer command returned exit status 1. Those
two checks were not replaced by claimed manual or compiler passes. Re-run
`make check` after the required tools are installed on the development host.

## Reproduction and interpretation

Run `sha256sum -c BOOTSTRAP.SHA256` before editing the delivered files, then
`make check` and `make toolchain`. Exact tool versions and observed failures on
the development host take precedence over this container result. Do not mark
any JANUS milestone approved or implemented from this report.

The transport manifest becomes stale after legitimate edits. It is not a
cryptographic signature, a source licence, or an ongoing release attestation.

## Archive round-trip

PASS: archive member paths were checked for traversal, absolute paths and
unexpected entry types; extraction into a fresh temporary directory succeeded;
all per-file SHA-256 checks passed; `make check` passed again in that extracted
copy, with the same explicit ShellCheck/clang-format NOT RUN limitations.
