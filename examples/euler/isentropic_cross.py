# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

from pathlib import Path

import exahype2

END_TIME = 0.3

parser = argparse.ArgumentParser()
parser.add_argument("--solver", choices=("fv", "aderdg", "rkdg"), default="fv")
parser.add_argument("--scheme", choices=("godunov", "muscl-hancock"), default="godunov")
parser.add_argument("--precision", choices=("double", "float"), default="double")
arguments, _ = parser.parse_known_args()

project = exahype2.Project(
    namespace=["examples", "euler"],
    project_name="IsentropicCross",
    dimensions=2,
    offset=[-0.5, -0.5, -0.5],
    size=[1, 1, 1],
    periodic_bc=[True, True, True],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=2,
    max_grid_levels=4,
    directory=str(Path(__file__).resolve().parent),
)
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))
project.add_runtime_parameter("gamma", "double", 1.4)
project.add_runtime_parameter("refineTolerance", "double", 0.05)
project.add_runtime_parameter("eraseTolerance", "double", 0.02)

unknowns = {"rho": 1, "rhoU": project.dimensions, "rhoE": 1}
if arguments.solver == "fv":
    solver = exahype2.fv.Solver(
        name="IsentropicCross",
        patch_size=4,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        scheme=(
            exahype2.fv.Godunov()
            if arguments.scheme == "godunov"
            else exahype2.fv.MusclHancock()
        ),
        precision=arguments.precision,
    )
elif arguments.solver == "aderdg":
    solver = exahype2.dg.ader.Solver(
        name="IsentropicCross",
        order=3,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        precision=arguments.precision,
    )
else:
    solver = exahype2.dg.rk.Solver(
        name="IsentropicCross",
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
    refinement_criterion=True,
    initial_condition=True,
)
project.output.makefile.add_h_file("IsentropicCross.cpph")
project.add_solver(solver)
project.generate_and_build()
