# Candidate world and resource contract

EXPERIMENTAL. J-001, J-004, J-006, J-009, J-011, J-012, J-018.

Each world declares NATIVE or VM class and RECONSTRUCTIBLE, CHECKPOINTABLE or
EPHEMERAL persistence. Both classes follow the same model without claiming the
same protection mechanism. Initial incarnation is one, state DECLARED, no grants.

| Event | Actor/precondition | Postcondition |
| --- | --- | --- |
| START | OWNER; DECLARED | RUNNING |
| QUIESCE | OWNER; RUNNING | QUIESCING; new work denied |
| SUSPEND | BACKEND; QUIESCING; no pending work | SUSPENDED; no durable checkpoint implied |
| STOP | BACKEND; RUNNING/QUIESCING/FAULTED; no pending work | STOPPED; viewer/focus withdrawn; device unchanged |
| FAULT | BACKEND; RUNNING/QUIESCING/SUSPENDED | FAULTED; viewer/focus withdrawn; device unchanged |
| CHECKPOINT | OWNER; SUSPENDED and CHECKPOINTABLE | increment committed snapshot counter |
| RESTORE | OWNER; SUSPENDED/STOPPED/FAULTED; not EPHEMERAL | RUNNING with new incarnation; viewer/focus cleared |
| BUDGET | OWNER; requested 0..12; at least outstanding work | replace budget if sum <=12 |

Checkpointable restore requires a committed snapshot; reconstructible restore
uses committed objects without a CPU snapshot. Restore rejects unfinished work
or any device state still owned by that world (including quarantine). An injected
restore failure changes nothing. Successful restore retains committed snapshot,
objects, authority records and terminal external outcomes; old grants/events are
stale. Owner may explicitly issue new grants against current policy afterwards.
Counter exhaustion rejects before mutation. No RNG/session cryptography is
modelled. Backend identity and hardware confirmation remain future obligations.
