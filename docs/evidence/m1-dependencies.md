<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M1 dependency and fixture provenance

Observed 20 September 2026. No upstream implementation source was copied or
vendored. JANUS source is independently authored ISC; this document is CC BY 4.0.
External binaries, headers and guest artefacts retain their existing terms.
The inherited instruction-wording ambiguity in LICENSING.md remains unresolved.

The table identifies Debian binary versions installed during testing (four removed at handoff), source packages
and licence labels from their installed `/usr/share/doc/PACKAGE/copyright`.
Those notices, including per-file exceptions and attributions, govern; the
labels are not a replacement licence grant. Package build suffixes are retained.
No locally modified or redistributed upstream package is part of this change.

| Binary package | Version | Source package | Declared licence labels |
| --- | --- | --- | --- |
| clang | 1:19.0-63 | llvm-defaults | GPL-2+ |
| gcc | 4:14.2.0-1 | gcc-defaults | See package notice |
| libaml0t64 | 0.3.0-3 | aml | BSD-3-Clause, CC0-1.0, ISC |
| libc6-dev | 2.41-12+deb13u4 | glibc | LGPL-2.1+ and numerous per-file terms; see installed copyright |
| libfreerdp-server3-3 | 3.15.0+dfsg-2.1+deb13u3 | freerdp3 | Apache-2.0, Apache-2.0 or BSD-2-clause or BSD-3-clause or BSL-1.0 or Expat or Zlib or public-domain or OFL-1.1, BSD-2-clause, BSD-3-clause, BSL-1.0, Expat, HPND-sell-variant~and/or~NTP~disclaimer, LGPL-2.1+, OFL-1.1, Zlib, public-domain | | Apache-2.0 and per-file third-party terms; see installed copyright |
| libfreerdp3-3 | 3.15.0+dfsg-2.1+deb13u3 | freerdp3 | Apache-2.0, Apache-2.0 or BSD-2-clause or BSD-3-clause or BSL-1.0 or Expat or Zlib or public-domain or OFL-1.1, BSD-2-clause, BSD-3-clause, BSL-1.0, Expat, HPND-sell-variant~and/or~NTP~disclaimer, LGPL-2.1+, OFL-1.1, Zlib, public-domain | | Apache-2.0 and per-file third-party terms; see installed copyright |
| libgtk-vnc-2.0-0 | 1.5.0-1 | gtk-vnc | GPL-3+, LGPL-2 or LGPL-2+, LGPL-2+, LGPL-2.1+, MIT, MIT and LGPL-2 |
| libgvnc-1.0-0 | 1.5.0-1 | gtk-vnc | GPL-3+, LGPL-2 or LGPL-2+, LGPL-2+, LGPL-2.1+, MIT, MIT and LGPL-2 |
| libneatvnc0 | 0.9.1+dfsg-1 | neatvnc | BSD-3-Clause, BSD-3-Clause or GPL-2.0+, CC0-1.0, Expat, GPL-2.0+, ISC |
| libphodav-3.0-0 | 3.0-9+b2 | phodav | LGPL-2.0+ |
| libphodav-3.0-common | 3.0-9 | phodav | LGPL-2.0+ |
| libseat1 | 0.9.1-1 | seatd | Expat, LGPL-2.1+ |
| libspice-client-glib-2.0-8 | 0.42-3 | spice-gtk | BSD-3-clause, GPL-2.0+, LGPL-2.0+, LGPL-2.1+, LGPL-2.1+ and MIT, MIT, other |
| libspice-client-gtk-3.0-5 | 0.42-3 | spice-gtk | BSD-3-clause, GPL-2.0+, LGPL-2.0+, LGPL-2.1+, LGPL-2.1+ and MIT, MIT, other |
| libturbojpeg0 | 1:2.1.5-4 | libjpeg-turbo | BSD-3-clause, BSD-BY-LC-NE, Expat, NTP, Zlib |
| liburiparser1 | 0.9.8+dfsg-2 | uriparser | BSD-3-clause, Expat, GPL-3+, LGPL-2.1+ |
| libusbredirhost1t64 | 0.15.0-1 | usbredir | GPL-2+, LGPL-2.1+ |
| libvirt-dev | 11.3.0-3+deb13u3 | libvirt | AS-IS-I, BSD-3-Clause, GPL-2.0+, GPL-2.0+ or BSD-3-clause, GPL-3.0+, LGPL-2.1+, SIL-1.1 |
| libvirt-glib-1.0-0 | 5.0.0-2+b4 | libvirt-glib | Expat, GPL-2, GPL-3+, LGPL-2.1+ |
| libweston-14-0 | 14.0.2-1 | weston | CC-BY-SA-3.0, MIT, X11 |
| libwinpr3-3 | 3.15.0+dfsg-2.1+deb13u3 | freerdp3 | Apache-2.0, Apache-2.0 or BSD-2-clause or BSD-3-clause or BSL-1.0 or Expat or Zlib or public-domain or OFL-1.1, BSD-2-clause, BSD-3-clause, BSL-1.0, Expat, HPND-sell-variant~and/or~NTP~disclaimer, LGPL-2.1+, OFL-1.1, Zlib, public-domain | | Apache-2.0 and per-file third-party terms; see installed copyright |
| make | 4.4.1-2 | make-dfsg | GPL-3+ |
| python3 | 3.13.5-1 | python3-defaults | See package notice |
| qemu-system-x86 | 1:10.0.13+ds-0+deb13u1 | qemu | See package notice |
| qemu-utils | 1:10.0.13+ds-0+deb13u1 | qemu | See package notice |
| spice-client-glib-usb-acl-helper | 0.42-3 | spice-gtk | BSD-3-clause, GPL-2.0+, LGPL-2.0+, LGPL-2.1+, LGPL-2.1+ and MIT, MIT, other |
| virt-viewer | 11.0-3+b1 | virt-viewer | GPL-2 (package notice) |
| weston | 14.0.2-1 | weston | CC-BY-SA-3.0, MIT, X11 |

Primary provenance: installed Debian package records and copyright notices;
libvirt's notice identifies https://libvirt.org/git/?p=libvirt.git, Weston
https://wayland.freedesktop.org/, and virt-viewer's package notice records its
original virt-manager download origin. Main libvirt code is LGPL-2.1+;
virt-viewer's installed notice specifies GPL version 2; Weston is predominantly
X11 with additional file-specific terms as recorded above. JANUS dynamically
links libvirt and invokes virt-viewer/Weston as separate processes. No JANUS
software licence is asserted over them. Upstream modifications: none. Required
notices stay with installed packages; no permission to distribute them was added.

## Host change and rollback

Exact simulation: `apt-get --simulate --no-remove --no-install-recommends install
libvirt-dev virt-viewer weston` (exit 0). Actual approved command:
`sudo apt-get --no-remove --no-install-recommends install libvirt-dev virt-viewer
weston` (exit 0). 21 newly installed packages, zero upgraded, zero removed.
The three requested packages and their dependencies occupy about 18.3 MB.

New packages (only this list is a rollback candidate): libvirt-dev, virt-viewer,
weston, libaml0t64, libfreerdp-server3-3, libfreerdp3-3, libgtk-vnc-2.0-0,
libgvnc-1.0-0, libneatvnc0, libphodav-3.0-0, libphodav-3.0-common, libseat1,
libspice-client-glib-2.0-8, libspice-client-gtk-3.0-5, libturbojpeg0, liburiparser1,
libusbredirhost1t64, libvirt-glib-1.0-0, libweston-14-0, libwinpr3-3,
spice-client-glib-usb-acl-helper. The USB ACL helper was a package dependency. The final audit found its mode
4755 root helper and org.spice-space.lowlevelusbaccess policy (`allow_active=yes`).
Although no USB device was exposed to the fixture, this package default broadened
host authority outside M1's need. It was not treated as implicitly approved.

Remaining-package rollback is NOT RUN: first simulate `apt-get --simulate remove` with that exact
list, inspect reverse dependencies and any later consumers, then separately
approve removal if wanted. Do not autoremove, purge unrelated packages, reinstall
the hypervisor or revert host policy. No manual changes to groups, services, network, display manager, firmware or
KVM modules. Normal package scripts installed the policy/helper noted above;
their removal, not a custom overriding policy, restored this boundary. The lab used the ordinary user's session libvirt.

The maintainer explicitly requested removal of the four affected packages after
testing. Simulation `apt-get --simulate remove spice-client-glib-usb-acl-helper`
exited 0 and listed only four newly installed packages. Executed:

```
sudo apt-get --yes remove virt-viewer libspice-client-gtk-3.0-5 libspice-client-glib-2.0-8 spice-client-glib-usb-acl-helper
```

Exit 0, PASS. Verified `/usr/bin/virt-viewer`, the setuid helper and its policy
file are absent. No autoremove/purge or existing-package removal. Seventeen new
packages remain, including libvirt-dev and Weston. A full remaining rollback
must omit those four already removed and review reverse dependencies first.
Logs: artifacts/m1-usb-helper-rollback-plan.log and m1-usb-helper-rollback.log.

All live evidence predates this deliberate runtime cleanup. Code and logs are
preserved. Live reruns are now **BLOCKED pending an approved viewer dependency
setup**; do not silently reinstall Debian's package and its policy. Unit tests,
compilation and review remain available. This does not retroactively turn a
completed live pass into a missing test, nor authorise a package-policy exception.

An early viewer environment error caused libvirt to auto-start an extra session
daemon under the isolated display runtime. It had no JANUS-started domain, was
not killed by name or PID, and exited through libvirt's normal idle timeout.
The final viewer uses the normal user runtime for libvirt and an absolute
isolated Wayland socket. This diagnostic side effect is not hidden as a clean
initial run. Existing guests were not operated on.

## Disposable guest provenance

Exactly one UUID: `c4a20229-6664-49ce-96f1-b8b267d66965`, qemu:///session.
Name initially janus-m1-disposable; renamed solely for the collision test.
One vCPU, 256 MiB, KVM, ACPI, local Unix VNC and serial sockets, no NIC, sharing,
physical disk, PCI/USB assignment, credentials or management socket in the guest.
One 64 MiB virtual qcow2 blank base (mode 0400) and disposable overlay.

The copied kernel is the installed Debian
`linux-image-6.12.107+deb13-amd64` version `6.12.107-1` (Linux GPL-2 and applicable
per-file terms; installed package copyright retained). No external image download.
The initramfs is generated by `tests/hosted/live.py` from independently authored
`tests/hosted/guest_init.c`, compiled by GCC with the installed static glibc.
The libc terms remain applicable to the generated binary; it is ignored and not
distributed. This is a small Linux boot/reboot/poweroff probe, not a full appliance
OS image or a new JANUS kernel. A serial marker from PID1 establishes actual boot.

Final hashes (SHA-256):
- kernel: `2b2358b37674d2505350528875bb17afae2a36522a9e8a9417eaca65a7da0e08`
- initramfs: `ce953046a7308fdc8b626836c5b05c1316dd02c0c1497e64c673c088db038c54`
- base.qcow2: `1b7af784841125887e688254229e282829e17245d0e47fe7280da9b68a30d9e1`
- PID1 source: `54d13d01d6da48adfa5ddcefe169e183374f7ea70a5ad1b0af27aba461a2bab8`

The same UUID/base were reused after exact teardown; retries archive old runtime
records and verify hashes. The fixture was stopped and undefined after each run.
Generated images/XML/raw logs remain in ignored `artifacts/m1-live/`; none are
committed. Host-specific paths and boot/process identities are omitted here.
