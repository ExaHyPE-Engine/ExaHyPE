# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4, exahype2

from FWave import fWave, rusanov

project = exahype2.Project(
    ["tests", "exahype2", "aderdg"], ".", executable="ADERDG-Limiting"
)

dimensions = 2
min_level = 4
max_depth = 0
order = 5
end_time = 7000.0
plot_interval = 500.0

offset = [0.0, 1.2e6]
size = [2.0e6, 2.0e6]

max_h = 1.1 * min(size) / (3.0**min_level)
min_h = max_h / (3.0**max_depth)

initial_conditions = """
  static tarch::reader::TopologyParser topologyParser(
    \"tohoku_gebco_ucsb3_2000m_hawaii_bath.nc\",
    \"tohoku_gebco_ucsb3_2000m_hawaii_displ.nc\",
    7000000,//DomainSize(0),
    4000000,//DomainSize(1),
    0.,//DomainOffset(0),
    0.//DomainOffset(1)
  );

  const double bathymetryBeforeEarthquake = topologyParser.sampleBathymetry(x(0), x(1));
  const double displacement               = topologyParser.sampleDisplacement(x(0), x(1));
  const double bathymetryAfterEarthquake  = bathymetryBeforeEarthquake + displacement;

  Q[0] = -1*std::min(bathymetryBeforeEarthquake, 0.0);
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = bathymetryAfterEarthquake;
"""

boundary_conditions = """
  initialCondition(
    Qoutside, x, h, false
  );
"""

flux = """
  double ih = 1.0 / Q[0];

  F[0] = Q[1 + normal];
  F[1] = Q[1 + normal] * Q[1] * ih;
  F[2] = Q[1 + normal] * Q[2] * ih;
  F[3] = 0.0;
"""

ncp = """
  std::fill_n(BTimesDeltaQ, NumberOfUnknowns, 0.0);

  constexpr double grav = 9.81;
  BTimesDeltaQ[normal+1] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
"""

max_eval = """
  constexpr double grav = 9.81;

  const double u = Q[1 + normal] / Q[0];
  const double c = std::sqrt(grav * Q[0]);

  return std::max(std::abs(u + c), std::abs(u - c));
"""

limiting_criterion = """
if(x[0]<=0.5*DomainSize[0]){
  return Q[0] > 100.0;
}
else{
  return true;
}
"""

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=order,
    unknowns=4,
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.9,
    flux=flux, ncp=ncp,
    eigenvalues=max_eval,
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
)
aderdg_solver.add_kernel_optimisations(
  polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Lobatto,
  riemann_solver_implementation = rusanov,
#    precision = args.precision
)
aderdg_solver.add_user_solver_includes("""
#include "tarch/reader/TopologyParser.h"
""")
project.add_solver(aderdg_solver)

##########################################

# aderdg_solver2 = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
#     name="ADERDGSolver2",
#     order=order,
#     unknowns=4,
#     auxiliary_variables=0,
#     min_cell_h=min_h,
#     max_cell_h=max_h,
#     time_step_relaxation=0.9,
#     flux=flux, ncp=ncp,
#     eigenvalues=max_eval,
#     initial_conditions=initial_conditions,
#     boundary_conditions=boundary_conditions,
# )
# aderdg_solver2.add_kernel_optimisations(
#   polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Lobatto,
#   riemann_solver_implementation = rusanov,
#   precision = "fp64"
# )
# aderdg_solver2.add_user_solver_includes("""
# #include "tarch/reader/TopologyParser.h"
# """)
# project.add_solver(aderdg_solver2)

# ##########################################

# from fuseADERSolvers import fuseADERSolvers

# fuseADERSolvers(
#   aderdg_solver, aderdg_solver2, 
#   "\nmarker.x()[0]<=(DomainOffset[0]+0.5*DomainSize[0]+0.51*marker.h()[0])\n", 
#   "\nmarker.x()[0]>=(DomainOffset[0]+0.5*DomainSize[0]+0.49*marker.h()[0])\n"
# )

##########################################

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=2 * order + 1,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"b": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.9,

)
fv_solver.set_implementation(
    flux=flux,
    ncp=ncp,
    max_eigenvalue=max_eval,
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    riemann_solver=fWave
)
fv_solver.add_user_solver_includes("""
#include "tarch/reader/TopologyParser.h"
""")
project.add_solver(fv_solver)

##########################################

limiter = exahype2.solvers.limiting.StaticLimiting(
    name="LimitingSolver",
    regular_solver=aderdg_solver,
    limiting_solver=fv_solver,
    limiting_criterion_implementation=limiting_criterion,
)
project.add_solver(limiter)

##########################################
# limiter._start_time_step_implementation += """
#   if(_solverState==SolverState::LimiterStatusSpreadingToNeighbours){
#     repositories::instanceOfADERDGSolver2.suspendSolversForOneGridSweep();
#   }
# """

# compute_time_step_size = """
#     double timeStepSize = std::min( repositories::instanceOfADERDGSolver.getAdmissibleTimeStepSize(),
#                           std::min( repositories::instanceOfADERDGSolver2.getAdmissibleTimeStepSize(),
#                                     repositories::instanceOfFVSolver.getAdmissibleTimeStepSize()
# ));
# """

# aderdg_solver._compute_time_step_size = compute_time_step_size
# aderdg_solver2._compute_time_step_size = compute_time_step_size
# fv_solver._compute_time_step_size = compute_time_step_size
##########################################

project.set_output_path("solutions")

build_mode = peano4.output.CompileMode.Release

project.set_global_simulation_parameters(
    dimensions=dimensions,
    offset=offset[0:dimensions],
    size=size[0:dimensions],
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=plot_interval,
    periodic_BC=[False, False, False][0:dimensions],
)

project.set_load_balancing(
    "toolbox::loadbalancing::strategies::SpreadOut",
    "new ::exahype2::LoadBalancingConfiguration()",
)
project.set_Peano4_installation("../../../", build_mode)
project = project.generate_Peano4_project(verbose=False)
# project.set_fenv_handler(True)  # TODO: Static limiter crashes with this
project.build(make_clean_first=True)
