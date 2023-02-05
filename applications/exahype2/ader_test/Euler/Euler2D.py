
import peano4
import exahype2

import argparse

import exahype2.solvers.new_ader
from exahype2.solvers.new_ader.rusanov.GlobalAdaptiveTimeStep import GlobalAdaptiveTimeStep
from exahype2.solvers.new_ader.ADERDG import ADERDG
from exahype2.solvers.new_ader.SingleSweep import SingleSweep
from exahype2.solvers.new_ader.ADERDG import Polynomials

modes = {
  "release": peano4.output.CompileMode.Release,
  "trace":   peano4.output.CompileMode.Trace,
  "assert":  peano4.output.CompileMode.Asserts,
  "stats":   peano4.output.CompileMode.Stats,
  "debug":   peano4.output.CompileMode.Debug,
}

parser = argparse.ArgumentParser(description='ExaHyPE 2 - ADER testing script')
parser.add_argument("-cs", "--cell-size",              dest="h",              type=float, required=True, help="Mesh size" )
parser.add_argument("-o",  "--output",                 dest="out",                      default="EULER2D", help="Executable name" )
parser.add_argument("-amr", "--adaptive-levels",       dest="adaptivity_levels", type=int, default=0, help="Number of AMR grid levels on top of hmax (0 by default)" )
parser.add_argument("-d",  "--dim",                    dest="dim",            type=int, default=2, help="Dimensions" )
parser.add_argument("-m",  "--mode",                   dest="mode",                     default="release", help="|".join(modes.keys()) )
parser.add_argument("-pdt", "--plot-dt",               dest="plot_snapshot_interval", default=0.01, help="Time interval in-between two snapshots (switched off by default")
parser.add_argument("-et", "--end-time",               dest="end_time",                 type=float, default=0.05, help="Number of timesteps" )
parser.add_argument("-order",   "--order",       dest="order",          type=int, default=3, help="DG Order" )
parser.add_argument("-p",   "--p",       dest="polynomials",          type=int, default=0, help="Polynomial type, 0 is gauss-legendre, 1 is gauss-lobatto" )
args = parser.parse_args()


project = exahype2.Project( ["examples", "exahype2", "euler2D"], "euler2D", ".", executable=args.out )

#stability condition: CFL
# dt <= 0.5*dx/lambda_max

unknowns       = 4
auxiliary_variables = 0
max_h          = args.h
min_h          = 0.9 * args.h * 3.0**(-args.adaptivity_levels)
order=args.order

if args.polynomials == 0:
  polynomials = Polynomials.Gauss_Legendre
elif args.polynomials == 1:
  polynomials = Polynomials.Gauss_Lobatto
else:
  raise AttributeError("not acceptable choice of polynomials")  

theSolver=GlobalAdaptiveTimeStep(
  name="euler2D",  order=order, polynomials=polynomials,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_relaxation=0.9)

theSolver.set_implementation( refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation)

project.add_solver(theSolver)

dimensions = args.dim
build_mode = modes[args.mode]



project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [0.0,0.0,0.0],
  size = [1.0,1.0,1.0],
  min_end_time = args.end_time,
  max_end_time = args.end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = args.plot_snapshot_interval,      # snapshots
  periodic_BC = [False,False,False]
)



#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../", build_mode )
peano4_project = project.generate_Peano4_project("False")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)
