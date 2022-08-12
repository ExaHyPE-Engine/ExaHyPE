"""

 Simple, hard-coded version of the self-similarity test of the MNRAS
 paper. A more elegant implementation of the solver via a proper subclass
 can be found in the actual application folder.

"""


import os, sys
import argparse
import peano4
import exahype2


print( """

ExaHyPE 2 Finite Volume version of the self-similarity tests of the MNRAS paper

@author 2022 Han Zhang, Baojiu Li, Tobias Weinzierl

""")

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("-j",   "--parallel-builds",   dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",         dest="peanodir",       default="../../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",           dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument(        "--debug",             dest="debug"  ,        action="store_true", default=False, help="Verbose")
parser.add_argument("-ps",  "--patch-size",        dest="patch_size",     type=int, default=6, help="Dimensions" )
parser.add_argument("-f",   "--file",              dest="file",           default="", help="Specify to convert an output file into a bar plot. If you omit this flag, the script will actually build the benchmark" )
args = parser.parse_args()



    
    
#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["examples", "exahype2", "euler", "sphericalaccretionupscaling"], "fv", ".", executable="fv" )

    
#
# Add the Finite Volumes solver
#
solver_name = "SelfSimilarInfall"
dimensions  = 3
unknowns = {
  "rho": 1,
  "j":   dimensions,
  "E":   1,
}
number_of_unknowns = sum(unknowns.values())
if args.debug:
  max_h          = 0.5/args.patch_size
  min_h          = 0.5/args.patch_size
else:
  max_h          = 0.1/args.patch_size
  min_h          = 0.1/args.patch_size
auxiliary_variables = 0
    
thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
      solver_name,
      args.patch_size,
      number_of_unknowns, auxiliary_variables,
      min_volume_h = 0.01, 
      max_volume_h = 0.01,
      time_step_relaxation = 0.01,
      flux        = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
      eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
      source_term = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
      pde_terms_without_state = False
    )

thesolver._action_set_postprocess_solution = exahype2.solvers.fv.actionsets.VolumeWisePostprocessSolution(thesolver)
thesolver._action_set_postprocess_solution.guard = "repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()"
thesolver._action_set_postprocess_solution.add_postprocessing_kernel( """
  repositories::{{SOLVER_INSTANCE}}.addDensity(volumeX,volumeH,value[0]);  
"""     )
        
project.add_solver( thesolver )
    
    
#
# Configure build
#
if args.debug:
  build_mode = peano4.output.CompileMode.Debug
else:
  build_mode = peano4.output.CompileMode.Release
  
cube_size = 1
project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [-cube_size/2.0, -cube_size/2.0, -cube_size/2.0],
  size   = [ cube_size, cube_size, cube_size],
  min_end_time = 4.0,
  max_end_time = 4.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.04,
  periodic_BC           = [False, False, False]
)

project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( args.peanodir, build_mode )
    
    
#
# Generate code and then copy cpp file over main
#
peano4_project = project.generate_Peano4_project(args.verbose)


peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/euler/EulerKernels.cpp" )
peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/euler/spherical-accretion/MassAccumulator.cpp" )
peano4_project.output.makefile.add_header_search_path( "../../../../applications/exahype2/euler" )
peano4_project.generate()

os.system( "make clean" )
if args.j>0:
  os.system( "make -j{}".format(args.j) )
else:
  os.system( "make -j" )
  
print("Done")
