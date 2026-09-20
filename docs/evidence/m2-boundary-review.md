<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M2 authority and persistence boundary review

Date: 21 September 2026. Scope: new native header/library, object service/client,
launcher/note, tests, Make targets and ADR 0004 on m2-native-experience, based on
main 5cd4620. Review uses the janus-boundary-review workflow, separately from the
[execution evidence](m2.md). This is an implementation review, not maintainer
acceptance, formal verification or an independently discovered OS vulnerability.
One agent wrote code, tests and this review: shared assumptions remain a material
limitation despite separately specified wire bytes, SQL checks and live errno oracles.

## Findings corrected before handoff

| Finding / reachable case | Impact | Correction / regression |
| --- | --- | --- |
| WRITE-only stale PUT initially reused the object-read reply buffer | Conflict could disclose content without READ | update uses private current buffer and returns revision only; WRITE-only GET denied and conflict length zero asserted |
| Generation counter at its maximum initially prevented revoke | Exhaustion could leave an existing grant live | REVOKE always withdraws live state; increments only when possible; max-generation regrant LIMIT followed by successful revoke and denied ACQUIRE tested |
| Launcher reused successful owner-call return code on later setup errors | Setup failure could appear successful | Reset failure result before preparing child; exec/sandbox failures remain nonzero |
| Raw inherited standard output might have been a readable owner terminal | App could gain ambient input/display access through a descriptor | /dev/null stdin and bounded write-only report pipe; direct stdout read EBADF, fd 4–15 EBADF in live probe |
| Unrestricted prlimit64 would have reached another same-UID process | Availability/control beyond the native world | Seccomp allows only pid=0; live service-prlimit EPERM |
| Empty packet with attached descriptor initially returned before ancillary cleanup | Malformed-client resource leak | Parse/close ancillary descriptors even for zero bytes; unexpected FD requests rejected, truncation closes received FDs |
| Closed-session polling could conflict with priority owner launch/fd reuse | False BUSY or processing stale readiness | Reap disconnected sessions first and clear their event bits; validate current slot descriptor before world dispatch |
| GCC analyzer could not establish dup2/dup3 ownership; Clang reported dead stores | Validation failed / cleanup clarity poor | Retain returned descriptor slots, close explicitly on failed child setup, remove unused assignments; analyzers rerun without suppressions |

These were development findings in uncommitted M2 code, not claims about an
accepted M0/M1 boundary being compromised. Initial EXECUTE-only Landlock setup
also prevented exec; adding READ_FILE for the **same exact static executable**
allowed the app to load without permitting library/home/store trees.

## Entry-point trace

**Owner:** private 0700 runtime, regular no-follow 0600 credential, same-UID peer
plus 256-bit secret before decoding. Secret comparison processes every byte;
secret not present in argv, environment, logs or child endpoint. Owner service
and tools set dumpable=0. Outside-sandbox host owner remains trusted. Runtime
ancestors and executable selection belong to that trusted owner; this is not a
hostile-owner filesystem race defense.

**Native IPC:** only an inherited socketpair end; no global world listener. The
retained server session binds world/incarnation. Requests cannot select another
world or install a handle. Packet version/length/rights/reserved fields and exact
operation shapes are checked before changes. Unknown owner operations on this
endpoint return INVALID. The production service never sends a store/credential FD.
SCM_RIGHTS is restricted to authenticated launch and unexpected world FDs close.
Malformed structural input invalidates its session but makes no object mutation.

**Object access:** known ID only indexes a current recipient grant. Handles are
random and connection-local; regrant generation and incarnation are checked per
request. Tests explicitly submit the old numeric bytes in a new world/session,
not merely rely on fd closure. No string, activity ID or revision conveys access.
Delegation is explicit default-deny and unsupported live; M0 attenuation semantics
are neither replaced nor claimed as live implementation.

**Persistence:** fixed prepared statements, checked return values, bounded content,
foreign keys, current-to-revision validation and integrity checks at open. SQLite
limit-setting calls return prior limits, not error codes; all fallible operations
are checked. Activity reconstruction reads current authority and cannot update
its grant table. Whole-store rollback remains possible for the trusted owner.
An existing malformed SQLite file fails closed; these checks are not protection
against a hostile host owner rewriting a valid schema or disk.

**Commit/revocation:** serial requests leave no client-held open transaction.
Expected revision check and new revision/current publication occur in one SQLite
transaction. Errors roll back, uncertain COMMIT/transport does not auto-retry.
WRITE-only conflict returns no content. Killing the storage process at staged and
precommit boundaries leaves old complete content; postcommit loss leaves complete
new content with no acknowledgement. Only process-crash evidence is claimed.
Quiescent revoke precedes all subsequent admissions and cannot erase copied data.

**Sandbox:** actual ABI 6 queried and filesystem denial observed. Landlock plus
no-new-privileges is installed before exec. Seccomp default-deny complements its
filesystem scope: no socket/connect, ptrace/process_vm, fork, ioctl or metadata
mutation calls. prlimit64 can address self only. The M1 UID-only owner socket is
reachable by the trusted test parent but not the sandboxed world. This mechanism
avoids broadening the accepted M1 protocol; removing seccomp would invalidate that
claim and require review. Landlock alone does not mediate every Unix socket path.

**Lifetime and resources:** static app gets only object fd 3 plus safe stdio;
executable fd CLOEXEC and close_range remove other inherited authority. Parent-
death setup checks the fork race. Resource/time/output limits bound the app.
Service has four worlds, eight handles/grants, bounded object/revision/activity
counts, one request transaction and separate priority owner capacity. A full
world queue cannot allocate new service work or consume the owner slot. No
hard real-time guarantee exists for kernel/filesystem stalls or trusted host-owner
DoS. SIGKILL targets only an unreaped child, never a saved PID; delayed kernel
termination is reported after the bounded reap attempt.

## Remaining limits and review gate

No further confirmed blocking finding in this reviewed scope after corrections.
This is not assurance against all Linux exploits, side channels or metadata leaks.
No native kernel/capability substrate, secure kiosk, physical recovery, device/DMA
isolation, anti-rollback, secure deletion, stable formats or power-loss test exists.
Static note itself lacks ASan instrumentation; service/store/protocol/launcher
instrumented coverage and live kernel checks do not erase that limitation.
SQLite/host kernel/libc and trusted owner/service/launcher remain in the TCB.
No independent reviewer or maintainer acceptance is asserted. M2 awaits review;
M3–M5 must not begin.
