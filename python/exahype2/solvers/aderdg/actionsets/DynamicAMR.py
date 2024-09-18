# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps


class DynamicAMR(AbstractADERDGActionSet):
    """
    ExaHyPE 2 ADER-DG handling of adaptive meshes.
    Despite the name this currently only handles static AMR,
    so this probably desperately needs to be renamed.
    """

    def __init__(
        self,
        solver,
        delete_guard="false",
        interpolate_guard="false",
        restrict_guard="false",
    ):
        super(DynamicAMR, self).__init__(solver)
        self._delete_guard = delete_guard
        self._interpolate_guard = interpolate_guard
        self._restrict_guard = restrict_guard

        self._Template_CreateHangingFace = """
  if ({{INTERPOLATE_GUARD}}) {
    const double timeStamp = fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(marker.getSelectedFaceNumber() < Dimensions ? 1 : 0);

    // Needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}

    #if Dimensions == 2
    constexpr int SpaceFaceSize      = {{ORDER}} + 1;
    #else
    constexpr int SpaceFaceSize      = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    #endif

    int facePosition = marker.getSelectedFaceNumber() < Dimensions ? 0 : SpaceFaceSize;

    if (!marker.isInteriorFaceWithinPatch()) {
      // Interpolate both estimates and fluxes
      kernels::{{SOLVER_NAME}}::faceUnknownsProlongation(
        fineGridFace{{SOLVER_NAME}}QEstimates.value + facePosition * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}),
        fineGridFace{{SOLVER_NAME}}QFluxEstimates.value + facePosition * {{NUMBER_OF_UNKNOWNS}},
        coarseGridFaces{{SOLVER_NAME}}QEstimates(marker.getSelectedFaceNumber()).value + facePosition * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}),
        coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).value + facePosition * {{NUMBER_OF_UNKNOWNS}},
        marker.getRelativePositionWithinFatherFace()
      );
    }

    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(0, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(0), fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(1, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(1), fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(0, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(0), fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(0)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(1, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(1), fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(1)));
  }
"""

        self._Template_DestroyHangingFace = """
  #if Dimensions == 2
  constexpr int SpaceFaceSize      = {{ORDER}} + 1;
  #else
  constexpr int SpaceFaceSize      = ({{ORDER}} + 1) * ({{ORDER}} + 1);
  #endif

  constexpr int FluxElementsPerFace   = SpaceFaceSize * {{NUMBER_OF_UNKNOWNS}};

  if ({{DELETE_GUARD}}) {

    int facePosition = marker.getSelectedFaceNumber() < Dimensions ? 0 : 1;
    auto* FOut = coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).value + (1-facePosition) * FluxElementsPerFace;

    //Delete the elements on the side of the face that the Riemann fluxes will be merged onto. Because of the
    //  merge operations this must be on the side on which the fine cells are.
    std::fill_n(FOut, FluxElementsPerFace, 0.0);
  }

  if ({{RESTRICT_GUARD}}) {

    int facePosition = marker.getSelectedFaceNumber() < Dimensions ? 0 : 1;

    // Fluxes
    kernels::{{SOLVER_NAME}}::faceFluxRestriction(
      coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).value + (1-facePosition) * FluxElementsPerFace, // const double* lQhbndCoarse
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.value + facePosition * FluxElementsPerFace, // double* lQhbndFine
      marker.getRelativePositionWithinFatherFace() // const tarch::la::Vector<Dimensions - 1, int>& subfaceIndex
    );
  }
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_body_of_operation(self, operation_name):
        result = "\n"
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)

        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_HANGING_FACE:
            d["INTERPOLATE_GUARD"] = self._interpolate_guard
            result = jinja2.Template(self._Template_CreateHangingFace).render(**d)
            pass
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_DESTROY_HANGING_FACE
        ):
            d["DELETE_GUARD"] = self._delete_guard
            d["RESTRICT_GUARD"] = self._restrict_guard
            result = jinja2.Template(self._Template_DestroyHangingFace).render(**d)
            pass
        return result

    def get_includes(self):
        return (
            super(DynamicAMR, self).get_includes()
            + """
#include "kernels/"""
            + self._solver._name
            + """/AMRRoutines.h"
"""
        )
