# Creating the JANUS repository

This is a runbook, not an automatically executed publisher. Use it only under the
explicit bootstrap request or a later maintainer instruction. [GH] in
`docs/SOURCES.md` documents the CLI syntax. Confirm the installed `gh ... --help`.

## Establish identity and target

Inspect parent Git roots before `git init`; the extracted JANUS directory should
be a standalone root. Inspect local status, branch, remotes and author identity.
Do not overwrite another repository or init below an unrelated Git worktree.

Use the actual authenticated personal account on github.com, not a remembered
username, author email, arbitrary organisation, or gh default host override:

```sh
gh api --hostname github.com user --jq .login
```

Do not display tokens or run `gh auth token`. Check that the returned identity is
the intended personal account. The target is that account's `JANUS` repository.
If multiple accounts make the target ambiguous, ask rather than publishing under
a different identity. The user did not request organisation administration.

Inspect whether the target already exists. A name collision requires reconciliation
with the maintainer, not deletion, force-push, a new name or changed visibility.
An authentication/network failure is not proof of absence. Complete safe local
work when the remote operation is blocked.

## Local repository

For a confirmed fresh standalone tree:

```sh
git init -b main
```

If local identity is missing, set it in this repo only:

```sh
git config --local user.name 'Danyal A. Samak'
git config --local user.email 'dabsamak@tuta.com'
```

Preserve an already suitable owner-configured identity. Do not change global Git
settings, disable signing requirements, rewrite branches, or replace origin.

Review candidate files and `.gitignore`. Stage only reviewed paths, inspect
`git diff --cached --name-status`, `git diff --cached --check`, and the actual
staged diff before a scoped commit. Keep machine inventories, images, raw logs,
credentials, compiler outputs and private agent state out of Git. Pattern checks
are aids, not proof that content contains no secrets.

## Create PRIVATE, then verify before push

After the identity and absence checks, the intended command is:

```sh
# owner must be the verified result of the identity query above.
gh repo create "$owner/JANUS" \
    --private \
    --source=. \
    --remote=origin \
    --description 'Experimental personal operating system of isolated worlds' \
    --homepage 'https://www.cryogenix.org'
```

This command requires working github.com authentication. If GH_HOST points
elsewhere, resolve it for this command without editing global configuration.
Do not include `--license`, `--public`, or `--push` in the creation step.

Verify the resulting owner/name, remote URL (without credentials) and private
visibility with `gh repo view`/API before pushing the reviewed main branch:

```sh
git push -u origin main
```

Then verify the remote commit identity and visibility. Report the actual URL and
commit only after they exist. A partially completed create/push is recoverable:
inspect the real state; never rerun blindly, delete the repo, or force the push.

This bootstrap authorises scoped initial and M0 commits/pushes to that private
repository. It does not authorise future automatic publication, public Actions
artifacts, releases, added collaborators, paid resources, or a public licence.
