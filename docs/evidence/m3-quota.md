<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M3 denied-request quota correction

Date: 1 October 2026. J-009/J-016. Source: e372f85 on
m3-capability-substrate plus the bounded correction. The initial local task
requested implementation of the quota-correction plan; no commit, push, PR
message, visibility change, merge or milestone acceptance is included.

Observed execution environment: Ubuntu 26.04.1 LTS, Linux 7.0.0-34-generic,
GCC 15.2.0, Clang 21.1.8, Python 3.14.4 and QEMU 10.2.1. These are this session's
results, not a rerun on the historically recorded Debian development host.
Existing clang-format 21.1.8 was located at /opt/llvm-mercuron/bin/clang-format;
only that executable was used with JANUS's configuration. No packages, host
policy or project dependencies were changed. Sanitizers and signed-SDK live runs
used approved execution outside the tracing sandbox after sandbox failures.

## Commands and outcomes

Raw logs below are under ignored artifacts/. The log prefix is
`artifacts/m3-quota-20261001-`. Both host compiler modes pass **16,839 checks**.

| Exact command | Exit | Outcome | Log suffix |
| --- | --- | --- | --- |
| `make test-m3-host CC=gcc`, new tests before implementation fix | 2 | FAIL, expected regression at authority state comparison | before-fix.log |
| `make test-m3-host CC=gcc` | 0 | PASS | check-0.log |
| `make test-m3-sanitize CC=gcc`, sandbox | 2 | FAIL, LeakSanitizer cannot run under ptrace | check-1.log |
| `make test-m3-host CC=clang` | 0 | PASS | check-2.log |
| `make test-m3-sanitize CC=clang`, sandbox | 2 | FAIL, same LeakSanitizer startup restriction | check-3.log |
| `make test-m3-sanitize CC=gcc`, approved outside sandbox | 0 | PASS, ASan/UBSan with leak detection enabled | sanitize-gcc.log |
| `make test-m3-sanitize CC=clang`, approved outside sandbox | 0 | PASS, ASan/UBSan with leak detection enabled | sanitize-clang.log |
| `make analyze-m3 CC=gcc` | 0 | PASS | check-4.log |
| `make analyze-m3 CC=clang 'ANALYZE=--analyze -Xanalyzer -analyzer-output=text'` | 2 | FAIL, Clang 21 rejects unused `-c` with `-Werror` | check-5.log |
| Direct Clang analyzer invocation below | 0 | PASS, no warnings disabled | analyze-clang.log |
| `make format-m3` with default PATH | 2 | FAIL, formatter absent from PATH | check-6.log |
| `PATH=/opt/llvm-mercuron/bin:$PATH make format-m3`, initial | 2 | FAIL, test wrapping and unchanged object.c line 19 | format.log |
| `PATH=/opt/llvm-mercuron/bin:$PATH make format-m3`, final | 2 | FAIL, only unchanged object.c line 19 | format-final.log |
| Changed-file formatter invocation below | 0 | PASS | format-changed.log |
| Baseline formatter invocation below | 1 | FAIL, same object.c line at HEAD | format-baseline.log |
| `make check` | 0 | PASS available checks; formatter check NOT RUN | check-7.log |
| `PATH=/opt/llvm-mercuron/bin:$PATH make check` | 0 | PASS, includes formatter configuration check | bootstrap-final.log |
| `git diff --check` | 0 | PASS | check-8.log |
| GCC live invocation below, sandbox | 2 | FAIL before build, GPG agent startup denied | live-gcc.log |
| GCC live invocation below, approved outside sandbox | 0 | PASS, signed inputs, topology, A-L and quota marker | live-gcc-retry.log |
| Clang live invocation below, approved outside sandbox | 0 | PASS, signed inputs, topology, A-L and quota marker | live-clang.log |

Direct Clang command removes only the incompatible compile-only option:

```sh
clang -std=c17 -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow \
	-Wstrict-prototypes -Wmissing-prototypes -g -O1 -Iinclude --analyze \
	-Xanalyzer -analyzer-output=text lib/substrate/model.c \
	-o out/analyze-m3-quota.plist
```

Changed-file and unchanged-baseline checks:

```sh
/opt/llvm-mercuron/bin/clang-format --dry-run --Werror \
	lib/substrate/model.c tests/substrate/test_model.c \
	platform/microkit/world.c platform/microkit/control.c
git show HEAD:platform/microkit/object.c | \
	/opt/llvm-mercuron/bin/clang-format --dry-run --Werror \
	--assume-filename=platform/microkit/object.c
```

Live command ran sequentially with CC=gcc then CC=clang. Logs use the matching
compiler suffix. Each build reverified the pinned signature and all 76 consumed
SDK inputs against the signed archive before executing the builder.

```sh
make test-m3-live CC=gcc \
	MICROKIT_SDK=artifacts/toolchains/microkit-sdk-2.3.1 \
	MICROKIT_ARCHIVE=artifacts/toolchains/microkit-2.3.1-download/microkit-sdk-2.3.1-linux-x86-64.tar.gz \
	MICROKIT_SIGNATURE=artifacts/toolchains/microkit-2.3.1-download/microkit-sdk-2.3.1-linux-x86-64.tar.gz.asc \
	MICROKIT_KEY=artifacts/toolchains/microkit-2.3.1-download/signing-key.asc \
	M3_LOG=artifacts/m3-quota-20261001-serial-gcc.log
```

The exact QEMU argv is retained in each live log: TCG, one CPU, 1 GiB, headless,
no NIC, disk or host sharing, the verified seL4 ELF and newly built loader image.
Both owned children were waited for with exit 0 after the completion marker.
Harness completion is not evidence of an orderly guest shutdown. A process-list
check found no QEMU command using out/m3-release after both runs.

## Regression oracle and bounded review

Confirmed Medium accounting bug in world(): well-shaped owner-only calls returned
DENIED before quota admission, allowing unlimited logical admissions contrary to
the written contract. They did not gain owner authority. The fix moves that denial
after the quota check/increment, with no interface or representation changes.

The corrected authority expectations failed against the original implementation.
Literal contract-derived request rows cover all six owner-only operations on both
worlds. Every one of 64 denials changes only that caller's request count; requests
after exhaustion return LIMIT with zero error data and no state mutation.
Malformed traffic remains INVALID and uncharged, including after exhaustion.
CONTROL can inspect/revoke while exhausted; the other world reads successfully.
Rotation clears accounting, and acquiring a revoked object remains DENIED.

Live K creates four handles and attempts a fifth, then consumes the remaining
59 admissions with denied owner operations. All six owner operations and a GET
then return LIMIT. CONTROL verifies quota=64, handles=4 and unchanged content;
it revokes/regrants and verifies B's continued read. The verifier now requires
the new `M3 PASS denied-owner quota` marker, as well as existing A-L, fault and
caller-context scheduling markers. Live coverage of this exhaustion is World A;
the portable matrix covers both caller identities. Test reports remain test
oracles rather than attestation from hostile programs.

Boundary trace: channel-derived identity is unchanged; shape validation precedes
admission; the counter is bounded before increment; the owner path remains
separate. No denied request executes owner(), modifies grants or acquires a
handle. Error replies remain zeroed. No new ownership, allocation, concurrency,
persistence, host authority or device operation is introduced. A single agent
implemented and reviewed this patch; shared-assumption risk remains.

## Limits and next gate

Whole-M3 formatting is FAIL with Clang 21 on an unchanged baseline line, despite
changed-file formatting PASS. The Make Clang analysis target is FAIL on this
tool version; the equivalent direct analysis is PASS. These are retained
diagnostics, not a claim that all Make targets pass.

The historical GCC target CONTROL SDK diagnostic remains a documented analysis
coverage limitation. Target static analysis was NOT RUN for this correction;
portable analysis and real target builds/live tests are distinct evidence.
M0-M2 suites were NOT RUN: their source is unchanged. Exact proof inheritance
remains NOT ESTABLISHED. No physical hardware, DMA, nested KVM, power-loss,
real PD reconstruction or dynamic delegation result is inferred.

Read-only GitHub recheck: repository visibility PRIVATE; PR #3 OPEN against main
at e372f85, with the existing quota finding and COMMENTED automated review at
91a0775. There is no maintainer approval in that review. Sandbox network access
failed first; approved read-only access succeeded. Historical public-visibility
evidence is preserved; no current visibility change was made.

The initial local correction was ready for review with the stated limitations.
The following publication disposition supersedes that initial local-only scope.

## Publication disposition and final validation

On 1 October 2026 the maintainer accepted this quota correction for publication
to PR #3, explicitly authorising a scoped commit/normal push, PR update and PUBLIC
visibility restoration. M3 itself is **READY FOR SECOND REVIEW**, not accepted.
The source identity is the commit introducing this correction record, based on
e372f85; its exact SHA and the verified remote PR head are supplied at handoff.
No M0-M2 implementation, SDK, SDF, shared semantic code or live-test/verifier
code changed after the accepted runs. Live and sanitizer tests were not rerun
unnecessarily. The original regression FAIL and historical 8,907 results remain.

Final rerun logs use prefix `artifacts/m3-quota-publication-20261001-`:

| Exact command | Exit | Outcome | Log suffix |
| --- | --- | --- | --- |
| `make test-m3-host CC=gcc` | 0 | PASS, 16,839 checks | check-0.log |
| `make test-m3-host CC=clang` | 0 | PASS, 16,839 checks | check-1.log |
| `make analyze-m3 CC=gcc` | 0 | PASS | check-2.log |
| Direct Clang invocation above, output `out/analyze-m3-publication.plist` | 0 | PASS | check-3.log |
| Changed-file formatter invocation above | 0 | PASS | check-4.log |
| `PATH=/opt/llvm-mercuron/bin:$PATH make check` | 0 | PASS | check-5.log |
| `git diff --check` | 0 | PASS | check-6.log |

Makefile unchanged: Clang 21's Make analyzer invocation remains FAIL / portability
debt; direct analysis PASS. Whole-M3 formatter remains FAIL / unchanged Clang 21
baseline difference; changed-file checks PASS. Historical GCC target CONTROL
analysis remains **FAIL / coverage limitation**; normal live path passes, with
no confirmed JANUS runtime defect established. Proof inheritance NOT ESTABLISHED.

Publication review reconfirmed shape validation before admission; wrong versions,
counts, labels, operations, reserved and unused words retain INVALID with complete
state equality. All six owner-only operations cover both callers through 64/65;
the expected table is independent of dispatcher policy. Check-before-increment
bounds counters. CONTROL inspection, revoke/regrant and rotation remain independent
of World A's exhausted quota; World B's authorised read passes.

Before visibility restoration, gh authenticated account was lex0de, PR #3 was
OPEN/unmerged, main <- m3-capability-substrate at exactly
e372f85e2f499e795885c2573158eb3a374e9020. ADR 0005's provenance clarification is
committed in ancestor 2b271799; licensing/source records have no local changes.
Tracked-file/candidate scans found no credential-pattern hits or tracked
artifacts/out paths; generated SDK files, binaries and raw logs remain ignored.
Pattern scans aid review, not proof of absence. Historical audited files are
unchanged apart from the bounded correction/evidence. Staged content is inspected
explicitly before commit; no blanket add, author change, amend or force push.

Visibility observations and actual supported command:

| Observation or command | Exit / HTTP | Result |
| --- | --- | --- |
| `gh repo view lex0de/JANUS --json visibility,isPrivate,url` before | 0 | PRIVATE, isPrivate=true |
| Unauthenticated `curl -sS` lookup of GitHub repository API before | 0 / 404 | Repository unavailable publicly |
| `gh repo edit lex0de/JANUS --visibility public` | 0 | Visibility restored under explicit maintainer authority |
| `gh repo view lex0de/JANUS --json visibility,isPrivate,url` after | 0 | PUBLIC, isPrivate=false |
| Unauthenticated `curl -sS` lookup of GitHub repository API after | 0 / 200 | Public repository accessible |

gh 2.46.0 help confirms --visibility but does not support the newer
--accept-visibility-change-consequences option; the supported command above was
used. Raw command output and unauthenticated JSON remain ignored under the final
log prefix. The visibility reversal's cause is unknown; prior evidence is retained.
No release, tag or announcement. Stop at maintainer second review; no merge, M3
acceptance, permanent substrate, M4 or M5 authority.
