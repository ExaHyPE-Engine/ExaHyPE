# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""
Finite-volume coupled shallow-water and landslide benchmark.

The setup evolves the water layer and the granular landslide layer on the same
grid, with reflective water boundaries and zero inflow of granular material.
"""

import math

import exahype2
import peano4

DOMAIN_OFFSET = [0.0, 0.0]
DOMAIN_SIZE = [60.0, 60.0]

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; ++i) {
    Q[i] = 0.0;
  }

  const double initCenterX = 4.0;
  const double initCenterY = 30.0;
  const double initRadius  = 3.5;
  const double initHeight  = 8.0;

  const tarch::la::Vector<DIMENSIONS, double> granularMaterialCenter = {initCenterX, initCenterY};
  const bool nearCentre = tarch::la::norm2(x - granularMaterialCenter) < initRadius;
  Q[Shortcuts::lsh] = nearCentre ? initHeight : 0.0;

  Q[Shortcuts::hu] = 0.0;
  Q[Shortcuts::hv] = 0.0;

  if (x[0] >= 19.5) {
    Q[Shortcuts::h] = 4.0;
  } else if (x[0] >= 17.5) {
    Q[Shortcuts::h] = 4.0 * (x[0] - 17.5) / 2.0;
  } else {
    Q[Shortcuts::h] = 0.0;
  }

  Q[Shortcuts::z] = std::max(20.0 - x[0], 1.0);
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hv] = -Qinside[Shortcuts::hv];
  Qoutside[Shortcuts::z]  = Qinside[Shortcuts::z];

  Qoutside[Shortcuts::lsh]  = 0.0;
  Qoutside[Shortcuts::lshu] = 0.0;
  Qoutside[Shortcuts::lshv] = 0.0;
"""

flux = """
  ::applications::exahype2::exaskred::coupled::flux<Shortcuts, NumberOfUnknowns>(
    Q, x, h, t, dt, normal, F
  );
"""

max_eigenvalue = """
  return ::applications::exahype2::exaskred::coupled::maxEigenvalue<Shortcuts>(
    Q, x, h, t, dt, normal
  );
"""

nonconservative_product = """
  ::applications::exahype2::exaskred::coupled::nonconservativeProduct<Shortcuts, NumberOfUnknowns, true>(
    Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ
  );
"""

diffusive_source_term = """
  ::applications::exahype2::exaskred::coupled::diffusiveSourceTerm<
    Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables
  >(Q, deltaQ, x, h, t, dt, S);
"""

riemann_solver = """
  return ::applications::exahype2::exaskred::coupled::solveRiemannProblem<
    Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables
  >(QL, QR, x, h, t, dt, normal, FL, FR);
"""

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=4,
    end_time=30.0,
)
args = parser.parse_args()

dg_order = args.degrees_of_freedom - 1
max_h = 1.1 * min(DOMAIN_SIZE) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

constants = {
    "g": [9.81, "double"],
    "phi": [25.0, "double"],
    "invXi": [1.0 / 200.0, "double"],
    "hThreshold": [1e-1, "double"],
}
constants["mu"] = [
    math.tan(math.pi / 180.0 * constants["phi"][0]),
    "double",
]

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=2 * dg_order + 1,
    unknowns={"h": 1, "hu": 1, "hv": 1, "lsh": 1, "lshu": 1, "lshv": 1},
    auxiliary_variables={"z": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.45,
)
fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    ncp=nonconservative_product,
    diffusive_source_term=diffusive_source_term,
    riemann_solver=riemann_solver,
)
fv_solver.add_user_solver_includes(
    """
#include <algorithm>
#include "Flux.h"
#include "NonconservativeProduct.h"
#include "Riemann.h"
#include "SourceTerm.h"
"""
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="CoupledFVLandslideTsunami",
    directory=".",
    executable="ExaSkred-Coupled",
)

project.set_output_path("solutions")
project.add_solver(fv_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=DOMAIN_SIZE,
    offset=DOMAIN_OFFSET,
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[False, False],
)

project.set_load_balancer(
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees})"
)
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
peano_project = project.generate_Peano4_project(verbose=False)

for const_name, (const_val, const_type) in constants.items():
    peano_project.constants.export_constexpr_with_type(
        const_name, str(const_val), const_type
    )

peano_project.output.makefile.set_target_device(args.target_device)
peano_project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
