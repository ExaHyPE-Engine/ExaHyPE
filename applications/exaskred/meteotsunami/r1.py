# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""
Meteotsunami validation case based on the canonical R1 Proudman-resonance setup.

A travelling atmospheric pressure disturbance moves across an idealised flat basin,
and the generated free-surface response is compared for different water depths to
recover the expected Froude-number-dependent amplification.

The model advances the two-dimensional nonlinear shallow-water system in terms of
water depth ``h``, depth-averaged momenta ``hu`` and ``hv``, and bathymetry ``b``.
In addition to the bathymetric source term, this setup includes atmospheric pressure
forcing, a constant Coriolis term, and Manning bottom friction.
"""

import math

import exahype2
import peano4

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
  if (x(0) < PhysicalDomainOffsetX || x(0) > PhysicalDomainOffsetX + PhysicalDomainSizeX) {
    return false;
  }
  if (x(1) < PhysicalDomainOffsetY || x(1) > PhysicalDomainOffsetY + PhysicalDomainSizeY) {
    return false;
  }

  if (std::abs(x[0] - DomainOffset[0]) < h[0] || std::abs(x[0] - DomainOffset[0] - DomainSize[0]) < h[0]) {
    return false;
  }
  if (std::abs(x[1] - DomainOffset[1]) < h[1] || std::abs(x[1] - DomainOffset[1] - DomainSize[1]) < h[1]) {
    return false;
  }

  if (x(0) - PhysicalDomainOffsetX > 0.85 * PhysicalDomainSizeX) {
    return false;
  }
  if (x(0) - PhysicalDomainOffsetX < 0.15 * PhysicalDomainSizeX) {
    return false;
  }

  return true;
"""

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=2,
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
    "water_density": [1000.0, "double"],
    "manning_param": [manning_coefficient(DEPTH), "double"],
    "corriolis_param": [9.35e-5, "double"],
    "pressure": [100.0, "double"],
    "speed_pressure": [22.15, "double"],
    "period_pressure": [1800.0, "double"],
    "background_pressure": [101300.0, "double"],
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
#include "MathDerivatives.h"
#include "MeteotsunamiLaws.h"
#include "R1AtmosphericPressure.h"
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
#include "MathDerivatives.h"
#include "MeteotsunamiLaws.h"
#include "R1AtmosphericPressure.h"
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
    project_name="MeteotsunamiR1",
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

peano_project.output.makefile.add_cpp_file("MathDerivatives.cpp")
peano_project.output.makefile.add_cpp_file("MeteotsunamiLaws.cpp")
peano_project.output.makefile.add_cpp_file("R1AtmosphericPressure.cpp")
peano_project.output.makefile.set_target_device(args.target_device)
peano_project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
