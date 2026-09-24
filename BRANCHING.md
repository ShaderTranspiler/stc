# Branching

The git branching model this repository uses, how it works together with versioning and releases, and what CI does on each branch.

## Overview

Two long-lived branches, one medium-lived one for releases and three kinds of short-lived ones. A branch keeps its own commits when it lands: merge commits are fine, squashing is not (see [Merge Policy](#merge-policy)). Tags serve as an indicator for releases and other events in the history.

```
feature/*  fix/*
      \       \
       \       \
develop ●───●───●───●───●────────●─────●───●─────●───────●
                   /     \      /         /     /         \
release/0.9.0     /       ●────●────●────●─────●      (next cycle)
                 /              \         \     \
main ───────────●────────────────●─────────●─────●───────
               /                rc.1     rc.2   v0.9.0
              /
          hotfix/*
```

## Long-lived Branches

### `main`

The released state. Nothing is developed here directly: `main` only ever advances by merging a `release/*` or `hotfix/*` branch, and it is protected so that it cannot advance any other way. The head always points to a valid release or release candidate.

Every merge into `main` publishes: the release workflow reads the version block off what arrives and creates the tag and the GitHub release from it, so reaching `main` is what makes a version real.

Because branches land with their commits intact, `main` holds the full history of everything merged into it rather than one commit per version. The tags, not the commits, are what mark the published versions.

### `develop`

The integration branch, and the default target for everyday work. `feature/*` and `fix/*` branches are opened from it and merged back into it. It holds work that is finished but not yet part of a version.

For the version number, `develop` holds the next planned version with the `-dev` suffix, updated when a release branch is opened from it.

After `main` advances (with a `hotfix/*` or release), `develop` is brought back up to it so that `develop` never diverges too far with new features that are incompatible with the released state.

## Release Branches

### `release/*`

Release preparation, named for the version being prepared: `release/x.y.z`.

Opened from `develop` when a version's scope is settled. From that point the branch carries only release work (version bumps, release-blocking fixes, documentation for the release). New development continues on `develop` in parallel and lands in the **next** version.

The branch stays open for the whole candidate cycle and is only deleted after the final release is merged into `main` (i.e. published).

It (or equivalently `main`) should be merged back into `develop` after every publish, whether it's a candidate or a final release. This is to ensure that release branches don't introduce any changes that are specific to that given version only, and are incompatible with features planned for the next release. When merging back release changes into `develop`, make sure the `-dev` suffix is not overwritten with `-rc.N`, though this should surface as a merge conflict.

Not merging back release work per commit, only per release is meant to reduce the frequency of having to integrate target version changes into next-release ones, while maintaining a consistent routine.

## Short-lived Branches

### `feature/*`

New functionality. Normally branches from `develop`, merges back into `develop` once it's done.

Name them after what they add: `feature/spirv-backend`, `feature/struct-lowering`.

**NOTE:** in very rare and justifiable situations, `release/*` branches may open a new feature branch for last-minute functionality that **CANNOT** wait for the next release. These feature branches are merged back into the release branch first, and then into `develop` when a release candidate is published.

### `fix/*`

Corrections to unreleased work, or a non-urgent fix for work introduced in a previous release. Branches from `develop`, merges back into `develop` once it's done.

`fix/*` branches are also the normal way to correct a release candidate. In that case they open from a given `release/x.y.z` branch and merge back into it as soon as the fix is done. The next candidate is published from the release branch afterwards.

### `hotfix/*`

An urgent correction to something already published. Branches from `main` at the latest release and merges back into `main` to publish the patch. A hotfix always carries a patch version bump, which CI enforces on the PRs.

**`main` is the distribution point for a hotfix.** The hotfix branch merges into `main` and nowhere else. `main` is then merged into `develop`, and into any release branch that is currently open. This ensures every line picks the fix up from one place instead of the same branch being merged into three, which keeps merge commits in the history clean.

This is the only path other than a release branch that may touch `main`.

## The Release Cycle

1. **Open `release/x.y.z` from `develop`**, once the version's scope is settled, and set the version block to `x.y.z` with the `rc.0` suffix in its first commit. The branch is not expected to be stable yet, it is a preview that can be tried out, not something to build on, and an `rc.0` build is meant to state exactly that. Bump `develop` to the next planned release version, with the `-dev` suffix.
2. **Land release work on it.** Release-finalisation commits, stability fixes and the rare last-minute features, each through a `fix/*` or `feature/*` branch opened from the release branch and merged back into it.
3. **Reach a candidate-ready state.** Feature-complete and stable: as far as the tests and developer testing can tell, this code would work if it were pulled into production as-is. From here consumers can expect nothing but fixes.
4. **Declare the candidate:** raise the version suffix from `rc.N` to `rc.N+1` (`rc.1` for the first candidate). See [VERSIONING.md](VERSIONING.md#choosing-the-release-version) for how the version itself is chosen.
5. **Merge into `main`**, and confirm the pre-release was published and the tag was created.
6. **Merge the release branch into `develop`.** Every candidate's fixes go back immediately, so `develop` never drifts far from what is being stabilised and the two cannot accumulate incompatible changes.
7. **Fix whatever the candidate surfaces**, using the release branch for integration, then repeat from step 4 with the next `rc.N`.
8. **Accept the candidate:** clear the suffix and merge into `main`.
9. **Confirm the pipeline ran** — the tag, the artifacts, and the documentation deployment.
10. **Publish the draft release**, with a description and a summarised changelog.
11. **Merge back into `develop`**, from `main` or equivalently from the release branch, which is at the same commit.
12. **Delete the release branch.** The tags and releases remain as the markers of what was published.

Steps 5 and 8 are guarded by the version-step rules in [VERSIONING.md](VERSIONING.md#choosing-the-release-version), which CI checks on every pull request into `main`. `develop` and release branches are guarded by a suffix-guard job in CI.

Publishing the JLL and bringing the downstream packages up to the new version happen after step 5 or step 10, outside the branch model and CI entirely (see [VERSIONING.md](VERSIONING.md#the-jll)).

## What CI Does

| workflow | trigger | branches |
|---|---|---|
| **CI** (build & test) | `push` | `main`, `develop`, `release/**` |
| **CI** (build & test) | `pull_request` | targeting `main`, `develop` or `release/**` |
| **Version Check** | `push` | `develop`, `release/**` |
| **Version Check** | `pull_request` | targeting `main` or `develop` |
| **Julia Digests** | `pull_request` | any target; checks only when the Julia manifest changed |
| **Release** | `push` | `main` only |
| **Docs** | `push` | `main` only |

CI, Release, Docs and Julia Digests also accept `workflow_dispatch`, so any of them can be run by hand.

Consequences worth knowing:

**Build and test runs on pushes to the long-lived branches only.** `feature/*`, `fix/*` and `hotfix/*` are verified by their pull request run instead, so that a branch with an open pull request is not built twice for every commit. The practical effect is that work on one of those branches is unverified until a pull request is opened against `main`, `develop` or a `release/**` branch, which is usually what you want while a branch is still under active development. One can use the `workflow_dispatch` trigger to run the testing workflow manually.

**A newer commit cancels the run it supersedes.** CI and Version Check cancel any run still in flight for the same ref, so pushing a fix on top of a broken commit does not leave the old run occupying a slot. Release and Docs deliberately do not cancel: they queue instead, because interrupting a publish half-way can leave a tag created with artifacts missing, and GitHub Pages permits only one deployment at a time (for docs).

**Anything reaching `main` publishes.** Pushing without a version change is harmless, since the release workflow skips silently when the composed version already has a tag, but it does mean the version block is the thing standing between a merge and a public release. The docs workflow likewise redeploys the Doxygen documentation to GitHub Pages from every push to `main`. However, these should not be able to be done unless bypassing the branch protection rulesets.

## Merge Policy

**A branch lands as the commits it was developed as, do not squash.**

That is the whole policy. The branch may be landed as a rebase or merge, or any way that doesn't squash the commit history.

The only thing the rule is meant to protect is that individual commits stay addressable. `git log` shows how the work actually happened rather than a list of branch-sized commits, `git bisect` has real steps to walk, and a single bad commit can be reverted without taking the rest of its branch with it.
