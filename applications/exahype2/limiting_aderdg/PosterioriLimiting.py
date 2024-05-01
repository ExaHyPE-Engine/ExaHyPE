import os
import sys

sys.path.insert(0, os.path.abspath("../../../python"))
sys.path.insert(0, os.path.abspath("./limiting"))

import peano4
import exahype2
from limiting import PosterioriLimiting

project = exahype2.Project(["examples", "limiting", "euler2D"], "limitedEuler", executable="SodShock")

dimensions          = 2
min_level           = 3
max_depth           = 0
order               = 5
e_t                 = 1.0 #end_time

offset              = [-1.0, -1.0, -1.0][0:dimensions]
size                = [2.0, 2.0, 2.0][0:dimensions]
max_h               = 1.1 * min(size) / (3.0**min_level)
min_h               = max_h / (3.0**max_depth)

initial_conditions = """
  constexpr double original_shock_position = 0.111;
  constexpr double gamma = 1.4;
  const double p = (x[0]<-original_shock_position && x[1]<-original_shock_position) ? 0.1 : 1.0;

  Q[0] = (x[0]<-original_shock_position && x[1]<-original_shock_position) ? 0.125 : 1.0;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = p/(gamma-1.0);
"""

boundary_conditions = """
  constexpr double original_shock_position = 0.111;
  constexpr double gamma = 1.4;
  const double p = (faceCentre[0]<-original_shock_position && faceCentre[1]<-original_shock_position) ? 0.1 : 1.0;

  Qoutside[0] = (faceCentre[0]<-original_shock_position && faceCentre[1]<-original_shock_position) ? 0.125 : 1.0;
  Qoutside[1] = 0.0;
  Qoutside[2] = 0.0;
  Qoutside[3] = p/(gamma-1.0);
"""

flux = """
  const double irho = 1.0 / Q[0];
  constexpr double gamma = 1.4;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  F[0] = Q[normal+1];
  F[1] = Q[normal+1]*Q[1]*irho;
  F[2] = Q[normal+1]*Q[2]*irho;
  F[3] = Q[normal+1]*irho*(Q[3]+p);

  F[normal+1] += p;
"""


max_eval = """
  const double irho = 1.0/Q[0];

  constexpr double gamma = 1.4;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  const double c   = std::sqrt(gamma * p * irho);
  const double u = Q[normal+1]*irho;

  return std::max(std::abs(u-c), std::abs(u+c));
"""

aderSolver = exahype2.solvers.aderdg.rusanov.GlobalAdaptiveTimeStep(
  name                  = "aderSolver", 
  order                 = order,
  unknowns              = 4,
  auxiliary_variables   = 0,
  min_cell_h            = min_h,
  max_cell_h            = max_h,
  time_step_relaxation  = 0.9,
  flux                  = flux,
  eigenvalues           = max_eval,
  initial_conditions    = initial_conditions,
  boundary_conditions   = boundary_conditions
)
project.add_solver(aderSolver)

fvSolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
  name                  = "fvSolver",
  patch_size            = 2*order+1,
  unknowns              = 4,
  auxiliary_variables   = 0,
  min_volume_h          = min_h,
  max_volume_h          = max_h,
  time_step_relaxation  = 0.9,
  flux                  = flux,
  eigenvalues           = max_eval,
  initial_conditions    = "", #get set by regular solver
  boundary_conditions   = boundary_conditions
)
project.add_solver(fvSolver)

#actual limiting solver
limiter = PosterioriLimiting(
  name            = "limitingSolver",
  regularSolver   = aderSolver,
  limitingSolver  = fvSolver,
  number_of_dmp_observables = 4,
  dmp_relaxation_parameter  = 0.01, #0.02 works for depth 2, 0.01 works for depth 3
  dmp_differences_scaling   = 0.03, #0.04 works for depth 2, 0.02 works for depth 3
  physical_admissibility_criterion = "return (Q[0]>0. && Q[3]>0.); //density and energy must be positive"
)
project.add_solver(limiter)

project.set_output_path("posterioriSolutions")

dimensions = 2
build_mode = peano4.output.CompileMode.Release

project.set_global_simulation_parameters(
    dimensions = dimensions,
    offset = offset[0:dimensions],
    size = size[0:dimensions],
    min_end_time = e_t,
    max_end_time = e_t,
    first_plot_time_stamp = 0.0,
    time_in_between_plots = 0.05,
    periodic_BC=[False, False, False][0:dimensions]
)

#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing(
    "toolbox::loadbalancing::strategies::RecursiveBipartition",
    "new ::exahype2::LoadBalancingConfiguration()",
)
project.set_Peano4_installation("../../../", build_mode)
peano4_project = project.generate_Peano4_project("False")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=16)
