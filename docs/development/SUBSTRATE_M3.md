<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M3 Microkit laboratory

Read [ADR 0006](../decisions/0006-m3-capability-substrate.md), the
[contract](../contracts/substrate-m3.md), [dependencies](../evidence/m3-dependencies.md)
and [evidence](../evidence/m3.md). This lab is non-VT-x, release, x86_64_generic;
TCG emulation needs no KVM/nested access. No host service changes or viewer stack.

Obtain the pinned official SDK archive, detached signature and documented key as
recorded in dependencies. Verify the fingerprint and signature **before extracting**.
Keep them and extracted SDK in ignored artifacts/. No test downloads anything.
`m3-build.py` rechecks pinned archive hash, fingerprint/signature and each consumed
SDK file against the signed archive on every target build. It uses an isolated
temporary GPG home. A locally changed SDK input fails rather than silently building.

```sh
make test-m3-host CC=gcc
make test-m3-host CC=clang
make test-m3-sanitize CC=gcc
make test-m3-sanitize CC=clang
make analyze-m3 CC=gcc
make analyze-m3 CC=clang ANALYZE='--analyze -Xanalyzer -analyzer-output=text'
make format-m3
make test-m3-live CC=gcc \
    MICROKIT_SDK=artifacts/toolchains/microkit-sdk-2.3.1 \
    MICROKIT_ARCHIVE=artifacts/toolchains/microkit-2.3.1-download/microkit-sdk-2.3.1-linux-x86-64.tar.gz \
    MICROKIT_SIGNATURE=artifacts/toolchains/microkit-2.3.1-download/microkit-sdk-2.3.1-linux-x86-64.tar.gz.asc \
    MICROKIT_KEY=artifacts/toolchains/microkit-2.3.1-download/signing-key.asc \
    M3_LOG=artifacts/m3-new-serial.log
```

Use a new log path for every run; existing logs are not overwritten. `make m3`
with the same four SDK arguments builds without booting. Use CC=clang for a
second target compiler. GCC/Clang compile target C with GNU C17 because upstream
libseL4 headers use `asm`; portable state tests use strict C17. No libc/runtime is
added. Target links external libmicrokit; host sanitizers cover portable logic,
not freestanding PDs. Do not infer target sanitizer coverage.

The generated capDL graph is checked for disjoint world/service mappings, exact
badges/endpoint rights, absent world control/device authority and scheduling
budgets. Generated report, ELF/image and graph stay in out/m3-release/.
The live harness runs one owned QEMU child, waits at most 120 seconds/1 MiB output,
requires every A–L serial milestone plus fault/scheduling markers, then terminates
only that child. It uses no disks, networking, host sharing, graphical display,
libvirt domain or existing VM. SIGTERM has a five-second bound before SIGKILL.
Retain the exact QEMU command and exit in evidence; harness termination is not a
guest shutdown. If any marker is missing the live test fails.

Retain M0/M1/M2 host suites. M1 viewer rerun remains BLOCKED by deliberate package
removal; accepted historical M1 evidence remains valid. M2 live need not rerun
when M2 code is untouched. Do not install packages to make those reruns incidental.
No M4/M5, actual PD reconstruction, dynamic delegation, persistence or physical
hardware claim follows from M3 results.
