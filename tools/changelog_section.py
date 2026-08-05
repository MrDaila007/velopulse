#!/usr/bin/env python3
"""Print the CHANGELOG.md section for a git tag (e.g. v0.2.0-beta.2)."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CHANGELOG = ROOT / "CHANGELOG.md"


def normalize_tag(tag: str) -> str:
    return tag[1:] if tag.startswith("v") else tag


def extract_section(changelog: str, version: str) -> str:
    pattern = re.compile(
        rf"^## \[{re.escape(version)}\].*$",
        re.MULTILINE,
    )
    match = pattern.search(changelog)
    if not match:
        raise SystemExit(f"No CHANGELOG section found for version {version!r}")

    start = match.start()
    next_heading = re.search(r"^## \[", changelog[match.end() :], re.MULTILINE)
    end = match.end() + next_heading.start() if next_heading else len(changelog)
    return changelog[start:end].strip() + "\n"


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <tag>", file=sys.stderr)
        return 2

    version = normalize_tag(sys.argv[1])
    changelog = CHANGELOG.read_text(encoding="utf-8")
    sys.stdout.write(extract_section(changelog, version))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
