<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M2 dependency and host-change record

Observed 21 September 2026 on the ordinary Debian 13.7 x86-64 development host,
Linux 6.12.107+deb13-amd64. No upstream implementation source was imported or
vendored. New JANUS C/header/Python test software is independently authored ISC;
new documentation is CC BY 4.0. Existing guideline-provenance ambiguity remains
in LICENSING.md. No package binary is relicensed by a JANUS SPDX marker.

## Exact change

Initial `pkg-config --modversion sqlite3` failed because development metadata was
absent. The runtime libsqlite3-0 already existed. Simulation:

```
apt-get --simulate --no-upgrade --no-remove --no-install-recommends install libsqlite3-dev
```

Exit 0: exactly one new package, no upgrades/removals. Approved scoped installation:

```
sudo apt-get --yes --no-upgrade --no-remove --no-install-recommends install libsqlite3-dev
```

Exit 0. Added libsqlite3-dev 3.46.1-7+deb13u2, source sqlite3; download 1109 kB,
additional installed disk 3480 kB. A noninteractive dpkg-preconfigure stdin warning
did not prevent installation. Raw evidence: artifacts/m2-sqlite-plan.log,
m2-sqlite-install.log, m2-dependency-audit.log. No other package was added for M2.

Rollback simulation `apt-get --simulate remove libsqlite3-dev` exited 0, proposing
only that package's removal. Rollback, if requested, is scoped package removal,
not autoremove; retained M1 dependencies are unrelated. Removal has not been run.
No service/group/polkit/sudoers/device/network/desktop/firmware configuration changed.
No VM/domain/network/storage resource was created or operated on during M2.

## Provenance and retained terms

| Dependency | Actual Debian version / source | Applicable provenance/terms |
| --- | --- | --- |
| libsqlite3-0, libsqlite3-dev | 3.46.1-7+deb13u2 / sqlite3 | SQLite contributors' public-domain dedication; Debian packaging GPL-2+ and listed public-domain exceptions |
| libc6-dev static runtime | 2.41-12+deb13u4 / glibc | LGPL-2.1+ and per-file notices/exceptions in installed copyright |
| linux-libc-dev headers | 6.12.107-1 / linux | Linux UAPI notices and syscall exception; no kernel code vendored |
| libvirt-dev (retained M1 tests) | 11.3.0-3+deb13u3 / libvirt | Existing [M1 dependency record](m1-dependencies.md); no new installation |

Primary package records: `dpkg-query -W` with binary/version/source fields and
`/usr/share/doc/libsqlite3-dev/copyright`, `/usr/share/doc/libc6-dev/copyright`.
The SQLite notice identifies https://www.sqlite.org/cgi/src/dir?ci=trunk; exact
consumed revision is the Debian source-package version above, not an invented
upstream Git commit. Local modifications/imported paths: none. Required upstream
notices remain applicable. JANUS uses the SQLite C API via dynamic linking.

The ignored static note executable includes glibc; ISC applies to JANUS-authored
source, not the whole linked artifact or upstream runtime. Distribution would
need its own compliance review for static-link/relink/source obligations and all
applicable notices. No binary distribution or third-party relicensing is authorised
or performed. The per-file installed notices govern beyond these summary labels.

## Kernel support and unchanged removals

Actual Landlock version syscall returned ABI 6. Every live run also installed
rules and observed EACCES on store/key/repository/proc-memory opens. Seccomp
separately denied socket/connect, ptrace/process_vm and another process's prlimit.
Kernel version alone was not treated as support evidence. No privilege changes
were needed for either mechanism. Nested KVM: NOT RUN, unnecessary for M2.

The previously removed virt-viewer, libspice-client-glib-2.0-8,
libspice-client-gtk-3.0-5 and spice-client-glib-usb-acl-helper were not reinstalled.
Historical M1 viewer evidence remains accepted; current rerun is BLOCKED by the
intentional dependency removal. M2 uses no graphical display or viewer.
