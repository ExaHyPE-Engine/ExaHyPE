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

import numpy as np

REPOSITORY = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPOSITORY))

from postprocessing.solution_io import SolutionSource  # noqa: E402

EULER = {
    "rho": lambda fields: fields["rho"],
    "pressure": lambda fields: 0.4
    * (fields["rhoE"] - 0.5 * np.sum(fields["rhoU"] ** 2, axis=-1) / fields["rho"]),
}
SHALLOW_WATER = {"h": lambda fields: fields["h"]}
SCENARIOS = [
    {
        "script": "examples/euler/airfoil.py",
        "options": [],
        "arguments": [],
        "non_negative": EULER,
    },
    {
        "script": "examples/euler/point_explosion.py",
        "options": [],
        "arguments": [],
        "non_negative": EULER,
    },
    {
        "script": "examples/euler/point_explosion.py",
        "options": ["--dimensions", "3"],
        "arguments": [],
        "non_negative": EULER,
    },
    {
        "script": "examples/euler/rayleigh_taylor.py",
        "options": [],
        "arguments": [],
        "non_negative": EULER,
    },
    {
        "script": "examples/euler/rayleigh_taylor.py",
        "options": ["--dimensions", "3"],
        "arguments": ["-nt", "200"],
        "non_negative": EULER,
    },
    {
        "script": "examples/shallow-water/dry_ring.py",
        "options": [],
        "arguments": [],
        "non_negative": SHALLOW_WATER,
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


def get_violations(script, options, arguments, non_negative):
    output = OUTPUT / "-".join([Path(script).stem] + options).replace("--", "")
    shutil.rmtree(output, ignore_errors=True)
    build(script, options)
    run(script, ["-o", str(output), "-ns", "10", "-pdp"] + arguments)
    source = SolutionSource.open(output / "patch")
    violations = []
    for index in range(source.count):
        snapshot = source.snapshot(index)
        fields = {
            name: np.asarray(snapshot.field(name)[0], dtype=np.float64)
            for name in snapshot.variables
        }
        for name, values in fields.items():
            if not np.isfinite(values).all():
                violations.append("snapshot {}: {} is not finite".format(index, name))
        for name, quantity in non_negative.items():
            minimum = float(np.min(quantity(fields)))
            if minimum < 0.0:
                violations.append(
                    "snapshot {}: {} reaches {:.6e}".format(index, name, minimum)
                )
    return violations


if __name__ == "__main__":
    scenarios = [
        entry for entry in SCENARIOS if SELECTION.scenario in (None, entry["script"])
    ]
    if not scenarios:
        PARSER.error("The argument --scenario expects a script of the list SCENARIOS.")
    verdicts = []
    for scenario in scenarios:
        try:
            violations = get_violations(**scenario)
        except Exception:
            traceback.print_exc()
            violations = ["the run fails"]
        verdicts.append(
            (" ".join([scenario["script"]] + scenario["options"]), violations)
        )
    for label, violations in verdicts:
        print(
            "{}: {}".format(label, "; ".join(violations) if violations else "physical")
        )
    if any(violations for _, violations in verdicts):
        sys.exit("The physics test fails.")
