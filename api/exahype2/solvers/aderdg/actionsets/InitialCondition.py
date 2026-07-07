# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps


class InitialCondition(AbstractADERDGActionSet):
    _Template_InitialCondition = """
  if ({{PREDICATE}}) {
    int linearisedIndex = 0;

{% if INITIALISE_PATCHES is sameas True %}
    repositories::{{SOLVER_INSTANCE}}.initialCondition(
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      marker.getX(),
      marker.getH()
    );
{% else %}
    peano4::dForEach({{ORDER}} + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
      repositories::{{SOLVER_INSTANCE}}.initialCondition(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.data() + linearisedIndex,
        ::exahype2::dg::getQuadraturePoint(
          marker.getX(),
          marker.getH(),
          index,
          repositories::{{SOLVER_INSTANCE}}.Order + 1,
          kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes
        ),
        marker.getH()
      );
      linearisedIndex += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    });
{% endif %}

#if DIMENSIONS==2
    constexpr int SpaceBasisSize     = {{ORDER+1}} * {{ORDER+1}} * {{NUMBER_OF_UNKNOWNS+NUMBER_OF_AUXILIARY_VARIABLES}};
#else
    constexpr int SpaceBasisSize     = {{ORDER+1}} * {{ORDER+1}} * {{ORDER+1}} * {{NUMBER_OF_UNKNOWNS+NUMBER_OF_AUXILIARY_VARIABLES}};
#endif

    std::copy_n(fineGridCell{{UNKNOWN_IDENTIFIER}}.data(), SpaceBasisSize, fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data());

    // Relevant for tracing et al which kicks in if and only if cell has been updated
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
  }
"""

    def __init__(self, solver, guard):
        super(InitialCondition, self).__init__(solver)
        self._guard = guard
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
            d["INITIALISE_PATCHES"] = self._initialise_patches
            result = jinja2.Template(self._Template_InitialCondition).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = super(InitialCondition, self).get_includes()
        return result
