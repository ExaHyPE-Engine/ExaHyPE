# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import math

import exahype2

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


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=30.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=4)
    parser.add_argument("-l", "--grid-levels", type=int, default=4)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

dg_order = args.degrees_of_freedom - 1

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="CoupledFVLandslideTsunami",
    dimensions=2,
    offset=DOMAIN_OFFSET,
    size=DOMAIN_SIZE,
    periodic_BC=[False, False],
    directory=".",
    executable="ExaSkred-Coupled",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)
if project.dimensions != 2:
    raise ValueError(
        "The coupled finite-volume ExaSkred application is two-dimensional."
    )

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
#include "../Flux.h"
#include "../NonconservativeProduct.h"
#include "../Riemann.h"
#include "../SourceTerm.h"
"""
)

project.add_solver(fv_solver)
peano_project = project.generate_Peano4_project()

for const_name, (const_val, const_type) in constants.items():
    peano_project.constants.export_constexpr_with_type(
        const_name, str(const_val), const_type
    )

peano_project.generate_and_build()
