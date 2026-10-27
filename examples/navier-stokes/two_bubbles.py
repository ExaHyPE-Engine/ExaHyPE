# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

from pathlib import Path

import exahype2

END_TIME = 700
GAMMA = 1.4
GAS_CONSTANT = 287.058

parser = argparse.ArgumentParser()
parser.add_argument("--solver", choices=("aderdg", "rkdg"), default="aderdg")
parser.add_argument("--precision", choices=("double", "float"), default="double")
arguments, _ = parser.parse_known_args()

project = exahype2.Project(
    namespace=["examples", "navierstokes"],
    project_name="TwoBubbles",
    dimensions=2,
    offset=[-500, -500, -500],
    size=[1000, 1000, 1000],
    periodic_bc=[False, False, False],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=3,
    max_grid_levels=3,
    directory=str(Path(__file__).resolve().parent),
)
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))
project.add_runtime_parameter("gamma", "double", GAMMA)
project.add_runtime_parameter(
    "isochoricHeatCapacity", "double", GAS_CONSTANT / (GAMMA - 1)
)
project.add_runtime_parameter("gasConstant", "double", GAS_CONSTANT)
project.add_runtime_parameter("referenceViscosity", "double", 0.01)
project.add_runtime_parameter("prandtlNumber", "double", 0.71)
project.add_runtime_parameter("gravity", "double", 9.81)
project.add_runtime_parameter("departureTolerance", "double", 1.0e-4)

unknowns = {"rho": 1, "j": project.dimensions, "E": 1}
auxiliary_variables = {"height": 1, "backgroundstate": 2}
if arguments.solver == "aderdg":
    solver = exahype2.dg.ader.Solver(
        name="TwoBubbles",
        order=3,
        unknowns=unknowns,
        auxiliary_variables=auxiliary_variables,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        precision=arguments.precision,
    )
else:
    solver = exahype2.dg.rk.Solver(
        name="TwoBubbles",
        rk_order=2,
        dg_order=2,
        unknowns=unknowns,
        auxiliary_variables=auxiliary_variables,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        precision=arguments.precision,
    )
solver.set_pde_terms(
    max_eigenvalue=True,
    diffusive_flux=True,
    riemann_solver=True,
    refinement_criterion=True,
    initial_condition=True,
    boundary_conditions=True,
    source=True,
)
project.output.makefile.add_h_file("TwoBubbles.cpph")
project.add_solver(solver)
project.generate_and_build()
