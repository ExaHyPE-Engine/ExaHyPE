# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractFVActionSet import AbstractFVActionSet

import peano4
import jinja2


class EmptyPostprocessSolution(AbstractFVActionSet):
    """

    PostprocessSolution differs from other action sets, as I only create it once. See
    FV.create_action_sets(). Once the instance does exist, you can tailor it. But you
    can also completely overwrite it.

    """

    def __init__(self, solver):
        AbstractFVActionSet.__init__(self, solver)
        # Is there for compatibility reasons
        self.guard = "true"

    def get_body_of_operation(self, operation_name):
        result = ""
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")


class VolumeWisePostprocessSolution(AbstractFVActionSet):
    """!

    Run over solution volume by volume

    PostprocessSolution differs from other action sets, as I only create it once. See
    FV.create_action_sets(). Once the instance does exist, you can tailor it. But you
    can also completely overwrite it.

    See also FV.postprocess_updated_patch(self, kernel) for a discussion what options
    ExaHyPE offers for postprocessing. This volume-wise postprocessing might be too
    heavy-weight for some applications.

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
        AbstractFVActionSet.__init__(self, solver)
        self.guard = "true"
        self._compute_kernel = ""

    def add_postprocessing_kernel(self, operation_per_point):
        """

        Add a code snippet which is applied to each and every point. You have the following
        variables which are well-defined:

        - value: Is a pointer to the current finite volume's data
        - volumeX: A tarch::la::Vector over doubles
        - volumeH: A tarch::la::Vector over doubles


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
    
    int index = 0;
    dfor( volume, {{NUMBER_OF_VOLUMES_PER_AXIS}} ) {
      double* value   = fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index;
      auto    volumeX = ::exahype2::fv::getVolumeCentre( marker.x(), marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);
      auto    volumeH = ::exahype2::fv::getVolumeSize( marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}});
      
      """
            + operation_per_point
            + """
      
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
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
