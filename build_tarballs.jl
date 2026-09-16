"""
Provides the recipe for BinaryBuilder to build and deploy stc_jll.

Assembles its targets based on julia_targets.toml, and uses the official Julia archives to download the targeted Julia versions.
The downloaded installations are verified against julia_archive_digests.toml
"""

# CLEANUP: organize stuff into helpers

using BinaryBuilder
using Pkg
using LibGit2
using TOML

const REPO_ROOT = @__DIR__

# files and directories copied into the sandbox for building
const STAGED_FILES::Vector{String} = String["include", "src", "cli", "cmake", "packaging", "CMakeLists.txt", "LICENSE"]

const IS_PUBLISHING::Bool = any(arg -> startswith(arg, "--deploy") && arg != "--deploy=local", ARGS)
const SUFFIXED_BUILD_ARG::String = "--suffixed-build" # strips prerelease info from version (BinaryBuilder doesn't support it)
const ALLOW_NO_COMMIT_HASH_ARG::String = "--allow-no-commit-hash"
const ALLOW_DIRTY_TREE_ARG::String = "--allow-dirty-tree"

arg_active(arg)::Bool = !IS_PUBLISHING && arg in ARGS

# supported Julia builds
const TARGETS_TOML = joinpath(REPO_ROOT, "julia_targets.toml")
const DIGESTS_TOML = joinpath(REPO_ROOT, "julia_archive_digests.toml")
const TARGETS_TOML_PARSED = TOML.parsefile(TARGETS_TOML)
const JULIA_SERIES = TARGETS_TOML_PARSED["series"]
const JULIA_ARCHIVES = TARGETS_TOML_PARSED["archive"]
const JULIA_DIGESTS = TOML.parsefile(DIGESTS_TOML)

function get_archive_url(release, flavour, url_dir)::String
    ver = VersionNumber(release)
    return "https://julialang-s3.julialang.org/bin/$url_dir/$(ver.major).$(ver.minor)/julia-$release-$flavour.tar.gz"
end

function get_julia_archive_sources()
    sources = ArchiveSource[]

    for series in JULIA_SERIES
        release = series["build"]
        release_digests = get(JULIA_DIGESTS, release, nothing)

        if release_digests === nothing
            error("$DIGESTS_TOML: no checksums found for Julia $release\n" *
                "Ensure that the archive digests are up to date with 'julia scripts/update_julia_archive_digests.jl --check'")
        end

        for archive in JULIA_ARCHIVES
            flavour = archive["flavour"]

            if !haskey(release_digests, flavour)
                error("$DIGESTS_TOML: no $flavour checksum for Julia $release")
            end

            push!(sources, ArchiveSource(
                get_archive_url(release, flavour, archive["url_dir"]),
                release_digests[flavour];
                unpack_target="julia-$release-$flavour"
            ))
        end
    end

    return sources
end

# a bit hacky, but keeps the CMake file as the single source of truth for versioning
cmake_content = read("CMakeLists.txt", String)

version_match = match(r"project\([^ ]+ VERSION (\d+\.\d+\.\d+)\)", cmake_content)
if isnothing(version_match)
    error("could not strip VERSION from CMakeLists.txt file")
end

# only unsuffixed versions are allowed to be published (unless an override is provided, useful for local deployment)
suffix_match = match(r"set\(STC_VERSION_SUFFIX \"([^\"]*)\"\)", cmake_content)
if isnothing(suffix_match)
    error("could not strip STC_VERSION_SUFFIX from CMakeLists.txt file")
end

has_suffix = !isempty(suffix_match.captures) && !isempty(suffix_match[1])

version = VersionNumber(has_suffix ? "$(version_match[1])-$(suffix_match[1])" : version_match[1])

if version.prerelease != () || version.build != ()
    if !arg_active(SUFFIXED_BUILD_ARG)
        error("cannot publish builds with STC_VERSION_SUFFIX set to a non-empty value, unless $SUFFIXED_BUILD_ARG is provided explicitly for a non-publishing run." *
            "This will strip prerelease and build data from the version (which will have to be handled manually during deployment)")
    end

    # strips prerelease and build info
    version = VersionNumber(version.major, version.minor, version.patch)
end

# try to grab git hash for versioning on the host itself
commit_hash = try
    repo = LibGit2.GitRepo(@__DIR__)

    if LibGit2.isdirty(repo) && !arg_active(ALLOW_DIRTY_TREE_ARG)
        @warn "cannot use repository with active modifications for commit hash retrieval, unless $ALLOW_DIRTY_TREE_ARG is provided for a non-publishing run"
        nothing
    else
        oid = LibGit2.head_oid(repo)
        string(oid)[1:7]
    end
catch
    @warn "couldn't open git repo to obtain the latest commit hash" repo_dir = @__DIR__
    nothing
end

if commit_hash === nothing
    if arg_active(ALLOW_NO_COMMIT_HASH_ARG)
        commit_hash = "unknown"
    else
        error("cannot produce JLL without a commit hash, unless $ALLOW_NO_COMMIT_HASH_ARG is provided for a non-publishing run.")
    end
end

build_timestamp = Libc.strftime("%Y-%m-%d %H:%M:%S %z", time())

prod_dir = joinpath(@__DIR__, "products")
if isdir(prod_dir)
    for item in readdir(prod_dir; join=true)
        rm(item, recursive=true, force=true)
    end
end

# create staging dir, so that local builds are possible without having to delete build and other unnecessary local folders
staging_dir = mktempdir()
for item in STAGED_FILES
    item_path = joinpath(@__DIR__, item)

    # disallow symlinks explicitly (for now)
    if !isfile(item_path) && !isdir(item_path)
        error("couldn't locate file or directory required for building at '$item_path'")
    end

    cp(item_path, joinpath(staging_dir, item))
end

@info "build info" version commit_hash build_timestamp staging_dir

julia_release_cases = join([
    "*julia_version+$(VersionNumber(series["tag"]).major).$(VersionNumber(series["tag"]).minor).*)" *
        "JULIA_RELEASE=$(series["build"]) ;;"
    for series in JULIA_SERIES
], "\n")

julia_flavour_cases = join([
    "$(archive["target"]))" *
        "JULIA_FLAVOUR=$(archive["flavour"]) ;;"
    for archive in JULIA_ARCHIVES
], "\n")

script = """
cd \${WORKSPACE}/srcdir
mkdir -p build && cd build

case "\${bb_full_target}" in
    $julia_release_cases
    *) echo "no Julia release mapped for \${bb_full_target}"; exit 1 ;;
esac

case "\${target}" in
    $julia_flavour_cases
    *) echo "no Julia distribution mapped for \${target}"; exit 1 ;;
esac

JULIA_ROOT=\${WORKSPACE}/srcdir/julia-\${JULIA_RELEASE}-\${JULIA_FLAVOUR}/julia-\${JULIA_RELEASE}

JV_MAJOR=\$(grep \"#define JULIA_VERSION_MAJOR\" \"\${JULIA_ROOT}/include/julia/julia_version.h\" | awk '{print \$3}')
JV_MINOR=\$(grep \"#define JULIA_VERSION_MINOR\" \"\${JULIA_ROOT}/include/julia/julia_version.h\" | awk '{print \$3}')
JV_PATCH=\$(grep \"#define JULIA_VERSION_PATCH\" \"\${JULIA_ROOT}/include/julia/julia_version.h\" | awk '{print \$3}')
HEADER_JULIA_VER="\$JV_MAJOR.\$JV_MINOR.\$JV_PATCH"

if [ "\$HEADER_JULIA_VER" != "\$JULIA_RELEASE" ]; then
    echo "Julia version mismatch: expected \$JULIA_RELEASE, but headers say \$HEADER_JULIA_VER"
    exit 1
fi

cmake .. -DCMAKE_INSTALL_PREFIX=\${prefix} \\
         -DCMAKE_TOOLCHAIN_FILE=\${CMAKE_TARGET_TOOLCHAIN} \\
         -DCMAKE_BUILD_TYPE=Release \\
         -DJULIA_INCLUDE_DIR=\${JULIA_ROOT}/include/julia \\
         -DJULIA_LIB_DIR=\${JULIA_ROOT}/lib \\
         -DJULIA_BIN_DIR=\${JULIA_ROOT}/bin \\
         -DSTC_JULIA_VERSION=\${JULIA_RELEASE} \\
         -DSTC_COMMIT_HASH=\"$commit_hash\" \\
         -DSTC_BUILD_TIMESTAMP=\"$build_timestamp\" \\
         -DSTC_BUILD_TESTS=OFF \\
         -DSTC_USE_FORMAT=OFF \\
         -DSTC_OFFICIAL_BUILD=ON

make -j\${nproc}
make install

install_license \${WORKSPACE}/srcdir/LICENSE
"""

base_platforms = [parse(Platform, archive["target"]) for archive in JULIA_ARCHIVES]

sources = [
    DirectorySource(staging_dir),
    get_julia_archive_sources()...
]

platforms = Platform[]
for series in JULIA_SERIES, p in base_platforms
    p_copy = deepcopy(p)
    p_copy["julia_version"] = series["tag"]
    push!(platforms, p_copy)
end
platforms = expand_cxxstring_abis(platforms)

@info "build matrix" platforms

dependencies = [
    Dependency("Fmt_jll")
]

products = [
    LibraryProduct("libstc", :libstc; dont_dlopen=true),
    ExecutableProduct("stc", :stc)
]

min_series = minimum(VersionNumber(series["tag"]) for series in JULIA_SERIES)

name = "stc"

# BinaryBuilder requires stripping of custom args
stripped_args = String[]
for arg in ARGS
    if !(arg in (SUFFIXED_BUILD_ARG, ALLOW_NO_COMMIT_HASH_ARG, ALLOW_DIRTY_TREE_ARG))
        push!(stripped_args, arg)
    end
end

build_tarballs(
    stripped_args, name, version, sources, script, platforms, products, dependencies;
    julia_compat="$(min_series.major).$(min_series.minor)",
    preferred_gcc_version=v"12"
)
