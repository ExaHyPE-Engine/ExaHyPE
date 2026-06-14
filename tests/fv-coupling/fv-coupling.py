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
    parser.add_argument("-et", "--end-time", type=float, default=0.01)
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
    size=[1.0, 1.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_BC=[False, False, False],
    executable="ExaHyPE",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

#########
# EULER #
#########

euler_unknowns = {
    "rho": 1,
    "rhoU": 1,
    "rhoV": 1,
}
if project.dimensions == 3:
    euler_unknowns["rhoW"] = 1
euler_unknowns["rhoE"] = 1

euler_initial_conditions = """
  Q[Shortcuts::rho]  = 0.1;
  Q[Shortcuts::rhoU] = 0.0;
  Q[Shortcuts::rhoV] = 0.0;
#if DIMENSIONS == 3
  Q[Shortcuts::rhoW] = 0.0;
#endif
  Q[Shortcuts::rhoE] = ((x(0) < 0.5) ? (1.01) : (1.0));
"""

euler_boundary_conditions = """
  // Reflective boundary conditions
  Qoutside[Shortcuts::rho]  = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU] = -Qinside[Shortcuts::rhoU];
  Qoutside[Shortcuts::rhoV] = -Qinside[Shortcuts::rhoV];
#if DIMENSIONS == 3
  Qoutside[Shortcuts::rhoW] = -Qinside[Shortcuts::rhoW];
#endif
  Qoutside[Shortcuts::rhoE] = Qinside[Shortcuts::rhoE];
"""

compute_primitive_variables = r"""
  const auto irho = 1.0 / Q[Shortcuts::rho];
  const auto u0 = Q[Shortcuts::rhoU + 0] * irho;
  const auto u1 = Q[Shortcuts::rhoU + 1] * irho;
#if DIMENSIONS == 3
  const auto u2 = Q[Shortcuts::rhoU + 2] * irho;
#endif

#if DIMENSIONS == 3
  const auto uSq = u0 * u0 + u1 * u1 + u2 * u2;
  const auto u_n = (normal == 0) ? u0 : (normal == 1) ? u1 : u2;
#else
  const auto uSq = u0 * u0 + u1 * u1;
  const auto u_n = (normal == 0) ? u0 : u1;
#endif

  const auto internalE = Q[Shortcuts::rhoE] - 0.5 * Q[Shortcuts::rho] * uSq;
  const auto p = (GAMMA - 1.0) * internalE;
"""

euler_max_eigenvalue = r"""
  {compute_primitive_variables}
  const auto speedOfSound = std::sqrt(GAMMA * p * irho);
  auto result = std::fmax(0.0, std::fabs(u_n - speedOfSound));
  result = std::fmax(result, std::fabs(u_n + speedOfSound));
  return result;
""".format(
    compute_primitive_variables=compute_primitive_variables
)

euler_flux = r"""
  {compute_primitive_variables}

  F[Shortcuts::rho] = Q[Shortcuts::rhoU + normal];

  F[Shortcuts::rhoU + 0] = Q[Shortcuts::rhoU + 0] * u_n;
  F[Shortcuts::rhoU + 1] = Q[Shortcuts::rhoU + 1] * u_n;
#if DIMENSIONS == 3
  F[Shortcuts::rhoU + 2] = Q[Shortcuts::rhoU + 2] * u_n;
#endif

  F[Shortcuts::rhoU + normal] += p;

  F[Shortcuts::rhoE] = (Q[Shortcuts::rhoE] + p) * u_n;
""".format(
    compute_primitive_variables=compute_primitive_variables
)

euler_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="EulerFVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns=euler_unknowns,
    auxiliary_variables=0,
    time_step_relaxation=0.5,
)

euler_solver.set_implementation(
    initial_conditions=euler_initial_conditions,
    boundary_conditions=euler_boundary_conditions,
    flux=euler_flux,
    max_eigenvalue=euler_max_eigenvalue,
)

#############
# ADVECTION #
#############

advection_unknowns = {"scalar": 1}  # Represents some material that is "moved" (advected) through the velocity field computed by the Euler equations
advection_auxiliary_variables = {
    "rho": 1,
    "rhoU": 1,
    "rhoV": 1,
}
if project.dimensions == 3:
    advection_auxiliary_variables["rhoW"] = 1

advection_initial_conditions = """
  Q[Shortcuts::scalar]  = ((x(0) < 0.5) ? (50) : 0.0);
  Q[Shortcuts::rho]     = 0.1;
  Q[Shortcuts::rhoU]    = 0.0;
  Q[Shortcuts::rhoV]    = 0.0;
#if DIMENSIONS == 3
  Q[Shortcuts::rhoW]    = 0.0;
#endif
"""

advection_boundary_conditions = """
  // Reflective boundary conditions
  Qoutside[Shortcuts::scalar] = Qinside[Shortcuts::scalar];
  Qoutside[Shortcuts::rho]    = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU]   = -Qinside[Shortcuts::rhoU];
  Qoutside[Shortcuts::rhoV]   = -Qinside[Shortcuts::rhoV];
#if DIMENSIONS == 3
  Qoutside[Shortcuts::rhoW]   = -Qinside[Shortcuts::rhoW];
#endif
"""

advection_flux = """
  const auto irho = 1.0 / Q[Shortcuts::rho];
  const auto u_n = Q[Shortcuts::rhoU + normal] * irho;
  F[Shortcuts::scalar] = Q[Shortcuts::scalar] * u_n;
"""

advection_max_eigenvalue = """
  if (dt == 0) {
    // Return dummy value initially because all velocities are 0
    return 1.0;
  }
  // We could also return a dummy value here but this way, mistakes are more obvious.
  const auto irho = 1.0 / Q[Shortcuts::rho];
  const auto u_n = Q[Shortcuts::rhoU + normal] * irho;
  return u_n;
"""

advection_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="AdvectionFVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns=advection_unknowns,
    auxiliary_variables=advection_auxiliary_variables,
    time_step_relaxation=0.5,
    solvers_before_in_coupling=[euler_solver],
)

advection_solver.set_implementation(
    initial_conditions=advection_initial_conditions,
    boundary_conditions=advection_boundary_conditions,
    flux=advection_flux,
    max_eigenvalue=advection_max_eigenvalue,
)

############
# COUPLING #
############

advection_solver._compute_time_step_size = (
    """
  const double timeStepSize = repositories::"""
    + euler_solver.get_name_of_global_instance()
    + """.getAdmissibleTimeStepSize();
"""
)

number_of_variables_euler = euler_solver.auxiliary_variables + euler_solver.unknowns
number_of_variables_advection = (
    advection_solver.auxiliary_variables + advection_solver.unknowns
)

# TODO: This is really ugly!
euler_solver.postprocess_updated_patch += f"""// Copy density and momenta
  ::toolbox::blockstructured::copyUnknown(
    {euler_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{euler_solver.current_unknown_identifier()}.data(), // source
    {euler_solver.variable_names.index("rho")}, // density in source
    {number_of_variables_euler}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{advection_solver.current_unknown_identifier()}.data(), // dest
    {advection_solver.variable_names.index("rho")}, // density in dest
    {number_of_variables_advection}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );

  ::toolbox::blockstructured::copyUnknown(
    {euler_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{euler_solver.current_unknown_identifier()}.data(), // source
    {euler_solver.variable_names.index("rhoU")}, // x-momentum in source
    {number_of_variables_euler}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{advection_solver.current_unknown_identifier()}.data(), // dest
    {advection_solver.variable_names.index("rhoU")}, // x-momentum in dest
    {number_of_variables_advection}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );

  ::toolbox::blockstructured::copyUnknown(
    {euler_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{euler_solver.current_unknown_identifier()}.data(), // source
    {euler_solver.variable_names.index("rhoV")}, // y-momentum in source
    {number_of_variables_euler}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{advection_solver.current_unknown_identifier()}.data(), // dest
    {advection_solver.variable_names.index("rhoV")}, // y-momentum in dest
    {number_of_variables_advection}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );
"""

if project.dimensions == 3:
    euler_solver.postprocess_updated_patch += f"""
#if DIMENSIONS == 3
  ::toolbox::blockstructured::copyUnknown(
    {euler_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{euler_solver.current_unknown_identifier()}.data(), // source
    {euler_solver.variable_names.index("rhoW")}, // z-momentum in source
    {number_of_variables_euler}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{advection_solver.current_unknown_identifier()}.data(), // dest
    {advection_solver.variable_names.index("rhoW")}, // z-momentum in dest
    {number_of_variables_advection}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );
#endif
"""

euler_solver.add_user_action_set_includes('#include "toolbox/blockstructured/Copy.h"\n')

euler_solver.last_solver = advection_solver

###########
# PROJECT #
###########

project.add_solver(euler_solver)
project.add_solver(advection_solver)

project = project.generate_Peano4_project(verbose=False)
project.output.makefile.add_CXX_flag("-DGAMMA=1.4")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
