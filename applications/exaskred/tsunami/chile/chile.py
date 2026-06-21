# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

DOMAIN_OFFSET = [3000000.0, 0.0]
DOMAIN_SIZE = [4000000.0, 4000000.0]
END_TIME = 5000.0
PERIODIC_BOUNDARY_CONDITIONS = [False, False]
BATHYMETRY_FILE = "bathymetry/chile_gebco_usgs_2000m_bath.nc"
DISPLACEMENT_FILE = "bathymetry/chile_gebco_usgs_2000m_displ.nc"
BATHYMETRY_SIZE = [7000000.0, 4000000.0]
BATHYMETRY_OFFSET = [0.0, 0.0]
DISPLACEMENT_SIZE = [7000000.0, 4000000.0]
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

aderdg_riemann_solver = """
  ::applications::exahype2::exaskred::riemann::wellBalancedRusanov<Shortcuts, NumberOfUnknowns, Order>(
    QL,
    QR,
    x,
    h,
    t,
    dt,
    direction,
    FL,
    FR
  );
"""

fv_riemann_solver = """
  return ::applications::exahype2::exaskred::riemann::fWave<Shortcuts>(
    QL,
    QR,
    x,
    h,
    t,
    dt,
    normal,
    FL,
    FR
  );
"""

is_physically_admissible = """
  bool isAdmissible = Q[0] > 100.0;

  tarch::la::Vector<DIMENSIONS, double> bblPos = {3050000.0, 400000.0};
  isAdmissible &= tarch::la::norm2(x - bblPos) > 5000.0;

  tarch::la::Vector<DIMENSIONS, double> blPos = {3050000.0, 940000.0};
  isAdmissible &= tarch::la::norm2(x - blPos) > 5000.0;

  tarch::la::Vector<DIMENSIONS, double> mPos = {5350000.0, 2320000.0};
  isAdmissible &= tarch::la::norm2(x - mPos) > 25000.0;

  tarch::la::Vector<DIMENSIONS, double> tlPos = {3790000.0, 3360000.0};
  isAdmissible &= tarch::la::norm2(x - tlPos) > 5000.0;

  tarch::la::Vector<DIMENSIONS, double> ttlPos = {3665000.0, 3410000.0};
  isAdmissible &= tarch::la::norm2(x - ttlPos) > 50000.0;

  tarch::la::Vector<DIMENSIONS, double> jfiPos = {5960000.0, 1040000.0};
  isAdmissible &= tarch::la::norm2(x - jfiPos) > 10000.0;

  return isAdmissible;
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
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=7)
    parser.add_argument("-l", "--grid-levels", type=int, default=4)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
}

dg_order = args.degrees_of_freedom - 1

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="ChileTsunami",
    dimensions=2,
    offset=DOMAIN_OFFSET,
    size=DOMAIN_SIZE,
    periodic_BC=PERIODIC_BOUNDARY_CONDITIONS,
    directory=".",
    executable="ExaSkred-Tsunami",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The Chile tsunami application is two-dimensional.")

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={"h": 1, "hu": 1, "hv": 1, "b": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.9,
)

aderdg_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    ncp=nonconservative_product,
    riemann_solver=aderdg_riemann_solver,
)
aderdg_solver.add_user_solver_includes(
    """
#include <algorithm>
#include "../Flux.h"
#include "../NonconservativeProduct.h"
#include "../riemann/WellBalancedRusanov.h"
#include "tarch/reader/NetCDFFieldSampler.h"
"""
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=2 * dg_order + 1,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"b": 1},
    time_step_relaxation=0.9,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    ncp=nonconservative_product,
    riemann_solver=fv_riemann_solver,
)
fv_solver.add_user_solver_includes(
    """
#include "../Flux.h"
#include "../NonconservativeProduct.h"
#include "../riemann/FWave.h"
#include "tarch/reader/NetCDFFieldSampler.h"
"""
)

limiter_solver = exahype2.solvers.limiting.StaticLimiting(
    name="LimiterSolver",
    regular_solver=aderdg_solver,
    limiting_solver=fv_solver,
    physical_admissibility_criterion=is_physically_admissible,
)

project.add_solver(aderdg_solver)
project.add_solver(fv_solver)
project.add_solver(limiter_solver)

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

peano_project = project.generate_Peano4_project()

for const_name, (const_val, const_type) in constants.items():
    peano_project.constants.export_constexpr_with_type(
        const_name, str(const_val), const_type
    )

peano_project.generate_and_build()
