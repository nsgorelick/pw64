#!/usr/bin/env python3
"""Convert legacy PicWorks /** **/ comments to plain /* */ in pw/ sources."""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PW = ROOT / "pw"

INLINE = re.compile(r"/\*\*[ \t]*([^*\n]+?)[ \t]*\*\*/")

# Repair botched conversion: "/**" followed immediately by "/*".
BOTCHED = re.compile(r"^[ \t]*/\*\*\s*\n([ \t]*/\*)", re.MULTILINE)


def _star_lines(body: str) -> list[str]:
    lines = []
    for line in body.splitlines():
        line = re.sub(r"^[ \t]*\*\*[ \t]*", "", line)
        line = line.strip()
        if line:
            lines.append(line)
    return lines


def _replace_boxed(match: re.Match) -> str:
    indent = match.group(1)
    body = match.group(2)
    lines = _star_lines(body)
    if not lines:
        return match.group(0)
    if len(lines) == 1:
        return f"{indent}/* {lines[0]} */"
    inner = "\n".join(f"{indent} * {ln}" for ln in lines)
    return f"{indent}/*\n{inner}\n{indent} */"


BOXED = re.compile(
    r"^([ \t]*)/\*\*\s*\n((?:[ \t]*\*\*[^\n]*\n)+)[ \t]*\*\*/",
    re.MULTILINE,
)


def normalize(text: str) -> str:
    text = BOTCHED.sub(r"\1", text)
    text = BOXED.sub(_replace_boxed, text)
    text = INLINE.sub(r"/* \1 */", text)
    return text


def main() -> int:
    changed = 0
    for path in sorted(PW.glob("*.[ch]")):
        old = path.read_text(encoding="utf-8", errors="replace")
        new = normalize(old)
        if new != old:
            path.write_text(new, encoding="utf-8")
            changed += 1
            print(path.relative_to(ROOT))
    print(f"normalize-pw-comments: updated {changed} file(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
