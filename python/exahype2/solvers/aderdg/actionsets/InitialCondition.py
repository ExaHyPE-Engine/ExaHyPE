# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps


class InitialCondition(AbstractADERDGActionSet):
    _Template_InitialCondition = """
  if ({{PREDICATE}}) {
    logTraceIn("touchCellFirstTime(...)---InitialCondition");
    int linearisedIndex = 0;

    {% if INITIALISE_PATCHES is sameas True %}
    repositories::{{SOLVER_INSTANCE}}.initialCondition(
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
      marker.x(),
      marker.h(),
      0,
      {{GRID_IS_CONSTRUCTED}}
    );
    {% else %}
    dfor(index, {{ORDER}} + 1) {
      repositories::{{SOLVER_INSTANCE}}.initialCondition(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value + linearisedIndex,
        ::exahype2::dg::getQuadraturePoint(
          marker.x(),
          marker.h(),
          index,
          repositories::{{SOLVER_INSTANCE}}.Order + 1,
          kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes
        ),
        marker.h(),
        index,
        {{GRID_IS_CONSTRUCTED}}
      );
      linearisedIndex += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
    {% endif %}

    #if Dimensions==2
    constexpr int SpaceBasisSize     = {{ORDER+1}} * {{ORDER+1}} * {{NUMBER_OF_UNKNOWNS+NUMBER_OF_AUXILIARY_VARIABLES}};
    #else
    constexpr int SpaceBasisSize     = {{ORDER+1}} * {{ORDER+1}} * {{ORDER+1}} * {{NUMBER_OF_UNKNOWNS+NUMBER_OF_AUXILIARY_VARIABLES}};
    #endif

    std::copy_n(fineGridCell{{UNKNOWN_IDENTIFIER}}.value, SpaceBasisSize, fineGridCell{{UNKNOWN_IDENTIFIER}}_old.value);

    {% if USE_POINT_SOURCE %}
    repositories::{{SOLVER_INSTANCE}}.initPointSourceLocations(repositories::{{SOLVER_INSTANCE}}.pointSourceLocation);
    {% endif %}

    const double timeStamp = 0.0;
    const double timeStepSize = 0.0;
    {{SOLUTION_STORAGE_PRECISION}}* luh = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
    {{COMPUTE_NEW_TIME_STEP_SIZE}}

    // Relevant for tracing et al which kicks in if and only if cell has been updated
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);

    logTraceOut("touchCellFirstTime(...)---InitialCondition");
  }
"""

    def __init__(self, solver, guard, grid_is_constructed):
        super(InitialCondition, self).__init__(solver)
        self._guard = guard
        self._grid_is_constructed = grid_is_constructed
        self._initialise_patches = solver._initialise_patches

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATE"] = self._guard
            d["GRID_IS_CONSTRUCTED"] = self._grid_is_constructed
            d["INITIALISE_PATCHES"] = self._initialise_patches
            result = jinja2.Template(self._Template_InitialCondition).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(InitialCondition, self).get_includes()
            + """
#include "kernels/"""
            + self._solver._name
            + """/Quadrature.h"
#include "kernels/"""
            + self._solver._name
            + """/MaxScaledEigenvalue.h"
"""
        )
