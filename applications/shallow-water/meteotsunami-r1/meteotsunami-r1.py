# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np

import peano4
import exahype2

L = 300e3
physical_offset = [-39870.0, 0.0]
physical_size = [379700.0, L / 3]
square_side = max(physical_size)

offset = [
    physical_offset[0] - 0.5 * (square_side - physical_size[0]),
    physical_offset[1] - 0.5 * (square_side - physical_size[1]),
]
size = [square_side, square_side]


def manning_coefficient(H: float, z0: float = 0.003) -> float:
    """
    Compute the Manning coefficient n based on the Chezy formulation:

        C = 18 * log(0.37 * H / z0)
        n = H^(1/6) / C

    Parameters
    ----------
    H : float
        Water depth in meters.
    z0 : float, optional
        Roughness length in meters (default: 0.003).

    Returns
    -------
    n : float
        Manning coefficient (dimensionless).
    """
    if H <= 0:
        raise ValueError("Water depth H must be positive.")

    C = 18.0 * np.log10(0.37 * H / z0)
    n = H ** (1 / 6) / C
    return n


H = 50
manning_coefficient_value = manning_coefficient(H)
print("Manning coefficient:", manning_coefficient_value)

initial_conditions = f"""
  static tarch::reader::NetCDFFieldSampler fieldSampler(
    {{
      {{"bathymetry", "bathymetry/test_R1_dx100_H{H}_b.grd", "z", "x", "y"}}
    }},
    PhysicalDomainSizeX,
    PhysicalDomainSizeY,
    PhysicalDomainOffsetX,
    PhysicalDomainOffsetY
  );

  // TODO: This sampling is needed to make the square domain works without NaNs. However, it gives unphysical solutions.
  // The setup should be a rectangular domain, but this breaks limiting. The limiter needs to be fixed first.
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
    Q[Shortcuts::z]  = bathymetry;
"""

boundary_conditions = """
  if (normal == 0) {
    // x-normal: left and right boundary
    // transmissive
    Qoutside[0] = Qinside[0];
    Qoutside[1] = Qinside[1];
    Qoutside[2] = Qinside[2];
    Qoutside[3] = Qinside[3];
  } else if (normal == 1) {
    // y-normal: top and bottom boundary
    // wall boundary
    Qoutside[0] = Qinside[0];
    Qoutside[1] = Qinside[1];
    Qoutside[2] = -Qinside[2];
    Qoutside[3] = Qinside[3];
  }
"""

is_physically_admissible = """
  if (x(0) < PhysicalDomainOffsetX or x(0) > PhysicalDomainOffsetX + PhysicalDomainSizeX) {
    return false;
  }
  if (x(1) < PhysicalDomainOffsetY or x(1) > PhysicalDomainOffsetY + PhysicalDomainSizeY) {
    return false;
  }

  if (std::abs(x[0] - DomainOffset[0]) < h[0] or std::abs(x[0] - DomainOffset[0] - DomainSize[0]) < h[0]) {
    return false;
  }
  if (std::abs(x[1] - DomainOffset[1]) < h[1] or std::abs(x[1] - DomainOffset[1] - DomainSize[1]) < h[1]) {
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
    end_time=100.0,  # 13200.0,
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
    "water_density": [1000, "double"],
    "manning_param": [manning_coefficient_value, "double"],
    "corriolis_param": [9.35e-5, "double"],
    "pressure": [100, "double"],
    "speed_pressure": [22.15, "double"],
    "period_pressure": [1800, "double"],
    "background_pressure": [101300, "double"],
}

args.min_depth = 2
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
    source_term=f"meteotsunami::sourceTerm<double, Shortcuts, {fv_solver.unknowns}, {fv_solver.auxiliary_variables}>(Q, x, h, t, dt, S);",
    riemann_solver="return fWaveRiemannSolver<double, Shortcuts>(QL, QR, x, h, t, dt, normal, FL, FR);",
)

fv_solver.add_user_solver_includes(
    """
#include "../FWaveRiemannSolver.h"
#include "SourceTerm.h"
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
