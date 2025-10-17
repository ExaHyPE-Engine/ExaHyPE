# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractLimiterActionSet import AbstractLimiterActionSet

import peano4.solversteps

import jinja2


class VerifyTroubledness(AbstractLimiterActionSet):
    TemplateVerifyTroubledness = """
    if ({{PREDICATE}}) {
      const double timeStamp = fineGridCell{{REGULAR_SOLVER_NAME}}CellLabel.getTimeStamp();

      bool isUnTroubled = true;

      auto* Q = fineGridCell{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}.value;

      {% if USE_PAC %}
      isUnTroubled &= kernels::{{SOLVER_NAME}}::isPhysicallyAdmissible(
        repositories::{{SOLVER_INSTANCE}},
        Q,
        marker.x(),
        marker.h(),
        timeStamp
      );
      {% endif %}{# USE_PAC #}

      {% if USE_DMP %}
      constexpr int NumberOfObservables = repositories::{{SOLVER_INSTANCE}}.NumberOfDMPObservables;
      {{COMPUTE_PRECISION}} boundaryMinPerVariables[2 * DIMENSIONS * NumberOfObservables];
      {{COMPUTE_PRECISION}} boundaryMaxPerVariables[2 * DIMENSIONS * NumberOfObservables];

      for (int d = 0; d < DIMENSIONS; d++) {
        std::copy_n(fineGridFaces{{SOLVER_NAME}}QMinAndMax(d).value,                                        NumberOfObservables, &boundaryMinPerVariables[d * NumberOfObservables]);
        std::copy_n(fineGridFaces{{SOLVER_NAME}}QMinAndMax(d + DIMENSIONS).value + 2 * NumberOfObservables, NumberOfObservables, &boundaryMinPerVariables[(d + DIMENSIONS) * NumberOfObservables]);

        std::copy_n(fineGridFaces{{SOLVER_NAME}}QMinAndMax(d).value + NumberOfObservables,                  NumberOfObservables, &boundaryMaxPerVariables[d * NumberOfObservables]);
        std::copy_n(fineGridFaces{{SOLVER_NAME}}QMinAndMax(d + DIMENSIONS).value + 3 * NumberOfObservables, NumberOfObservables, &boundaryMaxPerVariables[(d + DIMENSIONS) * NumberOfObservables]);
      }

      isUnTroubled &= kernels::{{SOLVER_NAME}}::discreteMaximumPrincipleAndMinAndMaxSearch(
        repositories::{{SOLVER_INSTANCE}},
        Q,
        repositories::{{SOLVER_INSTANCE}}.RelaxationParameter,
        repositories::{{SOLVER_INSTANCE}}.DifferencesScaling,
        boundaryMinPerVariables,
        boundaryMaxPerVariables
      );
      {% endif %}{# USE_DMP #}

      if (!isUnTroubled) {
        repositories::{{SOLVER_INSTANCE}}.addTroubledCell();
        fineGridCell{{SOLVER_NAME}}CellLabel.setTroubled_Marker(celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::TROUBLED);
        for (int d = 0; d < 2 * DIMENSIONS; d++) {
          fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED);
        }
      }
    }
"""

    def __init__(self, solver, guard, use_PAC=False, use_DMP=False):
        super(VerifyTroubledness, self).__init__(solver)
        self.guard = guard
        self.use_PAC = use_PAC
        self.use_DMP = use_DMP

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATE"] = self.guard
            d["USE_PAC"] = self.use_PAC
            d["USE_DMP"] = self.use_DMP
            result = jinja2.Template(self.TemplateVerifyTroubledness).render(**d)
            pass
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(VerifyTroubledness, self).get_includes()
            + """
#include "kernels/"""
            + self._solver._name
            + """/Limiter.h"
    """
        )
