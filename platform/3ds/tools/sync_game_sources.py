#!/usr/bin/env python3
"""Usage: python3 platform/3ds/tools/sync_game_sources.py (from the repo root)"""
import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parents[3]
PC_CMAKE = REPO / "pc" / "CMakeLists.txt"
OUTPUT = REPO / "platform" / "3ds" / "cmake" / "game_sources.cmake"

# (first line, line that ends the block, excluded)
BLOCKS = [
    (r"^# Collect game source files", r"^# PC port source files"),
    (r"^# Exclude emu64_utility", r"^# jaudio_NES C files use nullptr"),
]
SECTION_RULE = re.compile(r"^# =+$")


def find_block(lines, start_pat, end_pat):
    start = next((i for i, l in enumerate(lines) if re.search(start_pat, l)), None)
    if start is None:
        sys.exit(f"pattern not found in {PC_CMAKE}: {start_pat}")
    end = next((i for i in range(start + 1, len(lines)) if re.search(end_pat, lines[i])), None)
    if end is None:
        sys.exit(f"pattern not found in {PC_CMAKE}: {end_pat}")
    while end > start and (SECTION_RULE.match(lines[end - 1]) or not lines[end - 1].strip()):
        end -= 1
    return start, end


def upstream_revision():
    try:
        out = subprocess.run(["git", "-C", str(REPO), "log", "-1", "--format=%h", "--", str(PC_CMAKE)],
                             capture_output=True, text=True, check=True)
        return out.stdout.strip() or "unknown"
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def main():
    lines = PC_CMAKE.read_text().splitlines()
    parts, ranges = [], []
    for start_pat, end_pat in BLOCKS:
        start, end = find_block(lines, start_pat, end_pat)
        parts.append("\n".join(lines[start:end]))
        ranges.append(f"{start + 1}-{end}")
    header = (
        "# Game sources for the 3DS build: a verbatim copy of the source selection in\n"
        f"# pc/CMakeLists.txt (commit {upstream_revision()}, lines {' and '.join(ranges)}), so the 3DS\n"
        "# build compiles exactly the files the PC port does. Regenerate with\n"
        "# platform/3ds/tools/sync_game_sources.py; don't edit by hand.\n"
        "#\n"
        "# Expects DECOMP_ROOT. Defines GAME_C_SOURCES and GAME_CXX_SOURCES and sets the\n"
        "# C++ language on the .c files that contain C++.\n"
        "\n"
        "# ---- copied from pc/CMakeLists.txt ----\n"
    )
    OUTPUT.write_text(header + "\n\n".join(parts) + "\n# ---- end of copy ----\n")
    print(f"wrote {OUTPUT.relative_to(REPO)} (lines {', '.join(ranges)})")


if __name__ == "__main__":
    main()
