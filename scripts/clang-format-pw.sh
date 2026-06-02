#!/bin/sh
# Reformat pw/ C sources with clang-format and repo-root .clang-format.
# For comments + format together, use scripts/modernize-pw.sh.
#
# Uses Apple CLT clang-format or Homebrew llvm if available.
# Install: xcode-select --install   OR   brew install llvm
set -e

ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$ROOT"

CLANG_FORMAT=${CLANG_FORMAT:-}
if [ -z "$CLANG_FORMAT" ]; then
	for candidate in \
		/opt/homebrew/opt/llvm/bin/clang-format \
		/opt/homebrew/bin/clang-format \
		/Library/Developer/CommandLineTools/usr/bin/clang-format \
		clang-format; do
		if command -v "$candidate" >/dev/null 2>&1; then
			CLANG_FORMAT=$candidate
			break
		fi
	done
fi
if [ -z "$CLANG_FORMAT" ]; then
	echo "clang-format-pw: need clang-format (CLT or brew install llvm)" >&2
	exit 1
fi

echo "clang-format-pw: using $CLANG_FORMAT ($($CLANG_FORMAT --version 2>&1 | head -1))"

DIRS=${1:-pw}
for dir in $DIRS; do
	[ -d "$dir" ] || continue
	find "$dir" \( -name '*.c' -o -name '*.h' \) -print |
		while read -r f; do
			"$CLANG_FORMAT" -i -style=file "$f" || exit 1
		done
done

echo "clang-format-pw: formatted sources under $DIRS"
