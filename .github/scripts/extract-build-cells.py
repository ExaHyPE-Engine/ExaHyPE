# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

"""Writes the code cells of a notebook up to the cell that calls generate_and_build() into an
IPython script. The script builds the executable of the notebook without running it."""

import argparse
import json
import sys


def get_build_cells(notebook_path):
    with open(notebook_path, encoding="utf-8") as notebook_file:
        notebook = json.load(notebook_file)
    cells = []
    for cell in notebook["cells"]:
        if cell["cell_type"] != "code":
            continue
        source = "".join(cell["source"])
        cells.append(source)
        if "generate_and_build(" in source:
            return cells
    sys.exit(
        f"The notebook {notebook_path} needs a cell that calls generate_and_build()."
    )


def build_parser():
    parser = argparse.ArgumentParser(
        description="Writes the code cells of a notebook up to its build into an IPython script."
    )
    parser.add_argument("notebook", help="Notebook to read")
    parser.add_argument("script", help="IPython script to write")
    return parser


if __name__ == "__main__":
    args = build_parser().parse_args()
    with open(args.script, "w", encoding="utf-8") as script_file:
        script_file.write("\n\n".join(get_build_cells(args.notebook)) + "\n")
