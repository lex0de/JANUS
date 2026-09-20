<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M1 hosted laboratory

This Linux-hosted C17 experiment separates execution, viewer process and JANUS
presentation ownership. It uses the libvirt C API, not command forwarding.
M0's capability model is not an enforcement layer for this broker. See
[ADR 0003](../decisions/0003-m1-hosted.md), [evidence](../evidence/m1.md) and
[boundary review](../evidence/m1-boundary-review.md).

## Build and test

Install only inspected missing dependencies after apt simulation. The development
build needs libvirt headers and pkg-config; the viewer needs virt-viewer. Weston
is solely the isolated Wayland test display. See the dependency evidence for
versions, licences and rollback. No dependency is vendored. The tested Debian
viewer package was removed at the maintainer's request because a dependency
installed a setuid USB helper and active-session policy. Do not reinstall it
without an approved dependency solution. Live reruns are currently blocked;
unit/build checks remain available. Historical live evidence is preserved.

```
make hosted CC=gcc
make test-hosted CC=gcc
make test-hosted-sanitize CC=gcc
make analyze-hosted CC=gcc ANALYZE=-fanalyzer
make analyze-hosted CC=clang ANALYZE=--analyze
make format-hosted
make check
```

Repeat builds and tests with Clang. `make test` and `make test-sanitize` retain
the M0 tests. Ordinary targets never start a guest. New code and the unit test
are independently authored ISC; this document is CC BY 4.0.

## Trusted startup inputs

`out/janusd profiles runtime-dir display-runtime wayland-name` runs as the ordinary
owner, never root. Both directories must already exist, be owned by that UID and
mode 0700. Use absolute paths with owner-controlled ancestors. The display path
is an isolated Wayland display directory, not the owner's real desktop. The
broker is not installed as a host service. It holds an exclusive runtime lock.

Profiles are regular, non-symlink, owner-owned files with no group/other access
and one hard link. At most eight exact lines are accepted, each of the form:

```
lab qemu:///session c4a20229-6664-49ce-96f1-b8b267d66965
```

This is an example, not permission to operate on an arbitrary existing VM.
IDs are 1–32 ASCII characters: initial lowercase letter, then lowercase letters,
digits, underscore or hyphen. URIs are exactly `qemu:///session` or
`qemu:///system`; the latter requires separately established need and existing
access. UUIDs must be canonical lowercase, nonzero, 36-character identifiers.
Reject duplicate IDs and duplicate URI/UUID pairs. Names confer no authority.
Profiles are loaded once; changes require restarting the broker. Restrictive
permissions protect against other UIDs, not a malicious process of the owner.

## Experimental control protocol

`out/janusctl runtime-dir operation profile` sends one Unix SOCK_SEQPACKET packet
and receives one reply. The socket is `runtime-dir/control`, mode 0600. Both sides
check SO_PEERCRED against their UID. Linux directory permissions are an additional
boundary. No child/client PID is authenticated as a native JANUS capability.

A request is exactly `operation`, one ASCII space, a valid profile ID and LF,
maximum 64 bytes. Allowed operations: select, status, run, leave, recover. No NUL,
CR, whitespace variants, extra token, second command, path, URI or expansion is
accepted. One request per connection; later packets are discarded on close.
At most eight pending clients, each with a one-second idle deadline. The client
uses nonblocking connect and send and waits at most 15 seconds for a reply.
A full socket backlog may cause a client to fail; no automatic retry starts work.

Replies are bounded to 512 bytes. Success begins `ok`; errors have explicit
names such as `unknown-profile`, `backend-error`, `start-uncertain`, `paused`,
`record-error`, `reconciliation-required`, `viewer-error` or `presentation-busy`.
State replies include profile, UUID, execution, incarnation, viewer and foreground.
`viewer` is a currently owned child PID for observation, never a client-supplied
signal target or evidence of a completed graphics handshake. `foreground=1`
means JANUS has assigned its presentation slot; it is not an OS keyboard grab or
proof of pixels displayed. Errors can report actual guest execution as active.

- select/status: inspect the exact UUID, reconcile backend and host record; never
  start execution. They can save observed stopped state or withdraw stale viewers.
- run: start only that UUID if verified inactive; otherwise reuse its known active
  execution. Never define a domain or fall back to a name. Paused execution stays
  paused. Only one presentation slot exists across profiles; no duplicate viewer.
- leave/recover: withdraw presentation, TERM the verified child, wait 300 ms, then
  KILL that child if needed and wait up to one second. Guest execution is untouched.
  Recovery acts before the libvirt query and needs no guest/display cooperation.

The serial loop uses signalfd, not asynchronous signal-handler logic. Backend
queries run in owned fork workers with four-second waits, then bounded cleanup.
A worker stuck in the kernel is retained and blocks further workers; it cannot
cause unbounded forks. A failed viewer cleanup retains ownership for recovery
and cannot silently regain foreground ownership. The loop polls viewer exit at
100 ms between commands. Requests queued behind backend work can wait several
bounded operations; this is not hard real-time recovery or a hostile-owner DoS
boundary. Storage syscalls and uninterruptible kernel tasks have no real-time
termination guarantee. No arbitrary PID from a file is ever signalled.

## Execution identity and failures

A host record binds profile ID, URI, UUID, phase, incarnation and backend epoch.
The epoch combines Linux boot ID, monolithic libvirtd PID **and process start
identity**, and libvirt's active domain ID. The daemon endpoint's peer UID and
executable are checked. This backend currently supports local monolithic
`/usr/sbin/libvirtd`; modular or remote transports fail closed. Backend worker
identity observations are checked before and after operations.

Before starting an inactive domain, persist a STARTING intent with the incremented
incarnation. Save STARTED only after successful active-state reconciliation. An
active domain with missing, malformed, STARTING or contradictory state requires
reconciliation; never adopt it by guessing. A known inactive observation records
STOPPED. A later JANUS start increments incarnation. A failed start can consume
an incarnation number. Guest-internal reboot, viewer crash, leave and re-entry
leave the active execution incarnation unchanged.

Records use bounded text, exclusive temporary files, fsync, rename and directory
fsync. They are **M1 experimental host records**, not a stable format, durable
JANUS store, anti-rollback or authoritative replay journal. Owner tampering,
rollback and external concurrent administration are outside this trust boundary.
Missing records while stopped can establish a fresh laboratory baseline; old
incarnation numbers are not globally durable identities. Do not erase records
for an active domain. No automatic active-state adoption or ambiguous restart.

The viewer receives fixed argv for `/usr/bin/virt-viewer`: --verbose, --connect,
trusted URI, --uuid, --attach, --kiosk, --kiosk-quit=on-disconnect, trusted UUID.
No --reconnect. Its environment is a fixed PATH, GDK_BACKEND=wayland, the normal
`/run/user/UID` runtime and an absolute WAYLAND_DISPLAY pointing at the isolated
display socket. This keeps libvirt's session connection separate from Wayland.
Exec uses an error pipe, closes unrelated descriptors and handles fork/exec errors.
Viewer and backend workers use PR_SET_PDEATHSIG(SIGKILL) plus a parent-race check.
Broker death leaves the VM running and kills its viewer; restart reconstructs
neither viewer ownership nor foreground from a PID file. There is no automatic
viewer reconnect at restart.

## Explicit disposable live test

```
make test-hosted-live CC=gcc FIXTURE=artifacts/a-new-m1-lab
```

The harness requires a NEW immediate child of ignored artifacts/. It creates
exactly one new UUID and checks name absence. It compiles the repository PID1
boot probe, uses the installed Debian kernel, an immutable blank qcow2 base and
a disposable overlay. It records provenance/hashes, boots with KVM, observes a
serial boot marker and confirms a VNC client through a bounded external test
query. There is no network, host sharing, credential injection or passthrough.
The guest's serial reboot/poweroff test interface is not used by owner recovery.

Headless Weston uses its own runtime directory, no real desktop connection.
A–L tests include viewer SIGKILL, viewer SIGSTOP/recovery escalation, broker
SIGKILL/restart, guest reboot/shutdown, UUID/name collision and malformed clients.
Test signalling verifies parentage and uses pidfds. Teardown verifies exact UUID
and overlay before destroying/undefining only that fixture. Images and raw logs
remain ignored for inspection. An interrupted harness requires inspecting those
exact resources before cleanup, never blanket destroy/kill.

`RETRY=1` reuses that same recorded UUID only after verifying it is undefined and
its name absent. It verifies preserved hashes and archives old records before
rebuilding the probe. It is not a general VM import/recovery command. Do not run
two harness instances for the same fixture. Sanitizer live runs pass CFLAGS and
LDFLAGS as recorded in evidence. No nested L2 boot is needed or inferred.

No secure kiosk, physical secure attention, native capability enforcement,
durable authority, anti-rollback, device/DMA isolation or final format is provided.
Do not begin M2 or merge the M1 branch without maintainer authority.
