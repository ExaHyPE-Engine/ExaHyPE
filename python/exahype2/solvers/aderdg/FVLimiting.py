# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .ADERDG import ADERDG, PrecisionType, Polynomials

from .PDETerms import PDETerms

from .kernelgenerator import generate_limiter_kernels
from .kernels import get_face_overlap_merge_implementation

import peano4.datamodel.DaStGen2
from .actionsets import RecomputeSolutionWithFV, SpreadLimiterStatus, HandleFVBoundary
import dastgen2
import jinja2


class FVLimiting(ADERDG):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        min_cell_h,
        max_cell_h,
        number_of_dmp_observables=0,
        dmp_relaxation_parameter = 0.0,
        dmp_differences_scaling = 0.0,
        physical_admissibility_criterion  = PDETerms.None_Implementation,
        halo_size=1,
        plot_grid_properties=False
    ):
        super(FVLimiting, self).__init__(
            name,
            order,
            unknowns,
            auxiliary_variables,
            min_cell_h,
            max_cell_h,
            plot_grid_properties,
        )

        self._solver_template_file_class_name = "SingleSweep"

        self._hold_previous_time_step = True
        self._use_troubled_marker = True
        self._use_limiting        = True

        self._number_of_dmp_observables = self._unknowns if number_of_dmp_observables is None else number_of_dmp_observables
        self._physical_admissibility_criterion  = physical_admissibility_criterion

        self._dmp_relaxation_parameter = dmp_relaxation_parameter
        self._dmp_differences_scaling  = dmp_differences_scaling

        self._halo_size = halo_size

        self.create_action_sets()
        self.create_data_structures()

    def create_data_structures(self):
        super(FVLimiting, self).create_data_structures()

        #Ensure that the face projection are big enough for the FV projections
        self._rhs_estimates_projection.dim = (2, 2*self._order+1, 2*self._order+1)

        """
        This will serve as a face marker to communicate with neighbouring cells whether
        a given cell is troubled, is neighbouring a troubled cell, or is fine.
        Cells that are troubled or neighbouring a troubled cell must rollback their solution
        to the previous one and perform one timestep using a more robust FV scheme.
        Therefore these and their neighbours must convert the DG representation of their
        previous solution to an FV representation and project it to their faces so that these
        can be exchanged with their neighbours.
        """
        Variants = ["ADER","DG_TO_FV","FV_TO_DG","TROUBLED"]
        self._cell_label.data.add_attribute(dastgen2.attributes.Enumeration( name="Troubled_Marker",      variants=Variants ) )
        self._face_label.data.add_attribute(dastgen2.attributes.Enumeration( name="Troubled_Marker",      variants=Variants ) )

        self._face_label.peano4_mpi_and_storage_aspect.merge_implementation += "\n  _Troubled_Marker = std::max(_Troubled_Marker, neighbour.getTroubled_Marker());"

        #this will contain the projection of the DG patch to finite volumes
        self._fv_projection = peano4.datamodel.Patch(
            (2*self._order + 1, 2*self._order + 1, 2*self._order + 1),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier() + "_fv",
            self._solution_persistent_storage_precision,
        )

        if self._hold_cell_data_on_heap_with_smart_pointer:
          self._fv_projection = peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
              self._fv_projection,
              self._solution_persistent_storage_precision
          )

        self._fv_projection.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_cell_data_default_guard(),
            self._store_cell_data_default_guard(),
        )

        self._fv_projection.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

        #this will contain the fv projections
        self._fv_estimates_projection = peano4.datamodel.Patch(
            (2, 2*self._order+1, 2*self._order+1),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier() + "_FV_Projections",
            self._corrector_computation_precision,
        )
        if self._hold_face_data_on_heap_with_smart_pointer:
          self._fv_estimates_projection.generator  = peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
              self._fv_estimates_projection,
              self._corrector_computation_precision
        )
          
        self._fv_estimates_projection.generator.merge_method_definition = (
          peano4.toolbox.blockstructured.get_face_merge_implementation(
              self._fv_estimates_projection
          )
        )

        self._fv_estimates_projection.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_face_data_default_guard(),
            self._store_face_data_default_guard()
        )

        self._fv_estimates_projection.generator.send_condition = (
            "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::LimiterStatusSpreadingToSecondNeighbours"
#            " or repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Prediction"
        )
        self._fv_estimates_projection.generator.receive_and_merge_condition = (
            "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::FVRecomputation"
#            " or repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Correction"
        )

        self._fv_estimates_projection.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""

        #This will contain the projection of the face-local minima and maxima
        self._min_and_max_projection = peano4.datamodel.Patch(
            (2, 2, 1),
            self._number_of_dmp_observables,
            self._unknown_identifier() + "_min_and_max",
            self._corrector_computation_precision,
        )
        if self._hold_face_data_on_heap_with_smart_pointer:
          self._min_and_max_projection.generator  = peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
              self._min_and_max_projection,
              self._corrector_computation_precision
        )
          
        self._min_and_max_projection.generator.merge_method_definition = get_face_overlap_merge_implementation(self._min_and_max_projection)

        self._min_and_max_projection.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_face_data_default_guard(),
            self._store_face_data_default_guard()
        )

        self._min_and_max_projection.generator.send_condition = (
            "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Prediction"
        )
        self._min_and_max_projection.generator.receive_and_merge_condition = (
            "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Correction"
        )

        self._min_and_max_projection.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""

        self.initialisation_sweep_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::GridInitialisation"
            + ")"
        )
        self.first_iteration_after_initialisation_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::TimeStepAfterGridInitialisation or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PlottingAfterGridInitialisation"
            + ")"
        )

        # face estimates from the space-time predictor should be sent in predictor step and received and used in the correction step
        self._rhs_estimates_projection.generator.send_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Prediction or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells"
        )
        self._rhs_estimates_projection.generator.receive_and_merge_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name + "::SolverState::Correction"
        )


        self._flux_estimates_projection.generator.send_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Prediction or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells"
        )
        self._flux_estimates_projection.generator.receive_and_merge_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Correction"
        )

    def create_action_sets(self):
        super(FVLimiting, self).create_action_sets()

        self._action_set_handle_fv_boundary = (
          HandleFVBoundary(
              self, super( FVLimiting, self )._store_boundary_data_default_guard() + 
              (" and repositories::{}.getSolverState()=={}::SolverState::FVRecomputation" +
              " and fineGridFace{}FaceLabel.getTroubled_Marker()>=facedata::{}FaceLabel::Troubled_Marker::DG_TO_FV").format(
            self.get_name_of_global_instance(), self._name, self._name, self._name
        )
          )
        )
        self._action_set_handle_fv_boundary.descend_invocation_order = 1

        self._action_set_prediction.guard = (
            self._store_cell_data_default_guard()
            + " and repositories::{}.getSolverState()=={}::SolverState::Prediction and not isHangingCell".format(
                self.get_name_of_global_instance(),
                self._name
            )
        )
        self._action_set_prediction_on_hanging_cells.guard = (
            self._store_cell_data_default_guard() +
            " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells and isHangingCell".format(
                self.get_name_of_global_instance(),
                self._name
            )
        )

        self._action_set_correction.guard = self._store_cell_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::Correction".format(
            self.get_name_of_global_instance(), self._name
        )
        if(self._use_kernel_generator):
            self._action_set_correction.riemann_guard = (
                self._store_cell_data_default_guard()
                + " and repositories::{}.getSolverState()=={}::SolverState::Correction".format(
                    self.get_name_of_global_instance(),self._name
                    )
                + " and not fineGridFace"
                + self._name
                + "FaceLabel.getAboveHanging()"
            )
        else:
            self._action_set_correction.riemann_guard = (
                self._store_cell_data_default_guard()
                + " and repositories::{}.getSolverState()=={}::SolverState::Correction".format(
                    self.get_name_of_global_instance(),
                    self._name
                )
            )

        self._action_set_spread_status = (
            SpreadLimiterStatus(
                self, self._store_cell_data_default_guard()
                + " and repositories::{}.getSolverState()=={}::SolverState::LimiterStatusSpreadingToNeighbours".format(
                self.get_name_of_global_instance(),self._name
                )
            )
        )
        self._action_set_spread_status.descend_invocation_order = 1

        self._action_set_recompute_with_fv = (
            RecomputeSolutionWithFV(
                self,
                self._store_cell_data_default_guard()
                + " and repositories::{}.getSolverState()=={}::SolverState::LimiterStatusSpreadingToSecondNeighbours".format(
                self.get_name_of_global_instance(),self._name ),
                self._store_cell_data_default_guard()
                + (" and repositories::{}.getSolverState()=={}::SolverState::FVRecomputation"
                + " and fineGridCell{}CellLabel.getTroubled_Marker()>=celldata::{}CellLabel::Troubled_Marker::FV_TO_DG").format(
                self.get_name_of_global_instance(),self._name,self._name,self._name )
            )
          )

        self._action_set_recompute_with_fv.descend_invocation_order = 1


    def add_to_Peano4_datamodel(self, datamodel, verbose):
      super(FVLimiting, self).add_to_Peano4_datamodel(datamodel, verbose)
      datamodel.add_cell(self._fv_projection)
      datamodel.add_face(self._fv_estimates_projection)
      datamodel.add_face(self._min_and_max_projection)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
      super(FVLimiting, self).add_use_data_statements_to_Peano4_solver_step(step)
      step.use_cell(self._fv_projection)
      step.use_face(self._fv_estimates_projection)
      step.use_face(self._min_and_max_projection)

    def add_actions_to_perform_time_step(self, step):
        super(FVLimiting, self).add_actions_to_perform_time_step(step)
        step.add_action_set(self._action_set_handle_fv_boundary)
        step.add_action_set(self._action_set_spread_status)
        step.add_action_set(self._action_set_recompute_with_fv)


    def generate_kernels(self, namespace, output, dimensions):
      super(FVLimiting, self).generate_kernels(namespace, output, dimensions)

      full_qualified_namespace = ""
      for i in namespace:
          full_qualified_namespace += i + "::"
      full_qualified_namespace += self._name

      generate_limiter_kernels(
          full_qualified_namespace,
          self._unknowns,
          self._auxiliary_variables,
          self._order,
          dimensions,
          limPatchSize=-1, #TODO
          numberOfObservable=self._number_of_dmp_observables,
          ghostLayerWidth=self._halo_size,
          useGaussLobatto=(
            True if self._polynomials is Polynomials.Gauss_Lobatto else False
          ),
          useLibxsmm=self._use_libxsmm,
          useBLIS=self._use_BLIS,
          useEigen=self._use_Eigen,
          useLibxsmmJIT=self._use_libxsmm_JIT,
          architecture=self._architecture
      )

      output.makefile.add_cpp_file(
        "generated/kernels/limiter/fv/Quadrature.cpp",
        generated=True,
      )

      output.makefile.add_cpp_file(
        "generated/kernels/limiter/fv/limiter.cpp",
        generated=True,
      )

      if self._use_libxsmm:
        output.makefile.add_cpp_file(
          "generated/kernels/limiter/fv/gemmsCPP.cpp",
          generated=True,
        )

    def get_user_action_set_includes(self):
        return (
            super(FVLimiting, self).get_user_action_set_includes()
            + """
"""
        )

    def _store_boundary_data_default_guard(self):
        return super(
            FVLimiting, self
        )._store_boundary_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::Correction".format(
            self.get_name_of_global_instance(), self._name
        )

    def _clear_face_data_default_guard(self):
        return super(
            FVLimiting, self
        )._clear_face_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells".format(
            self.get_name_of_global_instance(), self._name
        ) + " and fineGridFace" + self._name + "FaceLabel.getAboveHanging()"


    def _restrict_face_data_default_guard(self):    
        return super(
            FVLimiting, self
        )._restrict_face_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells".format(
            self.get_name_of_global_instance(), self._name
        )

    def _interpolate_face_data_default_guard(self):
        return super(
            FVLimiting,
            self
        )._interpolate_face_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells".format(
            self.get_name_of_global_instance(),
            self._name
        )


    def set_preprocess_reconstructed_patch_kernel(self, kernel):
        """

        Most subclasses will redefine/overwrite this operation as they have
        to incorporate the kernel into their generated stuff

        """
        self._preprocess_reconstructed_patch += kernel
        self.create_data_structures()
        self.create_action_sets()

    def set_postprocess_updated_patch_kernel(self, kernel):
        self._postprocess_updated_patch += kernel
        self.create_data_structures()
        self.create_action_sets()
