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
    namespace=["examples", "acoustic"],
    project_name="PointSource",
    dimensions=2,
    offset=[-1, -1, -1],
    size=[2, 2, 2],
    periodic_bc=[False, False, False],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=2,
    max_grid_levels=4,
    directory=str(Path(__file__).resolve().parent),
)
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))
project.add_runtime_parameter("density", "double", 1.0)
project.add_runtime_parameter("waveSpeed", "double", 1.0)
project.add_runtime_parameter("detailTolerance", "double", 1.0e-2)
project.add_runtime_parameter("staticRefinement", "bool", False)

solver = exahype2.dg.ader.Solver(
    name="PointSource",
    order=3,
    unknowns={"p": 1, "u": project.dimensions},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.9),
    linear=True,
    point_sources=1,
    precision=arguments.precision,
)
solver.set_pde_terms(
    max_eigenvalue=True,
    flux=True,
    refinement_criterion=True,
    initial_condition=True,
    boundary_conditions=True,
    point_source=True,
    point_source_locations=True,
)
project.output.makefile.add_h_file("PointSource.cpph")
project.add_solver(solver)
project.generate_and_build()
