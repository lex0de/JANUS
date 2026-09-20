# Candidate device ownership contract

EXPERIMENTAL. J-007, J-010, J-013, J-018.

One synthetic device/group starts AVAILABLE without an owner. Recovery input and
display are separate trusted resources, never this assignable device.

| Event | Required state/actor | Result |
| --- | --- | --- |
| RESERVE | AVAILABLE; OWNER; target RUNNING | RESERVED, bind world/incarnation |
| DRAIN | RESERVED; BACKEND | QUIESCING |
| ASSIGN | QUIESCING; BACKEND; owner RUNNING | ASSIGNED |
| WITHDRAW | RESERVED/QUIESCING/ASSIGNED; OWNER | REVOKING |
| RESET | REVOKING; BACKEND | RESETTING |
| RESET_OK | RESETTING; BACKEND | AVAILABLE, clear owner |
| RESET_FAIL | Any owned state; BACKEND | QUARANTINED, retain owner |

All other transitions are STATE; wrong owner/incarnation is STALE. No transition
out of QUARANTINED is supplied. Reassignment requires separately reviewed actual
recovery evidence and is beyond M0. Device ownership survives viewer loss, stop,
fault, recovery and failed restore. Active leases block successful restore.

BACKEND events stand for externally verified quiescence, mapping withdrawal,
reset and post-reset checks. No hardware action occurs and no universal physical
reset order is implied. DMA/interrupt isolation and reset evidence remain NOT RUN.
