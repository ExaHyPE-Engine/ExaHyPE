# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

DOMAIN_OFFSET = [-499000.0, -949000.0]
DOMAIN_SIZE = [1798000.0, 1798000.0]
END_TIME = 5600.0
PERIODIC_BOUNDARY_CONDITIONS = [False, False]
PROBE_COORDINATES = [
    [545735.266126, 62716.4740303],
    [1050678.21, 798352.124],
]
BATHYMETRY_FILE = "bathymetry/tohoku_gebco_ucsb3_2000m_hawaii_bath.nc"
DISPLACEMENT_FILE = "bathymetry/tohoku_gebco_ucsb3_2000m_hawaii_displ.nc"
BATHYMETRY_SIZE = [6998000.0, 3998000.0]
BATHYMETRY_OFFSET = [-499000.0, -2490000.0]
DISPLACEMENT_SIZE = [498000.0, 798000.0]
DISPLACEMENT_OFFSET = [-249000.0, -399000.0]
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
  return Q[0] > 1000.0;
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
    parser.add_argument(
        "--use-probes",
        action="store_true",
        help="Insert stationary probes and dump tracer data at their positions.",
    )
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
    project_name="TohokuTsunami",
    dimensions=2,
    offset=DOMAIN_OFFSET,
    size=DOMAIN_SIZE,
    periodic_bc=PERIODIC_BOUNDARY_CONDITIONS,
    executable="ExaSkred-Tsunami",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The Tohoku tsunami application is two-dimensional.")

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

if args.use_probes and PROBE_COORDINATES:
    probe_particles = project.add_tracer(name="Probe", particle_attributes=4)

    project.add_action_set_to_initialisation(
        exahype2.tracer.InsertParticlesByCoordinates(
            particle_set=probe_particles,
            coordinates=PROBE_COORDINATES,
        )
    )

    project.add_action_set_to_timestepping(
        exahype2.tracer.DiscontinuousGalerkinTracing(
            probe_particles,
            aderdg_solver,
            project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle",
        )
    )

    project.add_action_set_to_timestepping(
        exahype2.tracer.DumpTracerIntoDatabase(
            particle_set=probe_particles,
            solver=aderdg_solver,
            filename="Probes",
            time_delta_between_two_snapshots=50,
            output_precision=17,
        )
    )

    aderdg_solver._abstract_solver_user_declarations += (
        "double QuadraturePoints1d[Order+1];"
    )
    aderdg_solver._start_simulation_implementation += """
    std::copy_n(
      kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
      (Order+1),
      QuadraturePoints1d
    );"""

for const_name, (const_val, const_type) in constants.items():
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)

project.generate_and_build()
