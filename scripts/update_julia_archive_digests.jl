#!/usr/bin/env julia

"""
Regenerates julia_archive_digests.toml, which specifies the expected
checksums of Julia archives used for distribution by build_tarballs.jl

Input: the versions and platforms specified in julia_targets.toml

Pinning checksums manually ensures that upstream archive changes are
automatically detected (and rejected during build). This allows these
changes to be manually verified before CI and builds pull in broken
archives. It also keeps recipes reproducible.

Usage:
# regenerates julia_archive_digests.toml from upstream
julia scripts/update_julia_archive_digests.jl

# verifies julia_archive_digests.toml against upstream (updates nothing)
julia scripts/update_julia_archive_digests.jl --check
"""

using Downloads
using TOML

const CHECK_ONLY = "--check" in ARGS

const PROJECT_ROOT = normpath(joinpath(@__DIR__, ".."))
const TARGETS_TOML = joinpath(PROJECT_ROOT, "julia_targets.toml")
const DIGESTS_TOML = joinpath(PROJECT_ROOT, "julia_archive_digests.toml")

const SHA256_PATTERN = r"^[0-9a-f]{64}$"

# downloads the checksums directly from the official Julia archives
checksum_url(ver)::String = "https://julialang-s3.julialang.org/bin/checksums/julia-$ver.sha256"

function parse_targets()
    if !isfile(TARGETS_TOML)
        error("could not find target file (expected path: $TARGETS_TOML)")
    end

    try
        return TOML.parsefile(TARGETS_TOML)
    catch err
        error("could not parse targets from file ($TARGETS_TOML):\n$err")
    end
end

function get_flavours(targets)
    archives = get(targets, "archive", nothing)

    if !isa(archives, Vector) || isempty(archives)
        error("[[archive]] entries not found in $TARGETS_TOML")
    end

    flavours = String[]
    for (i, archive) in enumerate(archives)
        if !haskey(archive, "flavour")
            error("archive entry #$i has no 'flavour' field")
        end

        push!(flavours, archive["flavour"])
    end

    return flavours
end

function get_versions(targets)
    series = get(targets, "series", nothing)

    if !isa(series, Vector) || isempty(series)
        error("[[series]] entries not found in $TARGETS_TOML")
    end

    versions = String[]
    for (i, entry) in enumerate(series)
        if !haskey(entry, "build")
            error("series entry #$i has no 'build' field")
        end

        push!(versions, entry["build"])
    end

    # sort for deterministic output (no diff on identical regenerations)
    return sort(unique(versions); by=VersionNumber)
end

function fetch_checksums(ver)
    url = checksum_url(ver)

    @info "fetching checksums" julia_version = ver url

    body = try
        sprint(io -> Downloads.download(url, io))
    catch err
        error("couldn't download checksums for Julia v$ver from '$url':\n$err")
    end

    sums = Dict{String,String}()
    for line in eachline(IOBuffer(body))
        fields = split(strip(line))

        # skips unrecognized line formats
        # missing checksums will be caught later anyways
        length(fields) == 2 || continue

        sums[fields[2]] = fields[1]
    end

    if isempty(sums)
        error("the checksum file for Julia v$ver parsed to no entries (at $url)")
    end

    return sums
end

function archive_digests(ver, flavours)
    sums = fetch_checksums(ver)
    digests = Dict{String,String}()

    for flavour in flavours
        file_name = "julia-$ver-$flavour.tar.gz"

        digest = get(sums, file_name, nothing)
        if digest === nothing
            error("the checksum file for Julia v$ver doesn't have an entry for $file_name\n" *
                "available entries:\n" * join(sort(collect(keys(sums))), ", "))
        end

        if !occursin(SHA256_PATTERN, digest)
            error("the checksum available for $file_name is not a valid SHA256 digest:\n$digest")
        end

        digests[flavour] = digest
    end

    return digests
end

# file contents are pre-generated so that interrupted execution cannot produce a half-written output file
function render_file_contents(vers_to_digests, flavours)::String
    io = IOBuffer()

    println(io, "# GENERATED FILE, DO NOT EDIT BY HAND")
    println(io, "# Regenerate with: julia scripts/update_julia_archive_digests.jl")
    println(io, "#")
    println(io, "# SHA256 sums of the official Julia binaries, retrieved from the official archives at")
    println(io, "# $(checksum_url("<version>"))")

    for ver in sort(collect(keys(vers_to_digests)); by=VersionNumber)
        println(io)
        println(io, "[\"", ver, "\"]")
        for flavour in flavours
            println(io, "$flavour = \"$(vers_to_digests[ver][flavour])\"")
        end
    end

    return String(take!(io))
end

function main()
    if CHECK_ONLY && !isfile(DIGESTS_TOML)
        @error "no $(basename(DIGESTS_TOML)) found to check against\nrun: julia scripts/update_julia_archive_digests.jl"
        return 1
    end

    targets = parse_targets()
    flavours = get_flavours(targets)
    vers = get_versions(targets)

    @info "collecting checksums" julia_versions = vers flavours

    rendered = render_file_contents(Dict(ver => archive_digests(ver, flavours) for ver in vers), flavours)

    # since output is deterministic, we can compare the contents directly
    # this is a bit hacky, but avoids parsing another TOML file
    if CHECK_ONLY
        if read(DIGESTS_TOML, String) != rendered
            @error "$(basename(DIGESTS_TOML)) is out of date\nrun: julia scripts/update_julia_archive_digests.jl"
            return 1
        end

        @info "$(basename(DIGESTS_TOML)) is up to date"
        return 0
    end

    # writes through a temp file so interrupted runs don't produce truncated output at DIGESTS_TOML
    tmp = tempname(PROJECT_ROOT; suffix=".toml.tmp")
    @info "writing output through a temporary file" path = tmp
    write(tmp, rendered)
    mv(tmp, DIGESTS_TOML; force=true)

    @info "$(basename(DIGESTS_TOML)) updated successfully" digest_count = length(vers) * length(flavours)
    return 0
end

exit(main())
