# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Dry-ring comparison: select a scheme with ``--scheme`` (or ``--solver``).

``--weno-order`` is the ADER polynomial degree.
"""
import argparse
from pathlib import Path

import exahype2


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="Show this help message and continue with project help.",
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.15)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=2)
    parser.add_argument(
        "-s", "--solver", "--scheme",
        choices=["godunov", "musclhancock", "ader-weno"],
        default="musclhancock",
    )
    parser.add_argument(
        "--weno-order", type=int, choices=[2, 3, 4, 5], default=3,
        help="Polynomial degree for ADER-WENO (nominal accuracy is degree + 1).",
    )
    parser.add_argument(
        "--check-admissibility", action="store_true",
        help="Assert finite updated states and h >= -1e-12 (use an Asserts build).",
    )
    parser.add_argument(
        "--well-balanced", action="store_true",
        help="Use the hydrostatic Rusanov interface with stationary bathymetry.",
    )
    parser.add_argument(
        "--polynomial-limiter", action="store_true",
        help="Scale every ADER-WENO polynomial towards its average until h >= 0.",
    )
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()


initial_conditions = """
  const double radius = std::sqrt(x(0) * x(0) + x(1) * x(1));
  const double bathymetry = 0.3 * std::exp(-radius * radius / 0.0625);
  const bool wet = radius > 0.35 and radius < 0.65;

  Q[Shortcuts::h] = wet ? 0.2 - bathymetry : 0.0;
  Q[Shortcuts::v + 0] = 0.0;
  Q[Shortcuts::v + 1] = 0.0;
  Q[Shortcuts::b] = bathymetry;
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]     = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::v + 0] = Qinside[Shortcuts::v + 0];
  Qoutside[Shortcuts::v + 1] = Qinside[Shortcuts::v + 1];
  Qoutside[Shortcuts::b]     = Qinside[Shortcuts::b];
"""

flux = """
  const double u[2] = {
    dry_ring::velocity(Q[Shortcuts::h], Q[Shortcuts::v + 0], DESINGULARISATION),
    dry_ring::velocity(Q[Shortcuts::h], Q[Shortcuts::v + 1], DESINGULARISATION),
  };
  F[Shortcuts::h]     = Q[Shortcuts::h] * u[normal];
  F[Shortcuts::v + 0] = Q[Shortcuts::h] * u[normal] * u[0];
  F[Shortcuts::v + 1] = Q[Shortcuts::h] * u[normal] * u[1];
  F[Shortcuts::b]     = 0.0;
"""

ncp = """
  BTimesDeltaQ[Shortcuts::h] = 0.0;
  switch (normal) {
    case 0:
      BTimesDeltaQ[Shortcuts::v + 0] =
        GRAVITY * Q[Shortcuts::h]
        * (deltaQ[Shortcuts::h] + deltaQ[Shortcuts::b]);
      BTimesDeltaQ[Shortcuts::v + 1] = 0.0;
      break;
    case 1:
      BTimesDeltaQ[Shortcuts::v + 0] = 0.0;
      BTimesDeltaQ[Shortcuts::v + 1] =
        GRAVITY * Q[Shortcuts::h]
        * (deltaQ[Shortcuts::h] + deltaQ[Shortcuts::b]);
      break;
  }
  BTimesDeltaQ[Shortcuts::b] = 0.0;
"""

max_eigenvalue = """
  const double velocity =
    dry_ring::velocity(Q[Shortcuts::h], Q[Shortcuts::v + normal], DESINGULARISATION);
  return std::fabs(velocity) + std::sqrt(GRAVITY * std::fmax(0.0, Q[Shortcuts::h]));
"""

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name=f"DryRing-{args.solver}",
    dimensions=2,
    size=[2.0, 2.0, 2.0],
    offset=[-1.0, -1.0, -1.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

solver_arguments = dict(
    name="SWESolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "v": 2, "b": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.4),
)
if args.solver == "godunov":
    fv_solver = exahype2.fv.singlestep.Solver(scheme=exahype2.fv.Godunov(), **solver_arguments)
elif args.solver == "musclhancock":
    fv_solver = exahype2.fv.singlestep.Solver(
        scheme=exahype2.fv.MusclHancock(limiter=exahype2.fv.Limiter.minmod), **solver_arguments
    )
else:
    fv_solver = exahype2.fv.ader.Solver(
        scheme=exahype2.fv.Weno(
            order=args.weno_order,
            reconstruction_limiter=(
                exahype2.fv.PolynomialLimiter.admissibility_scaling
                if args.polynomial_limiter
                else exahype2.fv.PolynomialLimiter.none
            ),
        ),
        **solver_arguments,
    )

fv_solver.add_includes(str(Path(__file__).resolve().with_name("DryRing.h")))
fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    flux=flux,
    ncp=ncp,
    max_eigenvalue=max_eigenvalue,
)

if args.well_balanced:
    fv_solver.set_pde_terms(riemann_solver="""
  return dry_ring::hydrostaticRusanov(QL, QR, normal, GRAVITY, DESINGULARISATION, FL, FR);
""")

if args.polynomial_limiter:
    fv_solver.set_pde_terms(physical_admissibility="""
  for (int state = 0; state < numberOfStates; state++) {
    if (not(Q[state * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + Shortcuts::h] >= 0.0)) {
      return false;
    }
  }
  return true;
""")

if args.check_admissibility:
    fv_solver.postprocess_updated_patch = """
  for (int volume = 0; volume < PointsPerCell; volume++) {
    const auto* state = Q + volume * NumberOfData;
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      assertion(std::isfinite(state[unknown]), t, dt, x, volume, unknown, state[unknown]);
    }
    assertion(state[Shortcuts::h] >= -1e-12, t, dt, x, volume, state[Shortcuts::h]);
  }
"""

project.add_solver(fv_solver)

project.output.makefile.add_compile_definition("GRAVITY=9.81")
project.output.makefile.add_compile_definition("DESINGULARISATION=1e-3")
project.generate_and_build()
