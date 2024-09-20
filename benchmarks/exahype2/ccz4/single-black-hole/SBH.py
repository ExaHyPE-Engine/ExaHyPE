# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import argparse

import peano4
import exahype2
import dastgen2

import peano4.toolbox.particles
import numpy as np

import jinja2


import exahype2.solvers.fv.rusanov.kernels

from enum import Enum

# See comments in README.dox
# export PYTHONPATH=../../../../python
# export PYTHONPATH=$PYTHONPATH:../../../../applications/exahype2/ccz4
from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import (
    CCZ4Solver_FD4_SecondOrderFormulation_GlobalAdaptiveTimeStepWithEnclaveTasking,
)

from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStep
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStep


# for the coupling
from exahype2.solvers.fv.actionsets.AbstractFVActionSet import AbstractFVActionSet


BlackHoleRegion = 0.1


class KernelParallelisation(Enum):
    NONE         = "::peano4::utils::LoopPlacement::Serial"
    PARALLEL_FOR = "::peano4::utils::LoopPlacement::Nested"
    SUBTASKS     = "::peano4::utils::LoopPlacement::SpreadOut"



class Limiter(CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking):
    """!

    Construct the Finite Volume (limiter) scheme

    We assume that the underlying Finite Differences scheme has a patch
    size of 6x6x6. To make the Finite Volume scheme's time stepping (and
    accuracy) match this patch size, we have to employ a 16 times finer
    mesh.

    It is interesting to see that the limiter does not really have a min
    and max mesh size. The point is that the higher order solver dictates
    the mesh structure, and we then follow this structure with the 
    Finite Volume scheme.

    """
    def __init__(self,
                 name,
                 patch_size,
                 amend_priorities: bool,
                 parallelisation_of_kernels:       KernelParallelisation,
                 ):
        """!

        Construct the limiter
        
        patch_size: Integer
          Pass in the patch size of the FD4 scheme or, if you are using RKDG, 
          hand in the number 1. The Finite Volume patch then will be 16 times
          finer.

        """
        SubPatchSize = int(patch_size * 4 * 4)

        super(Limiter, self).__init__(
            name=name + "_FV",
            patch_size=SubPatchSize,
            min_volume_h=1.0/65536,
            max_volume_h=65536,
            pde_terms_without_state=True,
        )
        self.double_constants["BlackHoleFVRegion"] = BlackHoleRegion

        self._user_action_set_includes += """
#include "toolbox/blockstructured/Restriction.h"        
#include "toolbox/blockstructured/Interpolation.h"        
"""

        if amend_priorities:
          self.enclave_task_priority = "tarch::multicore::Task::DefaultPriority+1"

          #self._fused_compute_kernel_call_gpu  = exahype2.solvers.fv.rusanov.kernels.create_compute_Riemann_kernel_for_Rusanov(
          #  self._flux_implementation, self._ncp_implementation, self._source_term_implementation, 
          #  compute_max_eigenvalue_of_next_time_step=True, 
          #  solver_variant         = exahype2.solvers.fv.rusanov.kernels.SolverVariant.Accelerator,
          #  kernel_variant         = exahype2.solvers.fv.rusanov.kernels.KernelVariant.GenericAoS
          #  )

        self.set_implementation(
            initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
            refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
            boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
        )

        if parallelisation_of_kernels == KernelParallelisation.SUBTASKS:
          #self._compute_kernel_call            = exahype2.solvers.fv.rusanov.kernels.create_compute_Riemann_kernel_for_Rusanov(
          #  self._flux_implementation, self._ncp_implementation, self._source_term_implementation, 
          #  compute_max_eigenvalue_of_next_time_step=True, 
          #  solver_variant         = exahype2.solvers.fv.rusanov.kernels.SolverVariant.WithVirtualFunctions,
          #  kernel_variant         = exahype2.solvers.fv.rusanov.kernels.KernelVariant.PatchWiseAoSHeap
          #  )
    
            self._fused_compute_kernel_call_cpu  = exahype2.solvers.fv.rusanov.kernels.create_compute_Riemann_kernel_for_Rusanov(
              self._flux_implementation, self._ncp_implementation, self._source_term_implementation, 
              compute_max_eigenvalue_of_next_time_step=True, 
              solver_variant         = exahype2.solvers.fv.rusanov.kernels.SolverVariant.Multicore,
              kernel_variant         = exahype2.solvers.fv.rusanov.kernels.KernelVariant.PatchWiseAoSHeap
              )
        
        self.add_all_solver_constants()
        update_solver_parameters_for_single_black_hole(self)
        

    def _store_cell_data_default_guard(self):
        """!

        Mask out exterior cells

        """
        return (
            """("""
            + super(Limiter, self)._store_cell_data_default_guard()
            + " and repositories::instanceOf"
            + self._name
            + ".isCellOverlappingWithBHImpactArea(marker) )"
        )

    def _load_cell_data_default_guard(self):
        return (
            """("""
            + super(Limiter, self)._load_cell_data_default_guard()
            + " and repositories::instanceOf"
            + self._name
            + ".isCellOverlappingWithBHImpactArea(marker) )"
        )
           
    def _provide_cell_data_to_compute_kernels_default_guard(self):
        return (
            """("""
            + super(Limiter,self)._provide_cell_data_to_compute_kernels_default_guard()
            + " and repositories::instanceOf"
            + self._name
            + ".isCellOverlappingWithBHImpactArea(marker) )"
        )
      
    def _provide_face_data_to_compute_kernels_default_guard(self):
        return "{} and repositories::instanceOf{}.isOneAdjacentCellOverlappingWithBHImpactArea(marker)" .format(
                super(Limiter,self)._provide_face_data_to_compute_kernels_default_guard(),
                self._name
            )
        
    def _store_face_data_default_guard(self):
        return "({} and repositories::instanceOf{}.areBothAdjacentCellsOverlappingWithBHImpactArea(marker))".format(
                super(Limiter, self)._store_face_data_default_guard(),
                self._name
            )

    def _load_face_data_default_guard(self):
        return "({} and repositories::instanceOf{}.areBothAdjacentCellsOverlappingWithBHImpactArea(marker))".format(
                super(Limiter, self)._load_face_data_default_guard(),
                self._name
            )

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
        super(Limiter, self).create_action_sets()

        self._action_set_update_cell.guard = "({} and repositories::instanceOf{}.isCellOverlappingWithBHImpactArea(marker))".format(
            self._action_set_update_cell.guard, self._name
        )
        self._action_set_merge_enclave_task_outcome.guard = "({} and repositories::instanceOf{}.isCellOverlappingWithBHImpactArea(marker))".format(
            self._action_set_merge_enclave_task_outcome.guard, self._name
        )


class FD4SolverWithLimiter(CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking):
    """!

    4th order Finite Differences solver with a limiter

    The FD scheme is our primary solver, i.e. the one we wanna use (almost)
    everywhere. Hence, we have to be sure that we use the right boundary
    conditions. In our case, that should be Sommerfeld ones. As we work
    with a limited FD4 solver, we have to ensure that we get the
    @ref benchmarks_exahype2_ccz4_single_black_hole "solver coupling" right.

    The primary solver is a plain CCZ4 FD4 solver with enclave tasking. There
    are some modification though:

    1. We ensure that we use Sommerfeld boundary conditions. This means that
       we have to replace the whole generic boundary treatment with a bespoke
       action set.
    2. The coupling has to be injected. We model the coupling as separate
       (enclave) tasks to avoid that we totally serialise the solver steps
       whenever we encounter a coupling.

    """
    def __init__(self, 
                 name, 
                 patch_size,
                 min_cell_size, 
                 max_cell_size,
                 parallelisation_of_interpolation: KernelParallelisation,
                 parallelisation_of_kernels:       KernelParallelisation,
                 ):
        self._name_without_FD4_extension       = name
        self._parallelisation_of_interpolation = parallelisation_of_interpolation

        CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking.__init__(
              self,
              name=name + "_FD4",
              patch_size=patch_size,
              rk_order=1,
              min_meshcell_h=min_cell_size / patch_size,
              max_meshcell_h=max_cell_size / patch_size,
              pde_terms_without_state=False
        )

        self._user_action_set_includes += """
#include "toolbox/blockstructured/Projection.h"        
#include "toolbox/blockstructured/Restriction.h"        
#include "toolbox/blockstructured/Interpolation.h"        
#include "toolbox/blockstructured/Copy.h"        
"""

        self.double_constants["BlackHoleFVRegion"] = BlackHoleRegion
          
        self.set_implementation(
            initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
            refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
            boundary_conditions=exahype2.solvers.PDETerms.None_Implementation,
        )
        self.add_all_solver_constants()
                
        if parallelisation_of_kernels == KernelParallelisation.SUBTASKS:
            print( "WARNING: Unfortunately, we have not yet written parallelised kernels for FD4")
            pass
        
        update_solver_parameters_for_single_black_hole(self)
  
  
    def create_action_sets(self):
        """!

                Tailor action set behaviour

                We first make a few additional cells skeleton cells. The rationale
                behind additional skeletons is given in the @ref benchmarks_exahype2_ccz4_single_black_hole "generic overview".
                Given the first remark there on FD4-FV coupling, one would be tempted
                to use the predicate

                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                self._action_set_update_cell.additional_skeleton_guard = " " "(
          repositories::instanceOf" " " + self._name_without_FD4_extension + " " "_FV.isCellOverlappingWithBHImpactArea(marker)
          and
          not repositories::instanceOf" " " + self._name_without_FD4_extension + " " "_FV.areAllFaceConnectedCellsOverlappingWithBHImpactArea(marker)
        )
        " " "
                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

                Once we study the other items (notably the fourth), we see that it is
                reasonable to make all the overlap region a skeleton within the FD4
                solver.

        """
        super(FD4SolverWithLimiter, self).create_action_sets()

        self._action_set_handle_boundary.TemplateHandleBoundary_KernelCalls = (
            """
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
          for (int i=0; i<"""
            + str(self._unknowns + self._auxiliary_variables)
            + """; i++) {
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
        )

        self._action_set_preprocess_solution = exahype2.solvers.rkfd.actionsets.PreprocessReconstructedSolutionWithHalo(
            solver = self,
            enclave_task_cell_label = "fineGridCell" + self._name_without_FD4_extension + "_FVCellLabel",
            compute_kernel_implementation = """
const int sizeOfPatch = (repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfFiniteVolumesPerAxisPerPatch+2)
  * (repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfFiniteVolumesPerAxisPerPatch+2)
  * (repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfFiniteVolumesPerAxisPerPatch+2)
  * (repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfUnknowns + repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfAuxiliaryVariables);
const int sizeOfFace = 2
  * (repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfFiniteVolumesPerAxisPerPatch+2)
  * (repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfFiniteVolumesPerAxisPerPatch+2)
  * (repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfUnknowns + repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfAuxiliaryVariables);
  
double* interpolatedFVDataWithHalo = ::tarch::allocateMemory<double>(sizeOfPatch, ::tarch::MemoryLocation::Heap);

bool faceIsReal0 = repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.areBothAdjacentCellsOverlappingWithBHImpactArea(marker,0);
bool faceIsReal1 = repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.areBothAdjacentCellsOverlappingWithBHImpactArea(marker,1);
bool faceIsReal2 = repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.areBothAdjacentCellsOverlappingWithBHImpactArea(marker,2);
bool faceIsReal3 = repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.areBothAdjacentCellsOverlappingWithBHImpactArea(marker,3);
bool faceIsReal4 = repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.areBothAdjacentCellsOverlappingWithBHImpactArea(marker,4);
bool faceIsReal5 = repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.areBothAdjacentCellsOverlappingWithBHImpactArea(marker,5);
            
double* realFace0 = faceIsReal0 ? fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(0).value : nullptr;
double* realFace1 = faceIsReal1 ? fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(1).value : nullptr;
double* realFace2 = faceIsReal2 ? fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(2).value : nullptr;
double* realFace3 = faceIsReal3 ? fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(3).value : nullptr;
double* realFace4 = faceIsReal4 ? fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(4).value : nullptr;
double* realFace5 = faceIsReal5 ? fineGridFaces""" + self._name_without_FD4_extension + """_FVQNew(5).value : nullptr;

double* dummyFace0 = faceIsReal0 ? nullptr : ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);
double* dummyFace1 = faceIsReal1 ? nullptr : ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);
double* dummyFace2 = faceIsReal2 ? nullptr : ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);
double* dummyFace3 = faceIsReal3 ? nullptr : ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);
double* dummyFace4 = faceIsReal4 ? nullptr : ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);
double* dummyFace5 = faceIsReal5 ? nullptr : ::tarch::allocateMemory<double>(sizeOfFace, ::tarch::MemoryLocation::Heap);

::toolbox::blockstructured::interpolateCellDataAssociatedToVolumesIntoOverlappingCell_linear(
  repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FD4.NumberOfGridCellsPerPatchPerAxis,
  repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfFiniteVolumesPerAxisPerPatch,
  3, // halo in FD4
  1, // halo in FV
  repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfUnknowns + repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfAuxiliaryVariables,
  oldQWithHalo, 
  interpolatedFVDataWithHalo,
  """ + str(self._parallelisation_of_interpolation.value) + """
);

::toolbox::blockstructured::projectPatchHaloOntoFaces(
  repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfFiniteVolumesPerAxisPerPatch,
  1,      // int    haloSize,
  repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfUnknowns,
  repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.NumberOfAuxiliaryVariables,
  interpolatedFVDataWithHalo,
  faceIsReal0 ? realFace0 : dummyFace0,
  faceIsReal1 ? realFace1 : dummyFace1,
  faceIsReal2 ? realFace2 : dummyFace2,
  faceIsReal3 ? realFace3 : dummyFace3,
  faceIsReal4 ? realFace4 : dummyFace4,
  faceIsReal5 ? realFace5 : dummyFace5
);

::tarch::freeMemory(interpolatedFVDataWithHalo, tarch::MemoryLocation::Heap );
if (dummyFace0!=nullptr) ::tarch::freeMemory(dummyFace0, tarch::MemoryLocation::Heap );
if (dummyFace1!=nullptr) ::tarch::freeMemory(dummyFace1, tarch::MemoryLocation::Heap );
if (dummyFace2!=nullptr) ::tarch::freeMemory(dummyFace2, tarch::MemoryLocation::Heap );
if (dummyFace3!=nullptr) ::tarch::freeMemory(dummyFace3, tarch::MemoryLocation::Heap );
if (dummyFace4!=nullptr) ::tarch::freeMemory(dummyFace4, tarch::MemoryLocation::Heap );
if (dummyFace5!=nullptr) ::tarch::freeMemory(dummyFace5, tarch::MemoryLocation::Heap );
""",
        )

        self._action_set_preprocess_solution.guard = (
            "("
            + self._action_set_preprocess_solution.guard
            + """
                                                  and 
                                                  repositories::instanceOf"""
            + self._name_without_FD4_extension + """_FV.isCellOverlappingWithBHImpactArea(marker) 
                                                  and
                                                  not repositories::instanceOf"""
            + self._name_without_FD4_extension
            + """_FV.areAllFaceConnectedCellsOverlappingWithBHImpactArea(marker) 
                                                  and
                                                  not marker.willBeRefined()
                                                  )"""
        )

        self._action_set_postprocess_solution = exahype2.solvers.rkfd.actionsets.PatchWisePostprocessSolution(
            self,
            """
if (
  repositories::instanceOf{0}_FV.isCellOverlappingWithBHImpactArea(marker) 
  and
  not marker.willBeRefined()
) {{
  logTraceIn( "touchCellFirstTime(...)-inject" );

  repositories::instanceOf{0}_FV.incNumberOfPatches();
  
  //if ( repositories::instanceOf{0}_FV.areAllFaceConnectedCellsOverlappingWithBHImpactArea(marker) ) {{
    ::toolbox::blockstructured::restrictCellIntoOverlappingCell_inject(
      repositories::instanceOf{0}_FV.NumberOfFiniteVolumesPerAxisPerPatch,
      repositories::instanceOf{0}_FD4.NumberOfGridCellsPerPatchPerAxis,
      repositories::instanceOf{0}_FV.NumberOfUnknowns + repositories::instanceOf{0}_FV.NumberOfAuxiliaryVariables,
      fineGridCell{0}_FVQ.value,
      fineGridCell{0}_FD4Q.value
    );
  /*}}
  else {{
    ::toolbox::blockstructured::restrictCellIntoOverlappingCell_inject_and_average(
      repositories::instanceOf{0}_FV.NumberOfFiniteVolumesPerAxisPerPatch,
      repositories::instanceOf{0}_FD4.NumberOfGridCellsPerPatchPerAxis,
      repositories::instanceOf{0}_FV.NumberOfUnknowns + repositories::instanceOf{0}_FV.NumberOfAuxiliaryVariables,
      fineGridCell{0}_FVQ.value,
      fineGridCell{0}_FD4Q.value
    );
  }}*/

  logTraceOut( "touchCellFirstTime(...)-inject" );
}}
""".format(self._name_without_FD4_extension))




class FD4SolverWithoutLimiter(CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking):
    """!

    Construct 4th order Finite Differences solver without a limiter

    """

    def __init__(self, 
                 name, 
                 patch_size,
                 min_cell_size, 
                 max_cell_size, 
                 ):
        self._name_without_FD4_extension = name

        CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking.__init__(
            self,
            name=name + "_FD4",
            patch_size=patch_size,
            rk_order=1,
            min_meshcell_h=min_cell_size / patch_size,
            max_meshcell_h=max_cell_size / patch_size,
            pde_terms_without_state=False
        )

        self._user_action_set_includes += """
#include "toolbox/blockstructured/Projection.h"        
#include "toolbox/blockstructured/Restriction.h"        
#include "toolbox/blockstructured/Interpolation.h"        
#include "toolbox/blockstructured/Copy.h"        
"""
        ## For compatibility. Name makes no sense. But the value
        ## guides the AMR pattern
        self.double_constants["BlackHoleFVRegion"] = BlackHoleRegion

        self.set_implementation(
            initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
            refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
            boundary_conditions=exahype2.solvers.PDETerms.None_Implementation,
        )

        self.add_all_solver_constants()
        update_solver_parameters_for_single_black_hole(self)
        

    def create_action_sets(self):
        """!

                Tailor action set behaviour

                We first make a few additional cells skeleton cells. The rationale
                behind additional skeletons is given in the @ref benchmarks_exahype2_ccz4_single_black_hole "generic overview".
                Given the first remark there on FD4-FV coupling, one would be tempted
                to use the predicate

                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                self._action_set_update_cell.additional_skeleton_guard = " " "(
          repositories::instanceOf" " " + self._name_without_FD4_extension + " " "_FV.isCellOverlappingWithBHImpactArea(marker)
          and
          not repositories::instanceOf" " " + self._name_without_FD4_extension + " " "_FV.areAllFaceConnectedCellsOverlappingWithBHImpactArea(marker)
        )
        " " "
                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

                Once we study the other items (notably the fourth), we see that it is
                reasonable to make all the overlap region a skeleton within the FD4
                solver.

        """
        super(FD4SolverWithoutLimiter, self).create_action_sets()

        self._action_set_handle_boundary.TemplateHandleBoundary_KernelCalls = (
            """
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
          for (int i=0; i<"""
            + str(self._unknowns + self._auxiliary_variables)
            + """; i++) {
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
        )


class FVSolver(CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking):
    """!

    A finite volume solver
    
    This solver is not appropriate to simulate black holes as a stand-alone 
    solver, as it is way too diffusive. If you use it without another scheme, 
    you typically see the black hole disappear after a brief period. So we 
    have it in here merely for performance tests.
    
    """

    def __init__(self, 
                 name, 
                 patch_size,
                 min_cell_size, 
                 max_cell_size, 
                 ):
        """!
        
        Construct the Finite Volume solver
        
        @param patch_size: Integer
           Defines how big the individual patches are. If you pass in 10, each
           Finite Volume patch will have the dimensions 10x10x10.
        @param min_cell_size: Float
           This parameter refers to the cell size, i.e. the size of a whole 
           patch. We use this one here, to make the signature the same as for 
           the FD and DG solver variants. The superclass constructor argues 
           over finite volume sizes, and we hence have to recalibrate this 
           parameter with patch_size.
        
        """

        super(FVSolver, self).__init__(
            name=name + "_FV",
            patch_size=patch_size,
            min_volume_h=min_cell_size / patch_size,
            max_volume_h=max_cell_size / patch_size,
            pde_terms_without_state=True
        )

        self._user_action_set_includes += """
#include "toolbox/blockstructured/Projection.h"        
#include "toolbox/blockstructured/Restriction.h"        
#include "toolbox/blockstructured/Interpolation.h"        
#include "toolbox/blockstructured/Copy.h"        
"""
        ## For compatibility. Name makes no sense. But the value
        ## guides the AMR pattern
        self.double_constants["BlackHoleFVRegion"] = BlackHoleRegion
        
        self.set_implementation(
            initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
            refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
            boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
        )

        self.add_all_solver_constants()
        update_solver_parameters_for_single_black_hole(self)

    def create_action_sets(self):
        """!

                Tailor action set behaviour

                We first make a few additional cells skeleton cells. The rationale
                behind additional skeletons is given in the @ref benchmarks_exahype2_ccz4_single_black_hole "generic overview".
                Given the first remark there on FD4-FV coupling, one would be tempted
                to use the predicate

                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                self._action_set_update_cell.additional_skeleton_guard = " " "(
          repositories::instanceOf" " " + self._name_without_FD4_extension + " " "_FV.isCellOverlappingWithBHImpactArea(marker)
          and
          not repositories::instanceOf" " " + self._name_without_FD4_extension + " " "_FV.areAllFaceConnectedCellsOverlappingWithBHImpactArea(marker)
        )
        " " "
                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

                Once we study the other items (notably the fourth), we see that it is
                reasonable to make all the overlap region a skeleton within the FD4
                solver.

        """
        super(FVSolver, self).create_action_sets()



def update_solver_parameters_for_single_black_hole(solver):
    """!

    Update preconfigured solver parameters

    The default parameters of CCZ4 are tailored towards gauge waves or similar.
    For the single black hole, two parameters have to be changed. That's bs and
    sk which both have to be 1.0.

    """
    solver.double_constants["CCZ4bs"] = 1.0
    solver.double_constants["CCZ4sk"] = 1.0


