# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import exahype2

initial_conditions = """
  const auto bathymetrySize   = RuntimeParameters::getBathymetrySize();
  const auto bathymetryOffset = RuntimeParameters::getBathymetryOffset();
  static tarch::reader::NetCDFFieldSampler bathymetrySampler(
    {
      {\"bathymetry\", RuntimeParameters::getBathymetryFile()}
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
      {\"displacement\", RuntimeParameters::getDisplacementFile()}
    },
    displacementSize(0),
    displacementSize(1),
    displacementOffset(0),
    displacementOffset(1)
  );

  const auto displacementPosition         = RuntimeParameters::getDisplacementPosition();
  const double bathymetryBeforeEarthquake = bathymetrySampler.sample(\"bathymetry\", x(0), x(1));
  const double displacement               = displacementSampler.sample(\"displacement\", x(0) - displacementPosition(0), x(1) - displacementPosition(1));
  const double bathymetryAfterEarthquake  = bathymetryBeforeEarthquake + displacement;

  Q[Shortcuts::h]  = -std::min(bathymetryBeforeEarthquake, 0.0);
  Q[Shortcuts::hu] = 0.0;
  Q[Shortcuts::hv] = 0.0;
  Q[Shortcuts::z]  = bathymetryAfterEarthquake;
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = 0.0;
  Qoutside[Shortcuts::hv] = 0.0;
  Qoutside[Shortcuts::z]  = Qinside[Shortcuts::z];
"""

is_physically_admissible = """
  if (tarch::la::smallerEquals(Q[0], 1000.0)) {
    return false;
  }
  return true;
"""

parser = exahype2.ArgumentParser()
parser.add_argument(
    "--use-probes",
    action="store_true",
    help="Insert stationary probes and dump tracer data at their positions.",
)
parser.set_defaults(
    min_depth=4,
    end_time=5600.0,
    degrees_of_freedom=7,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
}

size = [1798000, 1798000]
offset = [-499000.0, -949000.0]

probe_coordinates = [
    [545735.266126, 62716.4740303],
    [1050678.21, 798352.124]
]

max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)
dg_order = args.degrees_of_freedom - 1

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={"h": 1, "hu": 1, "hv": 1, "z": 1},
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.9,
)

aderdg_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    flux="""
  double ih = 1.0 / Q[0];
  F[0] = Q[1 + normal];
  F[1] = Q[1 + normal] * Q[1] * ih;
  F[2] = Q[1 + normal] * Q[2] * ih;
  F[3] = 0.0;
""",
    max_eigenvalue="return ShallowWater::maxEigenvalue<double, Shortcuts>(Q, x, h, t, dt, normal);",
    ncp=f"ShallowWater::nonconservativeProduct<double, Shortcuts, {aderdg_solver.unknowns}>(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);",
    riemann_solver="""
  // Compute max eigenvalue over face
  double smax = 0.0;
  for(int xy=0; xy<Order+1; xy++){
    smax = std::max(smax, maxEigenvalue(&QL[xy*4], x, h, t, dt, direction));
    smax = std::max(smax, maxEigenvalue(&QR[xy*4], x, h, t, dt, direction));
  }

  for(int xy=0; xy<Order+1; xy++){
    // h gets added term from bathymetry, u and v are regular Rusanov, b does not change
    FL[xy*4+0] = 0.5*(FL[xy*4+0]+FR[xy*4+0] + smax*(QL[xy*4+0]+QL[xy*4+3]-QR[xy*4+0]-QR[xy*4+3]) );
    FL[xy*4+1] = 0.5*(FL[xy*4+1]+FR[xy*4+1] + smax*(QL[xy*4+1]-QR[xy*4+1]) );
    FL[xy*4+2] = 0.5*(FL[xy*4+2]+FR[xy*4+2] + smax*(QL[xy*4+2]-QR[xy*4+2]) );
    FL[xy*4+3] = 0.0;

    FR[xy*4+0] = FL[xy*4+0];
    FR[xy*4+1] = FL[xy*4+1];
    FR[xy*4+2] = FL[xy*4+2];
    FR[xy*4+3] = 0.0;

    // Contribution from NCP
    FL[xy*4+direction+1] += 0.5*g*0.5*(QL[xy*4+0]+QR[xy*4+0])*(QR[xy*4+3]+QR[xy*4+0]-QL[xy*4+3]-QL[xy*4+0]);
    FR[xy*4+direction+1] -= 0.5*g*0.5*(QL[xy*4+0]+QR[xy*4+0])*(QR[xy*4+3]+QR[xy*4+0]-QL[xy*4+3]-QL[xy*4+0]);
  }
""",
)

aderdg_solver.add_user_solver_includes(
    """
#include "../PDE.h"
#include "tarch/reader/NetCDFFieldSampler.h"
"""
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=dg_order * 2 + 1,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"z": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.9,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    riemann_solver="return fWaveRiemannSolver<double, Shortcuts>(QL, QR, x, h, t, dt, normal, FL, FR);",
)

fv_solver.add_user_solver_includes(
    """
#include "../FWaveRiemannSolver.h"
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
    namespace=["applications", "exahype2", "ShallowWater"],
    project_name="TohokuTsunami",
    directory=".",
    executable="ExaHyPE",
)

project.add_solver(aderdg_solver)
project.add_solver(fv_solver)
project.add_solver(limiter_solver)
project.add_runtime_parameter("bathymetryFile", "string", "tohoku_gebco_ucsb3_2000m_hawaii_bath.nc")
project.add_runtime_parameter("displacementFile", "string", "tohoku_gebco_ucsb3_2000m_hawaii_displ.nc")
project.add_runtime_parameter("bathymetrySize", ("double", 2), [6998000.0, 3998000.0])
project.add_runtime_parameter("bathymetryOffset", ("double", 2), [-499000.0, -2490000.0])
project.add_runtime_parameter("displacementSize", ("double", 2), [498000.0, 798000.0])
project.add_runtime_parameter("displacementOffset", ("double", 2), [-249000.0, -399000.0])
project.add_runtime_parameter("displacementPosition", ("double", 2), [0.0, 0.0])

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

if args.use_probes and probe_coordinates:
    os.makedirs(args.output, exist_ok=True)

    probe_particles = project.add_tracer(name="Probe", particle_attributes=4)

    project.add_action_set_to_initialisation(
        exahype2.tracer.InsertParticlesByCoordinates(
            particle_set=probe_particles,
            coordinates=probe_coordinates,
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
            data_delta_between_two_snapsots=1e16,
            time_delta_between_two_snapsots=50,
            output_precision=17,
            clear_database_after_flush=False,
        )
    )

    aderdg_solver._abstract_solver_user_declarations += "double QuadraturePoints1d[Order+1];"
    aderdg_solver._start_simulation_implementation += """
    std::copy_n(
      kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
      (Order+1),
      QuadraturePoints1d
    );"""

project.set_global_simulation_parameters(
    dimensions=2,
    size=size,
    offset=offset,
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ],
)

project.set_load_balancer(
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees})"
)
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)
project.output.makefile.set_target_device(args.target_device)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
