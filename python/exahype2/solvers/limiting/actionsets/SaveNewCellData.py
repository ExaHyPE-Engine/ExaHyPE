# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractLimiterActionSet import AbstractLimiterActionSet

import jinja2

import peano4.solversteps


class SaveNewCellData(AbstractLimiterActionSet):
    TemplateSaveNewCellData = """
  if ({{PREDICATE}}) {
    {% if USE_DMP %}
    // Resetting local min and max
    constexpr int NumberOfObservables = repositories::{{SOLVER_INSTANCE}}.NumberOfDMPObservables;
    {{COMPUTE_PRECISION}} localMinPerVariables[NumberOfObservables], localMaxPerVariables[NumberOfObservables];

    kernels::{{SOLVER_NAME}}::findCellLocalMinAndMax(
      repositories::{{SOLVER_INSTANCE}},
      fineGridCell{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}_old.data(),
      localMinPerVariables, localMaxPerVariables
    );

    // Projection of cell local min and max, order in any given face is [min_l, max_l, min_r, max_r]
    for (int d = 0; d < DIMENSIONS; d++) {
      // Left face
      std::copy_n(localMinPerVariables, NumberOfObservables, fineGridFaces{{SOLVER_NAME}}QMinAndMax(d).data() + 2 * NumberOfObservables);
      std::copy_n(localMaxPerVariables, NumberOfObservables, fineGridFaces{{SOLVER_NAME}}QMinAndMax(d).data() + 3 * NumberOfObservables);

      // Right face
      std::copy_n(localMinPerVariables, NumberOfObservables, fineGridFaces{{SOLVER_NAME}}QMinAndMax(d + DIMENSIONS).data());
      std::copy_n(localMaxPerVariables, NumberOfObservables, fineGridFaces{{SOLVER_NAME}}QMinAndMax(d + DIMENSIONS).data() + NumberOfObservables);
    }
    {% endif %}{# USE_DMP #}

    {% if RESET_TROUBLED_MARKERS %}
    // Mark down whether the cell was troubled in the previous timestep
    fineGridCell{{SOLVER_NAME}}CellLabel.setWasTroubledInPreviousStep(
      fineGridCell{{SOLVER_NAME}}CellLabel.getTroubled_Marker() >= celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::LIMITER_TO_REGULAR
    );

    // Resetting troubled markers
    fineGridCell{{SOLVER_NAME}}CellLabel.setTroubled_Marker(celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::REGULAR);
    for (int d = 0; d < 2 * DIMENSIONS; d++) {
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::REGULAR);
    }
    {% endif %}{# RESET_TROUBLED_MARKERS #}

    {% if TRANSMIT_CELL_TIMESTAMPS %}
    // Set time stamp for both cells to be the max of the two possible time stamps
    const double timeStamp = std::max(fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.getTimeStamp(), fineGridCell{{REGULAR_SOLVER_NAME}}CellLabel.getTimeStamp());
    fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp);
    fineGridCell{{REGULAR_SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp);
    {% endif %}{# TRANSMIT_CELL_TIMESTAMPS #}
  }
"""

    def __init__(
        self,
        solver,
        guard,
        use_DMP=False,
        reset_troubled_markers=False,
        transmit_cell_timestamps=False,
    ):
        super(SaveNewCellData, self).__init__(solver)
        self.guard = guard
        self._use_DMP = use_DMP
        self._reset_troubled_markers = reset_troubled_markers
        self._transmit_cell_timestamps = transmit_cell_timestamps

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATE"] = self.guard
            d["USE_DMP"] = self._use_DMP
            d["RESET_TROUBLED_MARKERS"] = self._reset_troubled_markers
            d["TRANSMIT_CELL_TIMESTAMPS"] = self._transmit_cell_timestamps
            result = jinja2.Template(self.TemplateSaveNewCellData).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(SaveNewCellData, self).get_includes()
            + """
#include "kernels/"""
            + self._solver._name
            + """/Limiter.h"
    """
        )
