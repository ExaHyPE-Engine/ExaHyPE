# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractFVActionSet import AbstractFVActionSet

import peano4.solversteps

import jinja2

class UpdatePatch(AbstractFVActionSet):
    _Template_TouchCellFirstTime = jinja2.Template(
        """
  if ({{GUARD}}) {
    auto* /*const*/ QIn = {{OLD_ACCESSOR}}.data();
    auto* /*const*/ QOut = {{NEW_ACCESSOR}}.data();

    const double timeStamp = {{CELL_METADATA_ACCESSOR}}.getTimeStamp();

    {{COMPUTE_TIME_STEP_SIZE}}
    assertion(tarch::la::greaterEquals(timeStamp,    0.0), timeStamp, timeStepSize);
    assertion(tarch::la::greaterEquals(timeStepSize, 0.0), timeStamp, timeStepSize);

#if PEANO_DEBUG
    ::exahype2::fv::validatePatch(
      QIn,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      0, // Halo size
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
    ); // Previous time step has to be valid
#endif

    {{PREPROCESS_RECONSTRUCTED_PATCH}}
    const {{STORAGE_PRECISION}}* faceQIn[TWO_TIMES_D];
    {{STORAGE_PRECISION}}* faceQOut[TWO_TIMES_D];
    for (int faceDir = 0; faceDir < TWO_TIMES_D; ++faceDir) {
      faceQIn[faceDir]  = {{OLD_FACE_ACCESSOR}}(faceDir).data();
      faceQOut[faceDir] = {{NEW_FACE_ACCESSOR}}(faceDir).data();
    }
    {% if COMPUTE_MAX_EIGENVALUE %}
    const auto maxEigenvalue = {{COMPUTE_KERNEL_CALL}}
    {% else %}
    const auto _ = {{COMPUTE_KERNEL_CALL}}
    {% endif %}
    {{POSTPROCESS_UPDATED_PATCH}}

#if PEANO_DEBUG
    ::exahype2::fv::validatePatch(
      QOut,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      0, // Halo size
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
    ); // Outcome has to be valid
#endif

    {{COMPUTE_NEW_TIME_STEP_SIZE}}
    {{CELL_METADATA_ACCESSOR}}.setHasUpdated(true);
    {{CELL_METADATA_ACCESSOR}}.setTimeStepSize(newTimeStepSize);
    repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp + timeStepSize, tarch::la::min(marker.getH()), tarch::la::max(marker.getH()));
    {{CELL_METADATA_ACCESSOR}}.setTimeStamp(timeStamp + timeStepSize);
    // set updated flag for all faces (needed for rollover)
    for (int d = 0; d < DIMENSIONS; d++) {
      {{FACE_METADATA_ACCESSOR}}(d).setUpdated(1, true);
      {{FACE_METADATA_ACCESSOR}}(d).setUpdatedTimeStamp(1, {{CELL_METADATA_ACCESSOR}}.getTimeStamp());
      {{FACE_METADATA_ACCESSOR}}(d + DIMENSIONS).setUpdated(0, true);
      {{FACE_METADATA_ACCESSOR}}(d + DIMENSIONS).setUpdatedTimeStamp(0, {{CELL_METADATA_ACCESSOR}}.getTimeStamp());
    }
  } else if (not marker.willBeRefined() and not marker.hasBeenRefined()) { // Required in cases where the if-guard above is extended by a solver
    const double timeStamp    = {{CELL_METADATA_ACCESSOR}}.getTimeStamp();
    [[maybe_unused]] const double timeStepSize = {{CELL_METADATA_ACCESSOR}}.getTimeStepSize();

    assertion(tarch::la::greaterEquals(timeStepSize, 0.0), timeStepSize, timeStamp);
    assertion(tarch::la::greaterEquals(timeStamp,    0.0), timeStepSize, timeStamp);

    repositories::{{SOLVER_INSTANCE}}.update(0.0, timeStamp, tarch::la::min(marker.getH()), tarch::la::max(marker.getH()));
  }
"""
    )

    def __init__(self, solver):
        super(UpdatePatch, self).__init__(solver)
        self.guard = (
            "not marker.willBeRefined() and not marker.hasBeenRefined()"
            + " and "
            + "("
            + f"repositories::{solver.get_name_of_global_instance()}.getSolverState() == Abstract{solver.name}::SolverState::IsSolving"
            + " or "
            + f"repositories::{solver.get_name_of_global_instance()}.getSolverState() == Abstract{solver.name}::SolverState::IsSolvingAfterGridInitialisation"
            + ")"
        )
        self.old_accessor = "fineGridCell" + solver.previous_unknown_identifier()
        self.new_accessor = "fineGridCell" + solver.current_unknown_identifier()
        self.old_face_accessor = "fineGridFaces" + solver.previous_unknown_identifier()
        self.new_face_accessor = "fineGridFaces" + solver.current_unknown_identifier()
        self._face_metadata_accessor = "fineGridFaces" + solver._face_label.name
        self._cell_metadata_accessor = "fineGridCell" "" + solver._cell_label.name

    def _add_action_set_entries_to_dictionary(self, d):
        d["GUARD"] = self.guard
        d["OLD_ACCESSOR"] = self.old_accessor
        d["NEW_ACCESSOR"] = self.new_accessor
        d["OLD_FACE_ACCESSOR"] = self.old_face_accessor
        d["NEW_FACE_ACCESSOR"] = self.new_face_accessor
        d["FACE_METADATA_ACCESSOR"] = self._face_metadata_accessor
        d["CELL_METADATA_ACCESSOR"] = self._cell_metadata_accessor

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            self._add_action_set_entries_to_dictionary(d)
            result = self._Template_TouchCellFirstTime.render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = super(UpdatePatch, self).get_includes()
        result += """
#include "exahype2/fv/PatchUtils.h"
"""
        return result
