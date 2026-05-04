# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import exahype2

DOMAIN_OFFSET = [-499000.0, -949000.0]
DOMAIN_SIZE = [1798000.0, 1798000.0]
OUTPUT_PATH = "solutions"
END_TIME = 5600.0
FIRST_PLOT_TIME_STAMP = 0.0
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

parser = exahype2.ArgumentParser()
parser.add_argument(
    "--use-probes",
    action="store_true",
    help="Insert stationary probes and dump tracer data at their positions.",
)
parser.set_defaults(
    output=OUTPUT_PATH,
    min_depth=4,
    end_time=END_TIME,
    degrees_of_freedom=7,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
}

size = DOMAIN_SIZE
max_h = 1.1 * min(size) / (3.0**args.min_depth)
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
    max_eigenvalue=max_eigenvalue,
    ncp=nonconservative_product,
    riemann_solver=aderdg_riemann_solver,
)
aderdg_solver.add_user_solver_includes(
    """
#include <algorithm>
#include "Flux.h"
#include "NonconservativeProduct.h"
#include "riemann/WellBalancedRusanov.h"
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
    max_eigenvalue=max_eigenvalue,
    ncp=nonconservative_product,
    riemann_solver=fv_riemann_solver,
)
fv_solver.add_user_solver_includes(
    """
#include "Flux.h"
#include "NonconservativeProduct.h"
#include "riemann/FWave.h"
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
    project_name="TohokuTsunami",
    directory=".",
    executable="ExaSkred-Tsunami",
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
    os.makedirs(args.output, exist_ok=True)

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
            data_delta_between_two_snapshots=1e16,
            time_delta_between_two_snapshots=50,
            output_precision=17,
            clear_database_after_flush=False,
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
