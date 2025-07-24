# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys

import peano4
import exahype2

sys.path.insert(0, os.path.abspath(".."))
from PDE import *

initial_conditions = """
  static tarch::reader::NetCDFFieldParser fieldParser(
    \"artificial_bath_1000.nc\",
    \"artificial_displ_1000.nc\",
    DomainSize(0),
    DomainSize(1),
    DomainOffset(0),
    DomainOffset(1)
  );

  const double bathymetryBeforeEarthquake = fieldParser.sampleTopology(x(0), x(1));
  const double displacement               = fieldParser.sampleDisplacement(x(0), x(1));
  const double bathymetryAfterEarthquake  = bathymetryBeforeEarthquake + displacement;

  Q[s::h]  = -std::min(bathymetryBeforeEarthquake, 0.0);
  Q[s::hu] = 0.0;
  Q[s::hv] = 0.0;
  Q[s::z]  = bathymetryAfterEarthquake;
"""

boundary_conditions = """
  Qoutside[s::h]  = Qinside[s::h];
  Qoutside[s::hu] = -Qinside[s::hu];
  Qoutside[s::hv] = -Qinside[s::hv];
  Qoutside[s::z]  = Qinside[s::z];
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;
  if (x[0] >= 4500.0 && x[1] >= 4500.0 && x[0] <= 5500.0 && x[1] <= 5500.0) {
    result = ::exahype2::RefinementCommand::Refine;
  }
  return result;
"""

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=4,
    end_time=50.0,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
}

size = [10000.0, 10000.0]  # [m]
max_h = (1.1 * min(size) / (3.0**args.min_depth))
min_h = max_h * 3.0 ** (-args.amr_levels)

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1, "z": 1},
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=args.time_step_relaxation,
)

aderdg_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    ncp=nonconservative_product,
    max_eigenvalue=eigenvalue + """
  return sFlux;
""",
)

aderdg_solver.set_plotter(args.plotter)
aderdg_solver.add_kernel_optimisations(is_linear=False, polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre)
aderdg_solver.add_user_solver_includes(
    """
#include "tarch/reader/NetCDFFieldParser.h"
"""
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "swe"],
    project_name="ArtificialTsunami",
    directory=".",
    executable=f"ArtificialTsunami.{args.build_mode}",
)
project.add_solver(aderdg_solver)

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

project.set_load_balancer(f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees}, {args.trees})")
project.set_Peano4_installation("../../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(
        const_name, str(const_val), const_type
    )
project.set_fenv_handler(args.fpe)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
