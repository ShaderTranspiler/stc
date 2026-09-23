# Versioning

How Shader Transpiler Core numbers its releases, what those numbers promise, and how publishing a release actually happens.

## Where the version info lives

Two lines in `CMakeLists.txt` are the single source of truth for STC's version. Nothing else in the repository stores a version number directly:

```cmake
project(ShaderTranspilerCore VERSION 0.9.0)
set(STC_VERSION_SUFFIX "")
```

Everything downstream derives from them: the generated compile-time metadata, the shared library's `SOVERSION`, the docs PDF's filename, the git tag, the GitHub release, and the JLL. To change the version of a build, change these lines and nothing else.

The two compose into `STC_VERSION`: an empty suffix leaves `0.9.0` alone, `rc.0` makes `0.9.0-rc.0`, `rc.1` makes `0.9.0-rc.1`, etc. The suffix is validated at configure time and rejects anything that is not empty or of the form `rc.N`.

## What the version numbers mean

Releases follow [semantic versioning](https://semver.org): `MAJOR.MINOR.PATCH`.

- **MAJOR** — incompatible changes in the C/C++ API or ABI.
- **MINOR** — functionality added in a backwards-compatible way.
- **PATCH** — backwards-compatible fixes.

**While the major version is 0, none of those guarantees are enforced.** Any 0.x release may change the API, the ABI, the CLI surface, or the generated GLSL in ways that break dependants. This is deliberate and is signalled in the ABI itself (see below). The guarantees begin at 1.0.0.

## Pre-release suffixes

The only permitted suffix form is `rc.N`. Alpha, beta or other prerelease stages are currently not used.

Release candidates are numbered from 1. A release candidate is a build that is believed to be the final release, published so that it can be tested against real consumers rather than as a preview of unfinished work. Feature work does not normally land during a candidate cycle.

`rc.0` is reserved and means something different: it marks a release branch that is *preparing* its first candidate. It exists so that a preview build can report the version it is heading for without claiming to be a candidate, and it is deliberately ordered below `rc.1`, so `0.8.5` < `0.9.0-rc.0` < `0.9.0-rc.1` < `0.9.0` all hold. **An `rc.0` build is never published:** it carries no tag and no release, it stays on the release branch, and a pull request carrying it into `main` is rejected by CI.

## ABI version

The C ABI carries its own version, returned by `stc_abi_version()`. It is not an independent number: it is the semantic major version, so today it returns `0`.

```c
#define EXPECTED_STC_ABI_VERSION 0

if (stc_abi_version() != EXPECTED_STC_ABI_VERSION)
    /* refuse to continue */
```

A value of `0` is an explicit statement that **no forwards or backwards compatibility is guaranteed**. Consumers acknowledge that they're responsible for keeping their usage of the library up-to-date when updating between 0.x versions. It will become `1` when 1.0 is released, and from then on a change to it means a genuine break.

The shared library's `SOVERSION` is set from the same number, so the linker enforces the same boundary.

## Julia compatibility is a separate axis

The stc version says nothing about which Julia versions a build works with. Those are tracked independently, because Julia's own ABI boundary is its *minor* series, `libjulia.so.1.12` is the SONAME for every 1.12 patch release.

A given stc release is built once per supported Julia series. Users on any patch within a supported series resolve the artifact for that series. The supported series are listed in the README and in the CI matrix; they may change between stc minor versions, but patch versions never break or change Julia compatibility.

## What publishing a release does

Pushing to `main` runs the release workflow, which reads the version block and branches on whether a suffix is present:

| suffix | GitHub release | tag | published automatically |
|---|---|---|---|
| `rc.N` | pre-release | `v0.9.0-rc.1` | yes |
| *(empty)* | full release | `v0.9.0` | no, only creates a draft |

**Final releases do not publish themselves.** They are created as drafts, on purpose. The artifacts are built and attached, but a human has to review and press publish. This is to ensure that a proper, manual description is added to each release before it's published. Release candidates skip that gate, because the point of a candidate is to be able to iterate quickly.

**The tag is created by the release, not before it.** The workflow does not push a tag as a separate step; the release action creates it when the release is made. This means a failed build cannot leave an orphan tag pointing at a commit that never produced artifacts, and a release whose tag already exists is skipped rather than re-cut. That skip is silent, which is why the version step into `main` is guarded — see [Choosing the number](#choosing-the-number).

## Build metadata

Every build records how it was produced, readable through `stc --version` and through `stc::meta` in C++:

| field | meaning |
|---|---|
| `version` | the composed `STC_VERSION`, suffix included |
| `version_major` / `_minor` / `_patch` | the numeric components |
| `version_suffix` | `rc.N`, or empty |
| `build_origin` | `official build` or `development build` |
| `build_type` | the CMake configuration |
| `compiler_id` / `compiler_ver` | the compiler used |
| `system_name` / `system_arch` | the target platform |
| `julia_ver` | the Julia version this build was compiled against |

`build_origin` is controlled by `-DSTC_OFFICIAL_BUILD=ON`, which only the release workflow and the JLL recipe pass. A build made locally reports `development build` even if its version number matches a real release, so a binary can always be traced back to whether it came from the project's own pipeline. This is a convenience indicator, not a validation or verification source. Anyone can pass `-DSTC_OFFICIAL_BUILD=ON` before building and have a binary whose `build_origin` will suggest it came from the pipeline.

## The version through a release

A release moves the version block through a fixed sequence of states. [BRANCHING.md](BRANCHING.md#the-release-cycle) covers which branch each step happens on, this section is about what the version itself is doing and what has to happen by hand once it is published.

### Choosing the number

The version describes what changed, not how many times the pipeline ran. A pass through develop, release branch, candidate and `main` does not mechanically increment anything. The number is chosen once, when the release branch is opened, and it is chosen from what the release is going to contain:

| the release contains | step | example |
|---|---|---|
| incompatible API or ABI changes | **major** | `1.2.3` -> `2.0.0` |
| new backwards-compatible functionality | **minor** | `1.2.3` -> `1.3.0` |
| only backwards-compatible fixes | **patch** | `1.2.3` -> `1.2.4` |

A hotfix is always a patch step, but patch steps are not exclusive to hotfixes. A planned release cut from `develop` that happens to contain only fixes is a patch release too.

Every merge into `main` is guarded on this. A release branch must be exactly one patch, minor or major step ahead of `main`, or at the same version with the candidate number moving forward, and it may not still be carrying `rc.0`. A hotfix must be exactly one patch step ahead and carry no suffix at all. Without those checks a forgotten bump fails silently, because the release workflow finds the tag already present and skips.

### The version block through the cycle

| stage | suffix | composed | reaching `main` |
|---|---|---|---|
| release branch opened | `rc.0` | `x.y.z-rc.0` | rejected by CI |
| first candidate declared | `rc.1` | `x.y.z-rc.1` | pre-release, published immediately |
| further candidates | `rc.2`, `rc.3`, … | `x.y.z-rc.2` | pre-release, published immediately |
| finalised | *(empty)* | `x.y.z` | full release, created as a draft |

### The JLL

The JLL is built separately and manually. This is a deliberate choice, since random things may break the `BinaryBuilder` pipeline. It also forces the publisher to manually update the official consumers (like `ShaderTranspiler.jl`), which should help surface any accidental API/ABI-breaking changes. A release candidate should not be deemed release-ready before `build_tarballs.jl` has successfully run with `--deploy=local`.

`BinaryBuilder` uses the `Pkg` devdir for the generated wrapper's location. This is set to `~/.julia/dev/` by default, and is referred to as `$DEVDIR` below.

**Building.** Start from a checkout of the release tag with a clean staging area and working directory, and with `BinaryBuilder` available in the Julia environment you're using. Delete `$DEVDIR/stc_jll/` first if it exists, otherwise `BinaryBuilder` will generate the wrapper additively over the previous version, which is **NOT** something we want.

`julia build_tarballs.jl --deploy=local` then builds both. For candidates, `--suffixed-build` is also required, which strips the prerelease component from the version info before forwarding it to `BinaryBuilder`, since it doesn't allow prerelease builds. This means the produced JLL artifacts lack prerelease information, which has to be added back later manually. The artifacts are placed in `./products/`, while the wrapper is generated at `$DEVDIR/stc_jll`.

**Uploading the artifacts.** The artifacts are renamed on the way out, from the `products/stc.v<stripped-version>.<triplet>.tar.gz` files `BinaryBuilder` produced into `products/stc_jll.vX.Y.Z[-rc.N].<triplet>.tar.gz`. Copying the files is recommended over a move, since that preserves the file structure `BinaryBuilder` generated the wrapper for, if any manual testing or debugging is required. For candidates, make sure the version suffix is included in the new names.

They are uploaded into the **stc** repo as release artifacts, for example via the GitHub CLI:

```sh
gh release upload vX.Y.Z[-rc.N] products/stc_jll.vX.Y.Z[-rc.N].*.tar.gz
```

Be careful that your glob doesn't catch the original files, logs or artifacts of previously built versions, the one above should be specific to the current build. If the assets are already attached from an earlier attempt, `--clobber` replaces them, which is only safe while no `stc_jll.jl` tag references the old ones.

**Finalising the wrapper.** Delete `src` in a local clone of `stc_jll.jl`, then copy the generated wrapper from `$DEVDIR/stc_jll/` into the repository, directly into the root. The resulting diff should be contained to `Artifacts.toml` changes, version info changes to `Project.toml` and `README.md`, and possibly some new or deleted files in `src`. Hash changes in `Artifacts.toml` are expected (`sha256` and `git-tree-sha1`), which is why it's important that the wrapper comes from the same build that produced the uploaded artifacts.

Three things then carry the version, and all three have to be corrected by hand:

- every `url` in `Artifacts.toml`, pointing at the artifacts uploaded above, with the target triplets and Julia versions matching properly
- `version` in `Project.toml`, as `X.Y.Z` for final releases and `X.Y.Z-rc.N` for candidates, with the build number (`+0`) stripped
- the version in the first line of `README.md`, the same way

Commit that as `stc_jll build X.Y.Z[-rc.N]` and tag it `vX.Y.Z[-rc.N]` locally. For a final release, publish stc's draft release before pushing, so that its artifacts are accessible. Then push the commit and the tag, and create a Release on the `stc_jll.jl` repo for the tag.

### Downstream

The consumers carry their own versions, and updating them is part of the release rather than follow-up work.

**`ShaderTranspiler.jl`** updates the `[compat]` entry for `stc_jll` in `Project.toml` and its `rev` in `[sources]` to the new version's tag, and moves its own version to match the underlying `stc` version. Run `update` on the package environment so that the changes are pulled in, run the Julia tests locally and make sure they pass, then commit, tag, push, and mark the tag as a release on GitHub. Make sure CI passes on all platforms.

**`ShaderSandbox.jl`** updates the versions of `stc_jll` and `ShaderTranspiler.jl` the same way, then runs `update` on the project environment. Update the example shaders if needed, verify the new `stc` version is running by passing `--info` to the app, and verify that all shaders transpile properly with it.

See [BRANCHING.md](BRANCHING.md) for more detail on where each of those steps happens.
