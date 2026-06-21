# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import math

import exahype2
import peano4

DOMAIN_OFFSET = [414895.5, 6904495.5]
DOMAIN_SIZE = [1900.0, 1900.0]
BATHYMETRY_FILE = "bathymetry/Tafjord_5m_EPSG25832.nc"
DISPLACEMENT_FILE = "bathymetry/ini_3.0Mm3_5m_EPSG25832.nc"

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; ++i) {
    Q[i] = 0.0;
  }

  static tarch::reader::NetCDFFieldSampler fieldSampler(
    {
      {"bathymetry", RuntimeParameters::getBathymetryFile(), "Band1", "x", "y"},
      {"displacement", RuntimeParameters::getDisplacementFile(), "Band1", "x", "y"}
    },
    DomainSize(0),
    DomainSize(1),
    DomainOffset(0),
    DomainOffset(1)
  );

  Q[Shortcuts::h] = fieldSampler.sample("displacement", x(0), x(1));
  Q[Shortcuts::z] = fieldSampler.sample("bathymetry", x(0), x(1));
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; ++i) {
    Qoutside[i] = Qinside[i];
  }
"""

refinement_criterion = """
  return ::exahype2::RefinementCommand::Keep;
"""

limiting_criterion = """
  const auto Qh = Q[0];
  if (!__builtin_isfinite(Qh)) {
    return false;
  }

  if ((Qh < hThreshold) and (Qh > -hThreshold)) {
    return true;
  }

  if (Qh <= -hThreshold) {
    return false;
  }

  const double leftDistanceX = x[0] - DomainOffset[0];
  const double rightDistanceX = x[0] - DomainOffset[0] - DomainSize[0];
  if (
    (leftDistanceX < h[0] and leftDistanceX > -h[0])
    or (rightDistanceX < h[0] and rightDistanceX > -h[0])
  ) {
    return false;
  }

  const double leftDistanceY = x[1] - DomainOffset[1];
  const double rightDistanceY = x[1] - DomainOffset[1] - DomainSize[1];
  if (
    (leftDistanceY < h[1] and leftDistanceY > -h[1])
    or (rightDistanceY < h[1] and rightDistanceY > -h[1])
  ) {
    return false;
  }

  return true;
"""

adjust_solution = """
  if (Q[0] < hThreshold) {
    Q[0] = (Q[0] < 0.0) ? 0.0 : Q[0];
    Q[1] = 0.0;
    Q[2] = 0.0;
  }
"""

flux = """
  ::applications::exahype2::exaskred::landslide::flux<Shortcuts, NumberOfUnknowns>(
    Q, x, h, t, dt, normal, F
  );
"""

max_eigenvalue = """
  return ::applications::exahype2::exaskred::landslide::maxEigenvalue<Shortcuts>(
    Q, x, h, t, dt, normal
  );
"""

fv_nonconservative_product = """
  ::applications::exahype2::exaskred::landslide::nonconservativeProduct<Shortcuts, NumberOfUnknowns, true>(
    Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ
  );
"""

aderdg_nonconservative_product = """
  ::applications::exahype2::exaskred::landslide::nonconservativeProduct<Shortcuts, NumberOfUnknowns, false>(
    Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ
  );
"""

fv_diffusive_source_term = """
  ::applications::exahype2::exaskred::landslide::diffusiveSourceTerm<
    Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables, false
  >(Q, deltaQ, x, h, t, dt, S);
"""

aderdg_diffusive_source_term = """
  ::applications::exahype2::exaskred::landslide::diffusiveSourceTerm<
    Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables, true
  >(Q, deltaQ, x, h, t, dt, S);
"""

fv_riemann_solver = """
  return ::applications::exahype2::exaskred::landslide::solveRiemannProblem<
    Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables
  >(QL, QR, x, h, t, dt, normal, FL, FR);
"""

aderdg_riemann_solver = """
  ::applications::exahype2::exaskred::landslide::aderdgRiemannSolver<
    Shortcuts, NumberOfUnknowns, Order
  >(
    FL, FR, QL, QR, t, dt, x, h, direction, isBoundaryFace, faceIndex,
    kernels::ADERDGSolver::Quadrature<double>::weights2
  );
"""


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=40.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=7)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=3)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=3)
    parser.add_argument(
        "--friction",
        type=float,
        default=200.0,
        help="Friction parameter.",
    )
    args, _ = parser.parse_known_args()
    if args.grid_levels is not None:
        args.min_grid_levels = args.grid_levels
        args.max_grid_levels = args.grid_levels
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

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

dg_order = args.degrees_of_freedom - 1

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="TafjordLandslide",
    dimensions=2,
    offset=DOMAIN_OFFSET,
    size=DOMAIN_SIZE,
    periodic_BC=[False, False],
    directory=".",
    executable="ExaSkred-Landslide",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The Tafjord landslide application is two-dimensional.")

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={"h": 1, "hu": 1, "hv": 1, "z": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.20,
)
aderdg_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    ncp=aderdg_nonconservative_product,
    diffusive_source_term=aderdg_diffusive_source_term,
    riemann_solver=aderdg_riemann_solver,
)
aderdg_solver.add_user_solver_includes(
    """
#include "../Flux.h"
#include "../NonconservativeProduct.h"
#include "../Riemann.h"
#include "../SourceTerm.h"
#include "tarch/reader/NetCDFFieldSampler.h"
"""
)
aderdg_solver.add_kernel_optimisations(
    is_linear=False, polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=2 * dg_order + 1,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"z": 1},
    time_step_relaxation=0.20,
)
fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    ncp=fv_nonconservative_product,
    diffusive_source_term=fv_diffusive_source_term,
    riemann_solver=fv_riemann_solver,
    adjust_solution=adjust_solution,
)
fv_solver.add_user_solver_includes(
    """
#include "../Flux.h"
#include "../NonconservativeProduct.h"
#include "../Riemann.h"
#include "../SourceTerm.h"
#include "tarch/reader/NetCDFFieldSampler.h"
"""
)

limiter_solver = exahype2.solvers.limiting.PosterioriLimiting(
    name="LimiterSolver",
    regular_solver=aderdg_solver,
    limiting_solver=fv_solver,
    number_of_dmp_observables=3,
    dmp_relaxation_parameter=0.001,
    dmp_differences_scaling=0.0001,
    physical_admissibility_criterion=limiting_criterion,
)

fv_solver.set_plotter(peano4.plotter.NetCDFPatchAMRPlotter)
aderdg_solver.set_plotter(peano4.plotter.NetCDFPatchNodalPlotter)

project.add_solver(aderdg_solver)
project.add_solver(fv_solver)
project.add_solver(limiter_solver)
project.add_runtime_parameter("bathymetryFile", "string", BATHYMETRY_FILE)
project.add_runtime_parameter("displacementFile", "string", DISPLACEMENT_FILE)

peano_project = project.generate_Peano4_project()

for const_name, (const_val, const_type) in constants.items():
    peano_project.constants.export_constexpr_with_type(
        const_name, str(const_val), const_type
    )

peano_project.generate_and_build()
