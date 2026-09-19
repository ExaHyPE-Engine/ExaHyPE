#!/usr/bin/env python3
"""Strip output artifacts from Jupyter notebooks.

Two modes:
  * git clean filter (no arguments): read a notebook on stdin, write the
    stripped notebook to stdout. Wired up via the ``*.ipynb filter=nbstripout``
    entry in .gitattributes together with ``git config filter.nbstripout.clean``.
  * manual (file arguments): strip the listed notebooks in place.

It removes code-cell outputs and execution counts plus run-specific cell and
notebook metadata, while leaving your working-tree copy untouched (the clean
filter only rewrites what enters the index). Pure standard library, so every
clone can run it with just python3 - no nbstripout/jupyter install required.

Enable once per clone, from the repository root:

    git config filter.nbstripout.clean "python3 documentation/strip-notebook-output.py"
"""

import json
import sys

# cell.metadata keys that pin a notebook to one particular run
_CELL_METADATA_TO_DROP = ("execution", "ExecuteTime", "collapsed", "scrolled")


def strip(notebook):
    notebook.get("metadata", {}).pop(
        "widgets", None
    )  # large, run-specific widget state
    for cell in notebook.get("cells", []):
        metadata = cell.get("metadata", {})
        for key in _CELL_METADATA_TO_DROP:
            metadata.pop(key, None)
        if cell.get("cell_type") == "code":
            cell["outputs"] = []
            cell["execution_count"] = None
    return notebook


def dumps(notebook):
    # indent=1 + ensure_ascii=False + trailing newline matches how Jupyter/nbformat
    # serialise notebooks, so stripping an already-clean notebook is a no-op (no diff).
    return json.dumps(notebook, indent=1, ensure_ascii=False) + "\n"


def main(argv):
    if argv:  # manual mode: strip the given files in place
        for path in argv:
            with open(path, encoding="utf-8") as handle:
                notebook = json.load(handle)
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(dumps(strip(notebook)))
        return 0

    data = sys.stdin.read()  # filter mode: stdin -> stdout
    try:
        notebook = json.loads(data)
    except json.JSONDecodeError:
        sys.stdout.write(
            data
        )  # not a JSON notebook (e.g. an LFS pointer); pass through
        return 0
    sys.stdout.write(dumps(strip(notebook)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
