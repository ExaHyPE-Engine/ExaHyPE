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


parser = argparse.ArgumentParser(description='ExaHyPE 2 - CCZ4-GaugeWave benchmarking script')
parser.add_argument("-j",   "--parallel-builds",   dest="j",                type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",         dest="peanodir",         default="../../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",           dest="verbose",          action="store_true", default=False, help="Verbose")
parser.add_argument("-et",  "--end-time",          dest="end_time",         type=float, default=1.0, help="End of simulation" )
parser.add_argument("-cs",  "--cell-size",         dest="cell_size",        type=float, default=0.5, help="Cell size (default 0.5)" )
parser.add_argument("-asserts", "--asserts",      dest="asserts",          action="store_true", default=False, help="Switch on assertions" )
parser.add_argument("-tracer",  "--tracer",       dest="tracer",           action="store_true", default=False, help="Activate tracer" )
parser.add_argument("-hs",      "--heap-storage",          dest="use_heap_storage", action="store_true", default=False, help="Store user data on heap via smart pointers" )

args = parser.parse_args()


project = exahype2.Project( ["benchmarks", "exahype2", "ccz4"], "ccz4", executable="peano_sbh_" + str(args.cell_size))


my_solver = None

my_solver = CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking(name                 = "CCZ4SBH" , 
                                                                      patch_size           = 6,
                                                                      rk_order             = 1,
                                                                      min_meshcell_h       = args.cell_size, 
                                                                      max_meshcell_h       = args.cell_size,
                                                                      )

my_solver.set_implementation(initial_conditions   = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                               refinement_criterion = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                               boundary_conditions=exahype2.solvers.fv.PDETerms.User_Defined_Implementation
                               )
                       

assert my_solver!=None

project.add_solver(my_solver)


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

project.set_global_simulation_parameters(
  dimensions,               # dimensions
  offset,  domain_size,
  args.end_time,
  0.0, 
  0.1,
  [False,False,False],
  8  # plotter precision
)

project.set_output_path("./")
#probe_point = [-12,-12,0.0]
#project.add_plot_filter( probe_point,[24.0,24.0,0.01],1 )
#my_solver.select_dofs_to_print = [0,12,16,17,18,54,59,60]

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

project.set_Peano4_installation(args.peanodir, build_mode)

#project.set_load_balancing("toolbox::loadbalancing::RecursiveBipartition","(new ::exahype2::LoadBalancingConfiguration(0.98,0))" )
project.set_load_balancing("toolbox::loadbalancing::SpreadOutHierarchically","(new ::exahype2::LoadBalancingConfiguration(0.98))" )

peano4_project = project.generate_Peano4_project(verbose=args.verbose)

my_solver.add_makefile_parameters(peano4_project, "../../../../applications/exahype2/ccz4" )

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
