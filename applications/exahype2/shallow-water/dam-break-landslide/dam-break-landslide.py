# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import math

import peano4
import exahype2

# https://arxiv.org/pdf/2106.13572
# https://brgm.hal.science/hal-03974933/document
initial_conditions = """
  // Determine the position of the material region
  bool insideTheSquare{true};
  insideTheSquare &= (x[0] >= (initCenterX - squareSide * 0.5));
  insideTheSquare &= (x[0] <= (initCenterX + squareSide * 0.5));
  insideTheSquare &= (x[1] >= (initCenterY - squareSide * 0.5));
  insideTheSquare &= (x[1] <= (initCenterY + squareSide * 0.5));

  // Assign initial conditions based on the position of the material region
  Q[Shortcuts::h]  = (insideTheSquare ? initHeight : 0.0);
  Q[Shortcuts::hu] = 0.0;
  Q[Shortcuts::hv] = 0.0;
  Q[Shortcuts::z]  = (DomainSize[0] - x[0]) * slopeAngleTan;
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hv] = -Qinside[Shortcuts::hv];
  Qoutside[Shortcuts::z]  = Qinside[Shortcuts::z];
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;
  return result;
"""

limiting_criterion = """
  const double Qh{Q[0]};
  if (!std::isfinite(Qh)) {
    return false;
  }

  // Try not to limit untouched cells initialised with 0.0
  if ((Qh < hThreshold) and (Qh > -hThreshold)) {
    return true;
  }

  // Low values of h are resolved on FV layer
  if (Qh <= -hThreshold) {
    return false;
  }

  // Limit close to boundaries
  // x - 0
  if (std::abs(x[0] - DomainOffset[0]) < h[0] or std::abs(x[0] - DomainOffset[0] - DomainSize[0]) < h[0]) {
    return false;
  }
  // y - 1
  if (std::abs(x[1] - DomainOffset[1]) < h[1] or std::abs(x[1] - DomainOffset[1] - DomainSize[1]) < h[1]) {
    return false;
  }

  return true;
"""

adjust_solution = r"""
  if (Q[Shortcuts::h] < hThreshold) {
    Q[Shortcuts::h]  = std::fmax(0.0, Q[Shortcuts::h]);
    Q[Shortcuts::hu] = 0.0;
    Q[Shortcuts::hv] = 0.0;
  }
"""

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=3,
    end_time=1.0,
    degrees_of_freedom=7,
)
args = parser.parse_args()

if args.build_mode == "Debug":
    args.end_time = 0.1

problem_constants = {
    "g": [9.81, "double"],
    "phi": [25.0, "double"],
    "invXi": [1.0 / 200.0, "double"],
    "slopeAngleTan": [math.tan(math.pi / 180.0 * 35.0), "double"],
    "initCenterX": [0.15, "double"],
    "initCenterY": [0.79, "double"],
    "squareSide": [0.1, "double"],
    "initHeight": [0.1, "double"],
}
problem_constants["hThreshold"] = [problem_constants["initHeight"][0] * 1e-3, "double"]
problem_constants["mu"] = [
    math.tan(math.pi / 180.0 * problem_constants["phi"][0]),
    "double",
]

size = [1.58, 1.58]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)
dg_order = args.degrees_of_freedom - 1

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={"h": 1, "hu": 1, "hv": 1, "z": 1},
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.45,
)

aderdg_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=f"ShallowWater::flux<double, Shortcuts, {aderdg_solver.unknowns}>(Q, x, h, t, dt, normal, F);",
    max_eigenvalue="return FrictionLaws::maxEigenvalue<double, Shortcuts>(Q, x, h, t, dt, normal);",
    ncp=f"ShallowWater::nonconservativeProduct<double, Shortcuts, {aderdg_solver.unknowns}>(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);",
    diffusive_source_term=f"FrictionLaws::sourceTermADERDG<double, Shortcuts, {aderdg_solver.unknowns},  {aderdg_solver.auxiliary_variables}>(Q, deltaQ, x, h, t, dt, S);",
    riemann_solver=f"rusanovRiemannSolverADERDG<double, Shortcuts, kernels::{aderdg_solver.name}::Quadrature<double>, {aderdg_solver.unknowns}, {aderdg_solver.auxiliary_variables}, {aderdg_solver.order}>(QL, QR, x, h, t, dt, direction, FL, FR);",
)

aderdg_solver.add_kernel_optimisations(
    is_linear=False, polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre
)
aderdg_solver.add_user_solver_includes(
    """
#include "../PDE.h"
#include "../FrictionLaws.h"
#include "../RusanovRiemannSolver.h"
"""
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=dg_order * 2 + 1,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"z": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.45,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    adjust_solution=adjust_solution,
    diffusive_source_term=f"FrictionLaws::sourceTermFV<double, Shortcuts, {fv_solver.unknowns}, {fv_solver.auxiliary_variables}>(Q, deltaQ, x, h, t, dt, S);",
    riemann_solver=f"return rusanovRiemannSolverFV<double, Shortcuts, {fv_solver.unknowns}, {fv_solver.auxiliary_variables}>(QL, QR, x, h, t, dt, normal, FL, FR);",
)

fv_solver.add_user_solver_includes(
    """
#include "../PDE.h"
#include "../FrictionLaws.h"
#include "../RusanovRiemannSolver.h"
"""
)

limiter_solver = exahype2.solvers.limiting.PosterioriLimiting(
    name="LimiterSolver",
    regular_solver=aderdg_solver,
    limiting_solver=fv_solver,
    number_of_dmp_observables=3,
    dmp_relaxation_parameter=0.2,  # MD 3: BOTH FRICTIONS: 0.2, ONLY ONE FRICTION TERM OR ZERO: 0.1,
    dmp_differences_scaling=1e-3,  # MD 3: BOTH FRICTIONS: 1e-3, ONLY ONE FRICTION TERM OR ZERO: 5e-4
    physical_admissibility_criterion=limiting_criterion,
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "ShallowWater"],
    project_name="DamBreakLandslide",
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
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
for const_name, const_info in problem_constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)
project.output.makefile.set_target_device(args.target_device)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
