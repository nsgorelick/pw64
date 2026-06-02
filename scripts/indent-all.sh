#!/bin/sh
# Reformat C sources using GNU indent and the repo-root .indent.pro.
#
# Prefer scripts/clang-format-pw.sh for pw/ (handles legacy comments; see .clang-format).
#
# Profile: Kernighan & Ritchie braces, spaces not tabs, 4-space indent, 120 cols.
# Install: brew install gnu-indent
set -e

ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$ROOT"

INDENT=${INDENT:-}
if [ -z "$INDENT" ]; then
	for candidate in gindent /opt/homebrew/bin/gindent; do
		if command -v "$candidate" >/dev/null 2>&1; then
			INDENT=$candidate
			break
		fi
	done
fi
if [ -z "$INDENT" ]; then
	echo "indent-all: need GNU indent (brew install gnu-indent)" >&2
	exit 1
fi

# BSD indent on macOS does not read .indent.pro the same way.
ver=$($INDENT --version 2>&1 | head -1)
case "$ver" in
*GNU\ indent*) ;;
*)
	echo "indent-all: $INDENT is not GNU indent (got: $ver)" >&2
	exit 1
	;;
esac

# Options come from repo-root .indent.pro (override with INDENT_FLAGS if needed).
INDENT_FLAGS=${INDENT_FLAGS:-}

# Project trees only — not vendored iomedley/ or X11 bitmap headers.
DIRS="lib pw sp tests"

for dir in $DIRS; do
	[ -d "$dir" ] || continue
	find "$dir" \( -name '*.c' -o -name '*.h' \) \
		! -name hershey.oc.h ! -name config.h -print |
		while read -r f; do
			# shellcheck disable=SC2086
			"$INDENT" $INDENT_FLAGS "$f" -o "$f"
		done
done

for f in Xfred.h acconfig.h; do
	[ -f "$f" ] || continue
	# shellcheck disable=SC2086
	"$INDENT" $INDENT_FLAGS "$f" -o "$f"
done

echo "indent-all: formatted sources under $DIRS (+ top-level headers)"
