<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# ADR 0003: narrow Linux-hosted M1 boundary

Status: EXPERIMENTAL implementation choices within explicit maintainer M1 scope;
not approval of stable APIs, wire/profile/host-record formats or native substrate.

Use C17, libvirt C API, owner-local Unix SOCK_SEQPACKET, SO_PEERCRED and fixed
virt-viewer argv. Profiles bind an ID to a local explicit URI and exact UUID.
No client-supplied paths, XML, QMP, executable, environment or generic commands.
Profile file and runtime directory are trusted owner-controlled startup inputs.
No root, permission broadening or libvirt policy changes.

One synchronous owner controls each broker. Fixed limits bound profiles, pending
clients and messages. Backend calls run in short-lived owned workers with bounded
waits so a stalled libvirt RPC cannot indefinitely block software recovery.
Viewer processes have PR_SET_PDEATHSIG with parent-race check and owned-child
wait/signal handling. A private exec-error pipe distinguishes failed exec from
successful handoff; it does not prove a display connection. No PID from disk is
used for signalling. Recovery withdraws presentation before querying the backend.

Experimental host records store incarnation and execution identity, never live
viewer state. Identity combines host boot ID, monolithic libvirtd process start
identity and libvirt runtime domain ID. A daemon/host restart or conflicting
active record requires reconciliation; no inference from a reused PID alone.
Only a verified inactive domain permits establishing a fresh known baseline.
Persist a STARTING intent before start; crash/uncertain completion cannot invent
an active execution. Atomic host record replacement is not durable JANUS object
storage, anti-rollback or an authority journal. No wire structs are persisted.

Alternatives: a daemon-wide lifecycle event journal and enforcing service would
expand scope; treating domain UUID alone as an execution identity misses shutdown
and restart. This prototype supports the local monolithic libvirtd transport;
other daemon layouts fail closed until separately implemented/tested.

Use headless Weston for safe Wayland viewer testing, not the owner's compositor.
No claim of secure kiosk, physical trusted input or device ownership. One direct
L0 disposable guest is sufficient; nested KVM remains optional/NOT RUN.

Dependencies: libvirt headers/library and virt-viewer as requested; Weston is a
build/test display dependency, not JANUS runtime architecture. No vendoring.
See M1 evidence for exact versions/licences/fixture. Rollback: terminate only
owned viewer/broker/lab resources; retain host and original guests; remove only
new packages after reviewing reverse dependencies. M2–M5 remain unauthorised.
