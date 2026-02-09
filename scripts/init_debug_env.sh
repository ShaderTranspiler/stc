#!/usr/bin/bash

# run with source or '. scripts/init_debug_env.sh' to allow 
# modification of the active environment's variables
# usage: . scripts/init_debug_env.sh [suppression file path]

if [ $# -gt 0 ]; then
    SUPP_PATH=$(realpath "$1")
else
    SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd &> /dev/null && pwd)
    SUPP_PATH="$SCRIPT_DIR/../stc_lsan.supp"
fi

if [ ! -f "$SUPP_PATH" ]; then
    echo "LSan suppression file not found at $SUPP_PATH"
    echo "If you moved this script outside the scripts/ directory, or you want to use a custom suppression file, you may specify the file path using ./init_debug_env.sh <PATH>"
    return 1
fi

export LSAN_OPTIONS=suppressions="$SUPP_PATH"

echo "Active LSan suppression file path: $SUPP_PATH"
