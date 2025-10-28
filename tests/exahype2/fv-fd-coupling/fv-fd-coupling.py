# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

parser = exahype2.ArgumentParser(
    "ExaHyPE 2 - Finite Volumes/Finite Differences Coupling Testing Script"
)

parser.set_defaults(
    min_depth=4,
    degrees_of_freedom=16,
    end_time=5000,
    number_of_snapshots=20,
    periodic_boundary_conditions_x=False,
    periodic_boundary_conditions_y=False,
    periodic_boundary_conditions_z=False,
)
args = parser.parse_args()

size = [1e6, 1e6, 1e6]
max_h = 1.1 * min(size) / 3.0**args.min_depth
min_h = max_h * 3.0 ** (-args.amr_levels)

#########
# EULER #
#########

euler_unknowns = {
    "rho": 1,
    "rhoU": 1,
    "rhoV": 1,
}
if args.dimensions == 3:
    euler_unknowns["rhoW"] = 1
euler_unknowns["rhoE"] = 1

euler_initial_conditions = f"""
  Q[Shortcuts::rho]   = 0.1;
  Q[Shortcuts::rhoU]  = 0.0;
  Q[Shortcuts::rhoV]  = 0.0;
#if DIMENSIONS == 3
  Q[Shortcuts::rhoW]  = 0.0;
#endif
  Q[Shortcuts::rhoE]  = ((x(0) < {size[0] / 2}) ? (1.01) : (1.0));
"""

euler_boundary_conditions = """
  // Reflective boundary conditions
  Qoutside[Shortcuts::rho]   = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU]  = -Qinside[Shortcuts::rhoU];
  Qoutside[Shortcuts::rhoV]  = -Qinside[Shortcuts::rhoV];
#if DIMENSIONS == 3
  Qoutside[Shortcuts::rhoW]  = -Qinside[Shortcuts::rhoW];
#endif
  Qoutside[Shortcuts::rhoE]  = Qinside[Shortcuts::rhoE];
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
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
)

euler_solver.set_implementation(
    initial_conditions=euler_initial_conditions,
    boundary_conditions=euler_boundary_conditions,
    flux=euler_flux,
    max_eigenvalue=euler_max_eigenvalue,
)

#############
# DIFFUSION #
#############

diffusion_unknowns = {
    "rhoU": 1,
    "rhoV": 1,
}
if args.dimensions == 3:
    diffusion_unknowns["rhoW"] = 1
diffusion_unknowns["rhoE"] = 1
diffusion_auxiliary_variables = {
    "rho": 1,
}

diffusion_initial_conditions = f"""
  Q[Shortcuts::rhoU] = 0.0;
  Q[Shortcuts::rhoV] = 0.0;
#if DIMENSIONS == 3
  Q[Shortcuts::rhoW] = 0.0;
#endif
  Q[Shortcuts::rhoE] = ((x(0) < {size[0] / 2}) ? (1.01) : (1.0));
  Q[Shortcuts::rho]  = 0.1;
"""

diffusion_boundary_conditions = """
  // Reflective boundary conditions
  Qoutside0[Shortcuts::rhoU] = -Qinside[Shortcuts::rhoU];
  Qoutside0[Shortcuts::rhoV] = -Qinside[Shortcuts::rhoV];
#if DIMENSIONS == 3
  Qoutside0[Shortcuts::rhoW] = -Qinside[Shortcuts::rhoW];
#endif
  Qoutside0[Shortcuts::rhoE] = Qinside[Shortcuts::rhoE];
  Qoutside0[Shortcuts::rho]  = Qinside[Shortcuts::rho];
"""

diffusion_stencil = """
  {{COMPUTE_PRECISION}} velocityLaplacianU = 0.0;
  {{COMPUTE_PRECISION}} velocityLaplacianV = 0.0;
#if DIMENSIONS == 3
  {{COMPUTE_PRECISION}} velocityLaplacianW = 0.0;
#endif

  const auto irho                 = 1.0 / Q[Shortcuts::rho];
  const auto uSquared             = (Q[Shortcuts::rhoU] * irho) * (Q[Shortcuts::rhoU] * irho);
  const auto vSquared             = (Q[Shortcuts::rhoV] * irho) * (Q[Shortcuts::rhoV] * irho);
#if DIMENSIONS == 3
  const auto wSquared             = (Q[Shortcuts::rhoW] * irho) * (Q[Shortcuts::rhoW] * irho);
  const auto squaredVelocities    = uSquared + vSquared + wSquared;
#else
  const auto squaredVelocities    = uSquared + vSquared;
#endif

  // x-direction
  velocityLaplacianU += ((QRight0[Shortcuts::rhoU] / QRight0[Shortcuts::rho]) - 2 * (Q[Shortcuts::rhoU] * irho) + (QLeft0[Shortcuts::rhoU] / QLeft0[Shortcuts::rho])) / (h(0) * h(0));
  velocityLaplacianV += ((QRight0[Shortcuts::rhoV] / QRight0[Shortcuts::rho]) - 2 * (Q[Shortcuts::rhoV] * irho) + (QLeft0[Shortcuts::rhoV] / QLeft0[Shortcuts::rho])) / (h(0) * h(0));
#if DIMENSIONS == 3
  velocityLaplacianW += ((QRight0[Shortcuts::rhoW] / QRight0[Shortcuts::rho]) - 2 * (Q[Shortcuts::rhoW] * irho) + (QLeft0[Shortcuts::rhoW] / QLeft0[Shortcuts::rho])) / (h(0) * h(0));
#endif

  // y-direction
  velocityLaplacianU += ((QTop0[Shortcuts::rhoU] / QTop0[Shortcuts::rho]) - 2 * (Q[Shortcuts::rhoU] * irho) + (QDown0[Shortcuts::rhoU] / QDown0[Shortcuts::rho])) / (h(0) * h(0));
  velocityLaplacianV += ((QTop0[Shortcuts::rhoV] / QTop0[Shortcuts::rho]) - 2 * (Q[Shortcuts::rhoV] * irho) + (QDown0[Shortcuts::rhoV] / QDown0[Shortcuts::rho])) / (h(0) * h(0));
#if DIMENSIONS == 3
  velocityLaplacianW += ((QTop0[Shortcuts::rhoW] / QTop0[Shortcuts::rho]) - 2 * (Q[Shortcuts::rhoW] * irho) + (QDown0[Shortcuts::rhoW] / QDown0[Shortcuts::rho])) / (h(0) * h(0));
#endif

#if DIMENSIONS == 3
  // z-direction
  velocityLaplacianU += ((QFront0[Shortcuts::rhoU] / QFront0[Shortcuts::rho]) - 2 * (Q[Shortcuts::rhoU] * irho) + (QBack0[Shortcuts::rhoU] / QBack0[Shortcuts::rho])) / (h(0) * h(0));
  velocityLaplacianV += ((QFront0[Shortcuts::rhoV] / QFront0[Shortcuts::rho]) - 2 * (Q[Shortcuts::rhoV] * irho) + (QBack0[Shortcuts::rhoV] / QBack0[Shortcuts::rho])) / (h(0) * h(0));
  velocityLaplacianW += ((QFront0[Shortcuts::rhoW] / QFront0[Shortcuts::rho]) - 2 * (Q[Shortcuts::rhoW] * irho) + (QBack0[Shortcuts::rhoW] / QBack0[Shortcuts::rho])) / (h(0) * h(0));
#endif

  QNew[Shortcuts::rhoU] = Q[Shortcuts::rhoU] + dt * Q[Shortcuts::rho] * KINEMATIC_VISCOSITY * velocityLaplacianU;
  QNew[Shortcuts::rhoV] = Q[Shortcuts::rhoV] + dt * Q[Shortcuts::rho] * KINEMATIC_VISCOSITY * velocityLaplacianV;
#if DIMENSIONS == 3
  QNew[Shortcuts::rhoW] = Q[Shortcuts::rhoW] + dt * Q[Shortcuts::rho] * KINEMATIC_VISCOSITY * velocityLaplacianW;
#endif

  auto temperature = [](const double* Q) {
    const auto irho                 = 1 / Q[Shortcuts::rho];
    // Calculate temperature based on energy density
    const auto uSquared             = (Q[Shortcuts::rhoU] * irho) * (Q[Shortcuts::rhoU] * irho);
    const auto vSquared             = (Q[Shortcuts::rhoV] * irho) * (Q[Shortcuts::rhoV] * irho);
#if DIMENSIONS == 3
    const auto wSquared             = (Q[Shortcuts::rhoW] * irho) * (Q[Shortcuts::rhoW] * irho);
    const auto squaredVelocities    = uSquared + vSquared + wSquared;
#else
    const auto squaredVelocities    = uSquared + vSquared;
#endif
    const auto internalEnergy       = Q[Shortcuts::rhoE] - 0.5 * (Q[Shortcuts::rho] * squaredVelocities);
    const auto temperature          = (internalEnergy * (GAMMA - 1.0)) / (Q[Shortcuts::rho] * UNIVERSAL_GAS_CONSTANT);
    return temperature;
};

  {{COMPUTE_PRECISION}} temperatureLaplacian = 0.0;
  const auto temperatureQ = temperature(Q);
  const auto doubleTemperatureQ = 2 * temperatureQ;

  // x-direction
  temperatureLaplacian += (temperature(QRight0) - doubleTemperatureQ + temperature(QLeft0)) / (h(0) * h(0));
  // y-direction
  temperatureLaplacian += (temperature(QTop0) - doubleTemperatureQ + temperature(QDown0)) / (h(1) * h(1));
#if DIMENSIONS == 3
  // z-direction
  temperatureLaplacian += (temperature(QFront0) - doubleTemperatureQ + temperature(QBack0)) / (h(2) * h(2));
#endif

  const auto newPressure = Q[Shortcuts::rho] * UNIVERSAL_GAS_CONSTANT * (temperatureQ + dt * THERMAL_DIFFUSIVITY * temperatureLaplacian);
  QNew[Shortcuts::rhoE] = newPressure / (GAMMA - 1.0) + 0.5 * Q[Shortcuts::rho] * vSquared;
"""

diffusion_solver = exahype2.solvers.fd.GlobalAdaptiveTimeStep(
    name="DiffusionSolver",
    patch_size=args.degrees_of_freedom,
    unknowns=diffusion_unknowns,
    auxiliary_variables=diffusion_auxiliary_variables,
    min_h=min_h,
    max_h=max_h,
    time_step_relaxation=0.5,
    solvers_before_in_coupling=[euler_solver],
    order=1,
)

diffusion_solver.set_implementation(
    initial_conditions=diffusion_initial_conditions,
    boundary_conditions=diffusion_boundary_conditions,
    solver=diffusion_stencil,
    max_eigenvalue="""
    return 1.0;
""",
)

############
# COUPLING #
############

diffusion_solver._compute_time_step_size = (
    """
  const double timeStepSize = repositories::"""
    + euler_solver.get_name_of_global_instance()
    + """.getAdmissibleTimeStepSize(false);
"""
)

number_of_variables_euler = euler_solver.auxiliary_variables + euler_solver.unknowns
number_of_variables_diffusion = (
    diffusion_solver.auxiliary_variables + diffusion_solver.unknowns
)

# EULER to DIFFUSION

euler_solver.postprocess_updated_patch += f"""// Copy density, energy, and momenta
  ::toolbox::blockstructured::copyUnknown(
    {euler_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{euler_solver.name}Q.value, // source
    {euler_solver._variable_names.index("rho")}, // density in source
    {number_of_variables_euler}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{diffusion_solver.name}Q.value, // dest
    {diffusion_solver._variable_names.index("rho")}, // density in dest
    {number_of_variables_diffusion}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );

  ::toolbox::blockstructured::copyUnknown(
    {euler_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{euler_solver.name}Q.value, // source
    {euler_solver._variable_names.index("rhoU")}, // x-momentum in source
    {number_of_variables_euler}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{diffusion_solver.name}Q.value, // dest
    {diffusion_solver._variable_names.index("rhoU")}, // x-momentum in dest
    {number_of_variables_diffusion}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );

  ::toolbox::blockstructured::copyUnknown(
    {euler_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{euler_solver.name}Q.value, // source
    {euler_solver._variable_names.index("rhoV")}, // y-momentum in source
    {number_of_variables_euler}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{diffusion_solver.name}Q.value, // dest
    {diffusion_solver._variable_names.index("rhoV")}, // y-momentum in dest
    {number_of_variables_diffusion}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );

  ::toolbox::blockstructured::copyUnknown(
    {euler_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{euler_solver.name}Q.value, // source
    {euler_solver._variable_names.index("rhoE")}, // energy in source
    {number_of_variables_euler}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{diffusion_solver.name}Q.value, // dest
    {diffusion_solver._variable_names.index("rhoE")}, // energy in dest
    {number_of_variables_diffusion}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );
"""

if args.dimensions == 3:
    euler_solver.postprocess_updated_patch += f"""
#if DIMENSIONS == 3
  ::toolbox::blockstructured::copyUnknown(
    {euler_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{euler_solver.name}Q.value, // source
    {euler_solver._variable_names.index("rhoW")}, // z-momentum in source
    {number_of_variables_euler}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{diffusion_solver.name}Q.value, // dest
    {diffusion_solver._variable_names.index("rhoW")}, // z-momentum in dest
    {number_of_variables_diffusion}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );
#endif
"""

euler_solver.add_user_action_set_includes('#include "toolbox/blockstructured/Copy.h"\n')

# DIFFUSION to EULER

diffusion_solver.postprocess_updated_patch += f"""// Copy energy and momenta
  ::toolbox::blockstructured::copyUnknown(
    {diffusion_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{diffusion_solver.name}Q.value, // source
    {diffusion_solver._variable_names.index("rhoU")}, // x-momentum in source
    {number_of_variables_diffusion}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{euler_solver.name}Q.value, // dest
    {euler_solver._variable_names.index("rhoU")}, // x-momentum in dest
    {number_of_variables_euler}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );

  ::toolbox::blockstructured::copyUnknown(
    {diffusion_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{diffusion_solver.name}Q.value, // source
    {diffusion_solver._variable_names.index("rhoV")}, // y-momentum in source
    {number_of_variables_diffusion}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{euler_solver.name}Q.value, // dest
    {euler_solver._variable_names.index("rhoV")}, // y-momentum in dest
    {number_of_variables_euler}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );

  ::toolbox::blockstructured::copyUnknown(
    {diffusion_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{diffusion_solver.name}Q.value, // source
    {diffusion_solver._variable_names.index("rhoE")}, // energy in source
    {number_of_variables_diffusion}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{euler_solver.name}Q.value, // dest
    {euler_solver._variable_names.index("rhoE")}, // energy in dest
    {number_of_variables_euler}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );
"""

if args.dimensions == 3:
    diffusion_solver.postprocess_updated_patch += f"""
#if DIMENSIONS == 3
  ::toolbox::blockstructured::copyUnknown(
    {diffusion_solver.patch_size}, // no of unknowns per dimension
    fineGridCell{diffusion_solver.name}Q.value, // source
    {diffusion_solver._variable_names.index("rhoW")}, // z-momentum in source
    {number_of_variables_diffusion}, // unknowns in source (incl material parameters)
    0, // no overlap/halo here
    fineGridCell{euler_solver.name}Q.value, // dest
    {euler_solver._variable_names.index("rhoW")}, // z-momentum in dest
    {number_of_variables_euler}, // unknowns in dest (incl material parameters)
    0 // no overlap/halo here
  );
#endif
"""

diffusion_solver.add_user_action_set_includes(
    '#include "toolbox/blockstructured/Copy.h"\n'
)

euler_solver.last_solver = diffusion_solver

###########
# PROJECT #
###########

project = exahype2.Project(
    namespace=["tests", "exahype2", "fvfd"],
    project_name=".",
    directory=".",
    executable="ExaHyPE",
)

project.add_solver(euler_solver)
project.add_solver(diffusion_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=size[0 : args.dimensions],
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
project.set_Peano4_installation(
    "../../../", mode=peano4.output.string_to_mode(args.build_mode)
)
project = project.generate_Peano4_project(verbose=False)
project.output.makefile.add_CXX_flag("-DGAMMA=1.4")
project.output.makefile.add_CXX_flag("-DUNIVERSAL_GAS_CONSTANT=8.31")
project.output.makefile.add_CXX_flag("-DKINEMATIC_VISCOSITY=100")
project.output.makefile.add_CXX_flag("-DTHERMAL_DIFFUSIVITY=150")
project.set_fenv_handler(args.fpe)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
