#!/usr/bin/env python3
"""Add APSL-required OSSD modification notices to changed XNU-derived files."""
from pathlib import Path
import subprocess
import sys

BASE = "5b4847b4d910bf78733454c9f4d161f062d953d1"
DATE = "2026-11-24"
MARKER = "OSSD MODIFICATION NOTICE: Modified by OSSD on " + DATE + "."

def main():
    changed = subprocess.check_output(
        ["git", "diff", "--name-only", BASE, "HEAD", "--", "XINU"],
        text=True,
    ).splitlines()
    updated = []
    skipped = []
    for name in changed:
        path = Path(name)
        if not path.is_file() or path.suffix.lower() not in {
            ".c", ".h", ".cc", ".hh", ".cpp", ".hpp", ".cxx", ".hxx",
            ".m", ".mm", ".inc", ".s", ".S", ".d", ".l", ".y",
        }:
            continue
        try:
            source = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            skipped.append(name)
            continue
        if MARKER in source:
            continue
        if "@APPLE_LICENSE_HEADER_START@" in source and "@APPLE_LICENSE_HEADER_END@" in source:
            end_tag = "@APPLE_LICENSE_HEADER_END@"
        elif ("@APPLE_OSREFERENCE_LICENSE_HEADER_START@" in source
              and "@APPLE_OSREFERENCE_LICENSE_HEADER_END@" in source):
            end_tag = "@APPLE_OSREFERENCE_LICENSE_HEADER_END@"
        else:
            skipped.append(name)
            continue
        line = " * " + MARKER + "\n"
        source = source.replace(end_tag, line + " * " + end_tag, 1)
        path.write_text(source, encoding="utf-8", newline="")
        updated.append(name)
    print(f"Added dated OSSD notices to {len(updated)} APSL-covered files.")
    print(f"Skipped {len(skipped)} changed files without a recognized APSL header.")
    if skipped:
        print("\n".join(skipped))
    if not updated:
        print("No file-level notices needed to be added.")

if __name__ == "__main__":
    main()
