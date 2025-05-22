# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys

import peano4
import exahype2

sys.path.insert(0, os.path.abspath(".."))
from FrictionLaws import *

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  static tarch::reader::TopologyParser topologyParser(
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

  Q[s::h] = topologyParser.sampleDisplacement(x(0), x(1));
  Q[s::b] = topologyParser.sampleBathymetry(x(0), x(1));
"""

boundary_conditions = """
  Qoutside[s::h]  = Qinside[s::h];
  Qoutside[s::hu] = -Qinside[s::hu];
  Qoutside[s::hv] = -Qinside[s::hv];
  Qoutside[s::b]  = Qinside[s::b];
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;
  return result;
"""

parser = exahype2.ArgumentParser()
parser.set_defaults(
    end_time=50.0,
)
args = parser.parse_args()

size = [1900, 1950]
max_h = (1.1 * min(size) / (3.0**args.min_depth))
min_h = max_h * 3.0 ** (-args.amr_levels)

riemann_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVRusanovSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={
    "b": 1, # Elevation
    "bx": 1,
    "by": 1,
},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=args.time_step_relaxation,
    number_of_enclave_tasks=args.ntasks,
)

riemann_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    ncp="""
  BTimesDeltaQ[s::h] = 0.0;
  BTimesDeltaQ[s::hu] = 0.0;
  BTimesDeltaQ[s::hv] = 0.0;
  BTimesDeltaQ[s::hu + normal] += GRAV * Q[s::h] * deltaQ[s::b];
""",
    max_eigenvalue=max_eigenvalue,
    source_term=coulomb_friction,
)

riemann_solver.set_plotter(args.plotter)
riemann_solver.add_user_solver_includes(
    """
#include "tarch/reader/TopologyParser.h"
"""
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "swe"],
    project_name="TafjordLandslide",
    directory=".",
    executable=f"TafjordLandslide.{args.build_mode}",
)
project.add_solver(riemann_solver)

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

project.set_Peano4_installation("../../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.output.makefile.set_target_device(args.target_device)
project.set_fenv_handler(args.fpe)
project.output.makefile.add_CXX_flag("-DGRAV=9.81")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
