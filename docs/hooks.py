"""MkDocs hook. Point Python at the staged package and write the binding index."""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

_DOCS = Path(__file__).resolve().parent
_BINDINGS = _DOCS / "api" / "bindings.md"


def on_pre_build(config) -> None:
    stage = os.environ.get("SJD_PYTHON_STAGE")
    if not stage:
        raise SystemExit("SJD_PYTHON_STAGE must point at build/<preset>/python")
    root = Path(stage)
    stub = root / "sjd" / "_core.pyi"
    if not stub.is_file():
        raise SystemExit(f"no generated stub at {stub}")
    sys.path.insert(0, str(root))
    _BINDINGS.write_text(_binding_page(stub), encoding="utf-8")


def _binding_page(stub: Path) -> str:
    text = stub.read_text(encoding="utf-8")
    names = _all_names(text)
    lines = [
        "# Bindings",
        "",
        "Generated from `_core.pyi`. Import these names from `sjd`.",
        "Signatures are in the stub. Behaviour is in the C++ reference.",
        "",
        "| Name | Text |",
        "| --- | --- |",
    ]
    for name in names:
        lines.append(f"| `{name}` | {_summary(text, name)} |")
    lines.append("")
    return "\n".join(lines)


def _all_names(text: str) -> list[str]:
    marker = "__all__: list[str] = ["
    start = text.find(marker)
    if start < 0:
        raise SystemExit("_core.pyi has no __all__")
    end = text.find("]", start + len(marker))
    body = text[start + len(marker) : end]
    return [part.strip().strip("'\"") for part in body.split(",") if part.strip()]


def _summary(text: str, name: str) -> str:
    """First paragraph of the class or function docstring, if it has one of its own."""
    match = re.search(rf"^(?:class|def) {re.escape(name)}\b", text, re.MULTILINE)
    if match is None:
        return ""
    rest = text[match.end() :]
    following = re.search(r"\n(?:class|def) ", rest)
    region = rest[: following.start()] if following else rest[:2000]
    quote = region.find('"""')
    if quote < 0 or re.search(r"\n\s+def ", region[:quote]):
        return ""
    close = region.find('"""', quote + 3)
    if close < 0:
        return ""
    paragraph = region[quote + 3 : close].strip().split("\n\n", 1)[0]
    return " ".join(paragraph.split()).replace("|", "\\|")
