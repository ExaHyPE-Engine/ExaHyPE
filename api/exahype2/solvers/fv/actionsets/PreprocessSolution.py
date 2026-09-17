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
  """
  Runs a code snippet over the solution volume by volume.

  FV.create_action_sets() creates this action set once, and a solver can tailor
  or replace the instance afterwards. The kernel plugs into touchCellLastTime(),
  so an action set that a subclass adds on touchCellFirstTime() still runs
  before it.
  """
  def __init__(self,solver):
    AbstractFVActionSet.__init__(self,solver)
    self.guard="true"
    self._compute_kernel=""
    self.descend_invocation_order=1
    self.new_accessor = "fineGridCell" + solver.current_unknown_identifier()

  def add_postprocessing_kernel(self, operation_per_point):
    """
    Adds the C++ snippet operation_per_point, which runs on every finite volume.

    The snippet reads value, the pointer to the volume's data, x, the volume
    centre, h, the volume size, and timeStamp, the time stamp of the enclosing
    cell.
    """
    self._compute_kernel += """
{
  {{COMPUTE_TIME_STEP_SIZE}}
  [[maybe_unused]] const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

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
