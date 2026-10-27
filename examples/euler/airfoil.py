# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

from pathlib import Path

import exahype2

END_TIME = 10

parser = argparse.ArgumentParser()
parser.add_argument("--precision", choices=("double", "float"), default="double")
arguments, _ = parser.parse_known_args()

project = exahype2.Project(
    namespace=["examples", "euler"],
    project_name="Airfoil",
    dimensions=2,
    offset=[-10, -60, -60],
    size=[120, 120, 120],
    periodic_bc=[False, False, True],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=3,
    max_grid_levels=4,
    directory=str(Path(__file__).resolve().parent),
)
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))
project.add_runtime_parameter("gamma", "double", 1.4)

solver = exahype2.dg.ader.Solver(
    name="Airfoil",
    order=3,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables={"volumeFraction": 1},
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    precision=arguments.precision,
)
solver.set_pde_terms(
    max_eigenvalue=True,
    flux=True,
    refinement_criterion=True,
    initial_condition=True,
    boundary_conditions=True,
)
solver.add_subcell_limiter(exahype2.fv.Godunov())
solver.set_pde_terms(physical_admissibility=True, subcell_riemann_solver=True)
project.output.makefile.add_h_file("Airfoil.cpph")
project.add_solver(solver)
project.generate_and_build()
