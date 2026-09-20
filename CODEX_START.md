Set up and begin JANUS in this directory. Use the supplied AGENTS.md as the
repository rules and docs/architecture/JANUS_Whitepaper_v0.1.md as the design
source. First read AI_CONTEXT.md, BOOTSTRAP_PLAN.md, docs/REQUIREMENTS.md,
docs/decisions/README.md and the relevant development runbooks. Use the local
janus-bootstrap skill if available; otherwise read its SKILL.md directly.

I approve the bootstrap defaults for this task: a standalone JANUS Git repository
on main; PRIVATE GitHub visibility under my currently authenticated personal
github.com account; C17 for hosted M0 code, restrained C++17 when justified; and
a small Make-based test build. These choices do not select the final kernel,
framework, stable ABI, persistent format, or release licence.

My author identity is Danyal A. Samak <dabsamak@tuta.com>; the project website is
https://www.cryogenix.org. Use it for missing local Git identity only. Do not alter
my global Git configuration or replace an existing suitable identity.

The development host is Debian 13. QEMU/libvirt, git and authenticated gh are
available; nested virtualisation is expected but must be verified. Inspect the
actual host, execution layer, resources, parent Git roots, status and remotes.
Run make check, the host preflight, the dependency simulation and the toolchain
probes. You may use sudo/doas through normal approval prompts for necessary,
explained development-host setup. Do not disable agent approvals, run Codex as
root, reinstall the host/hypervisor, broaden permissions, interrupt existing
VMs, reconfigure my desktop/network, change firmware or reboot the host.

Execute B0 and M0 from BOOTSTRAP_PLAN.md. Produce candidate world/object/authority/
foreground/device contracts, a threat model, and a small portable C reference
model with independent table-driven tests. Cover stale grants after restore,
revocation boundaries, invalid transitions, resource exhaustion, object commit
conflicts, viewer loss versus guest shutdown, and reset failure/quarantine.
Keep model semantics separate from actual OS enforcement. Add and run real M0
build/test and sanitizer targets; do not substitute the supplied toolchain probes
or documentation checks for contract implementation. Record proposed choices
without declaring them approved, and keep the whitepaper unchanged.

You are authorised to initialise this standalone repo, make scoped bootstrap/M0
commits, create the private JANUS GitHub repository, and push the reviewed commits.
Use gh to discover the account and verify repository absence/identity/visibility.
Do not guess a username, overwrite an existing repo, change visibility, force-push,
choose a public licence or publish a release. Inspect staged files for secrets,
private host data, generated outputs and licensing before every push. If the name
already exists or the remote is ambiguous, stop only that external operation and
finish independently safe local work.

Use the boundary-review and evidence workflows before handoff. Preserve exact
commands, exit statuses, negative tests and limitations; keep raw host logs in
ignored artifacts/. Update AI_CONTEXT.md only with observed facts and explicit
approvals. Report the real repository URL/commit if created, host changes and
rollback, tests by outcome, and remaining decisions.

Stop at M0 READY FOR REVIEW, or report the precise failures/blockers. Do not start
M1 guest management, deploy a kiosk, assign hardware, or select/implement a new
kernel until I approve the next scope. Ask about material design/safety decisions,
not reversible details already authorised here. Do not call your own review a
maintainer approval or claim a capability model enforces isolation.
