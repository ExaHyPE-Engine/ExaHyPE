# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.5)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=2)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

project = exahype2.Project(
    namespace=["tests", "exahype2", "fd"],
    project_name=".",
    dimensions=2,
    size=[0.1, 0.1, 0.1],
    offset=[0.0, 0.0, 0.0],
    periodic_BC=[False, False, False],
    executable="ExaHyPE",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

"""
/****************************************************************************************************************************************
 * Aluminium                                                                                                                            *
 *                                                                                                                                      *
 * References:                                                                                                                          *
 * Zhang A, Li Y. Thermal Conductivity of Aluminum Alloys-A Review. Materials (Basel). 2023 Apr 8;16(8):2972. doi: 10.3390/ma16082972.  *
 * https://webbook.nist.gov/cgi/inchi/InChI%3D1S/Al, accessed on 12/09/2025                                                             *
 ****************************************************************************************************************************************/
"""

MOLAR_HEAT_CAPACITY = 25.78  # in J / (mol * K)
MOLAR_WEIGHT = 0.0269815386  # in kg / mol
THERMAL_CONDUCTIVITY = 237  # in W / (m * K)
DENSITY = 2700  # in kg / m^3
SPECIFIC_HEAT_CAPACITY = MOLAR_HEAT_CAPACITY / MOLAR_WEIGHT  # in J / (kg * K)
THERMAL_DIFFUSIVITY = THERMAL_CONDUCTIVITY / (
    SPECIFIC_HEAT_CAPACITY * DENSITY
)  # in m^2 / s

initial_conditions = """
  Q[Shortcuts::T]     = 400; // in K
  Q[Shortcuts::kappa] = THERMAL_DIFFUSIVITY;
"""

boundary_conditions = """
  // normal == 0 => left
  // normal == 1 => bottom
  // normal == 2 => right
  // normal == 3 => top
  if (normal == 0) {
    // Heat sink
    Qoutside0[Shortcuts::T]     = 300; // in K
    Qoutside0[Shortcuts::kappa] = Qinside[Shortcuts::kappa];
  } else {
    // Insulation everywhere else
    Qoutside0[Shortcuts::T]     = Qinside[Shortcuts::T];
    Qoutside0[Shortcuts::kappa] = Qinside[Shortcuts::kappa];
  }
"""

fd_solver = exahype2.solvers.fd.GlobalFixedTimeStep(
    name="FDSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"T": 1},
    auxiliary_variables={"kappa": 1},
    normalised_time_step_size=(
        0.4
        * (1.1 * 0.1 / (3.0**args.grid_levels) / args.degrees_of_freedom) ** 2
        / THERMAL_DIFFUSIVITY
    ),
    order=1,
)

fd_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    solver="""
  {{COMPUTE_PRECISION}} temperatureLaplacian = 0.0;
  // x-direction
  temperatureLaplacian += (QRight0[Shortcuts::T] - 2.0 * Q[Shortcuts::T] + QLeft0[Shortcuts::T]) / (h(0) * h(0));
  // y-direction
  temperatureLaplacian += (QTop0[Shortcuts::T] - 2.0 * Q[Shortcuts::T] + QDown0[Shortcuts::T]) / (h(1) * h(1));
#if DIMENSIONS == 3
  // z-direction
  temperatureLaplacian += (QFront0[Shortcuts::T] - 2.0 * Q[Shortcuts::T] + QBack0[Shortcuts::T]) / (h(2) * h(2));
#endif

  QNew[Shortcuts::T] = Q[Shortcuts::T] + dt * Q[Shortcuts::kappa] * temperatureLaplacian;
""",
)

project.add_solver(fd_solver)

project = project.generate_Peano4_project()
project.output.makefile.add_compile_definition(f"""THERMAL_DIFFUSIVITY={THERMAL_DIFFUSIVITY}""")
project.generate_and_build()
