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

from postprocessing import check_symmetry  # noqa: E402

ALL_MIRRORS_2D = ["x", "y", "xy"]
ALL_MIRRORS_3D = ["x", "y", "z", "xy", "xz", "yz"]
AXIS_SWAPS_3D = ["xy", "xz", "yz"]
REGULAR_2D = ["-l", "3"]
REGULAR_3D = ["-l", "2"]
ADAPTIVE_2D = ["-lmin", "2", "-lmax", "4"]
ADAPTIVE_3D = ["-lmin", "2", "-lmax", "3"]
STATIC_REFINEMENT = ["--staticRefinement", "true"]
SCENARIOS = [
    {
        "script": "examples/acoustic/planar_waves.py",
        "options": ["--solver", "aderdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/acoustic/planar_waves.py",
        "options": ["--solver", "rkdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/advection/linear.py",
        "options": ["--solver", "aderdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/advection/linear.py",
        "options": ["--solver", "rkdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/elastic/planar_waves.py",
        "options": ["--solver", "aderdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/elastic/planar_waves.py",
        "options": ["--solver", "rkdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/elastic/ricker_pulse.py",
        "options": ["--solver", "aderdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["y"],
        "check": ["--symmetric-tensor-fields", "sigma:xx,yy,xy"],
    },
    {
        "script": "examples/elastic/ricker_pulse.py",
        "options": ["--solver", "rkdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["y"],
        "check": ["--symmetric-tensor-fields", "sigma:xx,yy,xy"],
    },
    {
        "script": "examples/euler/gaussian_bell.py",
        "options": ["--solver", "fv"],
        "arguments": REGULAR_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/euler/gaussian_bell.py",
        "options": ["--solver", "aderdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/euler/gaussian_bell.py",
        "options": ["--solver", "rkdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/euler/gaussian_bell.py",
        "options": ["--solver", "fv", "--dimensions", "3"],
        "arguments": REGULAR_3D,
        "mirrors": AXIS_SWAPS_3D,
        "check": [],
    },
    {
        "script": "examples/euler/gaussian_bell.py",
        "options": ["--solver", "aderdg", "--dimensions", "3"],
        "arguments": REGULAR_3D,
        "mirrors": AXIS_SWAPS_3D,
        "check": [],
    },
    {
        "script": "examples/euler/gaussian_bell.py",
        "options": ["--solver", "rkdg", "--dimensions", "3"],
        "arguments": REGULAR_3D,
        "mirrors": AXIS_SWAPS_3D,
        "check": [],
    },
    {
        "script": "examples/navier-stokes/cosine_bubble.py",
        "options": ["--solver", "aderdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["x"],
        "check": ["--scalar-fields", "backgroundstate"],
    },
    {
        "script": "examples/navier-stokes/cosine_bubble.py",
        "options": ["--solver", "rkdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["x"],
        "check": ["--scalar-fields", "backgroundstate"],
    },
    {
        "script": "examples/navier-stokes/taylor_green_vortex.py",
        "options": ["--solver", "aderdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["x", "y"],
        "check": [],
    },
    {
        "script": "examples/navier-stokes/taylor_green_vortex.py",
        "options": ["--solver", "rkdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["x", "y"],
        "check": [],
    },
    {
        "script": "examples/shallow-water/resting_lake.py",
        "options": ["--solver", "aderdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/shallow-water/resting_lake.py",
        "options": ["--solver", "rkdg"],
        "arguments": REGULAR_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/acoustic/point_explosion.py",
        "options": ["--solver", "aderdg"],
        "arguments": ADAPTIVE_2D + STATIC_REFINEMENT,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/acoustic/point_explosion.py",
        "options": ["--solver", "rkdg"],
        "arguments": ADAPTIVE_2D + STATIC_REFINEMENT,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/acoustic/point_explosion.py",
        "options": ["--solver", "aderdg", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D + STATIC_REFINEMENT,
        "mirrors": AXIS_SWAPS_3D,
        "check": [],
    },
    {
        "script": "examples/acoustic/point_explosion.py",
        "options": ["--solver", "rkdg", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D + STATIC_REFINEMENT,
        "mirrors": AXIS_SWAPS_3D,
        "check": [],
    },
    {
        "script": "examples/acoustic/point_source.py",
        "options": [],
        "arguments": STATIC_REFINEMENT,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/shallow-water/radial_step.py",
        "options": ["--solver", "fv"],
        "arguments": STATIC_REFINEMENT,
        "mirrors": ALL_MIRRORS_2D,
        "check": [],
    },
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "fv"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ALL_MIRRORS_2D,
        "check": [],
    },
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "aderdg"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ALL_MIRRORS_2D,
        "check": [],
    },
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "rkdg"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ALL_MIRRORS_2D,
        "check": [],
    },
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "fv", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": ALL_MIRRORS_3D,
        "check": [],
    },
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "aderdg", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": ALL_MIRRORS_3D,
        "check": [],
    },
    {
        "script": "examples/acoustic/free_stream.py",
        "options": ["--solver", "rkdg", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": ALL_MIRRORS_3D,
        "check": [],
    },
    {
        "script": "examples/acoustic/point_explosion.py",
        "options": ["--solver", "fv"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/acoustic/point_explosion.py",
        "options": ["--solver", "aderdg"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/acoustic/point_explosion.py",
        "options": ["--solver", "rkdg"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/acoustic/point_explosion.py",
        "options": ["--solver", "fv", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": AXIS_SWAPS_3D,
        "check": [],
    },
    {
        "script": "examples/acoustic/point_explosion.py",
        "options": ["--solver", "aderdg", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": AXIS_SWAPS_3D,
        "check": [],
    },
    {
        "script": "examples/acoustic/point_explosion.py",
        "options": ["--solver", "rkdg", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": AXIS_SWAPS_3D,
        "check": [],
    },
    {
        "script": "examples/advection/periodic_bump.py",
        "options": ["--solver", "fv"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/advection/periodic_bump.py",
        "options": ["--solver", "aderdg"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/advection/periodic_bump.py",
        "options": ["--solver", "rkdg"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ["xy"],
        "check": [],
    },
    {
        "script": "examples/advection/periodic_bump.py",
        "options": ["--solver", "fv", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": AXIS_SWAPS_3D,
        "check": [],
    },
    {
        "script": "examples/advection/periodic_bump.py",
        "options": ["--solver", "aderdg", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": AXIS_SWAPS_3D,
        "check": [],
    },
    {
        "script": "examples/advection/periodic_bump.py",
        "options": ["--solver", "rkdg", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": AXIS_SWAPS_3D,
        "check": [],
    },
    {
        "script": "examples/advection/radial_bump.py",
        "options": ["--solver", "fv"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ALL_MIRRORS_2D,
        "check": [],
    },
    {
        "script": "examples/advection/radial_bump.py",
        "options": ["--solver", "aderdg"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ALL_MIRRORS_2D,
        "check": [],
    },
    {
        "script": "examples/advection/radial_bump.py",
        "options": ["--solver", "rkdg"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ALL_MIRRORS_2D,
        "check": [],
    },
    {
        "script": "examples/advection/radial_bump.py",
        "options": ["--solver", "fv", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": ALL_MIRRORS_3D,
        "check": [],
    },
    {
        "script": "examples/advection/radial_bump.py",
        "options": ["--solver", "aderdg", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": ALL_MIRRORS_3D,
        "check": [],
    },
    {
        "script": "examples/advection/radial_bump.py",
        "options": ["--solver", "rkdg", "--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": ALL_MIRRORS_3D,
        "check": [],
    },
    {
        "script": "examples/shallow-water/radial_dam_break.py",
        "options": ["--solver", "aderdg"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ALL_MIRRORS_2D,
        "check": [],
    },
    {
        "script": "examples/shallow-water/radial_dam_break.py",
        "options": ["--solver", "rkdg"],
        "arguments": ADAPTIVE_2D,
        "mirrors": ALL_MIRRORS_2D,
        "check": [],
    },
    {
        "script": "examples/euler/airfoil.py",
        "options": [],
        "arguments": [],
        "mirrors": ["y"],
        "check": [],
    },
    {
        "script": "examples/euler/airfoil.py",
        "options": ["--dimensions", "3"],
        "arguments": ADAPTIVE_3D,
        "mirrors": ["y", "z"],
        "check": [],
    },
    {
        "script": "examples/euler/point_explosion.py",
        "options": [],
        "arguments": [],
        "mirrors": ALL_MIRRORS_2D,
        "check": [],
    },
    {
        "script": "examples/euler/point_explosion.py",
        "options": ["--dimensions", "3"],
        "arguments": [],
        "mirrors": ALL_MIRRORS_3D,
        "check": [],
    },
    {
        "script": "examples/euler/rayleigh_taylor.py",
        "options": [],
        "arguments": [],
        "mirrors": ["x"],
        "check": [],
    },
    {
        "script": "examples/euler/rayleigh_taylor.py",
        "options": ["--dimensions", "3"],
        "arguments": [],
        "mirrors": ["x", "z", "xz"],
        "check": [],
    },
    {
        "script": "examples/shallow-water/dry_ring.py",
        "options": [],
        "arguments": REGULAR_2D,
        "mirrors": ALL_MIRRORS_2D,
        "check": [],
    },
    {
        "script": "examples/grmhd/michel_spherical_accretion.py",
        "options": [],
        "arguments": [],
        "mirrors": AXIS_SWAPS_3D,
        "check": ["--symmetric-tensor-fields", "gij"],
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


def is_symmetric(script, options, arguments, mirrors, check):
    output = OUTPUT / "-".join([Path(script).stem] + options + arguments).replace(
        "--", ""
    )
    shutil.rmtree(output, ignore_errors=True)
    build(script, options)
    run(script, ["-o", str(output), "-nt", "40", "-ns", "4", "-pdp"] + arguments)
    try:
        check_symmetry.main([str(output / "patch"), "--mirrors"] + mirrors + check)
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
            symmetric = is_symmetric(**scenario)
        except Exception:
            traceback.print_exc()
            symmetric = False
        verdicts.append(
            (
                " ".join(
                    [scenario["script"]] + scenario["options"] + scenario["arguments"]
                ),
                symmetric,
            )
        )
    for label, symmetric in verdicts:
        print("{}: {}".format(label, "symmetric" if symmetric else "asymmetric"))
    if not all(symmetric for _, symmetric in verdicts):
        sys.exit("The symmetry test fails.")
