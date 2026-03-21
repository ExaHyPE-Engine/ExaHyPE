# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

physical_offset = [-1514683.37644735, -1108652.87362612]
physical_size = [3029257.84023389, 2284903.696092]
square_side = max(physical_size)

offset = [
    physical_offset[0] - 0.5 * (square_side - physical_size[0]),
    physical_offset[1] - 0.5 * (square_side - physical_size[1]),
]
size = [square_side, square_side]

initial_conditions = """
  static tarch::reader::NetCDFFieldParser fieldParser(
    "bathymetry/gebco_projection.nc",
    PhysicalDomainSizeX,
    PhysicalDomainSizeY,
    PhysicalDomainOffsetX,
    PhysicalDomainOffsetY,
    "Band1",
    "x",
    "y"
  );

  const double bathymetry = fieldParser.sampleTopology(x(0), x(1));

  if (bathymetry < -15000) {
    Q[Shortcuts::h]  = 999;
    Q[Shortcuts::hu] = 0.0;
    Q[Shortcuts::hv] = 0.0;
    Q[Shortcuts::z]  = -999;
  } else {
    Q[Shortcuts::h]  = -std::min(bathymetry, 0.0);
    Q[Shortcuts::hu] = 0.0;
    Q[Shortcuts::hv] = 0.0;
    Q[Shortcuts::z]  = bathymetry;
  }
"""

boundary_conditions = """
  Qoutside[0] = Qinside[0];
  Qoutside[1] = 0.0;
  Qoutside[2] = 0.0;
  Qoutside[3] = Qinside[3];
"""

is_physically_admissible = """
  // Low values of h are resolved on FV layer
  if (tarch::la::smallerEquals(Q[0], 10.0 /*hThreshold*/)) {
    return false;
  }

  // Limit close to boundaries
  if (std::abs(x[0] - DomainOffset[0]) < h[0] or std::abs(x[0] - DomainOffset[0] - DomainSize[0]) < h[0]) {
    return false;
  }
  if (std::abs(x[1] - DomainOffset[1]) < h[1] or std::abs(x[1] - DomainOffset[1] - DomainSize[1]) < h[1]) {
    return false;
  }

  // Right
  if (x(0) - PhysicalDomainOffsetX > 0.6 * PhysicalDomainSizeX) {
    return false;
  }

  // Bottom
  if (x(1) - PhysicalDomainOffsetY < 0.4 * PhysicalDomainSizeY) {
    return false;
  }

  // Left
  if (x(0) - PhysicalDomainOffsetX < 0.3 * PhysicalDomainSizeX) {
    return false;
  }

  // Top
  if (x(1) - PhysicalDomainOffsetY > 0.85 * PhysicalDomainSizeY) {
    return false;
  }

  return true;
"""

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=3,
    end_time=100.0,  # 50000.0,
    degrees_of_freedom=7,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
    "PhysicalDomainOffsetX": [physical_offset[0], "double"],
    "PhysicalDomainOffsetY": [physical_offset[1], "double"],
    "PhysicalDomainSizeX": [physical_size[0], "double"],
    "PhysicalDomainSizeY": [physical_size[1], "double"],
    "aeaEllipsoidA": [
        6378137.0,
        "double",
    ],  # Semi-major axis of the WGS84 ellipsoid (in meters)
    "aeaEllipsoidE": [
        0.08181919084262157,
        "double",
    ],  # First numerical eccentricity of the WGS84 ellipsoid
    "aeaStdParallel1": [18.3333333, "double"],  # Standard Parallel 1 (degrees)
    "aeaStdParallel2": [31.6666667, "double"],  # Standard Parallel 2 (degrees)
    "aeaLon0": [-84.0, "double"],  # Central Meridian (degrees)
    "aeaLat0": [25.0, "double"],  # Latitude of Origin (degrees)
    "aeaEarthRadius": [6371000.0, "double"],  # Projection Earth Radius
    "pointLonA": [-85.107, "double"],
    "pointLatA": [30.747, "double"],
    "pointLonB": [-81.562, "double"],
    "pointLatB": [23.4480, "double"],
    "water_density": [1000, "double"],
    "coriolis_omega": [7.2921150e-5, "double"],
    "manning_param": [0.025, "double"],
}

args.min_depth = 3
max_h = 1.1 * min(physical_size) / (3.0**args.min_depth)
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
    flux=f"ShallowWater::flux<double, Shortcuts, {aderdg_solver.unknowns}>(Q, x, h, t, dt, normal, F);",
    ncp=f"ShallowWater::nonconservativeProduct<double, Shortcuts, {aderdg_solver.unknowns}>(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);",
    max_eigenvalue="return ShallowWater::maxEigenvalue<double, Shortcuts>(Q,x,h,t,dt,normal);",
    riemann_solver="""
  // Compute max eigenvalue over face
  double smax = 0.0;
  for(int xy=0; xy<Order+1; xy++){
    smax = std::max(smax, maxEigenvalue(&QL[xy*4], x, h, t, dt, direction));
    smax = std::max(smax, maxEigenvalue(&QR[xy*4], x, h, t, dt, direction));
  }

  for(int xy=0; xy<Order+1; xy++){
    if (QL[xy*4+0] <= hThreshold or QR[xy*4+0] <= hThreshold) {
      FR[xy*4+0] = FL[xy*4+0] = 0.0;
      FR[xy*4+1] = FL[xy*4+1] = 0.0;
      FR[xy*4+2] = FL[xy*4+2] = 0.0;
      FR[xy*4+3] = FL[xy*4+3] = 0.0;
      continue;
    }

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
    source_term=f"meteotsunami::sourceTerm<double, Shortcuts, {aderdg_solver.unknowns}, {aderdg_solver.auxiliary_variables}>(Q, x, h, t, dt, S);",
)

aderdg_solver.add_user_solver_includes(
    """
#include "../PDE.h"
#include "SourceTerm.h"
#include "tarch/reader/NetCDFFieldParser.h"
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
    source_term=f"meteotsunami::sourceTerm<double, Shortcuts, {fv_solver.unknowns}, {fv_solver.auxiliary_variables}>(Q, x, h, t, dt, S);",
    riemann_solver="return fWaveRiemannSolver<double, Shortcuts>(QL, QR, x, h, t, dt, normal, FL, FR);",
)

fv_solver.add_user_solver_includes(
    """
#include "../FWaveRiemannSolver.h"
#include "SourceTerm.h"
#include "tarch/reader/NetCDFFieldParser.h"
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
    project_name="Meteotsunami",
    directory=".",
    executable="ExaHyPE",
)

project.add_solver(aderdg_solver)
project.add_solver(fv_solver)
project.add_solver(limiter_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

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

project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)
project.output.makefile.set_target_device(args.target_device)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
