# Evidence: task or milestone

Date:
Scope / requirement IDs:
Source commit and working-tree state:
Environment / execution layer:
Tool and dependency versions:
Authority for host/Git/guest changes:

## Commands and outcomes

| Command / exact invocation | Exit code | Outcome | Log or artifact |
| --- | --- | --- | --- |
| Not yet executed | n/a | NOT RUN | n/a |

Use PASS, FAIL, NOT RUN, BLOCKED, NOT APPLICABLE. Record timeouts explicitly.
Keep raw private logs in artifacts/; include sanitised, necessary evidence here.
A log path is not evidence unless the log exists and contains the stated result.

## What the evidence establishes

Separate syntax checks, compiler probes, model semantics, enforcement, actual VM
boot milestones, and exact physical-hardware testing.

## Negative cases and independent oracle

Expected behaviour source; malformed inputs; interruption/fault/replay scenarios;
shared-assumption risks when one agent wrote both implementation and tests.

## Changes, limitations, and recovery

Bounded diff, setup changes and rollback, failed/unrun checks, risks, and next
permitted action. Do not claim milestone acceptance without recorded review.
