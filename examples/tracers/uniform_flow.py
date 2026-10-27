# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import itertools

from pathlib import Path

import exahype2

END_TIME = 2
FLOW_VELOCITY = [1.0, 0.5, 0.25]
MASS = 1.0
RADIUS = 0.01

parser = argparse.ArgumentParser()
parser.add_argument("--solver", choices=("fv", "aderdg", "rkdg"), default="fv")
parser.add_argument("--precision", choices=("double", "float"), default="double")
arguments, _ = parser.parse_known_args()

project = exahype2.Project(
    namespace=["examples", "tracers"],
    project_name="UniformFlow",
    dimensions=2,
    offset=[0, 0, 0],
    size=[1, 1, 1],
    periodic_bc=[True, True, True],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=3,
    max_grid_levels=3,
    directory=str(Path(__file__).resolve().parent),
)
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))
velocity = FLOW_VELOCITY[: project.dimensions]
project.add_constant(
    "FlowVelocity",
    "{" + ", ".join(str(component) for component in velocity) + "}",
    cpp_type="tarch::la::Vector<DIMENSIONS, double>",
)

unknowns = {"rho": 1, "v": project.dimensions}
time_stepping = exahype2.TimeStepping.Adaptive(0.5)
if arguments.solver == "fv":
    solver = exahype2.fv.Solver(
        name="UniformFlow",
        patch_size=4,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=time_stepping,
        scheme=exahype2.fv.Godunov(),
        precision=arguments.precision,
    )
elif arguments.solver == "aderdg":
    solver = exahype2.dg.ader.Solver(
        name="UniformFlow",
        order=3,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=time_stepping,
        precision=arguments.precision,
    )
else:
    solver = exahype2.dg.rk.Solver(
        name="UniformFlow",
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
    particle_analytical_solution=True,
)

particles_per_axis = 128 if project.dimensions == 2 else 32
particles = project.add_tracer(
    name="Particles", particle_attributes=3 + 4 * project.dimensions
)
for index in itertools.product(range(particles_per_axis), repeat=project.dimensions):
    position = [(i + 0.5) / particles_per_axis for i in index]
    data = (
        [1.0]
        + velocity
        + velocity
        + [0.0] * project.dimensions
        + [MASS, RADIUS]
        + position
    )
    particles.insert_with_data([position + data])
solver.trace(
    particles,
    forces=exahype2.particles.Forces(
        interpolation_scheme=2,
        integration_scheme=1,
        force_method=4,
        fields_to_interpolate=list(range(1 + project.dimensions)),
    ),
)
solver.measure_particle_error(particles, file="Error", every=END_TIME / 100)
project.output.makefile.add_h_file("UniformFlow.cpph")
project.add_solver(solver)
project.generate_and_build()
