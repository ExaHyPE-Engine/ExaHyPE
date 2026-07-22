# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps


class EmptyPostprocessSolution(AbstractADERDGActionSet):
    """
    Base variant which emits only those kernels that have been added to it.

    PostprocessSolution differs from other action sets, as we create it only
    once. See ADERDG.create_action_sets(). Once the instance does exist, you can
    tailor it. You can also completely overwrite it.
    """

    def __init__(self, solver):
        super(EmptyPostprocessSolution, self).__init__(solver)
        self._cell_compute_kernel = ""
        self._face_compute_kernel = ""
        self.descend_invocation_order = 1

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
            and self._cell_compute_kernel != ""
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            result = jinja2.Template(
                self._cell_compute_kernel, undefined=jinja2.DebugUndefined
            ).render(**d)
            pass
        elif (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
            and self._face_compute_kernel != ""
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            result = jinja2.Template(
                self._face_compute_kernel, undefined=jinja2.DebugUndefined
            ).render(**d)
            pass
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = super(EmptyPostprocessSolution, self).get_includes()
        return result


class DoFWisePostprocessSolution(EmptyPostprocessSolution):
    """
    Run over the solution degree of freedom by degree of freedom.

    PostprocessSolution differs from other action sets, as we create it only
    once. See ADERDG.create_action_sets(). Once the instance does exist, you can
    tailor it. You can also completely overwrite it.

    The postprocessing plugs into touch cell last time. It is the last action set
    added by the default implementation, so it is the first action set of which
    touchCellLastTime() is called. We plug into the leaving of the cell because
    some solvers add further action sets to the solve. Those action sets typically
    plug into touchCellFirstTime(), and their first time is therefore called after
    the postprocessing's first time. Using touchCellLastTime() lets any action set
    added by a subclass still precede the postprocessing.
    """

    def __init__(self, solver):
        super(DoFWisePostprocessSolution, self).__init__(solver)
        self._guard = "true"

    def add_postprocessing_kernel(self, operation_per_point):
        """
        Add a code snippet which is applied to every degree of freedom.

        The snippet can use the following variables:

        - values: Pointer to the current degree of freedom's data.
        - x: Position of the quadrature point as tarch::la::Vector over doubles.
        - index: Position of the degree of freedom within the cell.
        - marker.getH(): Cell size as tarch::la::Vector over doubles.
        - timeStamp: Time stamp of the cell.

        ## Arguments

        operation_per_point: String
          C/C++ code snippet.
        """
        self._cell_compute_kernel += (
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

    peano4::dForEach({{ORDER}} + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
      double* values   = fineGridCell{{UNKNOWN_IDENTIFIER}}.data()
        + peano4::dLinearised(index, {{ORDER}} + 1) * valuesPerDoF;
      auto    x        =         ::exahype2::aderdg::getQuadraturePoint(
        marker.getX(),
        marker.getH(),
        index,
        repositories::{{SOLVER_INSTANCE}}.Order + 1,
        kernels::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes
      );
      """
            + operation_per_point
            + """
    });
  }
}
"""
        )

    def get_includes(self):
        return (
            super(DoFWisePostprocessSolution, self).get_includes()
            + """
#include "peano4/Loop.h"
"""
        )


class CellWisePostprocessSolution(EmptyPostprocessSolution):
    """
    Run over the solution cell by cell.

    The snippet is inserted once per cell and therefore has to run over the
    degrees of freedom itself. See DoFWisePostprocessSolution for the variant
    that provides that loop.
    """

    def __init__(self, solver):
        super(CellWisePostprocessSolution, self).__init__(solver)
        self._guard = "true"

    def add_postprocessing_kernel(self, operation_per_cell):
        """
        Add a code snippet which is applied to every cell.

        The snippet can use the following variables:

        - marker: Cell marker holding the cell's centre and size.
        - timeStamp: Time stamp of the cell.

        ## Arguments

        operation_per_cell: String
          C/C++ code snippet.
        """
        self._cell_compute_kernel += (
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
