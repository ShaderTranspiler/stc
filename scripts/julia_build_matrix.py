#!/usr/bin/env python3
"""
Emits the Julia version build matrix specified by julia_targets.toml for CI.
It's a simple utility script meant to serve the data to bash in runners.
Output is JSON for pinned/floating/release and a simple version number for min.

Usage: python3 scripts/julia_build_matrix.py <version_mode>
where <version_mode> is one of:
    pinned:   the exact matches the matrix builds, based on the series' 'test' list
    floating: the minor channels to monitor for upstream changes
    min:      the lowest supported series (as major.minor) to verify the README.md requirements section
    release:  the build specifications for release builds (containing both the exact build target and the series version)
"""

import argparse
import json
import sys
import tomllib
from pathlib import Path

TARGETS_TOML = Path(__file__).resolve().parent.parent / "julia_targets.toml"

def load_series():
    try:
        with TARGETS_TOML.open("rb") as file:
            parsed_versions = tomllib.load(file)
    except OSError as err:
        sys.exit(f"couldn't open Julia version information (expected path: {TARGETS_TOML}):\n{err}")
    except tomllib.TOMLDecodeError as err:
        sys.exit(f"invalid TOML in file {TARGETS_TOML}:\n{err}")

    series = parsed_versions.get("series")
    if not series:
        sys.exit(f"no [[series]] entries found in {TARGETS_TOML}")

    for i, entry in enumerate(series, start=1):
        for key in ("tag", "build", "test"):
            if key not in entry:
                sys.exit(f"series #{i} has no '{key}' field in {TARGETS_TOML}")
    
    return series

def series_of(ver):
    major, minor, *_ = ver.split(".")
    return f"{major}.{minor}"

def sort_key(ver):
    return tuple(int(part) for part in ver.split("."))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("pinned", "floating", "min", "release"))
    args = parser.parse_args()

    series = load_series()

    if args.mode == "min":
        print(min((series_of(entry["tag"]) for entry in series), key=sort_key))
        return 0

    if args.mode == "pinned":
        versions = sorted({ver for entry in series for ver in entry["test"]}, key=sort_key)
    elif args.mode == "floating":
        versions = sorted({series_of(entry["tag"]) for entry in series}, key=sort_key)
    else: # release
        entries = sorted(
            ({"build": entry["build"], "series": series_of(entry["tag"])}
             for entry in series),
             key=lambda entry: sort_key(entry["build"])
        )
        print(json.dumps(entries))
        return 0

    print(json.dumps(versions))
    return 0

if __name__ == "__main__":
    sys.exit(main())
