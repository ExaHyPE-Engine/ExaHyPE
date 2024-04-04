import os
import sys

sys.path.insert(0, os.path.abspath("../../../python"))
sys.path.insert(0, os.path.abspath("./limiting"))

import peano4
import exahype2
from limiting import StaticLimiting
from exahype2.solvers.aderdg.ADERDG import Polynomials

project = exahype2.Project(["examples", "limiting", "SWE"], "limitedSWE", executable="AbsorbingBoundary")

dimensions = 2
min_level  = 3
max_depth  = 0
order      = 5
e_t        = 1. #end_time

offset     = [-1.0, -1.0, -1.0][0:dimensions]
size       = [ 2.0,  2.0,  2.0][0:dimensions]
max_h      = 1.1 * min(size) / (3.0**min_level)
min_h      = max_h / (3.0**max_depth)

def initial_conditions(x):
  return """
  Q[0] = 4.0;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 2.0 - tarch::la::norm2(""" + x + """);
"""

boundary_conditions = """
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
"""

flux = """
  double ih = 1.0 / Q[0];

  F[0] = Q[1 + normal];
  F[1] = Q[1 + normal] * Q[1] * ih;
  F[2] = Q[1 + normal] * Q[2] * ih;
  F[3] = 0.0;
"""

ncp = """
  constexpr double grav = 9.81;

  BTimesDeltaQ[0] = 0.0;
  switch (normal) {
  case 0:
    BTimesDeltaQ[1] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
    BTimesDeltaQ[2] = 0.0;
    break;
  case 1:
    BTimesDeltaQ[1] = 0.0;
    BTimesDeltaQ[2] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
    break;
  }
  BTimesDeltaQ[3] = 0.0;
"""

max_eval = """
  constexpr double grav = 9.81;

  double u = Q[1 + normal] / Q[0];
  double c = std::sqrt(grav * Q[0]);

  return std::max(std::abs(u + c), std::abs(u - c));
"""

limiting_criterion = """
bool isBoundaryCell = (
  x[0] + 1.00 <  0.5*h[0] or
  x[1] + 1.00 <  0.5*h[1] or
  x[0] - 1.00 > -0.5*h[0] or
  x[1] - 1.00 > -0.5*h[1]
);
return !isBoundaryCell;
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
  ncp                   = ncp,
  eigenvalues           = max_eval,
  initial_conditions    = initial_conditions("x"),
  boundary_conditions   = boundary_conditions
)
aderSolver.add_kernel_optimizations(polynomials=Polynomials.Gauss_Lobatto)
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
  ncp                   = ncp,
  eigenvalues           = max_eval,
  initial_conditions    = initial_conditions("volumeCentre"),
  boundary_conditions   = boundary_conditions
)
project.add_solver(fvSolver)

#actual limiting solver
limiter = StaticLimiting(
  name            = "limitingSolver",
  regularSolver   = aderSolver,
  limitingSolver  = fvSolver,
  limiting_criterion_implementation = limiting_criterion
)
project.add_solver(limiter)

project.set_output_path("staticSolutions")

dimensions = 2
build_mode = peano4.output.CompileMode.Release

project.set_global_simulation_parameters(
    dimensions = dimensions,
    offset = offset[0:dimensions],
    size = size[0:dimensions],
    min_end_time = e_t,
    max_end_time = e_t,
    first_plot_time_stamp = 0.0,
    time_in_between_plots = 0.1,
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
