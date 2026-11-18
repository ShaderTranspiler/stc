#!/usr/bin/env python3
"""
Emits the Julia version build matrix specified by julia_targets.toml for CI.
It's a simple utility script meant to serve the data to bash in runners.
Output is JSON for pinned/floating/release and a simple version number for min.

Usage: python3 scripts/julia_build_matrix.py <mode>
where <mode> is one of:
    pinned:           the exact matches the matrix builds, based on the series' 'test' list
    floating:         the minor channels to monitor for upstream changes
    min:              the lowest supported series (as major.minor) to verify the README.md requirements section
    release:          the build specifications for release builds (containing both the exact build target and the series version)
    gl-utils-url:     the URL of the actively supported GLUtils repository
    gl-utils-rev:     the Git identifier to check out to get the currently supported GLUtils version
    gl-utils-version: the Pkg version that the current build supports
"""

import argparse
import json
import sys
import tomllib
from pathlib import Path
from typing import Any

TARGETS_TOML = Path(__file__).resolve().parent.parent / "julia_targets.toml"

def load_toml() -> dict[str, Any]:
    try:
        with TARGETS_TOML.open("rb") as file:
            return tomllib.load(file)
    except OSError as err:
        sys.exit(f"couldn't open Julia version information (expected path: {TARGETS_TOML}):\n{err}")
    except tomllib.TOMLDecodeError as err:
        sys.exit(f"invalid TOML in file {TARGETS_TOML}:\n{err}")


def load_series(toml: dict[str, Any]):
    series = toml.get("series")
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

class GLUtilsProps:
    def __init__(self, gl_utils_entry: dict[str, Any]):
        props = ["url", "rev", "version"]
        for prop in props:
            if prop not in gl_utils_entry:
                sys.exit(f"[[gl_utils]] entry is missing required property '{prop}' in {TARGETS_TOML}")
            if not isinstance(gl_utils_entry[prop], str):
                sys.exit(f"[[gl_utils]] entry property '{prop}' has invalid type: {type(gl_utils_entry[prop])}, expected: str")
        
        self.url: str     = gl_utils_entry["url"]
        self.rev: str     = gl_utils_entry["rev"]
        self.version: str = gl_utils_entry["version"]

def load_gl_utils(toml: dict[str, Any]) -> GLUtilsProps:
    gl_utils = toml.get("gl_utils")
    if not gl_utils:
        sys.exit(f"no [[gl_utils]] entry found in {TARGETS_TOML}")
        
    if len(gl_utils) > 1:
        sys.exit(f"more than one [[gl_utils]] entry found in {TARGETS_TOML}")
        
    return GLUtilsProps(gl_utils[0])

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("pinned", "floating", "min", "release",
                                         "gl-utils-url", "gl-utils-rev", "gl-utils-version"))
    args = parser.parse_args()

    toml = load_toml()
    
    is_gl_utils_query = args.mode.startswith("gl-utils")
    
    if not is_gl_utils_query:
        series = load_series(toml)
        match args.mode:
            case "min":
                print(min((series_of(entry["tag"]) for entry in series), key=sort_key))
                return 0
            case "release":
                entries = sorted(
                    ({"build": entry["build"], "series": series_of(entry["tag"])}
                     for entry in series),
                     key=lambda entry: sort_key(entry["build"])
                )
                print(json.dumps(entries))
                return 0
            case "pinned":
                versions = sorted({ver for entry in series for ver in entry["test"]}, key=sort_key)
            case "floating":
                versions = sorted({series_of(entry["tag"]) for entry in series}, key=sort_key)
            case _:
                raise ValueError("unexpected mode not caught by argparse")

        print(json.dumps(versions))
        return 0
    
    assert is_gl_utils_query
    
    gl_utils_props: GLUtilsProps = load_gl_utils(toml)

    match args.mode:
        case "gl-utils-url":
            print(gl_utils_props.url)
        case "gl-utils-rev":
            print(gl_utils_props.rev)
        case "gl-utils-version":
            print(gl_utils_props.version) # TODO: verify against this in ShaderTranspiler
        case _:
            raise ValueError("unexpected mode not caught by argparse")

    return 0

if __name__ == "__main__":
    sys.exit(main())
