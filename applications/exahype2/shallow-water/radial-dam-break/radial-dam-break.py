# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import re
import sys

import peano4
import exahype2

sys.path.insert(0, os.path.abspath(".."))
from sweargs import SWEArgumentParser

define_radial_dam = """
  constexpr double DAM_RADIUS = 1.0;

  const double domainSizeHalfX = DomainSize[0] / 2.0;
  const double domainSizeHalfY = DomainSize[1] / 2.0;

  const double distanceFromOrigin = std::sqrt(
    (x[0] - domainSizeHalfX) * (x[0] - domainSizeHalfX) +
    (x[1] - domainSizeHalfY) * (x[1] - domainSizeHalfY)
  );

  const bool isInsideDam = distanceFromOrigin <= DAM_RADIUS;
  const bool isOutsideDam = distanceFromOrigin > DAM_RADIUS;
  const bool isDryRing = distanceFromOrigin >= 2 * DAM_RADIUS and distanceFromOrigin <= 3 * DAM_RADIUS;
"""

initial_conditions = define_radial_dam + """
  constexpr double INITIAL_WATER_HEIGHT_INSIDE_DAM = 1.1;
  constexpr double INITIAL_WATER_HEIGHT_OUTSIDE_DAM = 1.0;
  constexpr double BATHYMETRY_DRY_RING = 1.2;

  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  using s = VariableShortcuts;

  Q[s::h] = isInsideDam ? INITIAL_WATER_HEIGHT_INSIDE_DAM : INITIAL_WATER_HEIGHT_OUTSIDE_DAM;

  if (isDryRing) {
    Q[s::h] = 0.0; // Dry ring
  }

  Q[s::b] = isDryRing ? BATHYMETRY_DRY_RING : 0.0;
"""

boundary_conditions = """
  using s = VariableShortcuts;
  Qoutside[s::h]  = Qinside[s::h];
  Qoutside[s::hu] = -Qinside[s::hu];
  Qoutside[s::hv] = -Qinside[s::hv];
  Qoutside[s::b]  = Qinside[s::b];
"""

refinement_criterion = define_radial_dam + """
  return isInsideDam ? ::exahype2::RefinementCommand::Refine : ::exahype2::RefinementCommand::Keep;
"""

limiting_criterion = define_radial_dam + """
  return distanceFromOrigin < DAM_RADIUS * 2; // Everything outside the dam (includes the dry ring) shall be limited
"""

parser = SWEArgumentParser()
parser.set_defaults(
    end_time=1.0,
    width=[10.0, 10.0],
)
args = parser.parse_args()

max_h = (1.1 * min(args.width) / (3.0**args.min_depth))
min_h = max_h * 3.0 ** (-args.amr_levels)
args.patch_size = args.dg_order * 2 + 1

unknowns={"h": 1, "hu": 1, "hv": 1, "b": 1}

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDG" + str(args.dg_order),
    order=args.dg_order,
    unknowns=unknowns,
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=args.time_step_relaxation,
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux="applications::exahype2::swe::flux(Q, x, h, t, dt, normal, F);",
    ncp="applications::exahype2::swe::nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);",
    eigenvalues="""
    double lambda[NumberOfUnknowns] = {0.};
    applications::exahype2::swe::maxEigenvalue(Q, x, h, t, dt, normal, lambda);
    return *std::max_element(lambda, lambda+NumberOfUnknowns);""",
)
aderdg_solver.plot_description = ", ".join(unknowns.keys())
aderdg_solver.switch_storage_scheme(exahype2.solvers.Storage[args.storage], exahype2.solvers.Storage[args.storage])
aderdg_solver.add_kernel_optimisations(is_linear=False, polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre)
aderdg_solver.add_user_solver_includes(
    """
#include "../SWE.h"
"""
)

unknowns = {"h": 1, "hu": 1, "hv": 1}
auxiliary_variables = {"b": 1}

riemann_solver = exahype2.solvers.fv.riemann.GlobalAdaptiveTimeStep(
    name="HLLEM" + str(args.patch_size),
    patch_size=args.patch_size,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=args.time_step_relaxation,
    pde_terms_without_state=args.stateless,
    initial_conditions=re.sub(r'\bx\b', 'volumeCentre', initial_conditions),
    boundary_conditions=boundary_conditions,
    riemann_solver="return applications::exahype2::swe::hllem(QR, QL, FR, FL, LR, LL, xR, xL, h, t, dt, normal, APDQ, AMDQ);",
)
riemann_solver.plot_description = ", ".join(unknowns.keys()) + ", "
riemann_solver.plot_description += ", ".join(auxiliary_variables.keys())
riemann_solver.switch_storage_scheme(exahype2.solvers.Storage[args.storage], exahype2.solvers.Storage[args.storage])
riemann_solver.add_user_solver_includes(
    """
#include "../HLLEM.h"
"""
)

limiting_solver = exahype2.solvers.limiting.StaticLimiting(
    name="Limiting",
    regular_solver=aderdg_solver,
    limiting_solver=riemann_solver,
    limiting_criterion_implementation=limiting_criterion
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "swe"],
    project_name="RadialDamBreak",
    directory=".",
    executable=f"RadialDamBreak.{args.build_mode}",
)
project.add_solver(aderdg_solver)
project.add_solver(riemann_solver)
project.add_solver(limiting_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=args.width,
    offset=args.offset,
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ],
)

if args.load_balancing_strategy != "None":
    strategy = f"toolbox::loadbalancing::strategies::{args.load_balancing_strategy}"
    assume_periodic_boundary_conditions = any([
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ])
    configuration = (
        f"new ::exahype2::LoadBalancingConfiguration("
        f"{args.load_balancing_quality},"
        f"0,"
        f"{'true' if assume_periodic_boundary_conditions else 'false'},"
        f"{args.trees},"
        f"{args.trees})"
    )
    project.set_load_balancing(strategy, configuration)

project.set_number_of_threads(args.threads)
project.set_timeout(3600)
project.set_Peano4_installation("../../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=True)
if args.fpe:
    project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

project.output.makefile.add_CXX_flag("-DGRAV=9.81")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)

print(args)
print(project)
print(aderdg_solver)
print(riemann_solver)
print(limiting_solver)
