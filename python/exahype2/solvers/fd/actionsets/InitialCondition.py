# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

import peano4.solversteps

from .AbstractFDActionSet import AbstractFDActionSet


class InitialCondition(AbstractFDActionSet):
    def __init__(
        self,
        solver,
        guard,
        grid_is_constructed,
    ):
        AbstractFDActionSet.__init__(self, solver)
        self.guard = guard
        self.grid_is_constructed = grid_is_constructed

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
            d["GRID_IS_CONSTRUCTED"] = self.grid_is_constructed
            result = jinja2.Template(self.TemplateInitialCondition).render(**d)

        return result

    def get_body_of_prepareTraversal(self):
        result = "\n"
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            """
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>
"""
            + self._solver._get_default_includes()
            + self._solver.user_action_set_includes
        )

    def get_static_initialisations(self, full_qualified_classname):
            return """\n"""

    def get_attributes(self):
            return """\n"""

    TemplateInitialCondition = """
  if ({{PREDICATE}}) {
    dfor(volume, {{NUMBER_OF_NODES_PER_AXIS}}) {
      int index = peano4::utils::dLinearised(volume,{{NUMBER_OF_NODES_PER_AXIS}}) * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});
      {% if STATELESS_INITIAL_CONDITIONS==True %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::initialCondition(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.data() + index,
        ::exahype2::fv::getVolumeCentre(marker.getX(), marker.getH(), {{NUMBER_OF_NODES_PER_AXIS}}, volume),
        ::exahype2::fv::getVolumeSize(marker.getH(), {{NUMBER_OF_NODES_PER_AXIS}}),
        {{GRID_IS_CONSTRUCTED}}
      );
      {% else %}
      repositories::{{SOLVER_INSTANCE}}.initialCondition(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.data() + index,
        ::exahype2::fv::getVolumeCentre(marker.getX(), marker.getH(), {{NUMBER_OF_NODES_PER_AXIS}}, volume),
        ::exahype2::fv::getVolumeSize(marker.getH(), {{NUMBER_OF_NODES_PER_AXIS}}),
        {{GRID_IS_CONSTRUCTED}}
      );
      {% endif %}
    }

    // Relevant for tracing et al which kicks in if and only if cell has been updated
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
  }
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(InitialCondition, self).get_includes()
            + """
#include "exahype2/fv/PatchUtils.h"
"""
        )
