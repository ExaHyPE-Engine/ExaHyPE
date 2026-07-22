# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet


import peano4
import jinja2


class EmptyPostprocessSolution(AbstractRungeKuttaDGActionSet):
    """
    Postprocessing variant that emits no code.

    PostprocessSolution differs from other action sets, as we create it only once.
    See RungeKuttaDG.create_action_sets(). Once the instance does exist, you can
    tailor it. But you can also completely overwrite it.
    """

    def __init__(self, solver):
        super(EmptyPostprocessSolution, self).__init__(solver)
        # Exists so that this variant offers the same interface as the other two.
        self.guard = "true"

    def get_body_of_operation(self, operation_name):
        result = ""
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")


class DoFWisePostprocessSolution(AbstractRungeKuttaDGActionSet):
    """
    Postprocessing variant that applies a snippet to every degree of freedom.

    PostprocessSolution differs from other action sets, as we create it only once.
    See RungeKuttaDG.create_action_sets(). Once the instance does exist, you can
    tailor it. But you can also completely overwrite it.

    The postprocessing plugs into touch cell last time. It is the last action set added
    by the default implementation. Therefore, it is the first action set of which
    touchCellLastTime() is called. It plugs into the leaving of the cell because some
    solvers may add further action sets to the solve. These action sets typically plug
    into touchCellFirstTime() - and consequently their first time is called after the
    postprocessing's first time. Using touchCellLastTime() ensures that any additional
    action set added by a subclass can still precede the postprocessing.
    """

    def __init__(self, solver):
        super(DoFWisePostprocessSolution, self).__init__(solver)
        self.guard = "true"
        self._compute_kernel = ""
        self.descend_invocation_order = 1

    def add_postprocessing_kernel(self, operation_per_point):
        """
        Add a code snippet which is applied to each and every degree of freedom.

        The snippet can rely on the following variables:

        - values: double* pointing to the data of the current degree of freedom
        - x: The quadrature point position as tarch::la::Vector over doubles
        - marker.getH(): A tarch::la::Vector over doubles
        - timeStamp, timeStepSize: double

        :: Arguments

        operation_per_point: String
          C/C++ code snippet.
        """
        self._compute_kernel += (
            """
{
  {{COMPUTE_TIME_STEP_SIZE}}
  const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

  if (
    not marker.hasBeenRefined()
    and
    {{PREDICATE}}
  ) {
    constexpr int valuesPerDoF = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    peano4::dForEach({{DG_ORDER}} + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
      double* values   = fineGridCell{{UNKNOWN_IDENTIFIER}}.data()
        + peano4::dLinearised(index, {{DG_ORDER}}+1) * valuesPerDoF;
      auto    x        = ::exahype2::dg::getQuadraturePoint(
        marker.getX(), marker.getH(), index, repositories::{{SOLVER_INSTANCE}}.DGOrder, repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d
      );
      """
            + operation_per_point
            + """
    });
  }
}
"""
        )

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATE"] = jinja2.Template(
                self.guard, undefined=jinja2.DebugUndefined
            ).render(**d)
            result = jinja2.Template(
                self._compute_kernel, undefined=jinja2.DebugUndefined
            ).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(DoFWisePostprocessSolution, self).get_includes()
            + """
#include "peano4/Loop.h"
"""
        )


class CellWisePostprocessSolution(AbstractRungeKuttaDGActionSet):
    """
    Postprocessing variant that applies a snippet once per cell.

    PostprocessSolution differs from other action sets, as we create it only once.
    See RungeKuttaDG.create_action_sets(). Once the instance does exist, you can
    tailor it. But you can also completely overwrite it.
    """

    def __init__(self, solver):
        super(CellWisePostprocessSolution, self).__init__(solver)
        self.guard = "true"
        self._compute_kernel = ""

    def add_postprocessing_kernel(self, operation_per_cell):
        """
        Add a code snippet which is applied once per cell.

        The snippet can rely on the following variables:

        - marker.getH(): A tarch::la::Vector over doubles
        - timeStamp, timeStepSize: double

        The per-degree-of-freedom pointers are not set up here. Use
        DoFWisePostprocessSolution if the snippet has to see individual nodes.

        :: Arguments

        operation_per_cell: String
          C/C++ code snippet.
        """
        self._compute_kernel += (
            """
{
  {{COMPUTE_TIME_STEP_SIZE}}
  const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

  if (
    not marker.hasBeenRefined()
    and
    {{PREDICATE}}
  ) {
    """
            + operation_per_cell
            + """

  }
}
"""
        )

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATE"] = jinja2.Template(
                self.guard, undefined=jinja2.DebugUndefined
            ).render(**d)
            result = jinja2.Template(
                self._compute_kernel, undefined=jinja2.DebugUndefined
            ).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
