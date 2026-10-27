# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from pathlib import Path

import exahype2

project = exahype2.Project(
    namespace=["examples", "fortran"],
    project_name="FortranCoupling",
    dimensions=2,
    offset=[-2.0, -2.0, -2.0],
    size=[4.0, 4.0, 4.0],
    periodic_bc=[False, False, False],
    min_end_time=0.2,
    max_end_time=0.2,
    min_grid_levels=3,
    max_grid_levels=3,
    directory=str(Path(__file__).resolve().parent),
)
if project.dimensions != 2:
    raise ValueError("The Fortran example runs in 2D only.")

solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=11,
    unknowns={"h": 1, "hu": 2, "b": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    scheme=exahype2.fv.Godunov(),
)
solver.set_pde_terms(
    initial_condition=True,
    boundary_conditions=True,
    max_eigenvalue=True,
    riemann_solver=True,
)
project.add_solver(solver)

project.output.makefile.add_cpp_file("FVSolver.cpp")
project.output.makefile.add_fortran_file("rpn2_geoclaw.f90")
project.generate_and_build()
