# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import json
import os
import subprocess
import sys
import tempfile

from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[1]

NOTEBOOKS = [
    "tutorials/01_acoustic/AcousticWaves.ipynb",
    "tutorials/02_shallow-water/ShallowWater.ipynb",
    "tutorials/03_euler/Euler.ipynb",
    "tutorials/04_limiting/EulerShock.ipynb",
    "tutorials/05_elastic/ElasticWaves.ipynb",
]


def build_cells(notebook):
    """Returns the code cells of a notebook up to the cell that calls generate_and_build()."""
    cells = []
    for cell in json.loads(notebook.read_text(encoding="utf-8"))["cells"]:
        if cell["cell_type"] == "code":
            cells.append("".join(cell["source"]))
            if "generate_and_build(" in cells[-1]:
                return cells
    raise ValueError(f"{notebook} calls generate_and_build() in none of its cells.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        epilog=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--build-only", action="store_true", help="Run the cells up to the build"
    )
    options, notebook_arguments = parser.parse_known_args()
    environment = dict(os.environ, PYTHONPATH=str(REPOSITORY / "api"))
    failures = []
    for notebook in NOTEBOOKS:
        print("=== " + notebook, flush=True)
        path = REPOSITORY / notebook
        try:
            with tempfile.TemporaryDirectory() as scratch:
                script = path
                if options.build_only:
                    script = Path(scratch) / "build.ipy"
                    script.write_text("\n\n".join(build_cells(path)) + "\n")
                subprocess.run(
                    ["ipython", str(script), "--"] + notebook_arguments,
                    cwd=path.parent,
                    env=environment,
                    check=True,
                )
        except (subprocess.CalledProcessError, OSError, ValueError) as error:
            print(error, flush=True)
            failures.append(notebook)
    for notebook in failures:
        print("Failed: " + notebook)
    sys.exit(1 if failures else 0)
