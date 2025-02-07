import os, sys
import peano4, exahype2

from exahype2.solvers.PDETerms import PDETerms

project = exahype2.Project(["exahype", "limiting", "swe"], ".", executable="SWE")

min_level           = 3
max_depth           = 0
order               = 5

# scenario = "simple_beach"
scenario = "oscillating_lake"

if scenario=="simple_beach":
  e_t                 = 100.0 #end_time
  offset              = [-10.0,  0.0]
  size                = [ 70.0, 70.0]
  init = """
  constexpr double Hd = 0.0185;
  constexpr double d = 0.3;

  constexpr double H = Hd * d;
  constexpr double beta = std::atan(1/19.85);

  constexpr double gamma = std::sqrt((3*H)/(4*d));
  constexpr double x0 = d * cos(beta)/sin(beta);
  constexpr double L = d * std::log(std::sqrt(20) + std::sqrt(20 - 1)) / gamma;
  const     double eta = H * (1/(std::cosh(gamma*(x[0]-(x0 + L))/d))) * (1/(std::cosh(gamma*(x[0]-(x0 + L))/d)));

  if (x[0] < 0){
    Q[0] = 0;
    Q[3] = -x[0] * std::sin(beta)/std::cos(beta) + d;
  }
  else if (x[0] <= x0){
    Q[0] = x[0] * std::sin(beta)/std::cos(beta);
    Q[3] = d - Q[0];
  }
  else{
    Q[0] =  H * (1/(std::cosh(gamma*(x[0]-(x0 + L))/d))) * (1/(std::cosh(gamma*(x[0]-(x0 + L))/d))) + d;
    Q[3] = 0;
  }
  Q[1] = -eta * std::sqrt(9.81/d) * Q[0];
  Q[2] = 0.0;
"""

elif scenario=="oscillating_lake":
  e_t                 = 3.0 #end_time
  offset              = [-2.0, -2.0]
  size                = [ 4.0,  4.0]
  init = """

  /*
  Q[3] = 0.1 * (x[0]*x[0] + x[1]*x[1]);
  Q[0] = std::max(0.0, 0.1 * (x[0] * cos(omega * 0) + x[1] * sin(omega * 0)) + 0.075 - Q[3]);
  Q[1] = 0.5 * omega * sin(omega * 0) * Q[0];
  Q[2] = 0.5 * omega * cos(omega * 0) * Q[0];
  */

  //Original source:
  // doi: 10.1017/S0022112081001882

  //More legible, translated into the form we are using:
  // doi: 10.4208/cicp.070114.271114a

  constexpr double a      = 1.0;
  constexpr double h0     = 0.1;
  constexpr double sigma  = 0.5;
  constexpr double omega  = sqrt(2*9.81*h0)/a;

  Q[3] = (h0 / (a*a)) * (x[0]*x[0] + x[1]*x[1]) ;
  Q[0] = std::max(0.0, (sigma*h0/(a*a)) * (2 * x[0] * cos(omega * 0) + 2 * x[1] * sin(omega * 0) - sigma) + h0 - Q[3]);
  Q[1] = -sigma * omega * sin(omega * 0) * Q[0];
  Q[2] =  sigma * omega * cos(omega * 0) * Q[0];
"""


max_h = 1.1 * min(size) / (3.0**min_level)
min_h = max_h / (3.0**max_depth)


boundary_conditions = """
  std::copy_n(Qinside, 4, Qoutside);
  Qoutside[normal+1] = -Qinside[normal+1];
"""

aderSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name  = "aderSWE", order = 3,
  min_cell_h = min_h, max_cell_h = max_h,
  time_step_relaxation  = 0.9,
  unknowns              = 4,
  auxiliary_variables   = 0,
  initial_conditions    = init,
  boundary_conditions   = boundary_conditions,
  eigenvalues = PDETerms.User_Defined_Implementation,
  flux        = PDETerms.User_Defined_Implementation,
  ncp         = PDETerms.User_Defined_Implementation,
#  refinement_criterion = PDETerms.User_Defined_Implementation
)

aderSolver.add_kernel_optimisations(
  polynomials=exahype2.solvers.aderdg.ADERDG.Polynomials.Gauss_Lobatto,
  riemann_solver_implementation=exahype2.solvers.PDETerms.User_Defined_Implementation
)

project.add_solver(aderSolver)

fvSolver = exahype2.solvers.fv.riemann.GlobalAdaptiveTimeStep(
  name = "fvSWE",
  patch_size = 2*order+1,
  min_volume_h          = min_h,
  max_volume_h          = max_h,
  time_step_relaxation  = 0.9,
  unknowns              = 3,
  auxiliary_variables   = 1,
  initial_conditions    = "", #get set by ader solver
  boundary_conditions   = boundary_conditions,
  eigenvalues           = PDETerms.User_Defined_Implementation,
  flux                  = PDETerms.User_Defined_Implementation,
  riemann_solver        = PDETerms.User_Defined_Implementation
)
project.add_solver(fvSolver)

#actual limiting solver
limiter = exahype2.solvers.limiting.PosterioriLimiting(
  name             = "limitingSolver",
  regular_solver   = aderSolver,
  limiting_solver  = fvSolver,
  number_of_dmp_observables = 0,
  dmp_relaxation_parameter  = 0.001,
  dmp_differences_scaling   = 0.02,
  physical_admissibility_criterion = """
  if(Q[0] == 0.0)
      return true;
  if(Q[0] <= 20 * 1e-3)
      return false;
  return true;"""
)
project.add_solver(limiter)

project.set_output_path(
  "posterioriSolutions_o_" + str(order) 
    + "_l_" + str(min_level)
    + "_d_" + str(max_depth)
)

# project._project.output.makefile.set_Fortran_compiler("gfortran")
# project._project.output.makefile.add_Fortran_file("rpn2_testing.f90")

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

project._project.output.makefile.set_Fortran_compiler("gfortran")
project._project.output.makefile.add_Fortran_file("rpn2_testing.f90")

project.set_load_balancing(
    "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates",
    "new ::exahype2::LoadBalancingConfiguration()",
)
project.set_Peano4_installation("../../../", build_mode)
peano4_project = project.generate_Peano4_project("False")

peano4_project.build(make_clean_first=True)
