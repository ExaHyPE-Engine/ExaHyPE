# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

from pathlib import Path

import exahype2

END_TIME = 2

parser = argparse.ArgumentParser()
parser.add_argument("--precision", choices=("double", "float"), default="double")
arguments, _ = parser.parse_known_args()

project = exahype2.Project(
    namespace=["examples", "euler"],
    project_name="RayleighTaylor",
    dimensions=2,
    offset=[-0.5, 0, -0.5],
    size=[1, 2, 1],
    periodic_bc=[True, False, True],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=2,
    max_grid_levels=3,
    directory=str(Path(__file__).resolve().parent),
)
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))
project.add_runtime_parameter("gamma", "double", 1.4)
project.add_runtime_parameter("detailTolerance", "double", 2.0e-2)

solver = exahype2.dg.ader.Solver(
    name="RayleighTaylor",
    order=3,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    precision=arguments.precision,
)
solver.set_pde_terms(
    max_eigenvalue=True,
    flux=True,
    source=True,
    refinement_criterion=True,
    initial_condition=True,
    boundary_conditions=True,
)
solver.add_subcell_limiter(
    exahype2.fv.Godunov(),
    number_of_dmp_observables=project.dimensions + 2,
    dmp_relaxation_parameter=1.0e-4,
    dmp_differences_scaling=1.0e-3,
)
solver.set_pde_terms(physical_admissibility=True, dmp_observables=True)
project.output.makefile.add_h_file("RayleighTaylor.cpph")
project.add_solver(solver)
project.generate_and_build()
