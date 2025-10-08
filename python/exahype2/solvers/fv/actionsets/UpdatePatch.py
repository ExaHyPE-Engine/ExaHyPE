# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractFVActionSet import AbstractFVActionSet

import peano4.solversteps

import jinja2


class UpdatePatch(AbstractFVActionSet):
    _Template_TouchCellFirstTime = jinja2.Template(
        """
  if ({{GUARD}}) {
    auto* /*const*/ QIn = {{OLD_ACCESSOR}}.value;
    auto* /*const*/ QOut = {{NEW_ACCESSOR}}.value;

    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(false);
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    {{COMPUTE_TIME_STEP_SIZE}}
    assertion2(tarch::la::greaterEquals(timeStamp,    0.0), timeStamp, timeStepSize);
    assertion2(tarch::la::greaterEquals(timeStepSize, 0.0), timeStamp, timeStepSize);

    if ({{ENCLAVE_GUARD}}) { // is skeleton cell
#if PEANO_DEBUG > 0
      ::exahype2::fv::validatePatch(
        QIn,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{HALO_SIZE}}, // Halo size
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
      ); // Previous time step has to be valid
#endif

      {{PREPROCESS_RECONSTRUCTED_PATCH}}
      {% if COMPUTE_MAX_EIGENVALUE %}
      const auto maxEigenvalue = {{COMPUTE_KERNEL_CALL}}
      {% else %}
      const auto _ = {{COMPUTE_KERNEL_CALL}}
      {% endif %}
      {{POSTPROCESS_UPDATED_PATCH}}

#if PEANO_DEBUG > 0
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
      fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
      repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp + timeStepSize, tarch::la::min(marker.h()), tarch::la::max(marker.h()));
    }
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp + timeStepSize);
  } else if (not marker.willBeRefined() and not marker.hasBeenRefined()) { // Required in cases where the if-guard above is extended by a solver
    const double timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    const double timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();

    assertion2(tarch::la::greaterEquals(timeStepSize, 0.0), timeStepSize, timeStamp);
    assertion2(tarch::la::greaterEquals(timeStamp,    0.0), timeStepSize, timeStamp);

    repositories::{{SOLVER_INSTANCE}}.update(0.0, timeStamp, tarch::la::min(marker.h()), tarch::la::max(marker.h()));
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
        self.old_accessor = "fineGridCell" + solver._patch_reconstructed.name
        self.new_accessor = "fineGridCell" + solver._patch.name

    def _add_action_set_entries_to_dictionary(self, d):
        d["GUARD"] = self.guard
        d["ENCLAVE_GUARD"] = (
            "marker.willBeSkeletonCell()"
            if self._solver._use_enclave_tasking
            else "true"
        )
        d["OLD_ACCESSOR"] = self.old_accessor
        d["NEW_ACCESSOR"] = self.new_accessor

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
