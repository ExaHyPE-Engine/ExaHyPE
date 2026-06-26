# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import math

import exahype2

LENGTH = 300e3
PHYSICAL_OFFSET = [-39870.0, 0.0]
PHYSICAL_SIZE = [379700.0, LENGTH / 3.0]
SQUARE_SIDE = max(PHYSICAL_SIZE)

DOMAIN_OFFSET = [
    PHYSICAL_OFFSET[0] - 0.5 * (SQUARE_SIDE - PHYSICAL_SIZE[0]),
    PHYSICAL_OFFSET[1] - 0.5 * (SQUARE_SIDE - PHYSICAL_SIZE[1]),
]
DOMAIN_SIZE = [SQUARE_SIDE, SQUARE_SIDE]


def manning_coefficient(depth: float, roughness_length: float = 0.003) -> float:
    if depth <= 0.0:
        raise ValueError("Water depth must be positive.")

    chezy = 18.0 * math.log10(0.37 * depth / roughness_length)
    return depth ** (1.0 / 6.0) / chezy


DEPTH = 50
BATHYMETRY_FILE = "bathymetry/test_R1_dx100_H50_b.grd"

initial_conditions = f"""
  static tarch::reader::NetCDFFieldSampler fieldSampler(
    {{
      {{"bathymetry", RuntimeParameters::getBathymetryFile(), "z", "x", "y"}}
    }},
    PhysicalDomainSizeX,
    PhysicalDomainSizeY,
    PhysicalDomainOffsetX,
    PhysicalDomainOffsetY
  );

  const double sampleX =
    x(0) < PhysicalDomainOffsetX ? PhysicalDomainOffsetX :
    (x(0) > PhysicalDomainOffsetX + PhysicalDomainSizeX ? PhysicalDomainOffsetX + PhysicalDomainSizeX : x(0));
  const double sampleY =
    x(1) < PhysicalDomainOffsetY ? PhysicalDomainOffsetY :
    (x(1) > PhysicalDomainOffsetY + PhysicalDomainSizeY ? PhysicalDomainOffsetY + PhysicalDomainSizeY : x(1));

  const double bathymetry = fieldSampler.sample("bathymetry", sampleX, sampleY);
  Q[Shortcuts::h]  = -std::min(bathymetry, 0.0);
  Q[Shortcuts::hu] = 0.0;
  Q[Shortcuts::hv] = 0.0;
  Q[Shortcuts::b]  = bathymetry;
"""

boundary_conditions = """
  if (normal == 0) {
    Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
    Qoutside[Shortcuts::hu] = Qinside[Shortcuts::hu];
    Qoutside[Shortcuts::hv] = Qinside[Shortcuts::hv];
    Qoutside[Shortcuts::b]  = Qinside[Shortcuts::b];
  } else {
    Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
    Qoutside[Shortcuts::hu] = Qinside[Shortcuts::hu];
    Qoutside[Shortcuts::hv] = -Qinside[Shortcuts::hv];
    Qoutside[Shortcuts::b]  = Qinside[Shortcuts::b];
  }
"""

source_term = """
  for (int n = 0; n < NumberOfUnknowns; ++n) {
    S[n] = 0.0;
  }

  if (Q[Shortcuts::h] <= hThreshold) {
    return;
  }

  const double xCoord = x(0);
  const double yCoord = x(1);
  const double coriolis =
    ::applications::exahype2::exaskred::meteotsunami::coriolisForce::constant(corriolis_param);
  const double friction =
    ::applications::exahype2::exaskred::meteotsunami::bottomFriction::manningFriction(
      Q[Shortcuts::h],
      Q[Shortcuts::hu],
      Q[Shortcuts::hv],
      g,
      manning_param
    );

  S[Shortcuts::hu] = coriolis * Q[Shortcuts::hv] + friction * Q[Shortcuts::hu];
  S[Shortcuts::hv] = -coriolis * Q[Shortcuts::hu] + friction * Q[Shortcuts::hv];
  S[Shortcuts::hu] += -Q[Shortcuts::h]
    * ::applications::exahype2::exaskred::meteotsunami::numerics::partialX(
      ::applications::exahype2::exaskred::meteotsunami::r1::atmosphericPressure,
      xCoord,
      yCoord,
      t
    ) / water_density;
  S[Shortcuts::hv] += -Q[Shortcuts::h]
    * ::applications::exahype2::exaskred::meteotsunami::numerics::partialY(
      ::applications::exahype2::exaskred::meteotsunami::r1::atmosphericPressure,
      xCoord,
      yCoord,
      t
    ) / water_density;
"""

flux = """
  ::applications::exahype2::exaskred::meteotsunami::flux<Shortcuts, NumberOfUnknowns>(
    Q, x, h, t, dt, normal, F
  );
"""

max_eigenvalue = """
  return ::applications::exahype2::exaskred::meteotsunami::maxEigenvalue<Shortcuts>(
    Q, x, h, t, dt, normal
  );
"""

nonconservative_product = """
  ::applications::exahype2::exaskred::meteotsunami::nonconservativeProduct<Shortcuts, NumberOfUnknowns>(
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
  if (x(0) < PhysicalDomainOffsetX - h[0] || x(0) > PhysicalDomainOffsetX + PhysicalDomainSizeX + h[0]) {
    return false;
  }
  if (x(1) < PhysicalDomainOffsetY - h[1] || x(1) > PhysicalDomainOffsetY + PhysicalDomainSizeY + h[1]) {
    return false;
  }

  if (std::abs(x[0] - DomainOffset[0]) < h[0] || std::abs(x[0] - DomainOffset[0] - DomainSize[0]) < h[0]) {
    return false;
  }
  if (std::abs(x[1] - DomainOffset[1]) < h[1] || std::abs(x[1] - DomainOffset[1] - DomainSize[1]) < h[1]) {
    return false;
  }

  const double targetMinX = PhysicalDomainOffsetX + 0.15 * PhysicalDomainSizeX;
  const double targetMaxX = PhysicalDomainOffsetX + 0.85 * PhysicalDomainSizeX;

  if (x(0) > targetMaxX + h[0]) {
    return false;
  }
  if (x(0) < targetMinX - h[0]) {
    return false;
  }

  return true;
"""


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=100.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=7)
    parser.add_argument("-l", "--grid-levels", type=int, default=2)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
    "PhysicalDomainOffsetX": [PHYSICAL_OFFSET[0], "double"],
    "PhysicalDomainOffsetY": [PHYSICAL_OFFSET[1], "double"],
    "PhysicalDomainSizeX": [PHYSICAL_SIZE[0], "double"],
    "PhysicalDomainSizeY": [PHYSICAL_SIZE[1], "double"],
    "water_density": [1000.0, "double"],
    "manning_param": [manning_coefficient(DEPTH), "double"],
    "corriolis_param": [9.35e-5, "double"],
    "pressure": [100.0, "double"],
    "speed_pressure": [22.15, "double"],
    "period_pressure": [1800.0, "double"],
    "background_pressure": [101300.0, "double"],
}

dg_order = args.degrees_of_freedom - 1

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="MeteotsunamiR1",
    dimensions=2,
    offset=DOMAIN_OFFSET,
    size=DOMAIN_SIZE,
    periodic_bc=[False, False],
    executable="ExaSkred-Meteotsunami",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The R1 meteotsunami application is two-dimensional.")

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
    ncp=nonconservative_product,
    max_eigenvalue=max_eigenvalue,
    riemann_solver=aderdg_riemann_solver,
    source_term=source_term,
)
aderdg_solver.add_user_solver_includes(
    """
#include <algorithm>
#include "../Flux.h"
#include "../NonconservativeProduct.h"
#include "../../tsunami/riemann/WellBalancedRusanov.h"
#include "../MathDerivatives.h"
#include "../MeteotsunamiLaws.h"
#include "AtmosphericPressure.h"
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
    ncp=nonconservative_product,
    max_eigenvalue=max_eigenvalue,
    riemann_solver=fv_riemann_solver,
    source_term=source_term,
)
fv_solver.add_user_solver_includes(
    """
#include "../Flux.h"
#include "../NonconservativeProduct.h"
#include "../../tsunami/riemann/FWave.h"
#include "../MathDerivatives.h"
#include "../MeteotsunamiLaws.h"
#include "AtmosphericPressure.h"
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
project.add_runtime_parameter("bathymetryFile", "string", BATHYMETRY_FILE)
for const_name, (const_val, const_type) in constants.items():
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)

project.output.makefile.add_cpp_file("../MathDerivatives.cpp")
project.output.makefile.add_cpp_file("../MeteotsunamiLaws.cpp")
project.output.makefile.add_cpp_file("AtmosphericPressure.cpp")
project.generate_and_build()
