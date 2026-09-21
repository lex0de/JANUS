<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# M3 dependencies, provenance and proof boundary

Observed 21 September 2026. No package installation, upgrade, removal, host policy,
service, group, device-permission or firmware change was needed. Removed M1 viewer /
SPICE USB helper packages were not restored. Apt installation/rollback simulation:
NOT APPLICABLE because no packages were selected or changed. Existing tools were
inspected before use; raw inventory and host preflight remain in artifacts/.

## Pinned signed SDK

- Upstream: [official Microkit 2.3.1 release](https://github.com/seL4/microkit/releases/tag/2.3.1).
- Asset: `microkit-sdk-2.3.1-linux-x86-64.tar.gz`, 72,548,289 bytes.
- SHA-256: `b1b285b8785db7557eba6d0665dd4fbf2f265ea5c0b39d6f91ae3dcc43d2c424`.
- Detached signature: `microkit-sdk-2.3.1-linux-x86-64.tar.gz.asc`.
- Key fingerprint: **FE91 4864 43B0 F4EB 9ECC 3652 4D86 8A34 EDF3 FDCA**.
- Signature identity: Julia Vassiliki `<julia.vassiliki@unsw.edu.au>`;
  signature timestamp 18 September 2026, acquisition 21 September 2026.
- Key source: `https://keys.openpgp.org/vks/v1/by-fingerprint/FE91486443B0F4EB9ECC36524D868A34EDF3FDCA`.
- Fingerprint matched the maintainer-supplied and official documented release key
  before isolated import. `gpg --verify` returned 0 and VALIDSIG with that exact
  fingerprint. PASS. GPG's TRUST_UNDEFINED warning remains recorded: identity is
  pinned by fingerprint, not certified by the local web of trust.

Archive/signature URLs are the official release download prefix
`https://github.com/seL4/microkit/releases/download/2.3.1/` plus the filenames above.
Local archive/key/signature: `artifacts/toolchains/microkit-2.3.1-download/`;
extracted SDK: `artifacts/toolchains/microkit-sdk-2.3.1/`. No SDK source, binaries,
licence files or linked images are vendored or published in JANUS Git.

Initial exact verification used `gpg --homedir artifacts/toolchains/microkit-gpg
--batch --with-colons --show-keys` on `signing-key.asc`, then isolated `--import`,
then `--batch --status-fd 1 --verify` with the signature and archive paths above.
All returned 0; raw `m3-key-inspection.log` and `m3-sdk-signature.log` are retained.
`tools/m3-build.py` repeats signature/fingerprint verification in a temporary GPG
home and compares 76 installed inputs (tool, VERSION, release board files) with
that signed archive. It rejects altered/missing/extra build inputs and downloads
nothing. The primary build never uses x86_64_generic_vtx.

## Source and licences

Microkit tag 2.3.1 resolves to commit
`ec86afdcd662b5976d11d4994acf1b11a2979882`.
[Successful release workflow](https://github.com/seL4/microkit/actions/runs/35307003226)
ran 18 September 2026 at that commit. Its SDK workflow uses the seL4/microkit-manifest
main.xml. The preceding manifest revision
[`c26cbe4e0c2ac60db925fb28b8cf9146f261bdcd`](https://github.com/seL4/microkit-manifest/blob/c26cbe4e0c2ac60db925fb28b8cf9146f261bdcd/main.xml)
pins **seL4 16.0.0**, commit
**`6e7c3b733d296cfd88d5fbf635c96e447a882374`**; VERSION at that commit is 16.0.0.
This is release-source traceability, not an independently reproduced SDK binary
build. The signed SDK does not supply a separate embedded seL4 VERSION file;
archive/config/kernel hashes below identify the actual tested binary precisely.

| External unit | Observed upstream licence/metadata | Usage |
| --- | --- | --- |
| seL4 kernel | GPL-2.0-only | External kernel ELF, distinct from JANUS ISC PD source |
| libseL4 / libmicrokit | BSD-2-Clause headers/notices | External headers/static runtime linked locally |
| Microkit monitor | BSD-2-Clause source header at pinned tag | External trusted fault infrastructure |
| Microkit tool and initialiser | BSD-2-Clause package/source metadata; initialiser uses sel4-capdl-initializer | External Rust tool / root task, no JANUS Rust added |
| SDK documentation | CC-BY-SA-4.0 | Reference manual, not copied into JANUS docs |
| SDK branding | LicenseRef-Trademark | No branding grant inferred |

The SDK LICENSES directory contains BSD-2-Clause, GPL-2.0-only, CC-BY-SA-4.0
and LicenseRef-Trademark text. Individual files and transitive crate notices still
govern; this is not a complete binary-redistribution SBOM or permission to relicense
all SDK dependencies as BSD. No upstream implementation was imported, translated
or rewritten into JANUS. Local modifications to upstream: none. Source URLs,
versions, paths and usage above are external dependency records, not imports.
JANUS C/Python/SDF source is independently authored ISC; JANUS prose CC BY 4.0.

Existing Debian packages (no added packages/disk-use delta):

| Package | Installed version | Source package / upstream licence family |
| --- | --- | --- |
| gcc | 4:14.2.0-1; compiler 14.2.0 | gcc-defaults GPLv2+; GCC GPLv3+ with applicable runtime exceptions |
| clang, clang-format | 1:19.0-63; tools 19.1.7 | llvm-defaults GPLv2+; LLVM Apache-2.0 with LLVM exception and retained notices |
| binutils | 2.44-3 | binutils / GPLv3+ components |
| qemu-system-x86 | 1:10.0.13+ds-0+deb13u1 | qemu / GPL-2.0 with individually licensed components |
| gnupg | 2.4.7-21+deb13u1 | gnupg2 / GPLv3+ |
| gpg | 2.4.7-21+deb13u1+b5 | gnupg2 / GPLv3+ |
| curl | 8.14.1-2+deb13u5 | curl / curl licence |
| python3 | 3.13.5-1 | python3-defaults / PSF and retained notices |
| poppler-utils | 25.03.0-5+deb13u4 | poppler / GPL-2 or GPL-3, Apache-2.0 components |

Exact package notices remain in `/usr/share/doc/<package>/copyright`; these tools
are not vendored. Rollback consists only of retiring ignored local SDK/build/log
files after review. Owned QEMU children were terminated and reaped. No libvirt
resources were created or modified. Nested KVM and optional VTX spike: NOT RUN.

## Actual kernel configuration and proof assessment

Signed SDK path: `board/x86_64_generic/release/include/kernel/gen_config.json`.
SHA-256: `f9197ee1a5019f2b579c2a3b513582f2a72a4cd1e3549b5773d05da4d71111f9`.
This pinned file is the full exact configuration, not inferred from board naming.

| Setting / comparison | Observed release value | Assessment |
| --- | --- | --- |
| ARCH_X86_64 / PLAT / WORD_SIZE | true / pc99 / 64 | MATCHED published x64 platform family |
| VTX | false | MATCHED non-hypervisor scope |
| MAX_NUM_NODES / ENABLE_SMP_SUPPORT | 1 / false | MATCHED uniprocessor scope |
| KERNEL_MCS | true | NOT ESTABLISHED by published x64 proof configuration |
| FASTPATH | true | X64_verified.cmake also enables it, but published theorem scope explicitly excludes fastpath |
| IOMMU | true | Device address translation proof NOT ESTABLISHED; no assigned devices tested |
| VERIFICATION_BUILD / BINARY_VERIFICATION_BUILD | true / false | Build settings alone do not establish any proof |
| DEBUG_BUILD / PRINTING / benchmarks | false / false / none | MATCHED release exclusions |
| ROOT_CNODE_SIZE_BITS | 17 | PARTIAL: pinned X64_verified.cmake selects 19 |
| MAX_NUM_BOOTINFO_UNTYPED_CAPS | 230 | PARTIAL: pinned X64_verified.cmake selects 50 |
| NUM_DOMAINS / RETYPE_FAN_OUT_LIMIT | 16 / 256 | MATCHED pinned configuration values |
| IRQ controller / local APIC | IOAPIC / XAPIC | Actual SDK configuration |
| XSAVE / feature set / size | true / 3 / 576 | Actual SDK configuration, floating-point support present |
| HUGE_PAGE / FSGSBASE_INST / optimization | true / true / -O2 | Actual SDK configuration |

Comparison source: pinned
[X64_verified.cmake](https://github.com/seL4/seL4/blob/6e7c3b733d296cfd88d5fbf635c96e447a882374/configs/X64_verified.cmake)
and [published proof scope](https://docs.sel4.systems/projects/sel4/verified-configurations.html),
checked 21 September 2026. Overall configuration correspondence: **PARTIAL**.
Inherited proof claims for the tested kernel artifact: **NOT ESTABLISHED**.
No proof checker or reproducible kernel rebuild was run. JANUS policy, PDs,
Microkit composition/monitor/initialiser and QEMU do not inherit a whole-system
proof. Boot and device address translation are outside the cited proof scope.
VTX proof applicability: NOT APPLICABLE to this non-VTX experiment.

| Signed SDK binary | SHA-256 |
| --- | --- |
| elf/sel4.elf | `ccb46a063ebbe1f913df866fff34bbb5d1016ae620dfeea571a07ba4900210d5` |
| elf/sel4_32.elf | `dd743d4c95f38fd40d43bf82c1292938dffd5ce3d5dd32f225762d220c4aebee` |
| elf/initialiser.elf | `07c668e291d59252cbf15f06aa3a75596ad17bf6f3f20ec57728b5ab17393ac7` |
| elf/monitor.elf | `e613901ca7f8dd4095b8067bc6b2eb80e6b7aeb23b79210ed78c3bcbec82168e` |
| lib/libmicrokit.a | `5de04dec298835918464f20f72fd84cf11e739e22798851b516beb466f070786` |
