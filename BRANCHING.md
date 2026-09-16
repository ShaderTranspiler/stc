# Branching

The git branch model this repository uses, how it works together with releases and what CI does on each branch.

## Overview

Two long-lived branches, four kinds of short-lived ones. A branch keeps its own commits when it lands — merge commits are fine, squashing is not (see [Merge policy](#merge-policy)). Tags serve as an indicator for releases and other events in the history.

```
feature/*  fix/*
      \       \
       \       \
develop ●───●───●───●───●────────●─────●───●─────●───────●
                    /      \      /          /     /          \
release/0.9.0      /        ●────●────●────●─────●      (next cycle)
                  /               \          \     \
main ───────────●───────────────●─────────●─────●───────
                /                rc.1       rc.2  v0.9.0
               /
           hotfix/*
```

## Long-lived branches

### `main`

The released state. Nothing is developed here directly: `main` only ever advances by merging a `release/*` or `hotfix/*` branch, and it is protected so that it cannot advance any other way.

Every such merge publishes. The release workflow reads the version block off what arrives and cuts the tag and the GitHub release from it, so reaching `main` is what makes a version real.

Because branches land with their commits intact, `main` holds the full history of everything merged into it rather than one commit per version. The tags, not the commits, are what mark the published versions.

### `develop`

The integration branch, and the default target for everyday work. `feature/*` and `fix/*` branches are opened from it and merged back into it. It holds work that is finished but not yet part of a version.

After `main` advances (with a `hotfix/*` or release), `develop` is brought back up to it so the two never diverge backwards.

## Short-lived branches

### `feature/*`

New functionality. Branches from `develop`, merges back into `develop`.

Name them after what they add: `feature/hlsl-backend`, `feature/struct-lowering`.

**NOTE:** in very rare and justifiable situations, `release/*` branches may open a new feature branch for last-minute functionality that **CANNOT** wait for the next release. These feature branches are merged back into the release branch first, and then into `develop` when a release candidate is published.

### `fix/*`

Corrections to unreleased work. Branches from `develop`, merges back into `develop`.

`fix/*` branches are also the normal way to correct a release candidate. In that case they open from the release branch and merge back into it as soon as the fix is done; the next candidate is published from the release branch afterwards.

### `release/*`

Release preparation, named for the version being prepared: `release/x.y.z`.

Opened from `develop` when a version's scope is settled. From that point the branch carries only release work (version bumps, release-blocking fixes, documentation for the release). New development continues on `develop` in parallel and lands in the **next** version.

The branch stays open for the whole candidate cycle and is deleted after the final release merges.

The release branch should be merged back into `develop` after every publish (be that for a release candidate or a proper release). This is to ensure that release branches don't introduce any changes that are specific to that given version only, and `develop` doesn't diverge far from the release fixes with incompatible changes.

### `hotfix/*`

An urgent correction to something already published. Branches from `main` at the affected release and merges back into `main` to publish the patch. A hotfix always carries a patch version bump, which CI enforces on the pull request.

**`main` is the distribution point for a hotfix.** The hotfix branch merges into `main` and nowhere else. `main` is then merged into `develop`, and into any release branch that is currently open, so every line picks the fix up from one place instead of the same branch being merged into three.

Merging `main` into an open release branch is safe in a way that merging `develop` into it would not be. `main` carries only released code and hotfixes, so the merge brings the fix and nothing else — none of `develop`'s work aimed at the next version.

Leaving an open release branch to pick the fix up implicitly, when it eventually merges into `main`, does also work: `main` never loses the hotfix, so the published artifacts are correct either way. What it costs is that the release branch's own CI then runs against code missing the fix, and that any conflict between the hotfix and the release work surfaces during the merge into `main`, which is publish time. Merging `main` in straight away moves that discovery somewhere cheaper.

This is the only path other than a release branch that may touch `main`.

## The release cycle

1. **Open `release/x.y.z` from `develop`**, once the version's scope is settled, and set the version block to `x.y.z` with the `rc.0` suffix in its first commit. The branch is not expected to be stable yet — it is a preview others can try out, not something to build on, and `rc.0` says exactly that.
2. **Land release work on it.** Release-finalisation commits, stability fixes and the rare last-minute feature, each through a `fix/*` or `feature/*` branch opened from the release branch and merged back into it.
3. **Reach a candidate-ready state.** Feature-complete and stable: as far as the tests and developer testing can tell, this code would work if it were pulled into production as-is. From here consumers can expect nothing but fixes.
4. **Declare the candidate:** raise the suffix from `rc.0` to `rc.1`, or from `rc.N` to `rc.N+1` on a later pass. See [VERSIONING.md](VERSIONING.md) for how the version itself was chosen.
5. **Merge into `main`**, and confirm the pre-release was published and tagged.
6. **Merge the release branch into `develop`.** Every candidate's fixes go back immediately, so `develop` never drifts far from what is being stabilised and the two cannot accumulate incompatible changes.
7. **Fix whatever the candidate surfaces**, on the release branch, then repeat from step 4 with the next `rc.N`.
8. **Accept the candidate:** clear the suffix and merge into `main`.
9. **Confirm the pipeline ran** — the tag, the artifacts, and the documentation deployment.
10. **Publish the draft release**, with a description and a summarised changelog.
11. **Merge back into `develop`**, from `main` or equivalently from the release branch, which is at the same commit.
12. **Delete the release branch.** The tags and releases remain as the markers of what was published.

Steps 5 and 8 are guarded: a pull request into `main` from a release branch must move the version by exactly one patch, minor or major step, or keep the version and move the candidate number forward, and it may not still be carrying `rc.0`. A hotfix pull request must move the version by exactly one patch step. A forgotten bump would otherwise fail silently, since the release workflow finds the tag already present and skips.

## What CI does

| workflow | trigger | branches |
|---|---|---|
| **CI** (build & test) | `push` | `main`, `develop`, `release/**` |
| **CI** (build & test) | `pull_request` | targeting `main`, `develop` or `release/**` |
| **Version Check** | `pull_request` | targeting `main` |
| **Julia Digests** | `pull_request` | any target; checks only when the Julia manifest changed |
| **Release** | `push` | `main` only |
| **Docs** | `push` | `main` only |

CI, Release, Docs and Julia Digests also accept `workflow_dispatch`, so any of them can be run by hand. Version Check cannot, since it has nothing to compare without a pull request's branches.

Consequences worth knowing:

**Build and test runs on pushes to the long-lived branches only.** `feature/*`, `fix/*` and `hotfix/*` are verified by their pull request run instead, so that a branch with an open pull request is not built twice for every commit. The practical effect is that work on one of those branches is unverified until a pull request is opened against `main`, `develop` or a `release/**` branch, which is usually what you want while a branch is still churning.

**A newer commit cancels the run it supersedes.** CI and Version Check cancel any run still in flight for the same ref, so pushing a fix on top of a broken commit does not leave the old run occupying a slot. Release and Docs deliberately do not cancel: they queue instead, because interrupting a publish half-way can leave a tag cut with artifacts missing, and GitHub Pages permits only one deployment at a time.

**Anything reaching `main` publishes.** The release workflow runs on every push to `main` and reads the version block to decide what to do. It skips silently when the composed version already has a tag, so pushing to `main` without a version change is harmless, but it does mean the version block
is the thing standing between a merge and a public release. The docs workflow likewise redeploys the Doxygen documentation to GitHub Pages from every push to `main`.

## Merge policy

**A branch lands as the commits it was developed as. Do not squash.**

That is the whole rule. How the branch gets there is up to whoever lands it: rebase onto the target and fast-forward when the branch is short and the replay is trivial, or make a merge commit when it is not — a release branch that has been open for a candidate cycle, a feature with real conflicts to resolve, or simply because replaying twenty commits one at a time is not worth anyone's afternoon. A merge commit is a normal outcome here, not a failure to keep the history tidy.

What the rule protects is that individual commits stay addressable. `git log` shows how the work actually happened rather than a wall of branch-sized commits, `git bisect` has real steps to walk, and a single bad commit can be reverted without taking the rest of its branch with it.
