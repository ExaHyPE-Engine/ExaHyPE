# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os
import shlex
import shutil
import subprocess
import sys
import traceback

from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPOSITORY))

from postprocessing import check_error_norms  # noqa: E402

SCENARIOS = [
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "fv"],
        "arguments": [],
    },
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "aderdg"],
        "arguments": [],
    },
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "rkdg"],
        "arguments": [],
    },
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "fv", "--dimensions", "3"],
        "arguments": ["-lmax", "3"],
    },
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "aderdg", "--dimensions", "3"],
        "arguments": ["-lmax", "3"],
    },
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "rkdg", "--dimensions", "3"],
        "arguments": ["-lmax", "3"],
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


def is_exact(script, options, arguments):
    output = OUTPUT / "-".join([Path(script).stem] + options).replace("--", "")
    shutil.rmtree(output, ignore_errors=True)
    build(script, options)
    run(script, ["-o", str(output), "-ns", "0"] + arguments)
    errors = []
    for path in sorted(output.glob("Error*.csv")):
        _, rows = check_error_norms.read_error_norms(str(path))
        errors += [float(value) for row in rows for value in row[1:]]
    return len(errors) > 0 and all(error == 0.0 for error in errors)


if __name__ == "__main__":
    scenarios = [
        entry for entry in SCENARIOS if SELECTION.scenario in (None, entry["script"])
    ]
    if not scenarios:
        PARSER.error("The argument --scenario expects a script of the list SCENARIOS.")
    verdicts = []
    for scenario in scenarios:
        try:
            exact = is_exact(**scenario)
        except Exception:
            traceback.print_exc()
            exact = False
        verdicts.append((" ".join([scenario["script"]] + scenario["options"]), exact))
    for label, exact in verdicts:
        print("{}: {}".format(label, "exact" if exact else "inexact"))
    if not all(exact for _, exact in verdicts):
        sys.exit("The free-stream test fails.")
