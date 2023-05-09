import os
import argparse

import peano4
import exahype2
import dastgen2

import peano4.toolbox.particles
import numpy as np

# See comments in README.dox
#export PYTHONPATH=$PYTHONPATH:~/Peano/applications/exahype2/ccz4
from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import CCZ4Solver_FD4_SecondOrderFormulation_GlobalAdaptiveTimeStepWithEnclaveTasking


def construct_FV_solver(min_cell_size, max_cell_size):
    """!
    
    Construct the Finite Volume (limiter) scheme
    
    We assume that the underlying Finite Differences scheme has a patch 
    size of 6x6x6. To make the Finite Volume scheme's time stepping (and
    accuracy) match this patch size, we have to employ a 16 times finer
    mesh.
    
    """
    my_solver = None
    
    patch_size = 6 * 16
    
    my_solver = CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking(name                 = "CCZ4SBH_FV" , 
                                                                       patch_size           = patch_size,
                                                                       min_volume_h       = min_cell_size/patch_size, 
                                                                       max_volume_h       = max_cell_size/patch_size,
                                                                       )
    
    my_solver.set_implementation(initial_conditions   = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                 refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
                                 boundary_conditions  = exahype2.solvers.fv.PDETerms.Empty_Implementation
                                 )
    my_solver.add_all_solver_constants()
    update_solver_parameters_for_single_black_hole (my_solver)
    return my_solver


def construct_FD_solver(min_cell_size, max_cell_size):
    """!
    
    Construct finite differences solver
    
    The FD scheme is our primary solver, i.e. the one we wanna use (almost) 
    everywhere. Hence, we have to be sure that we use the right boundary 
    conditions. In our case, that should be Sommerfeld ones.
    
    """
    my_solver = None
    
    patch_size = 6
    
    my_solver = CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking(name                 = "CCZ4SBH_FD4" , 
                                                                        patch_size           = patch_size,
                                                                        rk_order             = 1,
                                                                        min_meshcell_h       = min_cell_size/patch_size, 
                                                                        max_meshcell_h       = max_cell_size/patch_size,
                                                                        )
    
    my_solver.set_implementation(initial_conditions   = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                 refinement_criterion = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                 boundary_conditions  = exahype2.solvers.fv.PDETerms.None_Implementation
                                 )
    my_solver.add_all_solver_constants()

    my_solver._action_set_handle_boundary.TemplateHandleBoundary_KernelCalls = """
          double Qinf[59]={1.0, 0.0, 0.0, 1.0, 0.0, 1.0,        //q0-5
             0.0, 0.0, 0.0, 0.0, 0.0, 0.0,                      //q6-11
             0.0, 0.0, 0.0, 0.0,                                //q12-15
             1.0, 0.0, 0.0, 0.0,                                //q16-19
             0.0, 0.0, 0.0, 0.0, 0.0, 0.0,                      //q20-25
             0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,       //q26-34
             0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
             0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,       //q35-52
             0.0, 1.0, 0.0, 0.0, 0.0, 0.0                       //q53-58
          }; //approximate background solution at infinity 
          ::exahype2::fd::applySommerfeldConditions(
            [&](
              const double * __restrict__                  Q,
              const tarch::la::Vector<Dimensions,double>&  faceCentre,
              const tarch::la::Vector<Dimensions,double>&  gridCellH,
              double                                       t,
              double                                       dt,
              int                                          normal
            ) -> double {
              return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, gridCellH, t, dt, normal );
            },
            [&](
              double * __restrict__                        Q,
              const tarch::la::Vector<Dimensions,double>&  faceCentre,
              const tarch::la::Vector<Dimensions,double>&  gridCellH
            ) -> void {
              for (int i=0; i<"""+str(my_solver._unknowns+my_solver._auxiliary_variables)+"""; i++) {
                Q[i] = 0.0;
              }
              Q[0] = 1.0; Q[3] = 1.0; Q[5] = 1.0;
              Q[16] = 0.95; Q[54] = 0.95;
            },
            marker.x(),
            marker.h(),
            {{FACE_METADATA_ACCESSOR}}.getOldTimeStamp(marker.getSelectedFaceNumber()<Dimensions ? 1 : 0),
            repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
            {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
            {{OVERLAP}},
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            marker.getSelectedFaceNumber(),
            {0.0, 0.0, 0.0},
            fineGridFace{{UNKNOWN_IDENTIFIER}}Old.value,
            fineGridFace{{UNKNOWN_IDENTIFIER}}New.value
          );
          """
    
    update_solver_parameters_for_single_black_hole (my_solver)
    return my_solver

        
def update_solver_parameters_for_single_black_hole(solver):
  """!

  Update preconfigured solver parameters
  
  The default parameters of CCZ4 are tailored towards gauge waves or similar.
  For the single black hole, two parameters have to be changed. That's bs and
  sk which both have to be 1.0.

  """
  solver.double_constants[ "CCZ4bs" ] = 1.0
  solver.double_constants[ "CCZ4sk" ] = 1.0

  
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ExaHyPE 2 - CCZ4-GaugeWave benchmarking script')
    parser.add_argument("-j",   "--parallel-builds",   dest="j",                type=int, default=-1, help="Parallel builds" )
    parser.add_argument("-pd",  "--peano-dir",         dest="peanodir",         default="../../../../", help="Peano4 directory" )
    parser.add_argument("-v",   "--verbose",           dest="verbose",          action="store_true", default=False, help="Verbose")
    parser.add_argument("-et",  "--end-time",          dest="end_time",         type=float, default=1.0, help="End of simulation" )
    parser.add_argument("-cs",  "--cell-size",         dest="cell_size",        type=float, default=0.5, help="Cell size (default 0.5)" )
    parser.add_argument("-asserts", "--asserts",       dest="asserts",          action="store_true", default=False, help="Switch on assertions" )
    parser.add_argument("-plot",    "--plot",          dest="plot",             action="store_true", default=False, help="Switch on plotting" )
    
    args = parser.parse_args()
    
    
    project = exahype2.Project( ["benchmarks", "exahype2", "ccz4"], "ccz4", executable="peano_sbh_" + str(args.cell_size))
    
    my_primary_solver   = construct_FD_solver( args.cell_size, args.cell_size )
    my_secondary_solver = construct_FV_solver( args.cell_size, args.cell_size )

    project.add_solver(my_primary_solver)
    project.add_solver(my_secondary_solver)

        
    ########################################################################################
    # Build the project
    ########################################################################################
    
    if args.asserts:
      build_mode = peano4.output.CompileMode.Asserts
    else:
      build_mode = peano4.output.CompileMode.Release
        
            
    
    dimensions  = 3
    offset      = [-9, -9, -9] 
    domain_size = [   18,    18,    18]
    
    if args.plot:
      plot_interval = 0.1
    else:
      plot_interval = 0.0
    
    project.set_global_simulation_parameters(
      dimensions,               # dimensions
      offset,  domain_size,
      args.end_time,
      0.0, 
      plot_interval, 
      [False,False,False],
      8  # plotter precision
    )
    
    project.set_output_path("./")
    #probe_point = [-12,-12,0.0]
    #project.add_plot_filter( probe_point,[24.0,24.0,0.01],1 )
    #my_solver.select_dofs_to_print = [0,12,16,17,18,54,59,60]
    
    
    project.set_Peano4_installation(args.peanodir, build_mode)


    ########################################################################################
    # Add tracer
    ########################################################################################
    number_of_tracers = 1000
    tracer_coordinates = [ [ (i/ domain_size[0] / 2.0),0,0] for i in range(0,number_of_tracers) ]
    my_primary_solver.add_tracer(name        = "PrimaryTracer", 
                                 coordinates = tracer_coordinates,
                                 project     = project
                                 )
    my_secondary_solver.add_tracer(name        = "SecondaryTracer", 
                                   coordinates = tracer_coordinates,
                                   project     = project
                                   )

    
    #project.set_load_balancing("toolbox::loadbalancing::RecursiveBipartition","(new ::exahype2::LoadBalancingConfiguration(0.98,0))" )
    project.set_load_balancing("toolbox::loadbalancing::SpreadOutHierarchically","(new ::exahype2::LoadBalancingConfiguration(0.98))" )
    
    peano4_project = project.generate_Peano4_project(verbose=args.verbose)
    
    my_primary_solver.add_makefile_parameters(peano4_project, "../../../../applications/exahype2/ccz4" )
    
    peano4_project.output.makefile.add_linker_flag( "-lm -L/cosma/local/gsl/2.4/lib -lgsl -L/cosma/local/gsl/2.4/lib -lgslcblas" )
    peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/TP_Utilities.cpp" )
    peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/TP_Parameters.cpp" )
    peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/TP_Logging.cpp" )
    peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/TwoPunctures.cpp" )
    peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/CoordTransf.cpp" )
    peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/Equations.cpp" )
    peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/FuncAndJacobian.cpp" )
    peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/Newton.cpp" )
    peano4_project.output.makefile.add_CXX_flag( "-DIncludeTwoPunctures" )
         
    
    peano4_project.generate( throw_away_data_after_generation=False )
    peano4_project.build( make_clean_first = True, number_of_parallel_builds=args.j )
