# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2
import peano4


TaskMarkerIdentifier = "TaskMarker"
UndefinedNumber = -1


def construct_marker_name(task_name):
    return task_name + "MeshNumber"


def create_vertex_marker(
    task_name,
    full_qualified_enumerator_type="tarch::Enumerator",
    enumerator_include=""" #include "tarch/Enumerator.h" """,
):
    """!

    Create vertex marker

    This marker can be used for vertices. The numbers are then typically used
    for some kind of task system or external referencing. Similar systems are
    also used linear algebra, i.e. where you enumerate mesh entities, but this
    one is different as a vertex holds information about its adjacent cells'
    numbers.

    If you want to use the action sets from this file, you have to run create
    markers for the vertices and the cells, and you have to add them to the
    mesh:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    cell_marker_for_tasks   = peano4.toolbox.api.EnumerateCellsAndVertices.create_cell_marker(current_species_set.name)
    vertex_marker_for_tasks = peano4.toolbox.api.EnumerateCellsAndVertices.create_vertex_marker(current_species_set.name)

    self._project.datamodel.add_cell(cell_marker_for_tasks)
    self._project.datamodel.add_vertex(vertex_marker_for_tasks)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Furthermore, you have to add the use_vertex and use_cell instructions to
    each observer (algorithm step) before you add the action sets to the sets
    in turn.

    Please note that this marker not only holds an index for the vertex. It
    also hosts the markers of the adjacent cells.

    """
    marker = peano4.datamodel.MetaData(construct_marker_name(task_name))
    marker.add_attribute(
        peano4.datamodel.Integer(
            name="number",
            initval=str(UndefinedNumber),
        )
    )
    marker.add_attribute(
        peano4.datamodel.IntegerArray(
            name="adjacentCellNumber", cardinality="TWO_POWER_D"
        )
    )
    marker.add_attribute(
        peano4.datamodel.StaticUserDefinedType(
            name="enumerator",
            type=full_qualified_enumerator_type,
            include=enumerator_include,
        )
    )
    marker.includes += enumerator_include
    return marker


def create_cell_marker(task_name):
    """!

    Create cell marker for tasking

    This marker can be used for cells.

    @see create_vertex_marker() for some usage information.

    """
    marker = peano4.datamodel.MetaData(construct_marker_name(task_name))
    marker.add_attribute(
        peano4.datamodel.Integer(
            name="number",
            initval=str(UndefinedNumber),
        )
    )
    marker.add_attribute(
        peano4.datamodel.StaticUserDefinedType(
            name="enumerator",
            type="tarch::Enumerator",
            include=""" #include "tarch/Enumerator.h" """,
        )
    )
    marker.includes += """ #include "tarch/Enumerator.h" """
    return marker


class AssignNumbersToMesh(ActionSet):
    """!

    Gives each mesh entity a unique number

    Also ensure that a vertex knows about the numbers of the adjacent cells.
    The numbers are unique per rank, but not unique globally. We achieve
    uniqueness by relying on the tarch::Enumerator.

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
        super(AssignNumbersToMesh, self).__init__()
        self._task_name = task_name
        pass

    def get_body_of_operation(self, operation_name):
        """!

        Very simple operation which basically resets all data

        """
        if operation_name == ActionSet.OPERATION_CREATE_VERTEX:
            return """
            fineGridVertex{}.setNumber(tarch::Enumerator::NoNumber);
            for (int i=0; i<TWO_POWER_D; i++) {{
              fineGridVertex{}.setAdjacentCellNumber(i,tarch::Enumerator::NoNumber);
            }}
            """.format(
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
            )
        if operation_name == ActionSet.OPERATION_DESTROY_VERTEX:
            return """
            fineGridVertex{}.getEnumerator().releaseNumber( fineGridVertex{}.getNumber() );
            """.format(
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
            )
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            return """
            if ( fineGridVertex{}.getNumber()==tarch::Enumerator::NoNumber ) {{
              fineGridVertex{}.setNumber( fineGridVertex{}.getEnumerator().getNumber() );
              logDebug("set number " << fineGridVertex{}.toString());
            }}
            """.format(
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
            )
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
              logDebug("set number " << fineGridCell{}.toString());
            }}
            for (int i=0; i<TWO_POWER_D; i++) {{
              fineGridVertices{}(i).setAdjacentCellNumber(
                TWO_POWER_D-1-i,
                fineGridCell{}.getNumber()
              );
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
        return "  return std::vector< ::peano4::grid::GridControlEvent >();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_") + "_AssignNumbersToMesh"

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return """
#include "tarch/Enumerator.h"
 """


class ClearNumbersOnMesh(ActionSet):
    """!

    Clear all the numbers on the mesh

    We set all entries to AdjacentCellNumber.

    """

    def __init__(self, task_name):
        self._task_name = task_name
        pass

    def get_body_of_operation(self, operation_name):
        """!

        Very simple operation which basically resets all data

        """
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            return """
            fineGridVertex{}.setNumber(tarch::Enumerator::NoNumber);
            for (int i=0; i<TWO_POWER_D; i++) fineGridVertex{}.setAdjacentCellNumber(i,tarch::Enumerator::NoNumber);
            logDebug("reset " << fineGridVertex{}.toString());
            """.format(
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
            )
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return """
            fineGridCell{}.setNumber(tarch::Enumerator::NoNumber);
            logDebug("reset " << fineGridCell{}.toString());
            """.format(
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
                construct_marker_name(self._task_name),
            )
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< ::peano4::grid::GridControlEvent >();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_") + "_ClearNumbersOnMesh"

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return """
#include "tarch/Enumerator.h"
"""
