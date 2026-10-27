# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

from pathlib import Path

import exahype2

END_TIME = 0.15

parser = argparse.ArgumentParser()
parser.add_argument("--precision", choices=("double", "float"), default="double")
arguments, _ = parser.parse_known_args()

project = exahype2.Project(
    namespace=["examples", "shallowwater"],
    project_name="DryRing",
    dimensions=2,
    offset=[-1, -1, -1],
    size=[2, 2, 2],
    periodic_bc=[False, False, False],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=2,
    max_grid_levels=2,
    directory=str(Path(__file__).resolve().parent),
)
if project.dimensions != 2:
    parser.error("The shallow water equations run in 2D only.")
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))
project.add_runtime_parameter("gravity", "double", 9.81)
project.add_runtime_parameter("dryTolerance", "double", 1.0e-6)
project.add_runtime_parameter("detailTolerance", "double", 1.0e-2)

solver = exahype2.dg.ader.Solver(
    name="DryRing",
    order=3,
    unknowns={"h": 1, "hv": 2, "b": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    precision=arguments.precision,
)
solver.add_subcell_limiter(exahype2.fv.Godunov())
solver.set_pde_terms(
    max_eigenvalue=True,
    flux=True,
    ncp=True,
    refinement_criterion=True,
    initial_condition=True,
    boundary_conditions=True,
    physical_admissibility=True,
)
project.output.makefile.add_h_file("DryRing.cpph")
project.add_solver(solver)
project.generate_and_build()
