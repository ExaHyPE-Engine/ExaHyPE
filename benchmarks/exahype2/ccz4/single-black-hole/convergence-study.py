import os
import argparse

import peano4
import exahype2
import dastgen2

import peano4.toolbox.particles
import numpy as np

import jinja2

# See comments in README.dox
#export PYTHONPATH=$PYTHONPATH:~/Peano/applications/exahype2/ccz4
from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import CCZ4Solver_FD4_SecondOrderFormulation_GlobalAdaptiveTimeStepWithEnclaveTasking

from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStep
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStep


# for the coupling
from exahype2.solvers.fv.actionsets.AbstractFVActionSet import AbstractFVActionSet



def construct_FV_limiter_for_FD4(min_cell_size, max_cell_size):
    """!
    
    Construct the Finite Volume (limiter) scheme
    
    We assume that the underlying Finite Differences scheme has a patch 
    size of 6x6x6. To make the Finite Volume scheme's time stepping (and
    accuracy) match this patch size, we have to employ a 16 times finer
    mesh.
    
    """


    class Limiter(CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking):
      SubPatchSize = int( FD4PatchSize*4*4 )
        
      def __init__(self,
                   name, 
                   min_cell_size, 
                   max_cell_size,
                   ):
        super( Limiter, self ).__init__(name               = name +  "_FV",
                                        patch_size         = self.SubPatchSize,
                                        min_volume_h       = min_cell_size/self.SubPatchSize, 
                                        max_volume_h       = max_cell_size/self.SubPatchSize,
                                        use_heap_storage   = False,
                                        )
        self.double_constants[ "BlackHoleFVRegion" ] = 1.0
 
        self._user_action_set_includes += """
#include "toolbox/blockstructured/Restriction.h"        
#include "toolbox/blockstructured/Interpolation.h"        
"""        

  
      def _store_cell_data_default_guard(self):
        """!
        
        Mask out exterior cells
        
        """
        return """(""" + super( Limiter, self )._store_cell_data_default_guard() + " and repositories::InstanceOf" + self._name + ".isCellOverlappingWithBHImpactArea(marker) )"
          

      def _load_cell_data_default_guard(self):      
        return """(""" + super( Limiter, self )._load_cell_data_default_guard() + " and repositories::InstanceOf" + self._name + ".isCellOverlappingWithBHImpactArea(marker) )"
    

      def _store_face_data_default_guard(self):
        return """(""" + super( Limiter, self )._store_face_data_default_guard() + """
                    and repositories::InstanceOf""" + self._name + """.areAdjacentCellsOverlappingWithBHImpactArea(marker) 
                  )"""
          

      def _load_face_data_default_guard(self):      
        return """(""" + super( Limiter, self )._load_face_data_default_guard() + """
                    and repositories::InstanceOf""" + self._name + """.areAdjacentCellsOverlappingWithBHImpactArea(marker) 
                  )"""


      def create_action_sets(self):
        """!
        
        Not really a lot of things to do here. The only exception that is 
        really important is that we have to ensure that we only solve stuff
        inside the local domain of the FV. By default, ExaHyPE 2 solves the
        PDE everywhere. If data is not stored persistently or loaded from 
        the persistent stacks, it still solves, as it then would assume that
        such data arises from dynamic AMR. In this particular case, we have
        to really mask out certain subdomains.
        
        It is not just a nice optimisation to do so. It is absolutely key, 
        as the application of the compute kernel on garbage would mean that
        we end up with invalid eigenvalues.
        
        """
        super( Limiter, self ).create_action_sets()
        
        self._action_set_update_cell.guard                = "({} and repositories::InstanceOf{}.isCellOverlappingWithBHImpactArea(marker))".format(self._action_set_update_cell.guard, self._name)
        self._action_set_merge_enclave_task_outcome.guard = "({} and repositories::InstanceOf{}.isCellOverlappingWithBHImpactArea(marker))".format(self._action_set_merge_enclave_task_outcome.guard, self._name)
        
        
    
    my_solver = Limiter(name           = "CCZ4SBH" , 
                        min_cell_size  = min_cell_size, 
                        max_cell_size  = max_cell_size,
                        ) 

    my_solver.set_implementation(initial_conditions   = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                 refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
                                 boundary_conditions  = exahype2.solvers.fv.PDETerms.Empty_Implementation
                                 )
    my_solver.add_all_solver_constants()
    update_solver_parameters_for_single_black_hole (my_solver)
    
    return my_solver


def construct_limited_FD4_solver(min_cell_size, max_cell_size):
    """!
    
    Construct 4th order Finite Differences solver with a limiter
    
    The FD scheme is our primary solver, i.e. the one we wanna use (almost) 
    everywhere. Hence, we have to be sure that we use the right boundary 
    conditions. In our case, that should be Sommerfeld ones. As we work 
    with a limited FD4 solver, we have to ensure that we get the 
    @ref benchmarks_exahype2_ccz4_single_black_hole "solver coupling" right.
    
    """

    class PrimarySolver(CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking):
      """!
      
      Construct primary solver
      
      The primary solver is a plain CCZ4 FD4 solver with enclave tasking. There
      are some modification though:
         
      1. We ensure that we use Sommerfeld boundary conditions. This means that 
         we have to replace the whole generic boundary treatment with a bespoke
         action set.      

      """
      def __init__(self,
                   name,
                   min_cell_size,
                   max_cell_size
                   ):
        self._name_without_FD4_extension = name
        
        CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking.__init__(self,
                                                                        name             = name + "_FD4", 
                                                                        patch_size       = FD4PatchSize,
                                                                        rk_order         = 1,
                                                                        min_meshcell_h   = min_cell_size/FD4PatchSize, 
                                                                        max_meshcell_h   = max_cell_size/FD4PatchSize,
                                                                        use_heap_storage = False,
                                                                        )

        self._user_action_set_includes += """
#include "toolbox/blockstructured/Projection.h"        
#include "toolbox/blockstructured/Restriction.h"        
#include "toolbox/blockstructured/Interpolation.h"        
#include "toolbox/blockstructured/Copy.h"        
"""        


      def create_action_sets(self):
        """!
        
        Tailor action set behaviour
        
        We first make a few additional cells skeleton cells. The rationale 
        behind additional skeletons is given in the @ref benchmarks_exahype2_ccz4_single_black_hole "generic overview". 
        Given the first remark there on FD4-FV coupling, one would be tempted 
        to use the predicate
        
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        self._action_set_update_cell.additional_skeleton_guard = " " "(
  repositories::InstanceOf" " " + self._name_without_FD4_extension + " " "_FV.isCellOverlappingWithBHImpactArea(marker) 
  and
  not repositories::InstanceOf" " " + self._name_without_FD4_extension + " " "_FV.areAllFaceConnectedCellsOverlappingWithBHImpactArea(marker) 
)
" " "        
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        
        Once we study the other items (notably the fourth), we see that it is
        reasonable to make all the overlap region a skeleton within the FD4
        solver.
        
        """
        super( PrimarySolver, self ).create_action_sets()
        
        self._action_set_handle_boundary.TemplateHandleBoundary_KernelCalls = """
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
              for (int i=0; i<"""+str(self._unknowns+self._auxiliary_variables)+"""; i++) {
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
 
        self._action_set_preprocess_solution = exahype2.solvers.rkfd.actionsets.PreprocessReconstructedSolutionWithHalo(self,
"""
    const int sizeOfPatch = (repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfFiniteVolumesPerAxisPerPatch+2)
      * (repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfFiniteVolumesPerAxisPerPatch+2)
      * (repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfFiniteVolumesPerAxisPerPatch+2)
      * (repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfUnknowns + repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfAuxiliaryVariables);
    const int sizeOfFace = 2
      * (repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfFiniteVolumesPerAxisPerPatch+2)
      * (repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfFiniteVolumesPerAxisPerPatch+2)
      * (repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfUnknowns + repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfAuxiliaryVariables);
      
    double* interpolatedFVDataWithHalo = ::tarch::allocateMemory<double>(sizeOfPatch, ::tarch::MemoryLocation::Heap);
    double* dummyFace0 = ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);
    double* dummyFace1 = ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);
    double* dummyFace2 = ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);
    double* dummyFace3 = ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);
    double* dummyFace4 = ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);
    double* dummyFace5 = ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);

    ::toolbox::blockstructured::interpolateCellDataAssociatedToVolumesIntoOverlappingCell_linear(
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FD4.NumberOfGridCellsPerPatchPerAxis,
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfFiniteVolumesPerAxisPerPatch,
      3, // halo in FD4
      1, // halo in FV
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfUnknowns + repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfAuxiliaryVariables,
      oldQWithHalo, 
      interpolatedFVDataWithHalo
    );

    ::toolbox::blockstructured::projectPatchHaloOntoFaces(
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfFiniteVolumesPerAxisPerPatch,
      1,      // int    haloSize,
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfUnknowns,
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfAuxiliaryVariables,
      interpolatedFVDataWithHalo,
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.areAdjacentCellsOverlappingWithBHImpactArea(marker,0) ? dummyFace0 : fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(0).value,
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.areAdjacentCellsOverlappingWithBHImpactArea(marker,1) ? dummyFace1 : fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(1).value,
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.areAdjacentCellsOverlappingWithBHImpactArea(marker,2) ? dummyFace2 : fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(2).value,
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.areAdjacentCellsOverlappingWithBHImpactArea(marker,3) ? dummyFace3 : fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(3).value,
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.areAdjacentCellsOverlappingWithBHImpactArea(marker,4) ? dummyFace4 : fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(4).value,
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.areAdjacentCellsOverlappingWithBHImpactArea(marker,5) ? dummyFace5 : fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(5).value
    );

    ::tarch::freeMemory(interpolatedFVDataWithHalo, tarch::MemoryLocation::Heap );
    ::tarch::freeMemory(dummyFace0, tarch::MemoryLocation::Heap );
    ::tarch::freeMemory(dummyFace1, tarch::MemoryLocation::Heap );
    ::tarch::freeMemory(dummyFace2, tarch::MemoryLocation::Heap );
    ::tarch::freeMemory(dummyFace3, tarch::MemoryLocation::Heap );
    ::tarch::freeMemory(dummyFace4, tarch::MemoryLocation::Heap );
    ::tarch::freeMemory(dummyFace5, tarch::MemoryLocation::Heap );
""")

        self._action_set_preprocess_solution.guard = "(" + self._action_set_preprocess_solution.guard + """
                                                      and 
                                                      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.isCellOverlappingWithBHImpactArea(marker) 
                                                      and
                                                      not repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.areAllFaceConnectedCellsOverlappingWithBHImpactArea(marker) 
                                                      and
                                                      not marker.willBeRefined()
                                                      )""" 

        self._action_set_postprocess_solution = exahype2.solvers.rkfd.actionsets.PatchWisePostprocessSolution(self,
                                                                                                              """
  if (
    repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.isCellOverlappingWithBHImpactArea(marker) 
    and
    not marker.willBeRefined()
  ) { 
    logTraceIn( "touchCellFirstTime(...)-inject" );
    
    repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.incNumberOfPatches();
    
    ::toolbox::blockstructured::restrictCellIntoOverlappingCell_inject(
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfFiniteVolumesPerAxisPerPatch,
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FD4.NumberOfGridCellsPerPatchPerAxis,
      repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfUnknowns + repositories::InstanceOf""" + self._name_without_FD4_extension + """_FV.NumberOfAuxiliaryVariables,
      fineGridCell""" + self._name_without_FD4_extension + """_FVQ.value,
      fineGridCell""" + self._name_without_FD4_extension + """_FD4Q.value
    );
  
    logTraceOut( "touchCellFirstTime(...)-inject" );
  }  
""")

        

    my_solver = PrimarySolver(name           = "CCZ4SBH" , 
                              min_cell_size  = min_cell_size, 
                              max_cell_size  = max_cell_size, 
                              )
    
    my_solver.set_implementation(initial_conditions   = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                 refinement_criterion = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                 boundary_conditions  = exahype2.solvers.fv.PDETerms.None_Implementation
                                 )
    my_solver.add_all_solver_constants()
  
    update_solver_parameters_for_single_black_hole (my_solver)
    return my_solver


def construct_FD4_solver(min_cell_size, max_cell_size):
    """!
    
    Construct 4th order Finite Differences solver without a limiter
    
    """

    class Solver(CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking):
      """!
      
      Construct solver
      
      """
      def __init__(self,
                   name,
                   min_cell_size,
                   max_cell_size,
                   use_heap_storage = False
                   ):
        self._name_without_FD4_extension = name
        
        CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking.__init__(self,
                                                                        name            = name + "_FD4", 
                                                                        patch_size      = FD4PatchSize,
                                                                        rk_order        = 1,
                                                                        min_meshcell_h  = min_cell_size/FD4PatchSize, 
                                                                        max_meshcell_h  = max_cell_size/FD4PatchSize,
                                                                        )

        self._user_action_set_includes += """
#include "toolbox/blockstructured/Projection.h"        
#include "toolbox/blockstructured/Restriction.h"        
#include "toolbox/blockstructured/Interpolation.h"        
#include "toolbox/blockstructured/Copy.h"        
"""        
     

      def create_action_sets(self):
        """!
        
        Tailor action set behaviour
        
        We first make a few additional cells skeleton cells. The rationale 
        behind additional skeletons is given in the @ref benchmarks_exahype2_ccz4_single_black_hole "generic overview". 
        Given the first remark there on FD4-FV coupling, one would be tempted 
        to use the predicate
        
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        self._action_set_update_cell.additional_skeleton_guard = " " "(
  repositories::InstanceOf" " " + self._name_without_FD4_extension + " " "_FV.isCellOverlappingWithBHImpactArea(marker) 
  and
  not repositories::InstanceOf" " " + self._name_without_FD4_extension + " " "_FV.areAllFaceConnectedCellsOverlappingWithBHImpactArea(marker) 
)
" " "        
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        
        Once we study the other items (notably the fourth), we see that it is
        reasonable to make all the overlap region a skeleton within the FD4
        solver.
        
        """
        super( Solver, self ).create_action_sets()
        
        self._action_set_handle_boundary.TemplateHandleBoundary_KernelCalls = """
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
              for (int i=0; i<"""+str(self._unknowns+self._auxiliary_variables)+"""; i++) {
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

    my_solver = Solver(name           = "CCZ4SBH" , 
                       min_cell_size  = min_cell_size, 
                       max_cell_size  = max_cell_size, 
                       )
    
    my_solver.set_implementation(initial_conditions   = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                 refinement_criterion = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
                                 boundary_conditions  = exahype2.solvers.fv.PDETerms.None_Implementation
                                 )
    my_solver.add_all_solver_constants()
  
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
    parser.add_argument("-no-tracer", "--no-tracer",   dest="no_tracer",        action="store_true", default=False, help="Switch off tracers (by default enabled)" )
    parser.add_argument("-plot",    "--plot",          dest="plot",             action="store_true", default=False, help="Switch on plotting" )
    parser.add_argument("-s",   "--solver",            dest="solver",           choices=["fd4-rk1-ps-3", "fd4-rk1-ps-6", "fd4-rk1-limited-ps-3", "fd4-rk1-limited-ps-6"], required=True, help="Pick solver type" )
    
    args = parser.parse_args()
    
    
    project = exahype2.Project( ["benchmarks", "exahype2", "ccz4"], "ccz4", executable="peano_sbh_" + str(args.cell_size))

    added_limiter = False
    if "fd4-rk1-ps-" in args.solver:    
      FD4PatchSize = int(args.solver[-1])
      my_primary_solver   = construct_FD4_solver( args.cell_size, args.cell_size )
      project.add_solver(my_primary_solver)
      added_limiter = False
    elif "fd4-rk1-limited-ps-" in args.solver:    
      FD4PatchSize = int(args.solver[-1])
      my_primary_solver   = construct_limited_FD4_solver( args.cell_size, args.cell_size )
      my_secondary_solver = construct_FV_limiter_for_FD4( args.cell_size, args.cell_size )
      project.add_solver(my_primary_solver)
      project.add_solver(my_secondary_solver)
      added_limiter = True
    else:
      raise "unknown solver type {}".format( args.solver )


        
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
      plot_interval = args.end_time/60.0
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
    if not args.no_tracer:
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
    project.set_load_balancing("toolbox::loadbalancing::SpreadOutHierarchically","(new ::exahype2::LoadBalancingConfiguration(0.98, 100))" )
    
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
    if added_limiter:
      peano4_project.output.makefile.add_CXX_flag( "-DCoupleWithFV" )
             
    peano4_project.generate( throw_away_data_after_generation=False )
    peano4_project.build( make_clean_first = True, number_of_parallel_builds=args.j )
