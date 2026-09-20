---
name: janus-boundary-review
description: Review JANUS C/C++, contracts or design changes for authority, ownership, lifecycle, persistence and hardware-recovery errors. Use for a bounded diff; this is not automatic approval, formal verification or authority to fix unrelated code.
---
# JANUS boundary review

Read the current diff, relevant implementation and callers, applicable AGENTS.md,
requirements and ADRs. Derive the expected behaviour from the contract before
reading its test's expected table. Challenge shared assumptions.

Trace each entry point: authenticated caller, granted rights/scope, world
incarnation, object lifetime, sizes and arithmetic, timeouts, cancellation,
concurrency, response encoding and error cleanup. Identify privileged dependencies
and confused-deputy paths. Search specifically for name-as-authority mistakes.

Review stale grants after restore, revocation completion/in-flight semantics,
old-incarnation callbacks, object commit conflicts, console-loss versus execution,
foreground withdrawal versus device release, reset failure/quarantine, and owner
recovery without guest input. Label modelled and actually enforced guarantees.

For C/C++, inspect resource ownership/unwind, integer widths, bounded copies,
format strings, allocation policy, and synchronization. Check proposed test
oracles independently; do not infer safety from clang-format or sanitizer success.

Report actionable findings by severity with file/function, reachable scenario,
impact, evidence, minimal fix and regression test. Distinguish confirmed bugs from
questions. No findings means only no findings in the reviewed scope, not proof.
Do not commit, publish or accept an ADR as a side effect of reviewing it.
