# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

from pathlib import Path

import exahype2

END_TIME = 1

parser = argparse.ArgumentParser()
parser.add_argument("--solver", choices=("fv", "aderdg", "rkdg"), default="fv")
parser.add_argument("--precision", choices=("double", "float"), default="double")
arguments, _ = parser.parse_known_args()

project = exahype2.Project(
    namespace=["examples", "advection"],
    project_name="PeriodicBump",
    dimensions=2,
    offset=[0, 0, 0],
    size=[1, 1, 1],
    periodic_bc=[True, True, True],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=2,
    max_grid_levels=4,
    directory=str(Path(__file__).resolve().parent),
)
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))
project.add_runtime_parameter("detailTolerance", "double", 1.0e-2)
project.add_runtime_parameter("bumpRadius", "double", 0.3)
project.add_runtime_parameter("smoothBump", "bool", True)

unknowns = {"q": 1}
if arguments.solver == "fv":
    solver = exahype2.fv.Solver(
        name="PeriodicBump",
        patch_size=4,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.5),
        scheme=exahype2.fv.Godunov(),
        precision=arguments.precision,
    )
elif arguments.solver == "aderdg":
    solver = exahype2.dg.ader.Solver(
        name="PeriodicBump",
        order=3,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        linear=True,
        precision=arguments.precision,
    )
else:
    solver = exahype2.dg.rk.Solver(
        name="PeriodicBump",
        rk_order=2,
        dg_order=2,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        linear=True,
        precision=arguments.precision,
    )
solver.set_pde_terms(
    max_eigenvalue=True,
    flux=True,
    refinement_criterion=True,
    initial_condition=True,
    analytical_solution=True,
)
solver.measure_error(file="Error", every=END_TIME / 100)
project.output.makefile.add_h_file("PeriodicBump.cpph")
project.add_solver(solver)
project.generate_and_build()
