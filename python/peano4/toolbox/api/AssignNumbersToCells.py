# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2
import dastgen2
import peano4


from .EnumerateCellsAndVertices import ClearNumbersOnMesh
from .EnumerateCellsAndVertices import create_cell_marker
from .EnumerateCellsAndVertices import construct_marker_name


class AssignNumbersToCells(ActionSet):
    """!

    Gives each mesh cell a unique number

    The numbers are unique per rank, but not globally unique. If you want to
    use this action set, you before have to add a marker to the mesh data 
    model:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    my_fancy_name = ...
    
    cell_marker_for_tasks = peano4.toolbox.api.EnumerateCellsAndVertices.create_cell_marker(my_fancy_name)
    self._project.datamodel.add_cell(cell_marker_for_tasks)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    @see create_vertex_marker()

    """

    def __init__(
        self,
        task_name,
    ):
        """!

        Initialise assignment

        @param task_name: String
          The name of the task (attribute tied to the mesh entity) to be 
          manipulated.

        """
        super( AssignNumbersToCells, self ).__init__()
        self._task_name = task_name
        pass

    def get_body_of_operation(self, operation_name):
        """!

        Very simple operation which basically resets all data

        """
        if operation_name == ActionSet.OPERATION_CREATE_CELL:
            return """
            fineGridCell{}.setNumber(tarch::Enumerator::NoNumber);
            """.format(
                construct_marker_name(self._task_name)
            )
        if operation_name == ActionSet.OPERATION_DESTROY_CELL:
            return """
            fineGridCell{}.getEnumerator().releaseNumber( fineGridCell{}.getNumber() );
            """.format(
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
            )
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return """
            if ( fineGridCell{}.getNumber()==tarch::Enumerator::NoNumber ) {{
              fineGridCell{}.setNumber( fineGridCell{}.getEnumerator().getNumber() );
              logDebug( "touchCellFirstTime(...)", "set number " << fineGridCell{}.toString() );
            }}
            """.format(
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
            )
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_") + "_AssignNumbersToCells"

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return """
#include "tarch/multicore/TaskEnumerator.h"
 """

