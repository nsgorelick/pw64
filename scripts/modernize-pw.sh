#!/bin/sh
# Comment normalization + clang-format for pw/ (see .clang-format).
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$ROOT"
python3 "$ROOT/scripts/normalize-pw-comments.py"
"$ROOT/scripts/clang-format-pw.sh" pw
