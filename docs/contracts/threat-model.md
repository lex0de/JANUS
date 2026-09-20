# Candidate threat model

EXPERIMENTAL; J-015, J-017 and whitepaper sections 5, 7–9, 12–13.

Assets: object contents and committed roots, current authority, owner credentials,
cross-world confidentiality, bounded service capacity and owner recovery control.
Adversaries: hostile worlds/guests, malformed inputs, stale/replayed callbacks,
compromised viewers/drivers/peers, and ordinary crashes/exhaustion/power loss.

| Component | Authority / failure consequence | Required eventual boundary |
| --- | --- | --- |
| Linux host, VMM and privileged services | Can inspect/change prototype memory and guests | Hosted TCB, not native isolation proof |
| Lifecycle/device broker | Can start/stop selected execution and affect devices | Authenticated bounded operations, UUID/incarnation mapping; no shell forwarding |
| Authority service and trusted grant UI | Can approve or revoke access | Separate shell requests from authenticated owner decision |
| Object/key service | Can expose plaintext, corrupt roots or deny access | Scoped grants, atomic durable publication, explicit key/recovery policy |
| Display/input/recovery | Can observe/spoof input and deny owner control | Reserved resources and host-owned trusted path |
| Native supervisor/platform | Will mediate memory, IPC, scheduling, DMA | Substrate/configuration-specific evidence not provided by M0 |

M0's entire process and caller are trusted. Actor enums and array indices are
forgeable in-process values. Tests demonstrate proposed decision semantics only;
they cannot resist memory corruption or malicious callers modifying model state.
There is no daemon, privileged command parser, network, runtime LLM, crypto,
on-disk authority record or guest lifecycle implementation.

Ordered tests challenge stale grants, descendant revocation, in-flight draining,
object conflicts, resource limits, viewer loss and failed reset. Real races,
power loss, disk rollback, copied secrets, DMA and physical recovery are outside
that evidence. Anti-rollback requires a separately chosen anchor/recovery policy;
restoring an entire model copy would restore old authority, just as a disk rollback
would without an external anchor. Do not persist the C struct as a solution.

Compromised firmware, invasive physical attacks and complete side-channel
resistance are not claimed. Confidentiality still trusts authorised readers and
their allowed output channels. Human M0 review must challenge these assumptions.
