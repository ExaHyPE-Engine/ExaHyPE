# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import math
import os
import re
import shlex
import shutil
import subprocess
import sys
import traceback

from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPOSITORY))

from postprocessing import check_error_norms  # noqa: E402

ORDER_TOLERANCE = 0.5
SCENARIOS = [
    {
        "script": "examples/acoustic/planar_waves.py",
        "options": ["--solver", "fv"],
        "formal_order": 1,
        "minimum_cells": 9,
        "end_time": 0.5,
    },
    {
        "script": "examples/acoustic/planar_waves.py",
        "options": ["--solver", "aderdg"],
        "formal_order": 4,
        "minimum_cells": 9,
        "end_time": 0.5,
    },
    {
        "script": "examples/acoustic/planar_waves.py",
        "options": ["--solver", "rkdg"],
        "formal_order": 2,
        "minimum_cells": 9,
        "end_time": 0.5,
    },
    {
        "script": "examples/acoustic/planar_waves.py",
        "options": ["--solver", "fv", "--dimensions", "3"],
        "formal_order": 1,
        "minimum_cells": 9,
        "end_time": 0.5,
    },
    {
        "script": "examples/acoustic/planar_waves.py",
        "options": ["--solver", "aderdg", "--dimensions", "3"],
        "formal_order": 4,
        "minimum_cells": 9,
        "end_time": 0.5,
    },
    {
        "script": "examples/acoustic/planar_waves.py",
        "options": ["--solver", "rkdg", "--dimensions", "3"],
        "formal_order": 2,
        "minimum_cells": 9,
        "end_time": 0.5,
    },
    {
        "script": "examples/euler/gaussian_bell.py",
        "options": ["--solver", "aderdg"],
        "formal_order": 4,
        "minimum_cells": 9,
        "end_time": 0.1,
    },
]

OUTPUT = Path(__file__).resolve().with_suffix(".output")
LAUNCHER = ["env"] + shlex.split(os.environ.get("PARALLEL_LAUNCH", ""))
PARSER = argparse.ArgumentParser(
    epilog="Every further argument goes to the scenario scripts."
)
PARSER.add_argument(
    "--scenario",
    help="the script of the scenarios to run, such as examples/euler/airfoil.py",
)
SELECTION, SCENARIO_SCRIPT_ARGUMENTS = PARSER.parse_known_args()


def build(script, options):
    subprocess.run(
        [sys.executable, str(REPOSITORY / script)]
        + SCENARIO_SCRIPT_ARGUMENTS
        + options,
        check=True,
    )


def run(script, arguments):
    result = subprocess.run(
        LAUNCHER + ["./ExaHyPE"] + arguments,
        cwd=(REPOSITORY / script).parent,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    sys.stdout.write(result.stdout)
    if result.returncode != 0:
        raise RuntimeError(
            "The run of {} ends with the exit code {}.".format(
                script, result.returncode
            )
        )
    return result.stdout


def get_children_per_axis(script, output):
    shutil.rmtree(output, ignore_errors=True)
    report = run(script, ["-o", str(output), "-l", "2", "-nt", "1", "-ns", "0"])
    return int(re.search(r"\((\d+) Children per Axis\)", report).group(1))


def get_l2_error(script, output, level, end_time):
    shutil.rmtree(output, ignore_errors=True)
    run(script, ["-o", str(output), "-l", str(level), "-et", str(end_time), "-ns", "0"])
    header, rows = check_error_norms.read_error_norms(str(output / "Error.csv"))
    return float(rows[-1][header.index("l2-error")])


def get_order(script, options, minimum_cells, end_time):
    name = "-".join([Path(script).stem] + options).replace("--", "")
    build(script, options)
    children = get_children_per_axis(script, OUTPUT / (name + "-report"))
    coarse_level = 1
    while children**coarse_level < minimum_cells:
        coarse_level += 1
    errors = []
    for level in (coarse_level, coarse_level + 1):
        output = OUTPUT / "{}-level-{}".format(name, level)
        errors.append(get_l2_error(script, output, level, end_time))
    return math.log(errors[0] / errors[1]) / math.log(children)


if __name__ == "__main__":
    scenarios = [
        entry for entry in SCENARIOS if SELECTION.scenario in (None, entry["script"])
    ]
    if not scenarios:
        PARSER.error("The argument --scenario expects a script of the list SCENARIOS.")
    verdicts = []
    for scenario in scenarios:
        try:
            order = get_order(
                scenario["script"],
                scenario["options"],
                scenario["minimum_cells"],
                scenario["end_time"],
            )
        except Exception:
            traceback.print_exc()
            order = math.nan
        verdicts.append(
            (
                " ".join([scenario["script"]] + scenario["options"]),
                order,
                scenario["formal_order"] - ORDER_TOLERANCE,
            )
        )
    for label, order, minimum_order in verdicts:
        print("{}: order {:.2f}, minimum {:.2f}".format(label, order, minimum_order))
    if not all(order >= minimum_order for _, order, minimum_order in verdicts):
        sys.exit("The convergence test fails.")
