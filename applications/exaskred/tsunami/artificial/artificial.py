# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

DOMAIN_OFFSET = [0.0, 0.0]
DOMAIN_SIZE = [10000.0, 10000.0]
END_TIME = 50.0
PERIODIC_BOUNDARY_CONDITIONS = [False, False]
BATHYMETRY_FILE = "bathymetry/artificial_bath_1000.nc"
DISPLACEMENT_FILE = "bathymetry/artificial_displ_1000.nc"
BATHYMETRY_SIZE = [10000.0, 10000.0]
BATHYMETRY_OFFSET = [0.0, 0.0]
DISPLACEMENT_SIZE = [10000.0, 10000.0]
DISPLACEMENT_OFFSET = [0.0, 0.0]
DISPLACEMENT_POSITION = [0.0, 0.0]

initial_conditions = """
  const auto bathymetrySize   = RuntimeParameters::getBathymetrySize();
  const auto bathymetryOffset = RuntimeParameters::getBathymetryOffset();
  static tarch::reader::NetCDFFieldSampler bathymetrySampler(
    {
      {"bathymetry", RuntimeParameters::getBathymetryFile()}
    },
    bathymetrySize(0),
    bathymetrySize(1),
    bathymetryOffset(0),
    bathymetryOffset(1)
  );

  const auto displacementSize   = RuntimeParameters::getDisplacementSize();
  const auto displacementOffset = RuntimeParameters::getDisplacementOffset();
  static tarch::reader::NetCDFFieldSampler displacementSampler(
    {
      {"displacement", RuntimeParameters::getDisplacementFile()}
    },
    displacementSize(0),
    displacementSize(1),
    displacementOffset(0),
    displacementOffset(1)
  );

  const auto displacementPosition         = RuntimeParameters::getDisplacementPosition();
  const double bathymetryBeforeEarthquake = bathymetrySampler.sample("bathymetry", x(0), x(1));
  const double displacement               = displacementSampler.sample(
    "displacement",
    x(0) - displacementPosition(0),
    x(1) - displacementPosition(1)
  );
  const double bathymetryAfterEarthquake  = bathymetryBeforeEarthquake + displacement;

  Q[Shortcuts::h]  = -std::min(bathymetryBeforeEarthquake, 0.0);
  Q[Shortcuts::hu] = 0.0;
  Q[Shortcuts::hv] = 0.0;
  Q[Shortcuts::b]  = bathymetryAfterEarthquake;
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = 0.0;
  Qoutside[Shortcuts::hv] = 0.0;
  Qoutside[Shortcuts::b]  = Qinside[Shortcuts::b];
"""

flux = """
  ::applications::exahype2::exaskred::tsunami::flux<Shortcuts, NumberOfUnknowns>(
    Q, x, h, t, dt, normal, F
  );
"""

max_eigenvalue = """
  return ::applications::exahype2::exaskred::tsunami::maxEigenvalue<Shortcuts>(
    Q, x, h, t, dt, normal
  );
"""

nonconservative_product = """
  ::applications::exahype2::exaskred::tsunami::nonconservativeProduct<Shortcuts, NumberOfUnknowns>(
    Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ
  );
"""

refinement_criterion = """
  return x[0] >= 4500.0 && x[1] >= 4500.0 && x[0] <= 5500.0 && x[1] <= 5500.0
    ? ::exahype2::RefinementCommand::Refine
    : ::exahype2::RefinementCommand::Keep;
"""


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=END_TIME)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=4)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=4)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=4)
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
    "hThreshold": [1e-5, "double"],
}

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="ArtificialTsunami",
    dimensions=2,
    offset=DOMAIN_OFFSET,
    size=DOMAIN_SIZE,
    periodic_bc=PERIODIC_BOUNDARY_CONDITIONS,
    executable="ExaSkred-Tsunami",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The artificial tsunami application is two-dimensional.")

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1, "b": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.5,
)

aderdg_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    ncp=nonconservative_product,
)

aderdg_solver.add_kernel_optimisations(
    is_linear=False,
    polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre,
)
aderdg_solver.add_user_solver_includes(
    """
#include "../Flux.h"
#include "../NonconservativeProduct.h"
#include "tarch/reader/NetCDFFieldSampler.h"
"""
)

project.add_solver(aderdg_solver)

for key, type_name in (
    ("bathymetryFile", "string"),
    ("displacementFile", "string"),
    ("bathymetrySize", ("double", 2)),
    ("bathymetryOffset", ("double", 2)),
    ("displacementSize", ("double", 2)),
    ("displacementOffset", ("double", 2)),
    ("displacementPosition", ("double", 2)),
):
    project.add_runtime_parameter(
        key,
        type_name,
        {
            "bathymetryFile": BATHYMETRY_FILE,
            "displacementFile": DISPLACEMENT_FILE,
            "bathymetrySize": BATHYMETRY_SIZE,
            "bathymetryOffset": BATHYMETRY_OFFSET,
            "displacementSize": DISPLACEMENT_SIZE,
            "displacementOffset": DISPLACEMENT_OFFSET,
            "displacementPosition": DISPLACEMENT_POSITION,
        }[key],
    )

for const_name, (const_val, const_type) in constants.items():
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)

project.generate_and_build()
