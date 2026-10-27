# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

from pathlib import Path

import exahype2

END_TIME = 2
MASSES = [0.01, 0.1, 0.5, 1.0]
RADIUS = 0.1
START_HEIGHT = 2.5

parser = argparse.ArgumentParser()
parser.add_argument("--solver", choices=("fv", "aderdg", "rkdg"), default="fv")
parser.add_argument("--precision", choices=("double", "float"), default="double")
arguments, _ = parser.parse_known_args()

project = exahype2.Project(
    namespace=["examples", "tracers"],
    project_name="SettlingSpheres",
    dimensions=2,
    offset=[0, 0, 0],
    size=[3, 3, 3],
    periodic_bc=[False, False, False],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=3,
    max_grid_levels=3,
    directory=str(Path(__file__).resolve().parent),
)
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))

time_stepping = exahype2.TimeStepping.Fixed(2.5e-4)
unknowns = {"rho": 1, "v": project.dimensions}
if arguments.solver == "fv":
    solver = exahype2.fv.Solver(
        name="SettlingSpheres",
        patch_size=4,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=time_stepping,
        scheme=exahype2.fv.Godunov(),
        precision=arguments.precision,
    )
elif arguments.solver == "aderdg":
    solver = exahype2.dg.ader.Solver(
        name="SettlingSpheres",
        order=3,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=time_stepping,
        precision=arguments.precision,
    )
else:
    solver = exahype2.dg.rk.Solver(
        name="SettlingSpheres",
        rk_order=2,
        dg_order=2,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=time_stepping,
        precision=arguments.precision,
    )
solver.set_pde_terms(
    max_eigenvalue=True,
    flux=True,
    refinement_criterion=True,
    initial_condition=True,
    boundary_conditions=True,
    particle_analytical_solution=True,
)

spheres = project.add_tracer(
    name="Spheres", particle_attributes=3 + 3 * project.dimensions
)
for number, mass in enumerate(MASSES):
    position = [0.75 + 0.5 * number] + [1.5] * (project.dimensions - 2) + [START_HEIGHT]
    spheres.insert_with_data(
        [position + [0.0] * (1 + 3 * project.dimensions) + [mass, RADIUS]]
    )
solver.trace(
    spheres,
    forces=exahype2.particles.Forces(
        interpolation_scheme=2,
        integration_scheme=2,
        force_method=14,
        fields_to_interpolate=list(range(1 + project.dimensions)),
    ),
)
solver.measure_particle_error(spheres, file="Error", every=END_TIME / 100)
project.output.makefile.add_h_file("SettlingSpheres.cpph")
project.add_solver(solver)
project.generate_and_build()
