# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import re
import sys

import peano4
import exahype2

sys.path.insert(0, os.path.abspath(".."))
from sweargs import SWEArgumentParser

# https://arxiv.org/pdf/2106.13572
# https://brgm.hal.science/hal-03974933/document
initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  using s = VariableShortcuts;
  const tarch::la::Vector<Dimensions, double>& granularMaterialCenter = { 0.15, 0.35 };
  const bool nearCentre = tarch::la::norm2(x - granularMaterialCenter) < 0.10;
  Q[s::h] = (nearCentre ? 0.008 : 0.000); // Granular material height (h)
"""

boundary_conditions = """
  using s = VariableShortcuts;
  Qoutside[s::h]  = Qinside[s::h];
  Qoutside[s::hu] = -Qinside[s::hu];
  Qoutside[s::hv] = -Qinside[s::hv];
  Qoutside[s::b]  = Qinside[s::b];
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;

  const tarch::la::Vector<Dimensions, double>& granularMaterialCenter = { 0.15, 0.35 };
  const bool nearCentre = tarch::la::norm2(x - granularMaterialCenter) < 0.10;
  if (tarch::la::equals(t, 0.0) and nearCentre) {
    result = ::exahype2::RefinementCommand::Refine;
  }

  return result;
"""

parser = SWEArgumentParser()
parser.set_defaults(
    end_time=1.0,
    width=[1.58, 0.70],
)
args = parser.parse_args()

max_h = (1.1 * min(args.width) / (3.0**args.min_depth))
min_h = max_h * 3.0 ** (-args.amr_levels)
args.patch_size = args.dg_order * 2 + 1

unknowns={"h": 1, "hu": 1, "hv": 1}
auxiliary_variables={
    "b": 1, # Elevation
    "hx": 1,
    "hy": 1,
    "ux": 1,
    "uy": 1,
    "uxx": 1,
    "uyy": 1,
    "uxy": 1,
    "vx": 1,
    "vy": 1,
    "vxx": 1,
    "vyy": 1,
    "vxy": 1,
    "bx": 1,
    "by": 1,
}

riemann_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
    name="Rusanov" + str(args.patch_size),
    patch_size=args.patch_size,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=args.time_step_relaxation,
    pde_terms_without_state=args.stateless,
    initial_conditions=re.sub(r'\bx\b', 'volumeCentre', initial_conditions),
    boundary_conditions=boundary_conditions,
    refinement_criterion=re.sub(r'\bx\b', 'volumeCentre', refinement_criterion),
    flux="applications::exahype2::swe::frictionlaws::flux(Q, faceCentre, volumeH, t, dt, normal, F);",
    ncp="applications::exahype2::swe::frictionlaws::nonconservativeProduct(Q, deltaQ, faceCentre, volumeH, t, dt, normal, BTimesDeltaQ);",
    eigenvalues="applications::exahype2::swe::frictionlaws::maxEigenvalue(Q, faceCentre, volumeH, t, dt, normal, maxEigenvalue);",
    source_term="applications::exahype2::swe::frictionlaws::materialDependentBasalFriction(Q, volumeCentre, volumeH, t, dt, S);",
)
riemann_solver.plot_description = ", ".join(unknowns.keys()) + ", "
riemann_solver.plot_description += ", ".join(auxiliary_variables.keys())
riemann_solver.switch_storage_scheme(exahype2.solvers.Storage[args.storage], exahype2.solvers.Storage[args.storage])
riemann_solver.add_user_solver_includes(
    """
#include "../SWE.h"
#include "../FrictionLaws.h"
#include "../ComputeFirstDerivatives.h"
"""
)
riemann_solver._preprocess_reconstructed_patch = """
      extrapolateHalo<{NumberOfDofs}, {NumberOfUnknowns}, {NumberOfAuxiliaryVariables}>(oldQWithHalo);
      computeFirstDerivatives<{NumberOfDofs}, {NumberOfUnknowns}, {NumberOfAuxiliaryVariables}>(oldQWithHalo);
""".format(
                NumberOfDofs=args.patch_size,
                NumberOfUnknowns=sum(unknowns.values()),
                NumberOfAuxiliaryVariables=sum(auxiliary_variables.values()),
            )

project = exahype2.Project(
    namespace=["applications", "exahype2", "swe"],
    project_name="DamBreakLandslide",
    directory=".",
    executable=f"DamBreakLandslide.{args.build_mode}",
)
project.add_solver(riemann_solver)

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
print(riemann_solver)
