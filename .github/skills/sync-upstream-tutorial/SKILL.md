---
name: sync-upstream-tutorial
description: 'Safely fetch and merge changes from the original KhronosGroup/Vulkan-Tutorial upstream repository while preserving in-progress tutorial work, then rebuild the local Antora HTML documentation. Use when asked to sync upstream, pull original repository changes, update the fork, or refresh local tutorial docs.'
argument-hint: '[push]'
user-invocable: true
---

# Sync Vulkan Tutorial Upstream

Integrate `upstream/main` into the current local branch without rewriting local
history or discarding in-progress work, then regenerate the local Antora site.

## Safety Rules

- Run only inside the `KhronosGroup/Vulkan-Tutorial` fork.
- Preserve the current branch. Do not rebase or switch branches.
- Never use `git reset --hard`, `git checkout --`, `git clean`, force push, or
  another operation that can discard local work.
- Never resolve a merge or stash conflict automatically by choosing one side.
- Do not create commits except for the merge commit Git may produce when
  integrating `upstream/main`.
- Do not push unless the user explicitly requested it, including by passing
  `push` to the slash command.
- Stop if a merge, rebase, cherry-pick, or revert is already in progress.
- Stop if HEAD is detached.

## Procedure

1. Locate the repository root with `git rev-parse --show-toplevel` and run all
   remaining commands there.
2. Inspect and record:
   - Current branch and HEAD.
   - `git status --short`.
   - `git remote -v`.
   - Whether Git reports an operation already in progress.
3. Verify the `upstream` remote:
   - Expected URL: `https://github.com/KhronosGroup/Vulkan-Tutorial.git`.
   - If it is missing, add it with `git remote add upstream <expected-url>`.
   - If it exists with another URL, stop and ask before changing it.
4. Protect uncommitted work:
   - If the working tree is dirty, create a uniquely named stash with
     `git stash push --include-untracked -m "copilot: pre-upstream-sync <timestamp>"`.
   - Record both the stash object ID and its `stash@{n}` reference from
     `git stash list --format='%gd %H %gs'`.
   - Verify the working tree is clean before continuing. If stashing fails,
     stop without fetching or merging.
5. Fetch the original repository using `git fetch upstream --prune`.
6. Confirm `upstream/main` exists, then record:
   - Ahead/behind counts from
     `git rev-list --left-right --count HEAD...upstream/main`.
   - Incoming commit IDs and subjects from
     `git log --format='%h %s' HEAD..upstream/main`.
7. Merge with `git merge --no-edit upstream/main`.
   - If the merge conflicts, list unresolved files, run `git merge --abort`,
     restore the named stash if one was created, and stop. Report that no
     upstream merge was retained.
   - Do not guess how tutorial work and upstream changes should be combined.
8. Restore dirty work, if stashed:
   - Apply the recorded stash object with `git stash apply <object-id>`.
   - Drop only the recorded stash entry after a conflict-free apply.
   - If applying conflicts, keep the stash, list unresolved files, and stop.
     Explain that the upstream merge succeeded but local work needs manual
     reconciliation and remains recoverable from the recorded stash.
9. Rebuild the local documentation only after Git integration and stash
   restoration are conflict-free:

   ```bash
   make -C antora
   npx --yes antora@3.1.15 \
     --ui-bundle-url='https://gitlab.com/antora/antora-ui-default/-/jobs/artifacts/HEAD/raw/build/ui-bundle.zip?job=bundle-stable' \
     --stacktrace \
     antora-ci-playbook.yml
   ```

10. Validate that `build/site/index.html` exists and report the number of
    generated HTML pages. Do not treat ignored generated files as source edits.
11. Show the final `git status --short` and summarize:
    - Previous and new HEAD.
    - Number and subjects of incoming upstream commits.
    - Whether local work was stashed and restored.
    - Antora build result and local entry point.
12. If and only if pushing was explicitly requested, push normally with
    `git push origin <current-branch>`. Never force push.

## Failure Handling

- Preserve the first failure and its relevant output; do not hide it by
  continuing into the documentation build.
- If restoring a stash fails, always report its object ID and stash reference.
- If Antora fails, leave the successful Git merge intact, report the build
  error, and keep the prior generated site rather than deleting it.