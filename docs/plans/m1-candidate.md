# M1 candidate scope — NOT AUTHORISED / NOT RUN

Prerequisites: maintainer M0 review and explicit M1 scope approval. This document
is the requested next-step proposal, not guest-management implementation.

1. Select one disposable lab topology and resource budget from fresh inventory.
   If nested acceleration is required, demonstrate an actual L2 KVM boot before
   claiming it works. Preserve existing guests; no physical-device assignment.
2. Define a small authenticated broker interface for select/run/recover/leave.
   Bind authorised profiles to explicit libvirt URI, domain UUID and incarnation.
   No arbitrary XML, QMP or shell forwarding; no guest network by default.
3. Reconcile backend execution independently of viewer/foreground state. Test
   viewer crash/disconnect, guest reboot/shutdown and broker restart. Reconnect
   or withdraw foreground without accidentally destroying execution.
4. Keep owner recovery input/display host-owned. Bound broker work and define
   cancellation/reconciliation for dead worlds before claiming revocation drains.
5. Retain immutable image base, per-test overlays, bounded timeouts and exact
   resource ownership. Verify teardown only for created resources. Record live
   backend evidence separately from model tests.

Acceptance proposal: J-007/J-013/J-016 live backend checks, no stale callbacks or
duplicate guests after restart, and recovery independent of guest cooperation.
Rollback: stop only recorded disposable resources and retain failure logs. No
native substrate, kiosk deployment, host-policy change or hardware decision here.
