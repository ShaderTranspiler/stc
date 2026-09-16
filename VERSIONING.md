# Versioning

How Shader Transpiler Core numbers its releases, what those numbers promise, and how publishing a release actually happens.

## Where the version info lives

Two lines in `CMakeLists.txt` are the single source of truth for STC's version. Nothing else in the repository stores a version number directly:

```cmake
project(ShaderTranspilerCore VERSION 0.9.0)
set(STC_VERSION_SUFFIX "")
```

Everything downstream derives from them — the generated compile-time metadata, the shared library's `SOVERSION`, the docs PDF's filename, the git tag, the GitHub release, and the JLL. To change the version of a build, change these lines and nothing else.

The two are composed into `STC_VERSION` in CMake:

| `PROJECT_VERSION` | `STC_VERSION_SUFFIX` | `STC_VERSION` |
|---|---|---|
| `0.9.0` | *(empty)* | `0.9.0` |
| `0.9.0` | `rc.0` | `0.9.0-rc.0` |
| `0.9.0` | `rc.1` | `0.9.0-rc.1` |

The suffix is validated at configure time and rejects anything that is not empty or of the form `rc.N`.

## What the version numbers mean

Releases follow [semantic versioning](https://semver.org): `MAJOR.MINOR.PATCH`.

- **MAJOR** — incompatible changes in the C/C++ API or ABI.
- **MINOR** — functionality added in a backwards-compatible way.
- **PATCH** — backwards-compatible fixes.

**While the major version is 0, none of those guarantees are enforced.** Any 0.x release may change the API, the ABI, the CLI surface, or the generated GLSL in ways that break dependants. This is deliberate and is signalled in the ABI itself (see below). The guarantees begin at 1.0.0.

## Pre-release suffixes

The only permitted suffix form is `rc.N`. Alpha, beta or other prerelease stages are currently not used.

Release candidates are numbered from 1. A release candidate is a build that is believed to be the final release, published so that it can be tested against real consumers rather than as a preview of unfinished work. Feature work does not normally land during a candidate cycle.

`rc.0` is reserved and means something different: it marks a release branch that is *preparing* its first candidate. It exists so that a preview build can report the version it is heading for without claiming to be a candidate, and it is deliberately ordered below `rc.1`, so `0.8.5` < `0.9.0-rc.0` < `0.9.0-rc.1` < `0.9.0` all hold.

**An `rc.0` build is never published.** It carries no tag and no release, it stays on the release branch, and a pull request carrying it into `main` is rejected by CI. It is a stopgap: a suffix stage that is honestly named, rather than a candidate number pressed into service, is planned for 1.0.

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

The stc version says nothing about which Julia versions a build works with. Those are tracked independently, because Julia's own ABI boundary is its *minor* series — `libjulia.so.1.12` is the SONAME for every 1.12 patch release.

A given stc release is built once per supported Julia series. Users on any patch within a supported series resolve the artifact for that series. The supported series are listed in the README and in the CI matrix; they may change between stc minor versions, but patch versions never break or change Julia compatibility.

## What publishing a release does

Pushing to `main` runs the release workflow, which reads the version block and branches on whether a suffix is present:

| suffix | GitHub release | tag | published automatically |
|---|---|---|---|
| `rc.N` | pre-release | `v0.9.0-rc.1` | yes |
| *(empty)* | full release | `v0.9.0` | **no — created as a draft** |

Two things follow from this that are easy to miss.

**Final releases do not publish themselves.** They are created as drafts, on purpose. The artifacts are built and attached, but a human has to review and press publish. This is to ensure that a proper, manual description is added to each release before it's published. Release candidates skip that gate, because the point of a candidate is to reach consumers quickly.

**The tag is created by the release, not before it.** The workflow does not push a tag as a separate step; the release action creates it when the release is made. This means a failed build cannot leave an orphan tag pointing at a commit that never produced artifacts. A release whose tag already exists
is skipped rather than re-cut.

Because the tag is derived from the composed version, a release candidate and its eventual final release occupy different tags (`v0.9.0-rc.1` and `v0.9.0`), and cutting the final does not conflict with the candidates that preceded it.

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

`build_origin` is controlled by `-DSTC_OFFICIAL_BUILD=ON`, which only the release workflow and the JLL recipe pass. A build made locally reports `development build` even if its version number matches a real release, so a binary can always be traced back to whether it came from the project's own
pipeline. This is a convenience indicator, not a validation or verification source. Anyone can pass `-DSTC_OFFICIAL_BUILD=ON` before building and have a binary whose `build_origin` will suggest it came from the pipeline.

## The version through a release

A release moves the version block through a fixed sequence of states. [BRANCHING.md](BRANCHING.md) covers which branch each step happens on; this section is about what the version itself is doing.

### Choosing the number

The version describes what changed, not how many times the pipeline ran. A pass through develop, release branch, candidate and `main` does not mechanically increment anything. The number is chosen once, when the release branch is opened, and it is chosen from what the release is going to contain:

| the release contains | step | example |
|---|---|---|
| incompatible API or ABI changes | **major** | `1.2.3` -> `2.0.0` |
| new backwards-compatible functionality | **minor** | `1.2.3` -> `1.3.0` |
| only backwards-compatible fixes | **patch** | `1.2.3` -> `1.2.4` |

A major or minor step resets the components below it to zero.

A hotfix is always a patch step, because a hotfix is by definition a backwards-compatible fix against something already published. Patch steps are not exclusive to hotfixes, though: a planned release cut from `develop` that happens to contain only fixes is a patch release too.

The reason hotfixes matter here is on the consuming side rather than the defining side. A consumer who sees `0.9.0` become `0.9.1` knows the update is free, because no breaking changes of any kind is exactly what a patch step promises, so an important fix can be taken without auditing what else came with it. Without the bump there is no way to signal that at all.

### The release flow

1. **Preview.** When the release branch is opened, set `project(... VERSION x.y.z)` to the version being worked towards and `set(STC_VERSION_SUFFIX "rc.0")`, as one commit. The composed version becomes `x.y.z-rc.0`, and stays there for the whole preview stage.
2. **Declare the first candidate.** `set(STC_VERSION_SUFFIX "rc.1")`, once the branch is feature-complete and believed stable.
3. **Publish it.** Reaching `main` publishes `vx.y.z-rc.1` as a pre-release, immediately and without a manual gate.
4. **Iterate.** Each further candidate raises the number: `rc.2`, `rc.3`. The version itself does not move. Every candidate of a release shares `x.y.z` and differs only in the suffix.
5. **Finalise.** Clear the suffix back to `""`. The composed version becomes `x.y.z`, and reaching `main` creates the draft release.
6. **Publish.** Review the draft, write the description and changelog, press publish.

The merges into `main`, at step 3 and inside step 5, are guarded. A release branch must be exactly one patch, minor or major step ahead of `main`, or at the same version with the candidate number moving forward, and it may not still be carrying `rc.0`. A hotfix must be exactly one patch step ahead and carry no suffix at all. Without those checks a forgotten bump fails silently, because the release workflow finds the tag already present and skips.

### What a release branch reports before its first candidate

`x.y.z-rc.0`, from the moment the branch is opened. That understates nothing and overstates nothing: it names the version being worked towards, and the `rc.0` says plainly that no candidate has been declared yet.

### The JLL

The JLL is built separately and manually. This is a deliberate choice, since random things may break the `BinaryBuilder` pipeline. It also forces the publisher to manually update the official consumers (like `ShaderTranspiler.jl`), which should help surface any accidental API/ABI-breaking changes. `build_tarballs.jl` refuses to build a suffixed version unless `--allow-suffixed-build` is passed explicitly, so a release candidate cannot be published to the Julia package ecosystem by accident. A release candidate should not be deemed release-ready before `build_tarballs.jl` successfully ran with `--deploy=local`.

See [BRANCHING.md](BRANCHING.md) for more detail on where each of those steps happens.
