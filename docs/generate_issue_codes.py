"""Check docs/issue-codes.md against include/sjd/tools/IssueCodes.h.

    python docs/generate_issue_codes.py --check
    python docs/generate_issue_codes.py --write
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

_ROOT = Path(__file__).resolve().parents[1]
_HEADER = _ROOT / "include" / "sjd" / "tools" / "IssueCodes.h"
_PAGE = _ROOT / "docs" / "issue-codes.md"
_CODE = re.compile(
    r"inline\s+constexpr\s+std::string_view\s+(\w+)\s*=\s*\"([^\"]+)\"\s*;",
    re.MULTILINE,
)
_ROW = re.compile(r"^\| `([A-Z0-9_]+)` \| (.*) \|$")
_MISSING = "No description yet."


def header_codes(text: str) -> dict[str, str]:
    found = dict(_CODE.findall(text))
    if not found:
        raise SystemExit(f"no issue codes in {_HEADER}")
    mismatched = [name for name, value in found.items() if name != value]
    if mismatched:
        raise SystemExit("issue code text differs from its name: " + ", ".join(mismatched))
    return found


def table_meanings(text: str) -> dict[str, str]:
    meanings = {}
    for line in text.splitlines():
        match = _ROW.match(line)
        if match:
            meanings[match.group(1)] = match.group(2)
    return meanings


def render(intro: str, codes: dict[str, str], meanings: dict[str, str]) -> str:
    rows = [
        "| Code | Meaning |",
        "| --- | --- |",
    ]
    for name in sorted(codes):
        rows.append(f"| `{name}` | {meanings.get(name, _MISSING)} |")
    body = "\n".join(rows)
    prologue = intro.rstrip()
    return f"{prologue}\n\n{body}\n"


def intro_of(text: str) -> str:
    marker = "| Code | Meaning |"
    start = text.find(marker)
    if start < 0:
        raise SystemExit(f"{_PAGE} has no issue-code table")
    return text[:start].rstrip()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args(argv)
    if args.check == args.write:
        parser.error("choose one of --check or --write")
    codes = header_codes(_HEADER.read_text(encoding="utf-8"))
    page = _PAGE.read_text(encoding="utf-8")
    meanings = table_meanings(page)
    missing = sorted(set(codes) - set(meanings))
    extra = sorted(set(meanings) - set(codes))
    untitled = sorted(name for name, meaning in meanings.items() if meaning.strip() in {"", _MISSING})
    if args.check:
        problems = []
        if missing:
            problems.append("missing from the table: " + ", ".join(missing))
        if extra:
            problems.append("not in the header: " + ", ".join(extra))
        if untitled:
            problems.append("no meaning: " + ", ".join(untitled))
        if problems:
            print("\n".join(problems), file=sys.stderr)
            return 1
        return 0
    if missing or extra or untitled:
        print("rewriting the table; fill any 'No description yet.' rows", file=sys.stderr)
    _PAGE.write_text(render(intro_of(page), codes, meanings), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
