# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

initial_conditions = """
  static tarch::reader::NetCDFFieldParser fieldParser(
    \"chile_gebco_usgs_2000m_bath.nc\",
    \"chile_gebco_usgs_2000m_displ.nc\",
    7000000.0,
    4000000.0,
    0.0,
    0.0
  );

  const double bathymetryBeforeEarthquake = fieldParser.sampleTopology(x(0), x(1));
  const double displacement               = fieldParser.sampleDisplacement(x(0), x(1));
  const double bathymetryAfterEarthquake  = bathymetryBeforeEarthquake + displacement;

  Q[Shortcuts::h]  = -std::min(bathymetryBeforeEarthquake, 0.0);
  Q[Shortcuts::hu] = 0.0;
  Q[Shortcuts::hv] = 0.0;
  Q[Shortcuts::z]  = bathymetryAfterEarthquake;
"""

boundary_conditions = """
  Qoutside[0] = Qinside[0];
  Qoutside[1] = 0.0;
  Qoutside[2] = 0.0;
  Qoutside[3] = Qinside[3];
"""

is_physically_admissible = """
  bool isAdmissible = Q[0] > 100.0;

  tarch::la::Vector<DIMENSIONS, double> bblPos = {3050000., 400000.};
  isAdmissible &= tarch::la::norm2(x-bblPos) > 5000.;

  tarch::la::Vector<DIMENSIONS, double> blPos = {3050000., 940000.};
  isAdmissible &= tarch::la::norm2(x-blPos) > 5000.;

  tarch::la::Vector<DIMENSIONS, double> mPos = {5350000., 2320000.};
  isAdmissible &= tarch::la::norm2(x-mPos) > 25000.;

  tarch::la::Vector<DIMENSIONS, double> tlPos = {3790000., 3360000.};
  isAdmissible &= tarch::la::norm2(x-tlPos) > 5000.;

  tarch::la::Vector<DIMENSIONS, double> ttlPos = {3665000., 3410000.};
  isAdmissible &= tarch::la::norm2(x-ttlPos) > 50000.;

  // Juan Fernández Islands
  tarch::la::Vector<DIMENSIONS, double> jfiPos = {5960000., 1040000.};
  isAdmissible &= tarch::la::norm2(x-jfiPos) > 10000.;

  return isAdmissible;
"""

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=4,
    end_time=5000.0,
    degrees_of_freedom=7,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
}

size = [4e6, 4e6]
offset = [3e6, 0.0]
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

aderdg_solver.set_plotter(args.plotter)
aderdg_solver.add_user_solver_includes(
    """
#include "../PDE.h"
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
    riemann_solver="return fWaveRiemannSolver<double, Shortcuts>(QL, QR, x, h, t, dt, normal, FL, FR);",
)

fv_solver.set_plotter(args.plotter)
fv_solver.add_user_solver_includes(
    """
#include "../FWaveRiemannSolver.h"
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
    project_name="ChileTsunami",
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

project.set_load_balancer(
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees}, {args.trees})"
)
project.set_Peano4_installation(
    "../../../../", mode=peano4.output.string_to_mode(args.build_mode)
)
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
