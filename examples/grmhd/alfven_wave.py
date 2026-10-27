# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import math

from pathlib import Path

import exahype2

END_TIME = 0.1

parser = argparse.ArgumentParser()
parser.add_argument("--solver", choices=("aderdg", "rkdg", "fv"), default="aderdg")
parser.add_argument("--precision", choices=("double", "float"), default="double")
arguments, _ = parser.parse_known_args()

project = exahype2.Project(
    namespace=["examples", "grmhd"],
    project_name="AlfvenWave",
    dimensions=2,
    offset=[0, 0, 0],
    size=[2 * math.pi, 2 * math.pi, 2 * math.pi],
    periodic_bc=[True, True, True],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=1,
    max_grid_levels=1,
    directory=str(Path(__file__).resolve().parent),
)
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))
project.add_runtime_parameter("detailTolerance", "double", 1.0e-2)

unknowns = {
    "rho": 1,
    "vel": 3,
    "E": 1,
    "B": 3,
    "psi": 1,
    "lapse": 1,
    "shift": 3,
    "gij": 6,
}
if arguments.solver == "fv":
    solver = exahype2.fv.Solver(
        name="AlfvenWave",
        patch_size=4,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.5),
        scheme=exahype2.fv.Godunov(),
        precision=arguments.precision,
    )
elif arguments.solver == "aderdg":
    solver = exahype2.dg.ader.Solver(
        name="AlfvenWave",
        order=5,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        precision=arguments.precision,
    )
else:
    solver = exahype2.dg.rk.Solver(
        name="AlfvenWave",
        rk_order=2,
        dg_order=2,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        precision=arguments.precision,
    )
solver.set_pde_terms(
    max_eigenvalue=True,
    flux=True,
    ncp=True,
    refinement_criterion=True,
    initial_condition=True,
    analytical_solution=True,
)
solver.measure_error(file="Error", every=END_TIME / 100)
project.output.makefile.add_h_file("AlfvenWave.cpph")
project.add_solver(solver)
project.generate_and_build()
