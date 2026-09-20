# Debian 13 development-host procedure

## Authority and baseline

The user permits necessary sudo/doas setup on the development host. This does not
make the host a disposable OS test image. Inspect hostname, OS, execution layer,
working directory, memory, free space and existing workloads before modifications.
The helpers in this bundle never infer a physical disk or enable hardware passthrough.

`tools/host-preflight.sh` only reads local status and prints a report. It may query
existing local libvirt sockets (and thus activate an installed socket-activated
daemon); use `--no-libvirt` to omit those queries. It does not start/stop any domain,
network or pool. Keep raw output private: domain names and paths can be personal.
It performs no GitHub authentication request and prints no environment or tokens.

```sh
umask 077
mkdir -p artifacts
# Check artifacts is a real, owner-controlled directory, not a symlink, first.
./tools/host-preflight.sh > artifacts/host-preflight.txt 2>&1
rc=$?
printf 'preflight exit: %s\n' "$rc"
```

Inspect the report. Command failures inside the inventory are labelled, not
converted into a claim that the whole host is ready. Preflight completion only
means inventory ran. It does not demonstrate nested KVM, IOMMU correctness or
physical-device compatibility.

## Dependencies

The M0 package list is `tools/debian13-packages.txt`. It contains hosted build and
review tools, not an operating-system runtime dependency list. Existing QEMU,
libvirt and gh installations are not reinstalled by this helper. The explicit
`libclang-rt-dev` dependency supplies runtime support for Clang sanitizer probes;
see [DEBIAN-CLANG] in `docs/SOURCES.md`. Inspect the actual installed compiler and
runtime pairing, especially when a non-Debian Clang is first in PATH.

```sh
make deps-plan
# Review the apt simulation and any configuration/upgrade side effects.
./tools/install-deps.sh --apply
make toolchain
```

The installer checks Debian VERSION_ID 13/13.x, filters already installed
packages, simulates the exact missing package set, and uses `--no-remove` and
`--no-install-recommends`. It does not run apt update/upgrade automatically or
rewrite apt sources. Missing package candidates are blockers; inspect the actual
repository state. If apt metadata needs refreshing, a separate targeted
`sudo apt-get update` or `doas apt-get update` is allowed after the need is stated.

A package install may bring dependencies or run maintainer scripts; simulation
is an inspection aid, not a promise of no side effects. Capture package versions
and review the real apt transaction. The helper requires an interactive apt
confirmation and never uses `-y`. Do not make global permissions broader just to
avoid an authentication prompt. Compile as the ordinary user afterward.

## Controlled additional changes

For a later required service, socket or group change, record purpose, authority,
exact file/command, previous state, test, and rollback. Changing membership in
libvirt/kvm groups grants ongoing device/control access; it is not a harmless
formatting step. Prefer the smallest existing authorised interface.

Do not restart shared libvirt/network/graphics services, unload KVM modules, enable
nested virtualisation through a host reboot, or change kernel command lines during
M0. Report the need and schedule an explicit owner-approved maintenance action.
Never edit sudoers/doas/polkit into a general passwordless agent policy.

## Resource discipline

Size jobs and VM allocations from actual available RAM/CPU/storage and existing
work. Leave headroom for the host and recovery. Preserve the host desktop, network,
SSH/gh authentication and known-good boot path. Use dedicated regular image files
and bounded timeouts. No physical machine in an old inventory is automatically
authorised for destructive testing.
