<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Hosted M2 laboratory

Read [ADR 0004](../decisions/0004-m2-hosted-native.md), the
[native contract](../contracts/native-m2.md) and
[evidence](../evidence/m2.md). Run as the ordinary owner, never root. M2 does not
need a VM, viewer, network or host policy changes. Do not reinstall the removed
virt-viewer/SPICE/USB-helper stack.

Dependencies on the tested Debian 13 host: existing GCC/Clang, Linux userspace
headers, static glibc development archive, libvirt development files for retained
M1 tests, and libsqlite3-dev. Inspect and simulate before installing anything;
see [dependency evidence](../evidence/m2-dependencies.md).

```
make native CC=gcc
make test-native CC=gcc
make test-native-sanitize CC=gcc
make analyze-native CC=gcc
make analyze-native CC=clang ANALYZE=--analyze
make format-native
make check
```

Repeat builds/tests with CC=clang. The native app is statically linked to permit
Landlock access to one exact executable rather than a dynamic-loader/library tree.
The service, client, launcher and unit/store code support ASan/UBSan with leak
checking. The static application is not ASan-instrumented: static glibc/ASan is
not this laboratory's supported combination. Its linked protocol code is covered
by instrumented unit tests; live enforcement exercises the actual static binary.
This is a stated coverage limit, not an assertion of equivalent sanitizer coverage.

## Explicit live fixture

```
make test-native-live CC=gcc FIXTURE=artifacts/new-m2-lab
```

The fixture must be a **new immediate child of artifacts/**. The harness creates
private runtime files, one database, a copied static note (0500), service/launcher
processes and bounded test endpoints. It also starts the M1 broker with an unused
dummy UUID solely to test its control socket cannot be reached from the native
sandbox. It issues no M1 backend/VM/viewer operation. All processes are tracked as
owned children and stopped; private databases/logs remain ignored for inspection.
No guest/domain/image/network/device is created. Never commit fixture credentials.

Sanitized live service/tools:

```
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
make test-native-live CC=clang FIXTURE=artifacts/new-m2-sanitized \
  CFLAGS='-fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all' \
  LDFLAGS='-fsanitize=address,undefined'
```

The harness independently encodes owner/world messages in Python and asserts
actual database contents and Linux errno results. A separate C storage-process
harness kills a real SQLite writer at four checkpoints, reopens the store and
checks complete revisions. Production packets cannot request those crash hooks.
No sudden power-loss test is performed. M1 live viewer reruns remain BLOCKED by
the intentionally absent viewer stack; accepted historical M1 results stand.

## Manual experiment

Prepare a new private directory yourself (0700), copy out/janus-note into it as
`note` (0500), then run `out/janus-objectd /absolute/private/runtime` in a terminal.
The owner client reads its credential from that directory; never copy the key
into commands, environment variables or logs.

```
out/janus-objectctl /absolute/private/runtime world 1
out/janus-objectctl /absolute/private/runtime create hello
out/janus-objectctl /absolute/private/runtime grant 1 OBJECT_ID rw
out/janus-run /absolute/private/runtime 1 - read OBJECT_ID
out/janus-run /absolute/private/runtime 1 - write OBJECT_ID 1 revised
out/janus-objectctl /absolute/private/runtime save 1 OBJECT_ID 2
out/janus-run /absolute/private/runtime 1 ACTIVITY_ID activity
out/janus-objectctl /absolute/private/runtime revoke 1 OBJECT_ID
out/janus-run /absolute/private/runtime 1 ACTIVITY_ID activity
```

Replace the uppercase IDs with returned values; the last read must be denied.
INSPECT is owner-only. STOP closes a world's service endpoint; the launcher's
bounded lifecycle then ends the app. A service restart discards live endpoints,
rotates owner credentials and requires new launch/incarnation/handles. Do not
retry an uncertain PUT blindly: inspect current revision/content first.

## Sandbox and trust boundary

Runtime Landlock ABI query must report at least 6 and rule installation must
succeed. On this x86-64 experiment, a default-deny seccomp filter additionally
blocks socket/connect, process inspection/control, fork, ioctl and filesystem
metadata mutation. Landlock permits READ_FILE/EXECUTE only on the opened static
note inode. No store/home/repository/key path is allowed. openat remains permitted
by seccomp specifically so the test demonstrates Landlock's filesystem denial.

Only fd 3 is the application authority endpoint. fd 0 is /dev/null, fd 1/2 a
bounded write-only report pipe, executable fd 4 is CLOEXEC, and higher descriptors
are closed before application execution. The launcher enforces restrictions before
exec, uses no-new-privileges, limits resources, and handles the parent-death race.
Service and owner tools set dumpable=0. No privileged launcher is involved.

This confines the tested application mechanism, not every same-UID process on the
host. The owner, service, launcher, executable selection, host kernel and SQLite
remain trusted. Metadata/side channels, host availability, whole-disk rollback,
secure deletion, physical recovery and M3 substrate selection are unresolved.
Landlock is not a complete native capability system. A new runtime or syscall
requires a fresh boundary review; do not broaden this allowlist to fix a demo.
