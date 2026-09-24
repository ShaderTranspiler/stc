---
name: Release candidate
about: Tasks for publishing a release candidate of stc
title: 'Release candidate v'
labels: 'release'
assignees: szgerii
---

Version: `X.Y.Z-rc.N`

The detailed procedure lives in [VERSIONING.md](https://github.com/ShaderTranspiler/stc/blob/main/VERSIONING.md#publishing-a-release) and [BRANCHING.md](https://github.com/ShaderTranspiler/stc/blob/main/BRANCHING.md#the-release-cycle). This is the checklist, not the instructions.

## Prerequisites

- [ ] `release/X.Y.Z` is candidate-ready: feature-complete and stable as far as the tests and developer testing can tell
- [ ] CI is green on `release/X.Y.Z`
- [ ] `julia_targets.toml` is current, and `julia_archive_digests.toml` has been regenerated if any archives changed upstream (`julia scripts/update_julia_archive_digests.jl --check`)
- [ ] the suffix in `VERSION.txt` is raised to `rc.N` (`rc.0` -> `rc.1` for the first candidate)

## Publish stc

- [ ] `release/X.Y.Z` merged into `main` through a PR with all required checks passing
- [ ] `release.yml` succeeded, created the tag, and published the **pre-release**
- [ ] library and CLI artifacts attached for every target

## Build and publish the JLL

NOTE: `$DEVDIR` lives at `~/.julia/dev` by default.

- [ ] `$DEVDIR/stc_jll/` deleted, then JLL and wrapper built from the release tag with `--deploy=local --suffixed-build`
- [ ] renamed artifacts uploaded to the stc release, with `-rc.N` in the file names
- [ ] wrapper finalised: `Artifacts.toml` URLs, `Project.toml` version, `README.md` first line, all carrying `-rc.N`
- [ ] committed as `stc_jll build X.Y.Z-rc.N`, tagged, pushed, and a Release created on `stc_jll.jl`

## Downstream

- [ ] `ShaderTranspiler.jl` updated, tested, tagged, released, and CI green on all platforms
- [ ] `ShaderSandbox.jl` updated, and its shaders verified against the new candidate

## Close out

- [ ] `release/X.Y.Z` merged back into `develop`, making sure the `-dev` suffix remains intact

`release/X.Y.Z` and the version milestone stay open!
