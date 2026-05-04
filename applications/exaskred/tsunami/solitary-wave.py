# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Solitary-wave runup benchmark."""

import exahype2
import peano4

DOMAIN_OFFSET = [-5.0, 0.0]
DOMAIN_SIZE = [15.0, 15.0]
OUTPUT_PATH = "solutions"
END_TIME = 5.0
FIRST_PLOT_TIME_STAMP = 0.0
PERIODIC_BOUNDARY_CONDITIONS = [False, False]

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  constexpr double depth           = 1.0;
  constexpr double beachSlopeAngle = 30.0 * std::numbers::pi / 180.0;
  constexpr double waveAmplitude   = 0.5;

  const double gamma    = std::sqrt((3.0 * waveAmplitude) / (4.0 * depth));
  const double wavePos  = std::acosh(std::sqrt(20.0)) / gamma;
  const double shorelineX = depth / std::tan(beachSlopeAngle);

  if (x[0] < 0.0) {
    Q[Shortcuts::b] = std::fabs(x[0]) * std::tan(beachSlopeAngle);
  } else if (x[0] < shorelineX) {
    Q[Shortcuts::b] = -(x[0] * std::tan(beachSlopeAngle));
  } else {
    Q[Shortcuts::b] = -depth;
  }

  const double innerTerm   = gamma * ((x[0] - (shorelineX + wavePos)) / depth);
  const double waveProfile = waveAmplitude / std::pow(std::cosh(innerTerm), 2);
  if (x[0] >= shorelineX) {
    Q[Shortcuts::h] = depth + waveProfile;
  } else if (x[0] >= 0.0) {
    Q[Shortcuts::h] = std::fabs(Q[Shortcuts::b]);
  } else if (x[0] >= (shorelineX + 2.0 * wavePos)) {
    Q[Shortcuts::h] = depth;
  }

  const double waveCelerity = std::sqrt(g * (waveAmplitude + depth));
  Q[Shortcuts::hu] = -((waveCelerity * waveProfile) / (depth + waveProfile)) * Q[Shortcuts::h];
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hv] = -Qinside[Shortcuts::hv];
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
  return ::exahype2::RefinementCommand::Keep;
"""

riemann_solver = """
  return ::applications::exahype2::exaskred::riemann::llf<Shortcuts>(
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

parser = exahype2.ArgumentParser()
parser.set_defaults(
    output=OUTPUT_PATH,
    min_depth=5,
    degrees_of_freedom=16,
    end_time=END_TIME,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
}

size = DOMAIN_SIZE
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"b": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    ncp=nonconservative_product,
    riemann_solver=riemann_solver,
)
fv_solver.add_user_solver_includes(
    """
#include <cmath>
#include <numbers>
#include "Flux.h"
#include "NonconservativeProduct.h"
#include "riemann/LLF.h"
"""
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="SolitaryWave",
    directory=".",
    executable="ExaSkred-Tsunami",
)
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
    first_plot_time_stamp=FIRST_PLOT_TIME_STAMP,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x or PERIODIC_BOUNDARY_CONDITIONS[0],
        args.periodic_boundary_conditions_y or PERIODIC_BOUNDARY_CONDITIONS[1],
    ],
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
