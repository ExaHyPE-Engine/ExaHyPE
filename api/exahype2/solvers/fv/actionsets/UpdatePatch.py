# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractFVActionSet import AbstractFVActionSet

import peano4.solversteps

import jinja2


class UpdatePatch(AbstractFVActionSet):
    _Template_TouchCellFirstTime = jinja2.Template(
        """
  if ({{GUARD}}) {
    auto* const QIn  = {{OLD_ACCESSOR}}.data();
    auto* const QOut = {{NEW_ACCESSOR}}.data();

    const auto timeStamp = {{CELL_METADATA_ACCESSOR}}.getTimeStamp();

    {{COMPUTE_TIME_STEP_SIZE}}
    assertion(tarch::la::greaterEquals(timeStamp,    0.0), timeStamp, timeStepSize);
    assertion(tarch::la::greaterEquals(timeStepSize, 0.0), timeStamp, timeStepSize);

#if PEANO_DEBUG
    ::exahype2::fv::validatePatch(
      QIn,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      0,
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
    );
#endif

    {{PREPROCESS_RECONSTRUCTED_PATCH}}
    const {{STORAGE_PRECISION}}* faceQIn[TWO_TIMES_D];
    {{STORAGE_PRECISION}}* faceQOut[TWO_TIMES_D];
    for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
      faceQIn[faceNumber]  = {{OLD_FACE_ACCESSOR}}(faceNumber).data();
      faceQOut[faceNumber] = {{NEW_FACE_ACCESSOR}}(faceNumber).data();
      // A projection, a restriction from finer cells, the boundary treatment or
      // the periodic exchange fills each half of the halo and takes that half's
      // transfer flag down.
      const auto&                 faceLabel    = {{FACE_METADATA_ACCESSOR}}(faceNumber);
      [[maybe_unused]] const int  ownSide      = faceNumber < DIMENSIONS ? 1 : 0;
      [[maybe_unused]] const bool isDomainWall = faceLabel.getBoundary()
        and not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[faceNumber % DIMENSIONS];
      assertion(
        faceLabel.getIsHanging() or not faceLabel.getRequiresDataTransfer(ownSide),
        marker.toString(),
        faceLabel.toString(),
        faceNumber,
        timeStamp
      );
      assertion(
        faceLabel.getIsHanging() or isDomainWall or faceLabel.getAboveHanging(1 - ownSide)
          or not faceLabel.getRequiresDataTransfer(1 - ownSide),
        marker.toString(),
        faceLabel.toString(),
        faceNumber,
        timeStamp
      );
    }

    {% if COMPUTE_MAX_EIGENVALUE %}
    const auto maxEigenvalue = {{COMPUTE_KERNEL_CALL}}
    {% else %}
    [[maybe_unused]] const auto maxEigenvalue = {{COMPUTE_KERNEL_CALL}}
    {% endif %}

    {{POSTPROCESS_UPDATED_PATCH}}

#if PEANO_DEBUG
    ::exahype2::fv::validatePatch(
      QOut,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      0,
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
    );
#endif

    {{COMPUTE_NEW_TIME_STEP_SIZE}}
    {{CELL_METADATA_ACCESSOR}}.setHasUpdated(true);
    {{CELL_METADATA_ACCESSOR}}.setTimeStepSize(newTimeStepSize);
    repositories::{{SOLVER_INSTANCE}}
      .update(
        timeStepSize,
        timeStamp + timeStepSize,
        tarch::la::min(marker.getH()),
        tarch::la::max(marker.getH())
      );
    {{CELL_METADATA_ACCESSOR}}.setTimeStamp(timeStamp + timeStepSize);
    const auto updatedTimeStamp = {{CELL_METADATA_ACCESSOR}}.getTimeStamp();
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      {{FACE_METADATA_ACCESSOR}}(dimension).setUpdated(1, true);
      {{FACE_METADATA_ACCESSOR}}(dimension).setUpdatedTimeStamp(1, updatedTimeStamp);
      {{FACE_METADATA_ACCESSOR}}(dimension + DIMENSIONS).setUpdated(0, true);
      {{FACE_METADATA_ACCESSOR}}(dimension + DIMENSIONS).setUpdatedTimeStamp(0, updatedTimeStamp);
    }
  } else if (not marker.willBeRefined() and not marker.hasBeenRefined()) {
    // Leaves excluded by solver-specific guards still contribute global mesh statistics.
    const auto                  timeStamp    = {{CELL_METADATA_ACCESSOR}}.getTimeStamp();
    [[maybe_unused]] const auto timeStepSize = {{CELL_METADATA_ACCESSOR}}.getTimeStepSize();

    assertion(tarch::la::greaterEquals(timeStepSize, 0.0), timeStepSize, timeStamp);
    assertion(tarch::la::greaterEquals(timeStamp, 0.0), timeStepSize, timeStamp);

    repositories::{{SOLVER_INSTANCE}}
      .updateTimeStampAndPatchSize(
        timeStamp,
        tarch::la::min(marker.getH()),
        tarch::la::max(marker.getH())
      );
  }
"""
    )

    def __init__(self, solver):
        super().__init__(solver)
        self.guard = (
            "not marker.willBeRefined() and not marker.hasBeenRefined()"
            + " and "
            + f"repositories::{solver.get_name_of_global_instance()}.getSolverState() == Abstract{solver.name}::SolverState::TimeStep"
        )
        self.old_accessor = "fineGridCell" + solver.previous_unknown_identifier()
        self.new_accessor = "fineGridCell" + solver.current_unknown_identifier()
        self.old_face_accessor = "fineGridFaces" + solver.previous_unknown_identifier()
        self.new_face_accessor = "fineGridFaces" + solver.current_unknown_identifier()
        self._face_metadata_accessor = "fineGridFaces" + solver._face_label.name
        self._cell_metadata_accessor = "fineGridCell" + solver._cell_label.name

    def _add_action_set_entries_to_dictionary(self, dictionary):
        dictionary["GUARD"] = self.guard
        dictionary["OLD_ACCESSOR"] = self.old_accessor
        dictionary["NEW_ACCESSOR"] = self.new_accessor
        dictionary["OLD_FACE_ACCESSOR"] = self.old_face_accessor
        dictionary["NEW_FACE_ACCESSOR"] = self.new_face_accessor
        dictionary["FACE_METADATA_ACCESSOR"] = self._face_metadata_accessor
        dictionary["CELL_METADATA_ACCESSOR"] = self._cell_metadata_accessor

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            dictionary = {}
            self._solver._init_dictionary_with_default_parameters(dictionary)
            self._solver.add_entries_to_text_replacement_dictionary(dictionary)
            self._add_action_set_entries_to_dictionary(dictionary)
            result = self._Template_TouchCellFirstTime.render(**dictionary)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
            result = (
                "  repositories::"
                + self._solver.get_name_of_global_instance()
                + ".reduceLocalUpdates();\n"
            )
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = super().get_includes()
        result += """
#include "exahype2/fv/PatchUtils.h"
"""
        return result
