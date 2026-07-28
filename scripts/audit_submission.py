#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import re
import sys

FORBIDDEN_NAMES = re.compile(r"(^|[-_.])(poc|crash|exploit|patch|bugs?)([-_.]|$)", re.I)
FORBIDDEN_TEXT = re.compile(
    r"use[- ]after[- ]free|double[- ]free|dangling|stale pointer|root cause|"
    r"reference solution|asan trace|addresssanitizer|\bTODO\b|\bFIXME\b|\bHACK\b|\bXXX\b",
    re.I,
)
ALLOWED_SUFFIXES = {".c", ".h", ".cc", ".md", ".sh", ".yaml", ".yml", ".txt", ""}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=pathlib.Path)
    args = parser.parse_args()
    root = args.root.resolve()
    problems: list[str] = []
    for path in sorted(root.rglob("*")):
        rel = path.relative_to(root)
        if any(part in {".git", "internal"} for part in rel.parts):
            problems.append(f"forbidden path: {rel}")
            continue
        if path.is_dir():
            continue
        if FORBIDDEN_NAMES.search(path.name):
            problems.append(f"revealing filename: {rel}")
        if path.suffix in {".o", ".a", ".so", ".dll", ".dylib", ".zip", ".tar", ".gz"}:
            problems.append(f"generated artifact: {rel}")
        if path.suffix.lower() in ALLOWED_SUFFIXES:
            try:
                text = path.read_text(encoding="utf-8")
            except UnicodeDecodeError:
                continue
            if FORBIDDEN_TEXT.search(text):
                problems.append(f"revealing text: {rel}")
    src_lines = 0
    for path in (root / "src").glob("*.[ch]"):
        src_lines += sum(1 for line in path.read_text(encoding="utf-8").splitlines() if line.strip())
    if src_lines < 10000:
        problems.append(f"source LOC gate not met: {src_lines} nonblank lines")
    required = [
        "build.sh", ".clusterfuzzlite/build.sh", ".clusterfuzzlite/project.yaml",
        "Makefile", "README.md", "ARCHITECTURE.md", "LICENSE",
    ]
    for item in required:
        if not (root / item).exists():
            problems.append(f"missing required file: {item}")
    fuzzers = list((root / "fuzz").glob("*_fuzzer.cc"))
    if len(fuzzers) < 3:
        problems.append("fewer than three fuzzer harnesses")
    if problems:
        print("submission audit failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print("submission audit passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
