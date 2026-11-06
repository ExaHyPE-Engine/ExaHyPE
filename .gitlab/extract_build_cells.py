# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

"""Writes the code cells of a notebook up to the cell that calls generate_and_build() into an
IPython script. The script builds the executable of the notebook without running it."""

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


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("Usage: extract_build_cells.py <notebook> <script>")
    with open(sys.argv[2], "w", encoding="utf-8") as script_file:
        script_file.write("\n\n".join(get_build_cells(sys.argv[1])) + "\n")
