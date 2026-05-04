# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""
Clearwater Beach meteotsunami scenario with projected GEBCO bathymetry and a
moving atmospheric pressure disturbance over the Gulf of Mexico.

The bathymetry is taken from a GEBCO subset over the geographic window
[-98 deg, -70 deg] x [15 deg, 35 deg] and reprojected to an Albers Equal Area
Cartesian grid. The atmospheric forcing is an asymmetric crest-trough pressure
footprint that is advected along a path from A = (-85.107 deg, 30.747 deg) to
B = (-81.562 deg, 23.448 deg).
"""

import exahype2
import peano4

PHYSICAL_OFFSET = [-1514683.37644735, -1108652.87362612]
PHYSICAL_SIZE = [3029257.84023389, 2284903.696092]
SQUARE_SIDE = max(PHYSICAL_SIZE)

DOMAIN_OFFSET = [
    PHYSICAL_OFFSET[0] - 0.5 * (SQUARE_SIDE - PHYSICAL_SIZE[0]),
    PHYSICAL_OFFSET[1] - 0.5 * (SQUARE_SIDE - PHYSICAL_SIZE[1]),
]
DOMAIN_SIZE = [SQUARE_SIDE, SQUARE_SIDE]
BATHYMETRY_FILE = "bathymetry/gebco_projection.nc"

initial_conditions = """
  static tarch::reader::NetCDFFieldSampler fieldSampler(
    {
      {"bathymetry", RuntimeParameters::getBathymetryFile(), "Band1", "x", "y"}
    },
    PhysicalDomainSizeX,
    PhysicalDomainSizeY,
    PhysicalDomainOffsetX,
    PhysicalDomainOffsetY
  );

  const double bathymetry = fieldSampler.sample("bathymetry", x(0), x(1));

  if (bathymetry < -15000.0) {
    Q[Shortcuts::h]  = 999.0;
    Q[Shortcuts::hu] = 0.0;
    Q[Shortcuts::hv] = 0.0;
    Q[Shortcuts::b]  = -999.0;
  } else {
    Q[Shortcuts::h]  = -std::min(bathymetry, 0.0);
    Q[Shortcuts::hu] = 0.0;
    Q[Shortcuts::hv] = 0.0;
    Q[Shortcuts::b]  = bathymetry;
  }
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = 0.0;
  Qoutside[Shortcuts::hv] = 0.0;
  Qoutside[Shortcuts::b]  = Qinside[Shortcuts::b];
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
  const auto   lonLat =
    ::applications::exahype2::exaskred::meteotsunami::projection::aea::inverseEllipsoid(xCoord, yCoord);
  const double coriolis =
    ::applications::exahype2::exaskred::meteotsunami::coriolisForce::sphere(lonLat.second, coriolis_omega);
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
      ::applications::exahype2::exaskred::meteotsunami::gulf_of_mexico::atmosphericPressure,
      xCoord,
      yCoord,
      t
    ) / water_density;
  S[Shortcuts::hv] += -Q[Shortcuts::h]
    * ::applications::exahype2::exaskred::meteotsunami::numerics::partialY(
      ::applications::exahype2::exaskred::meteotsunami::gulf_of_mexico::atmosphericPressure,
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
  if (tarch::la::smallerEquals(Q[0], 10.0)) {
    return false;
  }

  if (std::abs(x[0] - DomainOffset[0]) < h[0] || std::abs(x[0] - DomainOffset[0] - DomainSize[0]) < h[0]) {
    return false;
  }
  if (std::abs(x[1] - DomainOffset[1]) < h[1] || std::abs(x[1] - DomainOffset[1] - DomainSize[1]) < h[1]) {
    return false;
  }

  if (x(0) - PhysicalDomainOffsetX > 0.6 * PhysicalDomainSizeX) {
    return false;
  }
  if (x(1) - PhysicalDomainOffsetY < 0.4 * PhysicalDomainSizeY) {
    return false;
  }
  if (x(0) - PhysicalDomainOffsetX < 0.3 * PhysicalDomainSizeX) {
    return false;
  }
  if (x(1) - PhysicalDomainOffsetY > 0.85 * PhysicalDomainSizeY) {
    return false;
  }

  return true;
"""

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=3,
    end_time=100.0,
    degrees_of_freedom=7,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
    "PhysicalDomainOffsetX": [PHYSICAL_OFFSET[0], "double"],
    "PhysicalDomainOffsetY": [PHYSICAL_OFFSET[1], "double"],
    "PhysicalDomainSizeX": [PHYSICAL_SIZE[0], "double"],
    "PhysicalDomainSizeY": [PHYSICAL_SIZE[1], "double"],
    "aeaEllipsoidA": [6378137.0, "double"],
    "aeaEllipsoidE": [0.08181919084262157, "double"],
    "aeaStdParallel1": [18.3333333, "double"],
    "aeaStdParallel2": [31.6666667, "double"],
    "aeaLon0": [-84.0, "double"],
    "aeaLat0": [25.0, "double"],
    "aeaEarthRadius": [6371000.0, "double"],
    "pointLonA": [-85.107, "double"],
    "pointLatA": [30.747, "double"],
    "pointLonB": [-81.562, "double"],
    "pointLatB": [23.4480, "double"],
    "water_density": [1000.0, "double"],
    "coriolis_omega": [7.2921150e-5, "double"],
    "manning_param": [0.025, "double"],
}

max_h = 1.1 * min(PHYSICAL_SIZE) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)
dg_order = args.degrees_of_freedom - 1

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={"h": 1, "hu": 1, "hv": 1, "b": 1},
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
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
#include "Flux.h"
#include "NonconservativeProduct.h"
#include "../tsunami/riemann/WellBalancedRusanov.h"
#include "GulfOfMexicoAtmosphericPressure.h"
#include "MathDerivatives.h"
#include "MeteotsunamiLaws.h"
#include "ProjectionAEA.h"
#include "tarch/reader/NetCDFFieldSampler.h"
"""
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=2 * dg_order + 1,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"b": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
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
#include "Flux.h"
#include "NonconservativeProduct.h"
#include "../tsunami/riemann/FWave.h"
#include "GulfOfMexicoAtmosphericPressure.h"
#include "MathDerivatives.h"
#include "MeteotsunamiLaws.h"
#include "ProjectionAEA.h"
#include "tarch/reader/NetCDFFieldSampler.h"
"""
)

limiter_solver = exahype2.solvers.limiting.StaticLimiting(
    name="LimiterSolver",
    regular_solver=aderdg_solver,
    limiting_solver=fv_solver,
    physical_admissibility_criterion=is_physically_admissible,
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="MeteotsunamiGulfOfMexico",
    directory=".",
    executable="ExaSkred-Meteotsunami",
)
project.add_solver(aderdg_solver)
project.add_solver(fv_solver)
project.add_solver(limiter_solver)
project.add_runtime_parameter("bathymetryFile", "string", BATHYMETRY_FILE)

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
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ],
)

project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
peano_project = project.generate_Peano4_project(verbose=False)
for const_name, (const_val, const_type) in constants.items():
    peano_project.constants.export_constexpr_with_type(
        const_name, str(const_val), const_type
    )

peano_project.output.makefile.add_cpp_file("GulfOfMexicoAtmosphericPressure.cpp")
peano_project.output.makefile.add_cpp_file("MathDerivatives.cpp")
peano_project.output.makefile.add_cpp_file("MeteotsunamiLaws.cpp")
peano_project.output.makefile.add_cpp_file("ProjectionAEA.cpp")
peano_project.output.makefile.set_target_device(args.target_device)
peano_project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
