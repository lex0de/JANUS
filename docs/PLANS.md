# Scoped execution plans

For multi-file or multi-step work, keep a short plan in `docs/plans/` or the
current task's evidence file. Do not generate plans for trivial edits. The plan
is a checkpoint across sessions, not a substitute for implementation or proof.

Record the goal, authorised scope, relevant requirement IDs, affected paths,
current observations, test oracle, exact acceptance checks, and review stop.
Track work as NOT RUN / IN PROGRESS / PASS / FAIL / BLOCKED; distinguish a completed
change from an approved design. Record decisions in ADRs instead of burying them
in the plan. End with the next bounded action and recovery state.

Before resuming, inspect Git and current evidence; do not trust an old plan's
claim about the current machine. Keep source facts, agent proposals, and observed
outcomes distinct. Do not append transcripts or private host inventories.

For parallel work, name a single integration owner and assign non-overlapping
paths. Workers do not install packages, mutate libvirt, commit, or publish unless
the task explicitly delegates that action. Reconcile their results against the
same requirements and real test output before integration.
