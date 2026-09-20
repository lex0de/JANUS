<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M1 affected-boundary review

20 September 2026; branch m1-hosted-appliance, based on M0 acceptance db4e698.
Scope: all hosted headers, parser/files/state/libvirt/viewer adapters, broker,
client, PID1 probe, live harness and callers. Workflow: janus-boundary-review.
This is an agent review, not independent maintainer approval or formal proof.
Expected behaviour comes from the maintainer's M1 request and J-007/J-009/J-012/
J-013/J-015/J-016, not only the implementation's test tables.

## Findings resolved before handoff

1. **Presentation validation gap (high for acceptance evidence).** A live viewer
   process did not establish a console connection. The first harness checked
   process existence; QEMU query-vnc then showed zero clients. `jh_spawn` had
   given virt-viewer the test compositor's XDG_RUNTIME_DIR, redirecting its
   session libvirt discovery. Fix: preserve the canonical per-user runtime for
   libvirt and pass an absolute isolated WAYLAND_DISPLAY. Regression: live B
   requires one actual VNC client as well as a guest PID1 boot marker. No claim
   that process existence proves connection remains in the protocol contract.
2. **Failed-child ownership (medium, review finding).** A spawn timeout whose
   forced cleanup cannot finish must not lose its owned PID or let a later RUN
   promote failed presentation. Fix: retain the child on failure, withdraw
   foreground and require cleanup before reacquisition. Backend similarly
   retains a stuck worker and refuses more workers. Fake regression covers
   failure with a retained child, repeat RUN while stop fails and later recovery.
   An uninterruptible kernel task is still not promised to die within a deadline.
3. **Client backlog blocking (medium, review finding).** A blocking local connect
   could stall indefinitely behind a full backlog. Fix: nonblocking socket;
   failure is explicit, no automatic retry. Live L fills the bounded client
   slots and confirms expiry without operations. The client is not a general
   resilient RPC library.
4. **Fixture shutdown failure (test defect).** The first domain lacked ACPI;
   guest poweroff did not become inactive. Add ACPI only to the exact disposable
   domain. Preserve the failed run; rerun J verifies guest poweroff, stopped
   record and a later new incarnation. No host firmware or ACPI setting changed.

5. **Unwanted package authority (high for host scope).** The final package audit
   found Debian's viewer dependency installed a setuid USB helper and an
   active-session access policy. No USB assignment was used in the guest. The
   maintainer explicitly chose removal of the four new affected packages;
   simulation and actual removal passed, and helper/policy absence was checked.
   Live tests remain valid historical results; new live reruns require an
   approved dependency setup. No silent policy exception or reinstallation.

These are implementation/test findings, not a claim of an independently
identified operating-system or libvirt vulnerability.

## Entry-point and authority trace

- Owner supplies trusted startup paths; file ownership/modes, regular-file type,
  link count, bounded content, URI, UUID and duplicates are checked before bind.
  Final-component symlinks are refused. Ancestors must be owner-controlled;
  same-UID tampering is outside this owner-local boundary.
- Socket directory 0700, socket 0600, peer UID check on both ends. Parser rejects
  length, NUL, whitespace/trailing/shell syntax before invoking an adapter.
  Eight profiles and eight pending connections; exact command/profile lookup.
  Unknown profile never reaches libvirt. A replayed valid RUN is intentionally
  idempotent, not a cryptographically replay-protected message.
- Only UUID lookup and, after reconciliation, virDomainCreate are exposed by the
  production backend. No define/destroy, name lookup, shell, arbitrary XML, QMP,
  executable or request-supplied path. Test harness administration is separately
  bounded to its recorded UUID/overlay. Live K uses the one fixture's name
  matching a profile with a nonexistent UUID; RUN fails without name fallback.
- libvirt's connection has greater ambient authority than these commands expose.
  Linux user, broker, libraries, libvirtd/QEMU and trusted profile author remain
  in the hosted TCB. Malicious same-user processes can bypass this broker using
  their own libvirt access. This is not native capability isolation.

## Lifetime, concurrency and failures

Caller-owned fixed state is serial; no shared-memory atomics, async signal
handlers or callback references. signalfd handles INT/TERM/CHLD. Worker/viewer
forks close inherited unrelated descriptors; exec error pipe is CLOEXEC. Children
set PDEATHSIG before exec/work and verify the expected parent to close the death
race. `waitpid` establishes live owned-child identity before signalling. No disk
PID is used as identity. Harness signals use pidfds after parent verification.

Leave/recover clear foreground and stop the child before backend reconciliation.
Stop uses TERM, bounded wait, then KILL of that still-owned child. Viewer exit
clears viewer/focus only; actual guest state comes from UUID lookup. Backend
failure reports unknown; it does not imply shutdown, safe lease release or a
new incarnation. Guest execution can outlive the broker, tested with SIGKILL.

STARTING is saved before backend start; STARTED requires a verified active epoch.
Missing/contradictory active record never reconstructs execution identity or
viewer/focus. Paused domains are reported and RUN does not resume them. Atomic
file replacement and error reporting do not turn this record into M2 storage.
PID reuse is not accepted alone: boot ID, daemon start time and active domain ID
are included, with before/after daemon checks. External administration remains
trusted and must not race JANUS; there is no transaction locking other clients.
Counter exhaustion denies start. Save failure marks the record uncertain; failed
viewer creation can leave the already-started guest active and reports that fact.

Files/requests use bounded stack arrays and checked formatting/conversions.
No JANUS heap allocation in the core; the libvirt URI allocation is checked and
freed, domain/connection references released with checked results. Partial file
write, EINTR, fsync/rename failures and startup cleanup are reviewed. Backend
workers are bounded, but file I/O and kernel-uninterruptible tasks are not hard
real-time. SIGKILL prevents exit-time sanitizer leak scans in those children;
ordinary unit/client/broker exits retain leak detection. Guest image creation is
not production code and never runs the PID1 reboot probe as host root.

## Scope exclusions and remaining risks

M0's grant revocation, stale grants after restore, object commit conflicts and
device reset/quarantine are unchanged and remain model-only. M1 does not create
an authority engine, durable grant journal, physical device assignment or DMA
isolation. No reset/release inference follows from viewer/guest state.

No claim of trusted physical recovery, secure kiosk or native JANUS kernel.
Headless Wayland tests produce GTK no-input-seat diagnostics; they do not validate
physical input or a production compositor. Status reports owned viewer process,
not graphics readiness; actual connection is checked separately in live evidence.
Monolithic local libvirtd is the only tested backend layout. No independent
reviewer has accepted M1. Remaining material choices stay with the maintainer.

Disposition: no known unresolved blocker in the bounded implemented M1 boundary
following the recorded fixes and final matrix. Request maintainer review; no M2.
