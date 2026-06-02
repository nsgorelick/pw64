#!/usr/bin/env python3
"""Remove CVS/Subversion $Header$, $Log$, and Revision blocks from C sources."""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIRS = ("lib", "pw", "sp", "tests")
EXTRA = (ROOT / "Xfred.h",)

REVISION = re.compile(r"Revision\s+[\d.]+")
RCS_TAG = re.compile(r"\$Header\$|\$Log\$|\$Id\$")
RCS_IDENT = re.compile(r'^\s*static\s+char\s+rcs_ident\s*\[\s*\]\s*=\s*"\$Header\$"\s*;\s*$')
STANDALONE_HEADER = re.compile(r"^\s*/\*\s*\$Header\$\s*\*/\s*$")
TGIF_RCS = re.compile(r"@\(#\)\$Header\$|%W%")


def in_log_block(line: str) -> bool:
    s = line.strip()
    if not s.startswith("*"):
        return False
    if s == "*/":
        return False
    if RCS_TAG.search(line):
        return False
    if REVISION.search(line):
        return True
    if s in ("*",):
        return True
    # Continuation line inside $Log$ (indented changelog text).
    if s.startswith("* ") and "Revision" not in line:
        return True
    return False


def strip_text(text: str) -> str:
    lines = text.splitlines(keepends=True)
    out = []
    i = 0
    while i < len(lines):
        line = lines[i]

        if STANDALONE_HEADER.match(line) or RCS_IDENT.match(line):
            i += 1
            continue

        if TGIF_RCS.search(line):
            i += 1
            continue

        if RCS_TAG.search(line):
            i += 1
            while i < len(lines) and in_log_block(lines[i]):
                i += 1
            continue

        if REVISION.search(line) and line.strip().startswith("*"):
            i += 1
            while i < len(lines) and in_log_block(lines[i]):
                i += 1
            continue

        out.append(line)
        i += 1

    result = "".join(out)
    # Drop comment blocks that only held RCS tags.
    result = re.sub(r"/\*\s*\n\s*\*/\s*\n", "", result)
    result = re.sub(r"\n{3,}", "\n\n", result)
    return result


def main() -> int:
    changed = 0
    paths = list(EXTRA)
    for d in DIRS:
        p = ROOT / d
        if p.is_dir():
            paths.extend(sorted(p.rglob("*.c")))
            paths.extend(sorted(p.rglob("*.h")))

    for path in paths:
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        new = strip_text(text)
        if new != text:
            path.write_text(new, encoding="utf-8")
            print(path.relative_to(ROOT))
            changed += 1

    print(f"strip-rcs: updated {changed} file(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
