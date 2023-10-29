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
parser.add_argument("-t",   "--type",               dest="type",           choices=["plot-0","plot-1","plot-2","plot-3","reg-0"], required=True, help="Pick scenario" )
parser.add_argument(        "--optimise-stateless", dest="optimise_stateless",            action="store_true", default=False, help="Optimise stateless/side-effect free cells (required for GPU offloading)")
parser.add_argument("-s",   "--solver",             dest="solver",         choices=["dg1-rk1", "dg2-rk2", "dg3-rk3", "dg4-rk4"], required=True, help="Pick solver type" )
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

if args.type=="plot-0":
  baseline_max_h               = 0.05
  baseline_min_h               = 0.05
  end_time                     = 4.0
if args.type=="plot-1":
  baseline_max_h               = 0.005
  baseline_min_h               = 0.005
  end_time                     = 4.0
if args.type=="plot-2":
  baseline_max_h               = 0.5
  baseline_min_h               = 0.1
  end_time                     = 64.0
if args.type=="plot-3":
  baseline_max_h               = 0.5
  baseline_min_h               = 0.008
  end_time                     = 4.0
elif args.type=="reg-0":
  baseline_max_h               = 0.008
  baseline_min_h               = 0.008
  end_time                     = 4.0

#
# For Finite Volumes, we use the same mesh size, as the mesh size
# corresponds to volume sizes, and the patch size is, in principle,
# only a way to organise the volumes. For DG, we really have to make
# the cells coarser with increasing order (to fit into memory and
# to remain reasonable by means of accuracy).
#
if "dg0-" in args.solver:
  max_h = baseline_max_h
  min_h = baseline_min_h
if "dg1-" in args.solver:
  max_h = baseline_max_h
  min_h = baseline_min_h
if "dg2-" in args.solver:
  max_h = baseline_max_h * 3
  min_h = baseline_min_h * 3
elif "dg3-" in args.solver:
  max_h = baseline_max_h * 4
  min_h = baseline_min_h * 4 
elif "dg4-" in args.solver:
  max_h = baseline_max_h * 5
  min_h = baseline_min_h * 5



auxiliary_variables = 0


executable_name = "benchmark-" + args.type
if args.optimise_stateless:
  executable_name = executable_name + "-opt"
else:
  executable_name = executable_name + "-no-opt"


if args.mode!="release":
  executable_name = executable_name + "-" + args.mode



space_order     = int(args.solver[2])
time_order      = int(args.solver[6])
executable_name = executable_name + "-RK" + str(time_order) + "DG" + str(space_order)


class MyDGSolver(exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking):
  def __init__(self):
    super(MyDGSolver,self).__init__(name                 = solver_name + "DG",
                                                                   rk_order             = time_order,
                                                                   polynomials          = exahype2.solvers.GaussLegendreBasis(space_order),
                                                                   face_projections     = exahype2.solvers.rkdg.FaceProjections.Solution,
                                                                   unknowns             = number_of_unknowns,
                                                                   auxiliary_variables  = auxiliary_variables,
                                                                   min_cell_h           = min_h, 
                                                                   max_cell_h           = max_h,
                                                                   time_step_relaxation = 0.1,
                                                                   flux                 = exahype2.solvers.PDETerms.User_Defined_Implementation,
                                                                   eigenvalues          = exahype2.solvers.PDETerms.User_Defined_Implementation,
                                                                   source_term          = exahype2.solvers.PDETerms.User_Defined_Implementation,
                                                                   refinement_criterion = exahype2.solvers.PDETerms.User_Defined_Implementation,
                                                                   pde_terms_without_state = args.optimise_stateless
                                                                   )

    self._current_time_step.additional_load_and_store_arguments = [ 
      ("repositories::DataRepository::_CellDataAlphaPoissonMarkerStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top()","celldata::AlphaPoissonMarker","alpha")
    ]
    
    self._current_time_step.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
# @todo This is to be controlled via alpha marker later on
       self._provide_cell_data_to_compute_kernels_default_guard(),
       """
(""" + self._current_time_step.generator.load_persistent_condition + """
      and
      (alpha.getMarker()==celldata::AlphaPoissonMarker::Marker::AnotB or alpha.getMarker()==celldata::AlphaPoissonMarker::Marker::AdeterminesB)
    )
       """,
       """
(""" + self._current_time_step.generator.store_persistent_condition + """
      and
      (alpha.getMarker()==celldata::AlphaPoissonMarker::Marker::AnotB or alpha.getMarker()==celldata::AlphaPoissonMarker::Marker::AdeterminesB)
    )
""")
    
    
    # Bei compute bin ich mir echt unsicher
    
#    self._rhs_estimates                     = peano4.datamodel.Patch( (self.number_of_Runge_Kutta_steps()*(self._basis.dofs_per_axis),
#    self._linear_combination_of_estimates   = peano4.datamodel.Patch( (self._basis.dofs_per_axis,




dg_solver = MyDGSolver()

patch_size = 2*space_order+1

fv_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(name                 = solver_name + "FV", 
                                                                                 patch_size           = patch_size,
                                                                                 unknowns             = number_of_unknowns,
                                                                                 auxiliary_variables  = auxiliary_variables,
                                                                                 min_volume_h         = min_h, 
                                                                                 max_volume_h         = max_h,
                                                                                 time_step_relaxation = 0.1,
                                                                                 flux                 = exahype2.solvers.PDETerms.User_Defined_Implementation,
                                                                                 eigenvalues          = exahype2.solvers.PDETerms.User_Defined_Implementation,
                                                                                 source_term          = exahype2.solvers.PDETerms.User_Defined_Implementation,
                                                                                 refinement_criterion = exahype2.solvers.PDETerms.User_Defined_Implementation,
                                                                                 pde_terms_without_state = args.optimise_stateless
                                                                                )

#
# Get the global data accumulation in place. We do this only for the FV solver, 
# as we assume that the interesting stuff is all covered by the FV 
# discretisation.
#  
fv_solver._action_set_postprocess_solution = exahype2.solvers.fv.actionsets.VolumeWisePostprocessSolution(fv_solver)
fv_solver._action_set_postprocess_solution.guard = """
repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep() 
and 
not repositories::{{SOLVER_INSTANCE}}.patchCanUseStatelessPDETerms(marker.x(), marker.h(), timeStamp, timeStepSize)
"""
fv_solver._action_set_postprocess_solution.add_postprocessing_kernel( """
    repositories::{{SOLVER_INSTANCE}}.addDensity(volumeX,volumeH,value[0]);  
  """     )

#
# Get the marker in place which switches between the solvers, i.e. lets the 
# FV solver overrule the DG solver.
#
marker_solver = exahype2.solvers.elliptic.ConstrainedPoissonEquationForMarkerOnCells(
  name = "Alpha",
  plot = True)
marker_solver.postprocess_updated_cell = """
if ( not repositories::""" + fv_solver.get_name_of_global_instance() + """.patchCanUseStatelessPDETerms(
  marker.x(), 
  marker.h(), 
  // it would be nicer to use fineGridCellSelfSimilarInfallFVCellLabel here I guess
  repositories::""" + fv_solver.get_name_of_global_instance() + """.getMinTimeStamp(), 
  repositories::""" + fv_solver.get_name_of_global_instance() + """.getMinTimeStepSize()
)) {
  fineGridCell{{CELL_NAME}}.setValue(1.0);
  fineGridCell{{CELL_NAME}}.setInvariant(true);
}
else {
  fineGridCell{{CELL_NAME}}.setInvariant(false);
}
"""



#
# Configure plots and enable or disable dynamic AMR
#
fv_solver.plot_description = "rho,j_x,j_y,j_z,E,aux"
dg_solver.plot_description = "rho,j_x,j_y,j_z,E,aux"
if "dyn" in args.type:
  fv_solver.add_solver_constants("static constexpr bool DynamicAMR = true;" )
  dg_solver.add_solver_constants("static constexpr bool DynamicAMR = true;" )
else:
  fv_solver.add_solver_constants("static constexpr bool DynamicAMR = false;" )
  dg_solver.add_solver_constants("static constexpr bool DynamicAMR = false;" )
  
        
#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["benchmarks", "exahype2", "euler", "sphericalaccretionupscaling"], "benchmark", ".", executable=executable_name )
project.add_solver( marker_solver )
project.add_solver( dg_solver )
project.add_solver( fv_solver )



    
#
# Configure build
#
if args.mode=="release":
  build_mode = peano4.output.CompileMode.Release
if args.mode=="stats":
  build_mode = peano4.output.CompileMode.Stats
if args.mode=="asserts":
  build_mode = peano4.output.CompileMode.Asserts

if "plot" in args.type:
  plot_interval = end_time / 20.0
else:  
  plot_interval = 0.0
  

cube_size = 1.0
project.set_global_simulation_parameters(
  dimensions            = dimensions,
  offset                = [-cube_size/2.0, -cube_size/2.0, -cube_size/2.0],
  size                  = [ cube_size, cube_size, cube_size],
  min_end_time          = end_time,
  max_end_time          = end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = plot_interval,
  periodic_BC           = [False, False, False]
)


project.set_load_balancing( "toolbox::loadbalancing::strategies::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( args.peanodir, build_mode )

#
# Generate code and then copy cpp file over main
#
peano4_project = project.generate_Peano4_project(args.verbose)


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
