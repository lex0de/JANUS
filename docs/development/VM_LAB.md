# VM laboratory and nested virtualisation

Source: whitepaper sections 8-9 and 14-15; [NESTED] in `docs/SOURCES.md`.
M0 is a hosted reference model and requires no running VM. This document prepares
M1 and later work; it does not authorise that work before its review gate.

## Preferred experiment topology

```text
L0: existing Debian 13 development host, preserved
    local editor/Codex, ordinary builds, existing QEMU/libvirt
    |
    +-- L1: explicitly created disposable JANUS Linux lab
            candidate shell/broker/Wayland session plus QEMU/KVM
            |
            +-- L2: disposable guest world managed by the prototype
```

This lets a kiosk experiment take over the lab display rather than the owner's
real Debian session. Simple tests may run directly as bounded unprivileged host
processes when that is safer. Sibling VMs remain useful; do not require nesting
for every test. Do not provision either layer merely to make M0 look complete.

## Evidence ladder

1. Identify physical/VM/container layer, CPU vendor and VMX/SVM exposure.
2. Inspect `/dev/kvm`, current access, and the applicable nested module parameter.
3. Record QEMU/libvirt versions and capabilities at the explicit connection URI.
4. In the designated L1 guest, verify that the necessary CPU virtualisation
   features and usable KVM device exist. Use a recorded CPU policy; host-passthrough
   is an experiment-specific choice, not a portable machine profile.
5. Actually boot an L2 test guest with KVM requested, capture a guest-observed
   milestone and accelerator evidence, then shut down only the created guests.

A true nested-KVM pass needs the last step. If nesting is unavailable, record
BLOCKED or run explicitly labelled TCG functional tests; do not relabel software
emulation as hardware virtualisation. Enabling nesting may require host maintenance:
never unload a KVM module supporting other VMs. Suspend/save/migration of a lab
with an active L2 needs its own supported configuration and test; do not assume it.

## Inventory and resource ownership

Use an explicit connection URI for every operation. `qemu:///session` and
`qemu:///system` are different inventories and authority scopes. Query read-only
first, then record exact domain UUIDs, image paths, networks, firmware files and
sockets created for this experiment. Names alone are insufficient for destructive
cleanup. Prefer a `janus-` name prefix plus an ownership record, not a wildcard.

Allocate measured CPU/RAM/storage budgets and retain host headroom. Pin downloaded
images by origin, version and verified checksum/signature. Use regular files,
read-only known-good bases, disposable overlays, and per-guest firmware variables.
No credentials, home-directory sharing, host raw disks, PCI assignment, USB
controllers, or host management sockets in the default guest profile.

No NIC is the default. Add a narrowly described network only for a real test.
A libvirt isolated network does not automatically isolate its host from guests.
Never alter the existing default bridge/NAT, firewall, DNS, or other domains.
Bind consoles/debugging to private local sockets or approved loopback listeners.

## Guest, viewer, and device state

Track domain execution state through the backend, not the lifetime of
`virt-viewer --kiosk`. Record at least: guest shutdown, reboot, paused/saved state,
console loss, viewer crash, broker restart, owner leaving foreground, and failed
reconnection. Console loss does not prove guest shutdown or release hardware.

The broker must reconcile current backend identity/state after restart before
reporting success or starting duplicate guests. Reject stale-incarnation events.
Only explicitly selected, authorised profile resources may reach libvirt/QMP;
never forward a UI-supplied shell command or arbitrary XML/QMP payload.

Keep recovery independent of the guest and foreground viewer. Do not reconfigure
the real host's display manager or tty to create a demo. PCI/USB controller and
single-GPU passthrough remain outside bootstrap scope and require separate target
approval, IOMMU-group/reset evidence, and host-owned recovery input/display.

## Teardown

Shut down the exact created L2, verify its exit, then the exact created L1. Escalate
to force-stop only for those explicitly disposable domains and record why. Inspect
remaining ownership before deleting files. Never issue bulk `virsh destroy`,
`undefine --remove-all-storage`, `pkill qemu`, or a recursive cleanup of a general
image pool. Preserve failure logs and the known-good base.
