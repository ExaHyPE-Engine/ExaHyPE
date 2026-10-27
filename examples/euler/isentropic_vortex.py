# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

from pathlib import Path

import exahype2

END_TIME = 10

parser = argparse.ArgumentParser()
parser.add_argument("--solver", choices=("fv", "aderdg", "rkdg"), default="fv")
parser.add_argument("--precision", choices=("double", "float"), default="double")
arguments, _ = parser.parse_known_args()

project = exahype2.Project(
    namespace=["examples", "euler"],
    project_name="IsentropicVortex",
    dimensions=2,
    offset=[-5, -5, -5],
    size=[10, 10, 10],
    periodic_bc=[True, True, True],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=3,
    max_grid_levels=3,
    directory=str(Path(__file__).resolve().parent),
)
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))
project.add_runtime_parameter("gamma", "double", 1.4)
project.add_runtime_parameter("detailTolerance", "double", 2.0e-2)

unknowns = {"rho": 1, "rhoU": project.dimensions, "rhoE": 1}
if arguments.solver == "fv":
    solver = exahype2.fv.Solver(
        name="IsentropicVortex",
        patch_size=4,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.5),
        scheme=exahype2.fv.Godunov(),
        precision=arguments.precision,
    )
elif arguments.solver == "aderdg":
    solver = exahype2.dg.ader.Solver(
        name="IsentropicVortex",
        order=3,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        precision=arguments.precision,
    )
else:
    solver = exahype2.dg.rk.Solver(
        name="IsentropicVortex",
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
    analytical_solution=True,
)
solver.measure_error(file="Error", every=END_TIME / 100)
project.output.makefile.add_h_file("IsentropicVortex.cpph")
project.add_solver(solver)
project.generate_and_build()
