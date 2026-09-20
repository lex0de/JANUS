# Candidate object commit contract

EXPERIMENTAL. J-002, J-012, J-014, J-018.

Two synthetic objects begin at revision one. Knowledge of an index/revision does
not authorise BEGIN; a current scoped grant is required. Immutable content bytes,
schemas, collections, retention and multi-object transactions are not implemented.

WRITE BEGIN captures an expected base. DONE checks the current root revision:
same base increments the root; different base returns CONFLICT and preserves
both root and pending transaction. CANCEL abandons the pending transaction.
At UINT64_MAX, commit returns LIMIT without wrap or publication. Injected commit
failure returns IO and leaves the root and pending transaction intact.

The single copied-state publication is the model's atomic boundary, not durable
storage. Power loss, partial writes, fsync ordering, backup and physical authority
rollback remain untested. Checkpoint/restore never rolls back these object roots
or terminal external outcomes. Disclosed data cannot be recovered by revocation.
