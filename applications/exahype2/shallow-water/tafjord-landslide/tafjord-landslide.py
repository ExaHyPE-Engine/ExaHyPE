# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys
import math

import peano4
import exahype2

sys.path.insert(0, os.path.abspath(".."))
from PDE import *
from Rusanov import *

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  static tarch::reader::NetCDFFieldParser fieldParser(
    \"Tafjord_5m_EPSG25832.nc\",
    \"ini_3.0Mm3_5m_EPSG25832.nc\",
    DomainSize(0),
    DomainSize(1),
    DomainOffset(0),
    DomainOffset(1),
    "Band1",
    "x",
    "y",
    "Band1",
    "x",
    "y"
  );

  Q[Shortcuts::h] = fieldParser.sampleDisplacement(x(0), x(1));
  Q[Shortcuts::z] = fieldParser.sampleTopology(x(0), x(1));
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
  const auto Qh{Q[0]};
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
parser.add_argument(
    "--friction",
    type=float,
    help="Friction parameter.",
)
parser.set_defaults(
    min_depth=3,
    end_time=40.0,
    time_step_relaxation=0.45,
    degrees_of_freedom=7,
    friction=200.0,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "phi": [25.0, "double"],
    "invXi": [1.0 / args.friction, "double"],
    "hThreshold": [1e-1, "double"],
}
constants["mu"] = [
    math.tan(math.pi / 180.0 * constants["phi"][0]),
    "double",
]

size = [1900, 1950]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)
dg_order = args.degrees_of_freedom - 1

regular_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={"h": 1, "hu": 1, "hv": 1, "z": 1},
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.5,
)

regular_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    ncp=nonconservative_product,
    max_eigenvalue=eigenvalue
    + stiff_eigenvalue
    + """
  return std::fmax(sFlux, h[normal] * sSource);
""",
    diffusive_source_term=stiff_source_term_aderdg,
    riemann_solver=rusanov_aderdg,
)

regular_solver.set_plotter(args.plotter)
regular_solver.add_user_solver_includes(
    """
#include "tarch/reader/NetCDFFieldParser.h"
"""
)
regular_solver.add_kernel_optimisations(
    is_linear=False,
    polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre,
)

limiting_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=dg_order * 2 + 1,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"z": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
)

limiting_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    max_eigenvalue=eigenvalue
    + """
  return sFlux;
""",
    ncp=nonconservative_product + stiff_nonconservative_product,
    riemann_solver=rusanov_fv,
    diffusive_source_term=stiff_source_term_fv,
    adjust_solution=adjust_solution,
)

limiting_solver.set_plotter(args.plotter)
limiting_solver.add_user_solver_includes(
    """
#include "tarch/reader/NetCDFFieldParser.h"
"""
)

limiter_solver = exahype2.solvers.limiting.PosterioriLimiting(
    name="LimiterSolver",
    regular_solver=regular_solver,
    limiting_solver=limiting_solver,
    number_of_dmp_observables=3,
    dmp_relaxation_parameter=1.0,  # MD 3: BOTH FRICTIONS: 1.0, ONLY ONE FRICTION TERM OR ZERO: 0.5,
    dmp_differences_scaling=0.01,  # MD 3: BOTH FRICTIONS: 0.01, ONLY ONE FRICTION TERM OR ZERO: 0.005,
    physical_admissibility_criterion=limiting_criterion,
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "swe"],
    project_name="TafjordLandslide",
    directory=".",
    executable="ExaHyPE-ShallowWater",
)

project.add_solver(regular_solver)
project.add_solver(limiting_solver)
project.add_solver(limiter_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=size,
    offset=[414895.5, 6904495.5],
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
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees}, {args.trees})"
)
project.set_Peano4_installation(
    "../../../../", mode=peano4.output.string_to_mode(args.build_mode)
)
project = project.generate_Peano4_project(verbose=False)
for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)
project.output.makefile.set_target_device(args.target_device)
project.set_fenv_handler(args.fpe)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
