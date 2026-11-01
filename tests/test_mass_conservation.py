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

from postprocessing import check_conservation  # noqa: E402

ADAPTIVE_2D = ["-lmin", "2", "-lmax", "4"]
TIME_STEPS = 40
SCENARIOS = [
    {
        "script": "examples/advection/periodic_bump.py",
        "options": ["--solver", "fv"],
        "arguments": ADAPTIVE_2D,
        "check": ["--fields", "q"],
    },
    {
        "script": "examples/advection/periodic_bump.py",
        "options": ["--solver", "aderdg"],
        "arguments": ADAPTIVE_2D,
        "check": ["--fields", "q", "--gauss-legendre-nodes", "4"],
    },
    {
        "script": "examples/advection/periodic_bump.py",
        "options": ["--solver", "rkdg"],
        "arguments": ADAPTIVE_2D,
        "check": ["--fields", "q", "--gauss-legendre-nodes", "3"],
    },
    {
        "script": "examples/euler/advected_blobs_with_blast.py",
        "options": [],
        "arguments": ADAPTIVE_2D,
        "check": [],
    },
    {
        "script": "examples/euler/isentropic_cross.py",
        "options": ["--solver", "fv", "--scheme", "godunov"],
        "arguments": ADAPTIVE_2D,
        "check": [],
    },
    {
        "script": "examples/euler/isentropic_cross.py",
        "options": ["--solver", "fv", "--scheme", "muscl-hancock"],
        "arguments": ADAPTIVE_2D,
        "check": [],
    },
    {
        "script": "examples/euler/isentropic_cross.py",
        "options": ["--solver", "aderdg"],
        "arguments": ADAPTIVE_2D,
        "check": ["--gauss-legendre-nodes", "4"],
    },
    {
        "script": "examples/euler/isentropic_cross.py",
        "options": ["--solver", "rkdg"],
        "arguments": ADAPTIVE_2D,
        "check": ["--gauss-legendre-nodes", "3"],
    },
]

OUTPUT = Path(__file__).resolve().with_suffix(".output")
LAUNCHER = ["env"] + shlex.split(os.environ.get("PARALLEL_LAUNCH", ""))
PARSER = argparse.ArgumentParser(
    epilog="Every further argument goes to the scenario scripts."
)
PARSER.add_argument(
    "--scenario",
    help="the script of the scenarios to run, such as examples/euler/isentropic_cross.py",
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


def conserves_mass(script, options, arguments, check):
    output = OUTPUT / "-".join([Path(script).stem] + options + arguments).replace(
        "--", ""
    )
    shutil.rmtree(output, ignore_errors=True)
    build(script, options)
    run(
        script,
        ["-o", str(output), "-nt", str(TIME_STEPS), "-ns", "4", "-pdp"] + arguments,
    )
    try:
        check_conservation.main(
            [str(output / "patch"), "--time-steps", str(TIME_STEPS)] + check
        )
    except SystemExit:
        return False
    return True


if __name__ == "__main__":
    scenarios = [
        entry for entry in SCENARIOS if SELECTION.scenario in (None, entry["script"])
    ]
    if not scenarios:
        PARSER.error("The argument --scenario expects a script of the list SCENARIOS.")
    verdicts = []
    for scenario in scenarios:
        try:
            conserved = conserves_mass(**scenario)
        except Exception:
            traceback.print_exc()
            conserved = False
        verdicts.append(
            (
                " ".join(
                    [scenario["script"]] + scenario["options"] + scenario["arguments"]
                ),
                conserved,
            )
        )
    for label, conserved in verdicts:
        print("{}: {}".format(label, "conserved" if conserved else "not conserved"))
    if not all(conserved for _, conserved in verdicts):
        sys.exit("The mass conservation test fails.")
