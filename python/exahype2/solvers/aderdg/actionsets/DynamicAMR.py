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

        self._Template_TouchFaceFirstTime = """
  if ({{DELETE_GUARD}}) {
    #if DIMENSIONS == 2
    constexpr int SpaceFaceSize = {{ORDER}} + 1;
    #else
    constexpr int SpaceFaceSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    #endif

    constexpr int FluxElementsPerFace = SpaceFaceSize * {{NUMBER_OF_UNKNOWNS}};

    int facePosition = fineGridFace{{SOLVER_NAME}}FaceLabel.getAboveHanging()[0] ? 0 : 1;
    auto* FOut = fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + facePosition * FluxElementsPerFace;

    // Delete the elements on the side of the face that the Riemann fluxes will be merged onto.
    // Because of the merge operations this must be on the side on which the fine cells are.
    std::fill_n(FOut, FluxElementsPerFace, 0.0);
  }
"""

        self._Template_TouchHangingFaceFirstTime = """
  if ({{INTERPOLATE_GUARD}}) {
    #if DIMENSIONS == 2
    constexpr int SpaceFaceSize = {{ORDER}} + 1;
    #else
    constexpr int SpaceFaceSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    #endif

    int facePosition = marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;

    constexpr int FluxElementsPerFace  = SpaceFaceSize * {{NUMBER_OF_UNKNOWNS}};
    constexpr int BasisElementsPerFace = SpaceFaceSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});

    const double timeStamp = std::max(
      coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(1-facePosition),
      fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(facePosition)
    );

    {{COMPUTE_TIME_STEP_SIZE}}

    if (not marker.isInteriorFaceWithinPatch()) {
      // Interpolate both estimates and fluxes
      kernels::{{SOLVER_NAME}}::faceUnknownsProlongation(
        fineGridFace{{SOLVER_NAME}}QEstimates.data() + facePosition * BasisElementsPerFace,
        fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + facePosition * FluxElementsPerFace,
        coarseGridFaces{{SOLVER_NAME}}QEstimates(marker.getSelectedFaceNumber()).data() + facePosition * BasisElementsPerFace,
        coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).data() + facePosition * FluxElementsPerFace,
        marker.getRelativePositionWithinFatherFace()
      );

      auto* FL = fineGridFace{{SOLVER_NAME}}QFluxEstimates.data();
      auto* FR = fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + FluxElementsPerFace;
      auto* QL = fineGridFace{{SOLVER_NAME}}QEstimates.data();
      auto* QR = fineGridFace{{SOLVER_NAME}}QEstimates.data() + BasisElementsPerFace;
      const int direction = marker.getSelectedFaceNumber() % DIMENSIONS;
      const int faceNumber  = marker.getSelectedFaceNumber();
      const bool isBoundary = fineGridFace{{SOLVER_NAME}}FaceLabel.getBoundary();
      const auto& faceCentre = marker.getX();
      {{RIEMANN_SOLVER_CALL}}
    }

    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(0, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(0), fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(1, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(1), fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(0, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(0), fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(0)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(1, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(1), fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(1)));
  }
"""

        self._Template_CreateFace = """
    #if DIMENSIONS == 2
    constexpr int SpaceFaceSize = {{ORDER}} + 1;
    #else
    constexpr int SpaceFaceSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    #endif

    int facePosition = marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : SpaceFaceSize;

    // Interpolate both estimates and fluxes
    kernels::{{SOLVER_NAME}}::faceUnknownsProlongation(
      fineGridFace{{SOLVER_NAME}}QEstimates.data() + facePosition * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}),
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + facePosition * {{NUMBER_OF_UNKNOWNS}},
      coarseGridFaces{{SOLVER_NAME}}QEstimates(marker.getSelectedFaceNumber()).data() + facePosition * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}),
      coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).data() + facePosition * {{NUMBER_OF_UNKNOWNS}},
      marker.getRelativePositionWithinFatherFace()
    );

    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(0, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(0), fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(1, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(1), fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(0, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(0), fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(0)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(1, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(1), fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(1)));
    """

        self._Template_TouchHangingFaceLastTime = """
  if ({{RESTRICT_GUARD}}) {
    #if DIMENSIONS == 2
    constexpr int SpaceFaceSize = {{ORDER}} + 1;
    #else
    constexpr int SpaceFaceSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    #endif

    constexpr int FluxElementsPerFace = SpaceFaceSize * {{NUMBER_OF_UNKNOWNS}};

    int facePosition = marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;

    // Fluxes
    kernels::{{SOLVER_NAME}}::faceFluxRestriction(
      coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).data() + (1-facePosition) * FluxElementsPerFace, // lFhbndCoarse
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + facePosition * FluxElementsPerFace, // lFhbndFine
      marker.getRelativePositionWithinFatherFace() // const tarch::la::Vector<DIMENSIONS - 1, int>& subfaceIndex
    );
  }
"""

        self._Template_DestroyFace = """
    #if DIMENSIONS == 2
    constexpr int SpaceFaceSize = {{ORDER}} + 1;
    #else
    constexpr int SpaceFaceSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    #endif

    constexpr int UnknownsElementsPerFace = SpaceFaceSize * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    constexpr int FluxElementsPerFace     = SpaceFaceSize * {{NUMBER_OF_UNKNOWNS}};

    // Unknowns left
    kernels::{{SOLVER_NAME}}::faceUnknownsRestriction(
      coarseGridFaces{{SOLVER_NAME}}QEstimates(marker.getSelectedFaceNumber()).data(), // lQhbndCoarse
      fineGridFace{{SOLVER_NAME}}QEstimates.data(), // lQhbndFine
      marker.getRelativePositionWithinFatherFace() // const tarch::la::Vector<DIMENSIONS - 1, int>& subfaceIndex
    );
    // Unknowns right
    kernels::{{SOLVER_NAME}}::faceUnknownsRestriction(
      coarseGridFaces{{SOLVER_NAME}}QEstimates(marker.getSelectedFaceNumber()).data() + UnknownsElementsPerFace, // lQhbndCoarse
      fineGridFace{{SOLVER_NAME}}QEstimates.data() + UnknownsElementsPerFace, // lQhbndFine
      marker.getRelativePositionWithinFatherFace() // const tarch::la::Vector<DIMENSIONS - 1, int>& subfaceIndex
    );
    // Fluxes left
    kernels::{{SOLVER_NAME}}::faceFluxRestriction(
      coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).data(), // lFhbndCoarse
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.data(), // lFhbndFine
      marker.getRelativePositionWithinFatherFace() // const tarch::la::Vector<DIMENSIONS - 1, int>& subfaceIndex
    );
    // Fluxes right
    kernels::{{SOLVER_NAME}}::faceFluxRestriction(
      coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).data() + FluxElementsPerFace, // lFhbndCoarse
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + FluxElementsPerFace, // lFhbndFine
      marker.getRelativePositionWithinFatherFace() // const tarch::la::Vector<DIMENSIONS - 1, int>& subfaceIndex
    );
"""

        self._Template_CreateCell = (
            """
  kernels::{{SOLVER_NAME}}::cellUnknownsProlongation(
    fineGridCell{{SOLVER_NAME}}Q.data(),
    coarseGridCell{{SOLVER_NAME}}Q.data(),
    marker.getRelativePositionWithinFatherCell()
  );

  fineGridCell"""
            + solver._cell_label.name
            + """.setTimeStamp( coarseGridCell"""
            + solver._cell_label.name
            + """.getTimeStamp() );
  fineGridCell"""
            + solver._cell_label.name
            + """.setTimeStepSize( coarseGridCell"""
            + solver._cell_label.name
            + """.getTimeStepSize() );
"""
        )

        self._Template_DestroyCell = (
            """
  kernels::{{SOLVER_NAME}}::cellUnknownsRestriction(
    fineGridCell{{SOLVER_NAME}}Q.data(),
    coarseGridCell{{SOLVER_NAME}}Q.data(),
    marker.getRelativePositionWithinFatherCell()
  );

  coarseGridCell"""
            + solver._cell_label.name
            + """.setTimeStamp( fineGridCell"""
            + solver._cell_label.name
            + """.getTimeStamp() );
  coarseGridCell"""
            + solver._cell_label.name
            + """.setTimeStepSize( fineGridCell"""
            + solver._cell_label.name
            + """.getTimeStepSize() );
"""
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_body_of_operation(self, operation_name):
        result = "\n"
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)

        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            d["DELETE_GUARD"] = (
                self._delete_guard
            )  # TODO: Shouldn't be named like this anymore, as we are not deleting hanging faces anymore. "Reset" might be a better name.
            result = jinja2.Template(self._Template_TouchFaceFirstTime).render(**d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME
        ):
            d["INTERPOLATE_GUARD"] = self._interpolate_guard
            result = jinja2.Template(self._Template_TouchHangingFaceFirstTime).render(
                **d
            )
            pass
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME
        ):
            d["RESTRICT_GUARD"] = self._restrict_guard
            result = jinja2.Template(self._Template_TouchHangingFaceLastTime).render(
                **d
            )
            pass
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_FACE:
            result = jinja2.Template(self._Template_CreateFace).render(**d)
            pass
        if operation_name == peano4.solversteps.ActionSet.OPERATION_DESTROY_FACE:
            result = jinja2.Template(self._Template_DestroyFace).render(**d)
            pass
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_CELL:
            result = jinja2.Template(self._Template_CreateCell).render(**d)
            pass
        if operation_name == peano4.solversteps.ActionSet.OPERATION_DESTROY_CELL:
            result = jinja2.Template(self._Template_DestroyCell).render(**d)
            pass
        return result

    def get_includes(self):
        result = super(DynamicAMR, self).get_includes()
        return result
