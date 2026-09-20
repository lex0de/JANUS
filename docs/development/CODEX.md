# Codex setup and context use

Tool details below were checked against official OpenAI documentation listed in
`docs/SOURCES.md`. Confirm the installed CLI with `codex --version` and
`codex --help`; managed policies may override project settings.

## Local interactive use

Start from the standalone JANUS directory. Review the files before marking this
specific project trusted in Codex. Project configuration is not permission to
trust every repository or the whole home directory. The supplied config uses
workspace-write and on-request approvals; ordinary shell network access is off.
For GitHub, apt and necessary host changes, request a scoped execution approval.
No danger-full-access, automatic approval bypass, or root Codex invocation.

```sh
codex --sandbox workspace-write --ask-for-approval on-request
```

Paste `CODEX_START.md`. It explicitly directs the reading sequence, approved
bootstrap defaults, commands, GitHub work and M0 review gate. A local CLI/IDE is
needed for this host-specific task; a cloud container's checks do not prove
anything about the owner's Debian/libvirt installation.

## Files and loading behaviour

`AGENTS.md` is the automatically discovered project instruction entry point.
`AI_CONTEXT.md`, `BOOTSTRAP_PLAN.md` and the whitepaper are ordinary documents;
they are read because the instructions request them, not because their filenames
magically create memory. Keep current state concise and load technical sections
on demand. The root instructions stay below Codex's documented default 32 KiB
project-instruction budget; ancestor instructions also consume space.

Repo-local skills are in `.agents/skills/`. Codex can discover their names and
summaries and load their bodies when used. This bundle provides:

- `$janus-bootstrap`: the bounded setup/M0 workflow; explicit invocation only.
- `$janus-boundary-review`: independent architectural/code-boundary review.
- `$janus-evidence`: validation, evidence classification and truthful handoff.

Use `/skills` to inspect discovery. If the installed version does not discover a
skill, read its `SKILL.md` directly and follow the same rules; do not invent a
new configuration key or install random plugins to work around it.

## Safety limits and configuration

`.codex/config.toml` contains no API keys, account identity, MCP server, model pin,
experimental agent swarm, global write roots or background services. It does not
change OS privileges, guarantee credential isolation, or override managed policy.
AGENTS instructions are not an access-control boundary. Use normal approvals and
separate guests for untrusted execution as the project develops.

The account's existing model choice is retained. Do not pin a guessed model name
or unsupported reasoning setting. Longer context is not a reason to load every
file on every turn. Inspect current status and the task's affected implementation,
callers, tests, contracts and primary manuals, then edit a bounded scope.

For each new session, refresh the checkout and actual evidence. Update the short
session checkpoint on handoff. Subagents should receive specific paths and tests;
only their integration owner changes shared state or performs privileged writes.
