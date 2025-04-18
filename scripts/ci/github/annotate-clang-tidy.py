#!/usr/bin/env python3

import re
import sys
from pathlib import Path


def parse_tidy_log(log_path, source_path):
    diag_pattern = re.compile(
        r"^(.*?):(\d+):(\d+): (warning|error): (.*?) \[(.*?)\]$"
    )
    note_pattern = re.compile(r"^(.*?):(\d+):(\d+): note: (.*)$")
    marker_pattern = re.compile(r"^(\s*)\^~*\s*$")

    annotations = []
    last_diag = None
    end_line = None
    end_col = None

    lines = Path(log_path).read_text().splitlines()
    i = 0

    while i < len(lines):
        line = lines[i]
        m = diag_pattern.match(line)
        if m:
            file, line_num, col, level, message, check = m.groups()
            if file.startswith(source_path):
                file = file[len(source_path) :]

            key = (file, line_num, col)
            last_diag = key

            # Try to extract marker line after the code context line
            # (2 lines ahead)
            if i + 2 < len(lines):
                marker_line = lines[i + 2]
                marker_match = marker_pattern.match(marker_line)
                if marker_match:
                    start_col = int(col)
                    marker_len = len(marker_line.strip())
                    end_line = line_num
                    end_col = start_col + marker_len
                else:
                    end_line = end_col = None
            else:
                end_line = end_col = None

            annotations.append(
                {
                    "level": level,
                    "file": file,
                    "line": line_num,
                    "col": col,
                    "end_line": end_line,
                    "end_col": end_col,
                    "checks": check.split(","),
                    "message": message,
                }
            )
            i += 1
            continue

        n = note_pattern.match(line)
        if n:
            file, line_num, col, note_msg = n.groups()
            if file.startswith(source_path):
                file = file[len(source_path) :]

            key = (file, line_num, col)

            if last_diag == key:
                if "notes" not in annotations[-1]:
                    annotations[-1]["notes"] = list()
                annotations[-1]["notes"].append(note_msg)
            else:
                annotations.append(
                    {
                        "level": "notice",
                        "file": file,
                        "line": line_num,
                        "col": col,
                        "title": "note",
                        "message": note_msg,
                    }
                )

        i += 1

    return annotations


def emit_annotations(annotations):
    for entry in annotations:
        out_level = entry["level"]

        out_args = {
            "file": entry["file"],
            "line": entry["line"],
            "col": entry["col"],
        }
        if entry.get("end_line") and entry.get("end_col"):
            out_args["endLine"] = entry["end_line"]
            out_args["endColumn"] = entry["end_col"]
        if "title" in entry:
            out_args["title"] = entry["title"]
        elif "checks" in entry:
            out_args["title"] = "%2C ".join(entry["checks"])
        out_args_all = ",".join([f"{k}={v}" for k, v in out_args.items()])

        if "notes" in entry:
            msg1 = entry["message"].capitalize()
            msg2 = "".join([f"%0A- {e.capitalize()}" for e in entry["notes"]])
            out_msg = f"{msg1}%0AFixes:{msg2}"
        else:
            out_msg = entry["message"].capitalize()

        print(f"::{out_level} {out_args_all}::{out_msg}")


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <clang-tidy-log> <source-dir>")
        sys.exit(1)

    log_path = sys.argv[1]
    source_path = sys.argv[2]
    if source_path[-1] != "/":
        source_path += "/"
    annotations = parse_tidy_log(log_path, source_path)
    emit_annotations(annotations)


if __name__ == "__main__":
    main()
