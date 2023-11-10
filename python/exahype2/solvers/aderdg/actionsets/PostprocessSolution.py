# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet

import peano4.solversteps
import jinja2

class EmptyPostprocessSolution(AbstractAderDGActionSet):
    """
    
    PostprocessSolution differs from other action sets, as I only create it once. See 
    FV.create_action_sets(). Once the instance does exist, you can tailor it. But you
    can also completely overwrite it.
    
    """

    def __init__(self,solver):
        super(EmptyPostprocessSolution,self).__init__(solver)
        # Is there for compatibility reasons
        self._compute_kernel     = ""
      

    def get_body_of_operation(self,operation_name):
        result = ""
        if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            result = jinja2.Template(self._compute_kernel, undefined=jinja2.DebugUndefined).render(**d)
            pass 
        return result


    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")


class DoFWisePostprocessSolution(AbstractAderDGActionSet):
    """

    PostprocessSolution differs from other action sets, as I only create it once. See
    FV.create_action_sets(). Once the instance does exist, you can tailor it. But you
    can also completely overwrite it.

    The postprocessing plugs into touch cell last time. It is the last action set added
    by the default implementation. Therefore, it is the first action set of which
    touchCellLastTime() is called. The reason why I plug into the leaving of the cell
    is that some solvers may add further action sets to the solve. Enclave tasking for
    example adds the merger as additional action set. These action sets plug into
    touchCellFirstTime() - and consequently their first time is called after the
    postprocessing's first time. By making the postprocessing use touchCellLastTime(),
    I ensure that any additional action set added by a subclass can still precede the
    postprocessing.

    """

    def __init__(self, solver):
        super(DoFWisePostprocessSolution, self).__init__(solver)
        self.guard = "true"
        self._compute_kernel = ""
        self.descend_invocation_order = 1

    def add_postprocessing_kernel(self, operation_per_point):
        """

        Add a code snippet which is applied to each and every point. You have the following
        variables which are well-defined:

        - value: Is a pointer to the current finite volume's data
        - x: A tarch::la::Vector over doubles
        - marker.h(): A tarch::la::Vector over doubles


        operation_per_point: String
          C/C++ code snippet

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
    logTraceIn( "touchCellFirstTime(...)" );
    
    ::exahype2::enumerator::AoSLexicographicEnumerator enumerator(
      1, // only one patch
      {{ORDER}}+1,
      0, // int haloSize,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}}
    );
    dfor( index, {{ORDER}}+1 ) {
      double* values   = fineGridCell{{UNKNOWN_IDENTIFIER}}.value + enumerator(0,index,0);
      auto    x        =         ::exahype2::aderdg::getQuadraturePoint(
        marker.x(), marker.h(), index, repositories::{{SOLVER_INSTANCE}}.Order+1, repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d
      );
      """
            + operation_per_point
            + """
    }
    logTraceOut( "touchCellFirstTime(...)" );
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
            pass
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(DoFWisePostprocessSolution, self).get_includes()
            + """
#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
"""
        )


class CellWisePostprocessSolution(AbstractAderDGActionSet):
    """ """

    def __init__(self, solver):
        super(CellWisePostprocessSolution, self).__init__(solver)
        self.guard = "true"
        self._compute_kernel = ""

    def add_postprocessing_kernel(self, operation_per_cell):
        """

        Add a code snippet which is applied to each and every point. You have the following
        variables which are well-defined:

        - value: Is a pointer to the current finite volume's data
        - x: A tarch::la::Vector over doubles
        - marker.h(): A tarch::la::Vector over doubles


        operation_per_point: String
          C/C++ code snippet

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
    logTraceIn( "touchCellFirstTime(...)" );
    
    """
            + operation_per_cell
            + """

    logTraceOut( "touchCellFirstTime(...)" );
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
            pass
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")