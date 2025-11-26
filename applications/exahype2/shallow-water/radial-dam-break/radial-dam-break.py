# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

define_radial_dam = """
  constexpr double DamRadius = 1.0;

  const double domainSizeHalfX = DomainSize[0] / 2.0;
  const double domainSizeHalfY = DomainSize[1] / 2.0;

  const double distanceFromOrigin = std::sqrt(
    (x[0] - domainSizeHalfX) * (x[0] - domainSizeHalfX) +
    (x[1] - domainSizeHalfY) * (x[1] - domainSizeHalfY)
  );

  const bool isInsideDam = distanceFromOrigin <= DamRadius;
  const bool isOutsideDam = distanceFromOrigin > DamRadius;
  const bool isDryRing = distanceFromOrigin >= 2 * DamRadius and distanceFromOrigin <= 3 * DamRadius;
"""

initial_conditions = (
    define_radial_dam
    + """
  constexpr double InitialWaterHeightInsideDam = 1.1;
  constexpr double InitialWaterHeightOutsideDam = 1.0;
  constexpr double BathymetryDryRing = 1.2;

  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  Q[Shortcuts::h] = isInsideDam ? InitialWaterHeightInsideDam : InitialWaterHeightOutsideDam;

  if (isDryRing) {
    Q[Shortcuts::h] = 0.0; // Dry ring
  }

  Q[Shortcuts::z] = isDryRing ? BathymetryDryRing : 0.0;
"""
)

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hv] = -Qinside[Shortcuts::hv];
  Qoutside[Shortcuts::z]  = Qinside[Shortcuts::z];
"""

refinement_criterion = (
    define_radial_dam
    + """
  return isInsideDam ? ::exahype2::RefinementCommand::Refine : ::exahype2::RefinementCommand::Keep;
"""
)

limiting_criterion = (
    define_radial_dam
    + """
  return distanceFromOrigin < DamRadius * 2; // Everything outside the dam (includes the dry ring) shall be limited
"""
)

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=4,
    end_time=0.1,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
}

size = [10.0, 10.0]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1, "z": 1},
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.5,
)

aderdg_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=f"ShallowWater::flux<double, Shortcuts, {aderdg_solver.unknowns}>(Q, x, h, t, dt, normal, F);",
    max_eigenvalue="return ShallowWater::maxEigenvalue<double, Shortcuts>(Q, x, h, t, dt, normal);",
    ncp=f"ShallowWater::nonconservativeProduct<double, Shortcuts, {aderdg_solver.unknowns}>(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);",
)

aderdg_solver.set_plotter(args.plotter)
aderdg_solver.add_kernel_optimisations(
    is_linear=False, polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre
)
aderdg_solver.add_user_solver_includes(
    """
#include "../PDE.h"
"""
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom * 2 + 1,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"z": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    riemann_solver="return hllemRiemannSolver<double, Shortcuts>(QL, QR, x, h, t, dt, normal, FL, FR);",
)

fv_solver.set_plotter(args.plotter)
fv_solver.add_user_solver_includes(
    """
#include "../HLLEMRiemannSolver.h"
"""
)

limiter_solver = exahype2.solvers.limiting.StaticLimiting(
    name="LimiterSolver",
    regular_solver=aderdg_solver,
    limiting_solver=fv_solver,
    physical_admissibility_criterion=limiting_criterion,
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "ShallowWater"],
    project_name="RadialDamBreak",
    directory=".",
    executable="ExaHyPE",
)

project.add_solver(aderdg_solver)
project.add_solver(fv_solver)
project.add_solver(limiter_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=size,
    offset=[0.0, 0.0],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ],
)

project.set_load_balancer(
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees})"
)
project.set_Peano4_installation(
    "../../../../", mode=peano4.output.string_to_mode(args.build_mode)
)
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
