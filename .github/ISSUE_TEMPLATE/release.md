---
name: Release
about: Tasks for publishing a final version of stc
title: 'Release v'
labels: 'release'
assignees: szgerii
---

Version: `X.Y.Z`

The detailed procedure lives in [VERSIONING.md](https://github.com/ShaderTranspiler/stc/blob/main/VERSIONING.md#publishing-a-release) and [BRANCHING.md](https://github.com/ShaderTranspiler/stc/blob/main/BRANCHING.md#the-release-cycle). This is the checklist, not the instructions.

## Prerequisites

- [ ] all work targeting the release is merged into `release/X.Y.Z`, and CI is green on it
- [ ] every issue in the milestone is closed, other than this one
- [ ] `julia_targets.toml` is current, and `julia_archive_digests.toml` has been regenerated if any archives changed upstream (`julia scripts/update_julia_archive_digests.jl --check`)
- [ ] the version block in `CMakeLists.txt` is at `X.Y.Z` with the suffix cleared

## Publish stc

- [ ] `release/X.Y.Z` merged into `main` through a PR with all required checks passing, keeping the branch for now
- [ ] `release.yml` succeeded, created the tag, and left the release as a **draft**
- [ ] library and CLI artifacts attached for every target

## Build and publish the JLL

NOTE: `$DEVDIR` lives at `~/.julia/dev` by default.

- [ ] `$DEVDIR/stc_jll/` deleted, then JLL and wrapper built from the release tag with `--deploy=local`
- [ ] renamed artifacts uploaded to the stc release
- [ ] wrapper finalised: `Artifacts.toml` URLs, `Project.toml` version, `README.md` first line
- [ ] committed as `stc_jll build X.Y.Z` and tagged locally
- [ ] stc's draft release published, with a description and a summarised changelog
- [ ] wrapper commit and tag pushed, and a Release created on `stc_jll.jl`

## Downstream

- [ ] `ShaderTranspiler.jl` updated, tested, tagged, released, and CI green on all platforms
- [ ] `ShaderSandbox.jl` updated, and its shaders verified against the new version

## Close out

- [ ] `main` merged back into `develop`
- [ ] milestone closed in `stc` and in `ShaderTranspiler.jl`
- [ ] next milestone created in `stc` (with a due date) and in `ShaderTranspiler.jl` (without one), identically named
- [ ] Release-specific Project views and charts filtered to the new milestone
- [ ] `release/X.Y.Z` deleted
