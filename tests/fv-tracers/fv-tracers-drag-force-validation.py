# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""
A test of particles that settle under gravity, buoyancy and drag forces.
"""

import argparse
from pathlib import Path

import exahype2

interpolation_methods = {
    "None": 0,
    "Constant": 1,
    "Linear": 2,
}
integration_schemes = {
    "Static": 0,
    "Euler": 1,
    "SemiEuler": 2,
    "Verlet": 3,
    "Midpoint": 4,
    "RK3": 5,
    "RK4": 6,
}


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="Show this help message and continue with project help.",
    )
    parser.add_argument(
        "-interp",
        "--interpolation",
        choices=interpolation_methods.keys(),
        default="Linear",
    )
    parser.add_argument(
        "-integ",
        "--integration",
        choices=integration_schemes.keys(),
        default="SemiEuler",
    )
    parser.add_argument("-et", "--end-time", type=float, default=2.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name=".",
    dimensions=2,
    size=[3.0, 3.0, 3.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != 2:
    raise ValueError("The drag force tracer test runs in 2D only.")

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
  Q[0] = 1.0;
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
"""

flux = """
  for (int i = 0; i < NumberOfUnknowns; i++) {
    F[i] = 0.0;
  }
"""

max_eigenvalue = """
  return 1.0;
"""

analytical_solution = """
  // A sphere sinks under gravity, buoyancy and Stokes drag through the resting fluid. The entries
  // of solution are the position and the tracer data of tracer_attributes.
  constexpr double GravitationalAcceleration = 1.0;
  constexpr double DynamicViscosity          = 1.0;
  constexpr double FluidDensity              = 1.0;

  const double mass     = particle->getData(7);
  const double radius   = particle->getData(8);
  const double initialX = particle->getData(9);
  const double initialY = particle->getData(10);

  const double volume          = (4.0 / 3.0) * tarch::la::PI * radius * radius * radius;
  const double particleDensity = mass / volume;
  const double acceleration    = (particleDensity - FluidDensity) * GravitationalAcceleration / particleDensity;
  const double relaxationTime  = mass / (6.0 * tarch::la::PI * DynamicViscosity * radius);
  const double terminalSpeed   = acceleration * relaxationTime;
  const double decay           = std::exp(-t / relaxationTime);

  solution[0]  = initialX;
  solution[1]  = initialY + (-terminalSpeed * t + terminalSpeed * relaxationTime * (1.0 - decay));
  solution[2]  = FluidDensity;
  solution[3]  = 0.0;
  solution[4]  = 0.0;
  solution[5]  = 0.0;
  solution[6]  = -terminalSpeed * (1.0 - decay);
  solution[7]  = 0.0;
  solution[8]  = -acceleration * decay;
  solution[9]  = mass;
  solution[10] = radius;
  solution[11] = initialX;
  solution[12] = initialY;
"""

fv_solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns=3,
    auxiliary_variables=0,
    # The time step stays below the drag relaxation time of the lightest
    # particle on grid level 1 with the default patch size.
    time_stepping=exahype2.TimeStepping.Adaptive(0.1),
    scheme=exahype2.fv.Godunov(),
)

fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
)

project.add_solver(fv_solver)

tracer_attributes = {
    "unknowns": {
        "rho": 1,
        "fluid_u": 1,
        "fluid_v": 1,
        "u": 1,
        "v": 1,
        "a_x": 1,
        "a_y": 1,
        "mass": 1,
        "radius": 1,
        "initial_x": 1,
        "initial_y": 1,
    }
}

tracer_particles = project.add_tracer(
    name="Tracers", particle_attributes=tracer_attributes
)

tracer_particles.insert_with_data(
    """
  {
    2.0, 2.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.01, 0.1, 2.0, 2.5,
    1.5, 2.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.1,  0.1, 1.5, 2.5,
    1.0, 2.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5,  0.1, 1.0, 2.5,
    0.5, 2.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0,  0.1, 0.5, 2.5
  }
""",
    unknowns_per_particle=len(tracer_attributes["unknowns"]),
)

fv_solver.trace(
    tracer_particles,
    forces=exahype2.particles.Forces(
        integration_scheme=integration_schemes[args.integration],
        # The bit mask 14 adds buoyancy, Stokes drag and constant gravity.
        force_method=14,
        interpolation_scheme=interpolation_methods[args.interpolation],
        # The buoyancy and the drag force read the fluid density and the two
        # fluid velocity components from the first three particle attributes.
        fields_to_interpolate="0,1,2",
    ),
)

fv_solver.set_pde_terms(particle_analytical_solution=analytical_solution)
fv_solver.measure_particle_error(tracer_particles, file="Error_" + Path(__file__).name)

project.generate_and_build()
