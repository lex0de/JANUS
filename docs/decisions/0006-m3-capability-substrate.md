<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# ADR 0006: M3 capability-substrate experiment

Status: EXPERIMENTAL, authorised by the maintainer on 21 September 2026.
Microkit 2.3.1 is selected for this experiment only, not the permanent JANUS
kernel/framework. M4/M5 remain out of scope. This ADR precedes implementation.

## Comparison

Primary references were consulted on 21 September 2026. Genode 26.08 is the
framework release; Sculpt documentation is separately versioned (the retrieved
manual is 26.04). No distinct Sculpt 26.08 release is inferred or installed.

| Aspect | seL4 + Microkit 2.3.1 | Genode 26.08 / Sculpt ecosystem | Custom JANUS supervisor/kernel |
| --- | --- | --- | --- |
| Execution/isolation | seL4 address spaces/TCBs, Microkit PD event entry points | Hierarchical components on several kernels | Must implement and validate protection machinery |
| Authority | seL4 capabilities; SDF channels with restricted PPC/notify rights | Capability/session interfaces, parent-mediated service routing | Must design minting, delegation, revocation and IPC |
| Composition | Static SDF allocates PDs/resources; limited runtime lifecycle controls | Dynamic component deployment and service composition | Freedom, but all composition machinery is new |
| x86-64 | Generic non-VT-x and separate VT-x targets supplied | Established PC/Sculpt use, ongoing x86 kernel optimisation | No JANUS implementation/maturity evidence |
| VM support | Separate VTX config, 2.3.1 known single-VM/non-SMP restriction | VirtualBox 7 and Seoul integration in 26.08 | Requires VMM ecosystem or new code |
| Device/IOMMU direction | I/O caps and address-space descriptions; not M3 physical evidence | Device/session brokerage and broad driver integration reference | Entire policy/driver/reset integration remains to build |
| Drivers | Small SDK; external ecosystem needed for a complete computer | Wider driver, graphical composition and compatibility ecosystem | Largest engineering burden |
| Languages | C-facing runtime; host tool Rust, initialiser/kernel/runtime upstream code | C++ core/framework, mixed-language ports | Requested C direction; assembly where justified |
| Tooling | SDK builder, ELF/SDF reports, QEMU, debug builds | Framework build/deployment tools, Sculpt/Goa workflow | Debugger/loader/toolchain infrastructure must be built |
| Licensing | Kernel GPL-2.0-only; libseL4/libmicrokit BSD-2-Clause; SDK docs CC-BY-SA-4.0; retain other notices | AGPLv3 framework or commercial terms, ports keep upstream licences | New independent JANUS ISC, imports retain their terms |
| Assurance | Exact kernel configuration matters; MCS/fastpath/IOMMU here do not match published x64 proof scope | No automatic whole-framework proof; depends on chosen kernel and components | No proof or assurance inherited merely from clean design |
| JANUS worlds/activities | Small boundary experiment; JANUS identity/lifecycle is extra service policy | Richer integration but substantial mapping/policy review needed | Conceptually unconstrained, practically high implementation risk |
| JANUS-specific trusted work | CONTROL + narrow service now; dynamic composition/storage/device policy later | JANUS adapters/policy plus framework integration TCB | All supervisor, IPC, authority and recovery mechanisms |
| Route to M4/M5 | Evaluate static limits and capability policy before drivers/persistence integration | Strong ongoing reference/candidate for drivers, GUI and VMs | Deferred unless existing substrates demonstrably cannot meet requirements |

This comparison is architectural judgement, not benchmark evidence. Microkit's
small C-facing surface lets M3 test JANUS semantics without adopting a desktop
framework. Genode is not permanently rejected; it remains relevant for later
integration. No Genode source/binary is downloaded, built or imported.

## Mapping and mechanisms to test

A PD is a substrate execution/isolation mechanism, not the definition of a JANUS
world. Two preallocated PD slots carry distinct JANUS world identities, current
grant generations and incarnations. This experiment rotates semantic incarnations;
it will not claim that resetting a PC clears memory or reconstructs a process.
The sacrificial child fault handler can withhold its reply/stop only that child.

Use a passive object service with higher priority than all callers. Microkit
protected calls derive identity from the received channel, never payload claims.
CONTROL alone has owner PPC authority. Worlds have one service PPC and no owner
PPC, world-to-world channel or private-service-memory mapping. Service-to-world
notifications coordinate bounded tests; they grant no owner operation. CONTROL
receives only service progress notifications and trusted child faults.

Object contents use a few fixed integer message registers, never native structs
or pointers. Two public test IDs, bounded revisions, four handle slots per world,
recipient/generation/incarnation checks, default non-delegability and explicit
UNSUPPORTED delegation preserve narrow accepted semantics. Activities hold only
world/object/revision hints. No SQLite, disk or durable storage claim.

The static SDF is a real architectural constraint: a personal computer may need
dynamic PD allocation, application loading and capability topology beyond fixed
slots. Future delegation could remain service-mediated, map to minted seL4 caps,
or use another mechanism. No permanent choice is made here, and no raw seL4 API
is needed unless a documented finding establishes that Microkit is insufficient.

The release kernel disables debug printing. CONTROL alone therefore receives a
bounded emulated COM1 I/O-port range for serial evidence through documented
Microkit calls. This is test output, not an M4 device-management implementation.
Service private memory is a separate mapped page, deliberately absent from the
sacrificial child. Fault observation must be followed by intact service/other-world
operations. No shared content transport, network, GUI, filesystem, passthrough or VM.

Passive PPC executes on donated caller scheduling context. Explicit budgets,
periods, priorities and one CPU will be recorded from the SDF. Test bounded work
and quotas while CONTROL retains its own context and the other world remains
usable. This is not a hard real-time or host-wide scheduling guarantee.

## Assurance and licensing boundary

The signed SDK's x86_64_generic/release config has VTX=false, KERNEL_MCS=true,
IOMMU=true, FASTPATH=true, DEBUG_BUILD=false, PRINTING=false and one CPU.
VERIFICATION_BUILD=true is not proof applicability. Official x64 proof coverage
excludes hypervisor mode and fastpath; device address translation is not covered.
The actual configuration comparison and proof claims are recorded separately in
M3 evidence. Inherited proof applicability is NOT ESTABLISHED for this artifact;
JANUS policy/components, Microkit composition and QEMU are not thereby verified.

JANUS-authored C remains ISC, documentation CC BY 4.0. SDK kernel/runtime/tool and
its notices remain external, with no implementation copied into JANUS. The local
SDK includes GPL-2.0-only, BSD-2-Clause, CC-BY-SA-4.0 and trademark licence files;
individual source notices govern. Download signatures and hashes are recorded in
dependency evidence. Linked images stay ignored; no binary release is authorised.

Rollback: stop only the owned QEMU child, retain logs, remove local SDK/build files
only when no longer needed. No host policy/module/service changes are required.
Revisit after measured substrate mismatches, dynamic-world requirements, live
delegation, driver integration or a proposal to adopt a permanent kernel.

## Primary sources

- [Microkit 2.3.1 release](https://docs.sel4.systems/releases/microkit/2.3.1.html),
  signed SDK manual §§2, 6–8, 10; actual headers and kernel config are authoritative.
- [Microkit source at 2.3.1](https://github.com/seL4/microkit/tree/2.3.1),
  licence metadata and release build workflow; no code imported.
- [seL4 verified configurations](https://docs.sel4.systems/projects/sel4/verified-configurations.html).
- [Genode overview](https://genode.org/documentation/general-overview/index),
  [Foundations](https://genode.org/documentation/genode-foundations/index),
  [26.08 release](https://genode.org/documentation/release-notes/26.08),
  [commercial licensing](https://genode.org/commercial-use/index), and
  [Sculpt manual 26.04](https://genode.org/documentation/sculpt-26-04.pdf).
