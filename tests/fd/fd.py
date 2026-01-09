# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

parser = exahype2.ArgumentParser("ExaHyPE 2 - Finite Differences Testing Script")

parser.set_defaults(
    min_depth=-1,
    degrees_of_freedom=16,
    end_time=0.5,
    time_step_relaxation=0.4,
    periodic_boundary_conditions_x=False,
    periodic_boundary_conditions_y=False,
    periodic_boundary_conditions_z=False,
)
args = parser.parse_args()

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

max_h = 0.0007
min_h = 0.0007

fd_solver = exahype2.solvers.fd.GlobalFixedTimeStep(
    name="FDSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"T": 1},
    auxiliary_variables={"kappa": 1},
    min_h=min_h,
    max_h=max_h,
    normalised_time_step_size=(
        args.time_step_relaxation * (min_h) ** 2 / THERMAL_DIFFUSIVITY
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

project = exahype2.Project(
    namespace=["tests", "exahype2", "fd"],
    project_name=".",
    directory=".",
    executable="ExaHyPE",
)
project.add_solver(fd_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=[0.1, 0.1, 0.1][0 : args.dimensions],
    offset=[0.0, 0.0, 0.0][0 : args.dimensions],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
        args.periodic_boundary_conditions_z,
    ],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.output.makefile.add_CXX_flag(f"""-DTHERMAL_DIFFUSIVITY={THERMAL_DIFFUSIVITY}""")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
