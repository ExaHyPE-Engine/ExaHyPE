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
parser.add_argument("-j",   "--parallel-builds",    dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",          dest="peanodir",       default="../../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",            dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-m",   "--mode",               dest="mode",           choices=["release","stats","asserts"], required=True, help="Pick build type" )
parser.add_argument("-t",   "--type",               dest="type",           choices=["plot","reg-0"], required=True, help="Pick scenario" )
parser.add_argument(        "--optimise-stateless", dest="optimise_stateless",            action="store_true", default=False, help="Optimise stateless/side-effect free cells (required for GPU offloading)")
parser.add_argument("-s",   "--solver",             dest="solver",         choices=["fv-4", "fv-6", "fv-8", "dg1-rk1", "dg2-rk2", "dg3-rk3", "dg4-rk4"], required=True, help="Pick solver type. For the Finite Volume (fv) solvers, the second index is the patch/block size. For DG, you can pick different Runge-Kutta time integration orders" )
args = parser.parse_args()



    

#
# Add the Finite Volumes solver
#
solver_name = "SelfSimilarInfall"
dimensions  = 3
unknowns    = {
  "rho": 1,
  "j":   dimensions,
  "E":   1,
}
number_of_unknowns  = sum(unknowns.values())

if args.type=="plot" in args.solver:
  baseline_max_h               = 0.2
  baseline_min_h               = 0.2
elif args.type=="reg-0":
  baseline_max_h               = 0.008
  baseline_min_h               = 0.008

#
# For Finite Volumes, we use the same mesh size, as the mesh size
# corresponds to volume sizes, and the patch size is, in principle,
# only a way to organise the volumes. For DG, we really have to make
# the cells coarser with increasing order (to fit into memory and
# to remain reasonable by means of accuracy).
#
if "dg2-" in args.solver:
  max_h = baseline_max_h * 3
  min_h = baseline_max_h * 3
elif "dg3-" in args.solver:
  max_h = baseline_max_h * 4
  min_h = baseline_max_h * 4 
elif "dg4-" in args.solver:
  max_h = baseline_max_h * 5
  min_h = baseline_max_h * 5
elif "fv-" in args.solver:
  max_h = baseline_max_h 
  min_h = baseline_max_h 



auxiliary_variables = 0


executable_name = "benchmark-" + args.type
if args.optimise_stateless:
  executable_name = executable_name + "-opt"
else:
  executable_name = executable_name + "-no-opt"

    
if "fv" in args.solver:
  patch_size = int(args.solver[-1])
  thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(name                 = solver_name + "FV", 
                                                                                   patch_size           = patch_size,
                                                                                   unknowns             = number_of_unknowns,
                                                                                   auxiliary_variables  = auxiliary_variables,
                                                                                   min_volume_h         = max_h, 
                                                                                   max_volume_h         = max_h,
                                                                                   time_step_relaxation = 0.1,
                                                                                   flux                 = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                                                                   eigenvalues          = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                                                                   source_term          = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                                                                   pde_terms_without_state = args.optimise_stateless
                                                                                  )
  executable_name = executable_name + "-FV-" + str(patch_size)
  
  thesolver._action_set_postprocess_solution = exahype2.solvers.fv.actionsets.VolumeWisePostprocessSolution(thesolver)
  thesolver._action_set_postprocess_solution.guard = """
repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep() 
and 
not repositories::{{SOLVER_INSTANCE}}.patchCanUseStatelessPDETerms(marker.x(), marker.h(), timeStamp, timeStepSize)
"""
  thesolver._action_set_postprocess_solution.add_postprocessing_kernel( """
    repositories::{{SOLVER_INSTANCE}}.addDensity(volumeX,volumeH,value[0]);  
  """     )
if "rk" in args.solver and "dg" in args.solver:
  space_order = int(args.solver[2])
  time_order  = int(args.solver[6])
  thesolver = exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(name                 = solver_name + "DG",
                                                                   rk_order             = time_order,
                                                                   polynomials          = exahype2.solvers.GaussLegendreBasis(space_order),
                                                                   unknowns             = number_of_unknowns,
                                                                   auxiliary_variables  = auxiliary_variables,
                                                                   min_cell_h           = max_h, 
                                                                   max_cell_h           = min_h,
                                                                   time_step_relaxation = 0.1,
                                                                   flux                 = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                                                   eigenvalues          = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                                                   source_term          = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                                                   pde_terms_without_state = args.optimise_stateless
                                                                   )
  executable_name = executable_name + "-RK" + str(time_order) + "DG" + str(space_order)

  thesolver._action_set_postprocess_solution = exahype2.solvers.rkdg.actionsets.DoFWisePostprocessSolution(thesolver)
  thesolver._action_set_postprocess_solution.guard = """
repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep() 
and 
not repositories::{{SOLVER_INSTANCE}}.cellCanUseStatelessPDETerms(marker.x(), marker.h(), timeStamp, timeStepSize)
"""
  thesolver._action_set_postprocess_solution.add_postprocessing_kernel( """
    repositories::{{SOLVER_INSTANCE}}.addDensity(x, marker.h(), index, values[0]);  
  """     )


#thesolver.auxiliary_variables=1
#thesolver.add_user_action_set_includes( """
##include "exahype2/fv/PostprocessingKernels.h"
#""")
#thesolver.postprocess_updated_patch = """
#::exahype2::fv::mapInnerNeighbourVoxelAlongBoundayOntoAuxiliaryVariable(
# targetPatch,
# {{NUMBER_OF_VOLUMES_PER_AXIS}},
# {{NUMBER_OF_UNKNOWNS}},
# {{NUMBER_OF_AUXILIARY_VARIABLES}},
# 0,
# {{NUMBER_OF_UNKNOWNS}}
#);
#"""

thesolver.plot_description = "rho,j_x,j_y,j_z,E,aux"
        
#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["benchmarks", "exahype2", "euler", "sphericalaccretionupscaling"], "benchmark", ".", executable=executable_name )
project.add_solver( thesolver )
    
    
#
# Configure build
#
if args.mode=="release":
  build_mode = peano4.output.CompileMode.Release
if args.mode=="stats":
  build_mode = peano4.output.CompileMode.Stats
if args.mode=="asserts":
  build_mode = peano4.output.CompileMode.Asserts

if args.type=="plot":
  plot_interval = 0.1
else:  
  plot_interval = 0.0
  
cube_size = 1
project.set_global_simulation_parameters(
  dimensions            = dimensions,
  offset                = [-cube_size/2.0, -cube_size/2.0, -cube_size/2.0],
  size                  = [ cube_size, cube_size, cube_size],
  min_end_time          = 4.0,
  max_end_time          = 4.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = plot_interval,
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
peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/euler/spherical-accretion/GravityModel.cpp" )
peano4_project.output.makefile.add_header_search_path( "../../../../applications/exahype2/euler" )
peano4_project.generate()

os.system( "make clean" )
if args.j>0:
  os.system( "make -j{}".format(args.j) )
else:
  os.system( "make -j" )
  
print("Done")
