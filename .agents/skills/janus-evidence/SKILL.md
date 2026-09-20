---
name: janus-evidence
description: Validate and hand off a JANUS change with exact commands, real outcomes, scope and claim boundaries. Use after implementation or at a review gate; never manufacture logs, passing tests, approvals or hardware evidence.
---
# JANUS evidence

Read applicable instructions, the task plan, diff and requirement IDs. Identify
which checks actually exist; do not invent make targets. Run proportionate checks
and retain each failure/exit code. Add missing relevant tests rather than masking
a failure behind later commands.

Separate bootstrap syntax/configuration, compiler/sanitizer probes, reference
model tests, enforced runtime boundaries, real backend/guest boots, and exact
physical-device results. Use PASS, FAIL, NOT RUN, BLOCKED, NOT APPLICABLE.
A compiler probe or in-process mock cannot establish live capability isolation.

Prepare a sanitised report using docs/templates/EVIDENCE.md. Keep raw logs and
host-sensitive output in artifacts/. Include source identity, environment, exact
invocations, negative cases, oracle independence, limitations and recovery state.
Do not claim the user's Debian was tested from a different execution environment.

Inspect staged content before any separately authorised Git write. Update the
short AI_CONTEXT checkpoint from observed facts only. Mark a complete M0 package
READY FOR REVIEW, not ACCEPTED. End at the user's requested gate with the next
permitted action and unresolved decisions.
