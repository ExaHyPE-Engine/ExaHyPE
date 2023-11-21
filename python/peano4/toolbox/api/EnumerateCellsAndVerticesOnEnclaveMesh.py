# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2
import dastgen2
import peano4


TaskMarkerIdentifier = "TaskMarker"
UndefinedNumber      = -1


def construct_marker_name(task_name):
    return task_name + "EnclaveMeshEntityNumber"


def create_vertex_marker(task_name):
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
    cell_marker_for_tasks   = peano4.toolbox.EnumerateCellsAndVerticesOnEnclaveMesh.create_cell_marker(current_species_set.name)
    vertex_marker_for_tasks = peano4.toolbox.EnumerateCellsAndVerticesOnEnclaveMesh.create_vertex_marker(current_species_set.name)
 
    self._project.datamodel.add_cell(cell_marker_for_tasks)
    self._project.datamodel.add_vertex(vertex_marker_for_tasks)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Furthermore, you have to add the use_vertex and use_cell instructions to 
    each observer (algorithm step) before you add the action sets to the sets
    in turn.
    
    """
    dastgen_marker = peano4.datamodel.DaStGen2( construct_marker_name(task_name) )
    dastgen_marker.data.add_attribute(dastgen2.attributes.Integer("Number"))
    dastgen_marker.data.add_attribute(peano4.dastgen2.Peano4IntegerArray(name="AdjacentCellNumber",cardinality="TwoPowerD"))
    return dastgen_marker


def create_cell_marker(task_name):
    """!
    
    Create cell marker for tasking
    
    This marker can be used for cells.
    
    @see create_vertex_marker() for some usage information.
    
    """
    dastgen_marker = peano4.datamodel.DaStGen2( construct_marker_name(task_name) )
    dastgen_marker.data.add_attribute(dastgen2.attributes.Integer("Number"))
    return dastgen_marker



class AssignNumbersToMesh(ActionSet):
    """!
    
    Gives each mesh entity a unique number 
    
    Also ensure that a vertex knows about the numbers of the adjacent cells.
    The numbers are unique per tree, but not unique globally or even per rank.
    At the moment, we ignore face data here, i.e. we only assign numbers to 
    cells and vertices.
    
    """
    def __init__(self, 
                 task_name,
                 ):
        """!
        
        Initialise assignment
        
        @param touch_vertex_first_time_assignment_predicate: Boolean C++ expression
          This parameter determines if a vertex is assigned a task number of 
          not in touchVertexFirstTime().
          
          
        """
        self._task_name = task_name
        pass

    def get_body_of_operation(self, operation_name):
        """!
        
        Very simple operation which basically resets all data
        
        """
        if operation_name==ActionSet.OPERATION_CREATE_PERSISTENT_VERTEX or operation_name==ActionSet.OPERATION_CREATE_HANGING_VERTEX:
            return """
            fineGridVertex{}.setNumber(NoNumber);
            fineGridVertex{}.setNumber(NoNumber);
            for (int i=0; i<TwoPowerD; i++) fineGridVertex{}.setAdjacentCellNumber(i,NoNumber);
            """.format(construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       )
        if operation_name==ActionSet.OPERATION_DESTROY_PERSISTENT_VERTEX or operation_name==ActionSet.OPERATION_DESTROY_HANGING_VERTEX:
            return """
            tarch::multicore::releaseTaskNumber( fineGridVertex{}.getNumber() );
            tarch::multicore::releaseTaskNumber( fineGridVertex{}.getNumber() );
            """.format(construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       )
        if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            return """
            if ( 
              fineGridVertex{}.getNumber()==NoNumber 
              and
              {}
            ) {{
              fineGridVertex{}.setNumber( tarch::multicore::reserveTaskNumber() );
            }}
            if ( 
              fineGridVertex{}.getNumber()==NoNumber 
              and
              {}
            ) {{
              fineGridVertex{}.setNumber( tarch::multicore::reserveTaskNumber() );
            }}
            """.format(construct_marker_name(self._task_name),
                       self.touch_vertex_first_time_assignment_predicate,
                       construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       self.touch_vertex_last_time_assignment_predicate,
                       construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       )
        if operation_name==ActionSet.OPERATION_CREATE_CELL:
            return """
            fineGridCell{}.setNumber(NoNumber);
            """.format(construct_marker_name(self._task_name))
        if operation_name==ActionSet.OPERATION_DESTROY_CELL:
            return """
            tarch::multicore::releaseTaskNumber( fineGridCell{}.getTouchFirstNumber() );
            """.format(construct_marker_name(self._task_name))
        if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return """
            if ( 
              fineGridCell{}.getNumber()==NoNumber 
              and
              {}
            ) {{
              fineGridCell{}.setNumber( tarch::multicore::reserveTaskNumber() );
            }}
            if ( 
              fineGridCell{}.getNumber()==NoNumber 
              and
              {}
            ) {{
              fineGridCell{}.setNumber( tarch::multicore::reserveTaskNumber() );
            }}
            for (int i=0; i<TwoPowerD; i++) {{
              fineGridVertices{}(i).setNumber(
                TwoPowerD-1-i,
                fineGridCell{}.getNumber()
              );
            }}
            """.format(construct_marker_name(self._task_name),
                       self.touch_cell_first_time_assignment_predicate,
                       construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       self.touch_cell_last_time_assignment_predicate,
                       construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       )
        return ""


    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n" 

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_attributes(self):
        """!
        
        Propagates the no number through to the generated file.
        
        """
        return """
static constexpr int NoNumber = {};
""".format( UndefinedNumber )

#    def get_includes(self):
#        return """
##include "tarch/multicore/Tasks.h"        
#"""

#  /**
#   * Used to administer task numbers
#   */
#  tarch::multicore::BooleanSemaphore activeTasksSemaphore;
#  std::set<int>                      activeTaskNumbers;


class ClearNumbersOnMesh(ActionSet):
    def __init__(self, task_name):
        self._task_name = task_name
        pass

    def get_body_of_operation(self, operation_name):
        """!
        
        Very simple operation which basically resets all data
        
        """
        if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            return """
            fineGridVertex{}.setNumber(NoNumber);
            fineGridVertex{}.setNumber(NoNumber);
            for (int i=0; i<TwoPowerD; i++) fineGridVertex{}.setNumber(i,NoNumber);
            """.format(construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       )
        if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return """
            fineGridCell{}.setNumber(NoNumber);
            fineGridCell{}.setNumber(NoNumber);
            """.format(construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name))
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n" 

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

#    def get_includes(self):
#        return """
##include "tarch/multicore/Tasks.h"        
#"""
