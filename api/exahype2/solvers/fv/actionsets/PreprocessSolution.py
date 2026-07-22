# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractFVActionSet import AbstractFVActionSet

import peano4
import jinja2


class EmptyPreprocessSolution(AbstractFVActionSet):
  """
  Preprocessing variant which contributes no code to the solver.

  PreprocessSolution differs from other action sets, as we create it only once.
  See FV.create_action_sets(). Once the instance does exist, you can tailor it.
  You can also completely overwrite it.
  """
  def __init__(self,solver):
    AbstractFVActionSet.__init__(self,solver)
    # The guard is unused here, but it keeps the interface of the preprocessing
    # variants identical, so one can be swapped for another.
    self.guard               = "true"
    self.descend_invocation_order            = 2

  def get_body_of_operation(self,operation_name):
    result = ""
    return result

  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


class VolumeWisePreprocessSolution(AbstractFVActionSet):
  """!

  Run over the solution volume by volume.

  PreprocessSolution differs from other action sets, as we create it only once.
  See FV.create_action_sets(). Once the instance does exist, you can tailor it.
  You can also completely overwrite it.

  See also FV.postprocess_updated_patch(self, kernel) for a discussion of the
  options ExaHyPE offers. This volume-wise variant might be too heavy-weight for
  some applications.

  The kernel plugs into touch cell last time. It is the last action set added by
  the default implementation, so it is the first action set of which
  touchCellLastTime() is called. We plug into the leaving of the cell because
  some solvers add further action sets to the solve. Those action sets typically
  plug into touchCellFirstTime(), and their first time is therefore called after
  this action set's first time. Using touchCellLastTime() lets any action set
  added by a subclass still precede this one.
  """
  def __init__(self,solver):
    AbstractFVActionSet.__init__(self,solver)
    self.guard="true"
    self._compute_kernel=""
    self.descend_invocation_order=1
    self.new_accessor = "fineGridCell" + solver.current_unknown_identifier()

  def add_postprocessing_kernel(self, operation_per_point):
    """
    Add a code snippet which is applied to every finite volume.

    The snippet can use the following variables:

    - value: Pointer to the current finite volume's data.
    - x: Centre of the volume as tarch::la::Vector over doubles.
    - h: Size of the volume as tarch::la::Vector over doubles.
    - timeStamp: Time stamp of the enclosing cell.

    ## Arguments

    operation_per_point: String
      C/C++ code snippet.
    """
    self._compute_kernel += """
{
  {{COMPUTE_TIME_STEP_SIZE}}
  const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

  if (
    not marker.hasBeenRefined()
    and
    {{PREDICATE}}
  ) {
    int index = 0;
    peano4::dForEach({{NUMBER_OF_VOLUMES_PER_AXIS}}, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      {{STORAGE_PRECISION}}* value = {{NEW_ACCESSOR}}.data() + index;
      auto x = ::exahype2::fv::getVolumeCentre(marker.getX(), marker.getH(), {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);
      auto h = ::exahype2::fv::getVolumeSize(marker.getH(), {{NUMBER_OF_VOLUMES_PER_AXIS}});
      """ + operation_per_point + """
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    });
  }
}
"""

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d["NEW_ACCESSOR"] = self.new_accessor
      d[ "PREDICATE" ]  = jinja2.Template(self.guard, undefined=jinja2.DebugUndefined).render(**d)
      result = jinja2.Template(self._compute_kernel, undefined=jinja2.DebugUndefined).render(**d)
      pass
    return result

  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
