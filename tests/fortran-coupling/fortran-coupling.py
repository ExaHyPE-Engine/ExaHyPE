import argparse

import exahype2

order               = 5

# scenario = "simple_beach"
scenario = "oscillating_lake"


def parse_arguments():
  parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
  parser.add_argument(
    "-h",
    "--help",
    action="store_true",
    help="show this help message and continue with project help",
  )
  parser.add_argument("-et", "--end-time", type=float, default=None)
  parser.add_argument("-l", "--grid-levels", type=int, default=3)
  args, _ = parser.parse_known_args()
  if args.help:
    parser.print_help()
    print()
  return args


args = parse_arguments()

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
  const double omega  = std::sqrt(2*9.81*h0)/a;

  Q[3] = (h0 / (a*a)) * (x[0]*x[0] + x[1]*x[1]) ;
  Q[0] = std::max(0.0, (sigma*h0/(a*a)) * (2 * x[0] * cos(omega * 0) + 2 * x[1] * sin(omega * 0) - sigma) + h0 - Q[3]);
  Q[1] = -sigma * omega * sin(omega * 0) * Q[0];
  Q[2] =  sigma * omega * cos(omega * 0) * Q[0];
"""

if args.end_time is not None:
  e_t = args.end_time

dimensions = 2

project = exahype2.Project(
  ["tests", "exahype2", "fortran"],
  "FortranCoupling",
  dimensions=dimensions,
  offset=offset + [0.0],
  size=size + [size[-1]],
  periodic_BC=[False, False, False],
  executable="ExaHyPE",
  min_end_time=e_t,
  max_end_time=e_t,
  min_grid_levels=args.grid_levels,
  max_grid_levels=args.grid_levels,
)

if project.dimensions != dimensions:
  raise ValueError(
    f"Fortran coupling is a {dimensions}D testcase, but the project was "
    f"configured for {project.dimensions}D."
  )


boundary_conditions = """
  std::copy_n(Qinside, 4, Qoutside);
  Qoutside[normal+1] = -Qinside[normal+1];
"""

aderSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name  = "aderSWE", order = 3,
  time_step_relaxation  = 0.9,
  unknowns              = 4,
  auxiliary_variables   = 0,
)

aderSolver.set_implementation(
  initial_conditions    = init,
  boundary_conditions   = boundary_conditions,
  max_eigenvalue = exahype2.solvers.PDETerms.User_Defined_Implementation,
  flux        = exahype2.solvers.PDETerms.User_Defined_Implementation,
  ncp         = exahype2.solvers.PDETerms.User_Defined_Implementation,
#  refinement_criterion = exahype2.solvers.PDETerms.User_Defined_Implementation,
  riemann_solver=exahype2.solvers.PDETerms.User_Defined_Implementation,
)

aderSolver.add_kernel_optimisations(
  polynomials=exahype2.solvers.aderdg.ADERDG.Polynomials.Gauss_Lobatto,
)

project.add_solver(aderSolver)

fvSolver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
  name = "fvSWE",
  patch_size = 2*order+1,
  time_step_relaxation  = 0.9,
  unknowns              = 3,
  auxiliary_variables   = 1,
)
fvSolver.set_implementation(
  initial_conditions    = "", #get set by ader solver
  boundary_conditions   = boundary_conditions,
  max_eigenvalue        = exahype2.solvers.PDETerms.User_Defined_Implementation,
  flux                  = exahype2.solvers.PDETerms.User_Defined_Implementation,
  riemann_solver        = exahype2.solvers.PDETerms.User_Defined_Implementation
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

project._project.output.makefile.set_Fortran_compiler("gfortran")
# project._project.output.makefile.add_Fortran_file("rpn2_testing.f90")
project._project.output.makefile.add_cpp_file("fvSWE.cpp")
project._project.output.makefile.add_cpp_file("aderSWE.cpp")

peano4_project = project.generate_Peano4_project(verbose=False)
peano4_project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
