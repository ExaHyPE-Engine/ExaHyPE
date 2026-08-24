# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractFVActionSet import AbstractFVActionSet

import peano4.solversteps

import jinja2


class HandleBoundary(AbstractFVActionSet):
    """
    Periodic axes are read from the generated Constants.h.

    Boundary conditions fill the outer halo of the previous snapshot, which is the one
    holding the solution while this action set runs. The current snapshot is the empty
    destination that the sweep writes later.
    """

    TemplateHandleBoundary_Prologue = """
  if ({{PREDICATE}} and not marker.hasBeenRefined()) {
"""

    TemplateHandleBoundary_KernelCalls = """
    for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
      if (
        not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[faceNumber % DIMENSIONS]
        and fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getBoundary()
      ) {
        const auto insideEntry  = faceNumber < DIMENSIONS ? 1 : 0;
        const auto outsideEntry = 1 - insideEntry;
        ::exahype2::fv::handleBoundary<
          {{FULL_QUALIFIED_SOLVER_NAME}},
          {{NUMBER_OF_VOLUMES_PER_AXIS}},
          {{HALO_SIZE}},
          {{NUMBER_OF_UNKNOWNS}},
          {{NUMBER_OF_AUXILIARY_VARIABLES}},
          {{STORAGE_PRECISION}}>(
          repositories::{{SOLVER_INSTANCE}},
          marker.getX(),
          marker.getH(),
          {{FACE_METADATA_ACCESSOR}}(faceNumber).getOldTimeStamp(insideEntry),
          faceNumber,
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Old(faceNumber).data()
        );
"""

    TemplateHandleBoundary_Epilogue = """
        const auto updatedTimeStamp =
          {{FACE_METADATA_ACCESSOR}}(faceNumber).getUpdatedTimeStamp(insideEntry);
        {{FACE_METADATA_ACCESSOR}}(faceNumber).setUpdatedTimeStamp(outsideEntry, updatedTimeStamp);

        const auto newTimeStamp =
          {{FACE_METADATA_ACCESSOR}}(faceNumber).getNewTimeStamp(insideEntry);
        {{FACE_METADATA_ACCESSOR}}(faceNumber).setNewTimeStamp(outsideEntry, newTimeStamp);

        const auto oldTimeStamp =
          {{FACE_METADATA_ACCESSOR}}(faceNumber).getOldTimeStamp(insideEntry);
        {{FACE_METADATA_ACCESSOR}}(faceNumber).setOldTimeStamp(outsideEntry, oldTimeStamp);
      }
    }
  }
"""

    def __init__(self, solver, guard):
        super().__init__(solver)
        self.guard = guard

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            dictionary = {}
            self._solver._init_dictionary_with_default_parameters(dictionary)
            self._solver.add_entries_to_text_replacement_dictionary(dictionary)
            dictionary["PREDICATE"] = self.guard
            dictionary["FACE_METADATA_ACCESSOR"] = (
                "fineGridFaces" + self._solver._face_label.name
            )
            result = jinja2.Template(self.TemplateHandleBoundary_Prologue).render(
                **dictionary
            )
            result += jinja2.Template(self.TemplateHandleBoundary_KernelCalls).render(
                **dictionary
            )
            result += jinja2.Template(self.TemplateHandleBoundary_Epilogue).render(
                **dictionary
            )
        return result

    def get_includes(self):
        return (
            """
#include "exahype2/fv/HandleBoundary.h"
"""
            + super().get_includes()
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
