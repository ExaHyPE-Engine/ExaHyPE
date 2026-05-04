# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Artificial tsunami benchmark with imposed bathymetry displacement input data."""

import exahype2
import peano4

DOMAIN_OFFSET = [0.0, 0.0]
DOMAIN_SIZE = [10000.0, 10000.0]
OUTPUT_PATH = "solutions"
END_TIME = 50.0
FIRST_PLOT_TIME_STAMP = 0.0
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

parser = exahype2.ArgumentParser()
parser.set_defaults(
    output=OUTPUT_PATH,
    min_depth=4,
    end_time=END_TIME,
    degrees_of_freedom=4,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
}

size = DOMAIN_SIZE
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1, "b": 1},
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
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
#include <algorithm>
#include "Flux.h"
#include "NonconservativeProduct.h"
#include "tarch/reader/NetCDFFieldSampler.h"
"""
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="ArtificialTsunami",
    directory=".",
    executable="ExaSkred-Tsunami",
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
