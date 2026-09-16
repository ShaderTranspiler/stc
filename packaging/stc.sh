#!/bin/sh

# This script is shipped as a release artifact

# Runs the stc CLI with the proper Julia libraries reachable for loading
# For v1.0 these wrapper scripts are planned to be replaced by a binary launcher wrapper instead
# That would result on a single executable instead of wrapper scripts, but requires a bit of work

# define STC_JULIA_LIBDIR in the environment to avoid a dynamic lookup through julia executable each launch

set -e

cur_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if [ -n "$STC_JULIA_LIBDIR" ]; then
    julia_libdir=$STC_JULIA_LIBDIR
else
    if ! command -v julia > /dev/null 2>&1; then
        echo "stc: 'julia' is not reachable from PATH. Install Julia, or set STC_JULIA_LIBDIR to the directory containing libjulia manually." >&2
        exit 1
    fi
    julia_libdir=$(julia -e 'print(normpath(joinpath(Sys.BINDIR, Base.LIBDIR)))')
fi

LD_LIBRARY_PATH="$julia_libdir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LD_LIBRARY_PATH

exec "$cur_dir/stc" "$@"
