# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2
import dastgen2
import peano4


TaskMarkerIdentifier = "TaskMarker"


def construct_marker_name(task_name):
    return task_name + "TaskNumber"


def create_vertex_marker(task_name):
    """!
    
    Create vertex marker for tasking
    
    This marker can be used for vertices.We need two entries, as any user might
    plug into touchVertexFirstTime() and touchVertexLastTime().
    
    If you want to use the action sets from this file, you have to run create
    markers for the vertices and the cells, and you have to add them to the 
    mesh:
    
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    cell_marker_for_tasks   = peano4.toolbox.TaskNumbersOnMesh.create_cell_marker(current_species_set.name)
    vertex_marker_for_tasks = peano4.toolbox.TaskNumbersOnMesh.create_vertex_marker(current_species_set.name)
 
    self._project.datamodel.add_cell(cell_marker_for_tasks)
    self._project.datamodel.add_vertex(vertex_marker_for_tasks)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Furthermore, you have to add the use_vertex and use_cell instructions to 
    each observer (algorithm step) before you add the action sets to the sets
    in turn.
    
    By default, AdjacentCellsTouchLastNumber holds the index of touchCellLastTime().
    
    """
    dastgen_marker = peano4.datamodel.DaStGen2( construct_marker_name(task_name) )
    dastgen_marker.data.add_attribute(dastgen2.attributes.Integer("TouchFirstNumber"))
    dastgen_marker.data.add_attribute(dastgen2.attributes.Integer("TouchLastNumber"))
    dastgen_marker.data.add_attribute(peano4.dastgen2.Peano4IntegerArray(name="AdjacentCellsTouchLastNumber",cardinality="TwoPowerD"))
    return dastgen_marker


def create_cell_marker(task_name):
    """!
    
    Create cell marker for tasking
    
    This marker can be used for cells.
    We need two entries, as any user might plug into 
    touchCellFirstTime() and touchCellLastTime() or the corresponding cell
    variants.
    
    If you want to use the action sets from this file, you have to run create
    markers for the vertices and the cells, and you have to add them to the 
    mesh:
    
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    cell_marker_for_tasks   = peano4.toolbox.TaskNumbersOnMesh.create_cell_marker(current_species_set.name)
    vertex_marker_for_tasks = peano4.toolbox.TaskNumbersOnMesh.create_vertex_marker(current_species_set.name)
 
    self._project.datamodel.add_cell(cell_marker_for_tasks)
    self._project.datamodel.add_vertex(vertex_marker_for_tasks)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Furthermore, you have to add the use_vertex and use_cell instructions to 
    each observer (algorithm step) before you add the action sets to the sets
    in turn.
    
    """
    dastgen_marker = peano4.datamodel.DaStGen2( construct_marker_name(task_name) )
    dastgen_marker.data.add_attribute(dastgen2.attributes.Integer("TouchFirstNumber"))
    dastgen_marker.data.add_attribute(dastgen2.attributes.Integer("TouchLastNumber"))
    return dastgen_marker



class AssignTaskNumbersToMesh(ActionSet):
    def __init__(self, 
                 task_name,
                 touch_vertex_first_time_assignment_predicate = "true",
                 touch_vertex_last_time_assignment_predicate  = "true",
                 touch_cell_first_time_assignment_predicate   = "true",
                 touch_cell_last_time_assignment_predicate    = "true",
                 ):
        """!
        
        Initialise assignment
        
        @param touch_vertex_first_time_assignment_predicate: Boolean C++ expression
          This parameter determines if a vertex is assigned a task number of 
          not in touchVertexFirstTime().
          
          
        """
        self._task_name = task_name
        self.touch_vertex_first_time_assignment_predicate = touch_vertex_first_time_assignment_predicate
        self.touch_vertex_last_time_assignment_predicate  = touch_vertex_last_time_assignment_predicate
        self.touch_cell_first_time_assignment_predicate   = touch_cell_first_time_assignment_predicate
        self.touch_cell_last_time_assignment_predicate    = touch_cell_last_time_assignment_predicate
        pass

    def get_body_of_operation(self, operation_name):
        """!
        
        Very simple operation which basically resets all data
        
        """
        if operation_name==ActionSet.OPERATION_CREATE_PERSISTENT_VERTEX or operation_name==ActionSet.OPERATION_CREATE_HANGING_VERTEX:
            return """
            fineGridVertex{}.setTouchFirstNumber(tarch::multicore::NoDependency);
            fineGridVertex{}.setTouchLastNumber(tarch::multicore::NoDependency);
            for (int i=0; i<TwoPowerD; i++) fineGridVertex{}.setAdjacentCellsTouchLastNumber(i,tarch::multicore::NoDependency);
            """.format(construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       )
        if operation_name==ActionSet.OPERATION_DESTROY_PERSISTENT_VERTEX or operation_name==ActionSet.OPERATION_DESTROY_HANGING_VERTEX:
            return """
            tarch::multicore::releaseTaskNumber( fineGridVertex{}.getTouchFirstNumber() );
            tarch::multicore::releaseTaskNumber( fineGridVertex{}.getTouchLastNumber() );
            """.format(construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       )
        if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            return """
            if ( 
              fineGridVertex{}.getTouchFirstNumber()==tarch::multicore::NoDependency 
              and
              {}
            ) {{
              fineGridVertex{}.setTouchFirstNumber( tarch::multicore::reserveTaskNumber() );
            }}
            if ( 
              fineGridVertex{}.getTouchLastNumber()==tarch::multicore::NoDependency 
              and
              {}
            ) {{
              fineGridVertex{}.setTouchLastNumber( tarch::multicore::reserveTaskNumber() );
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
            fineGridCell{}.setTouchFirstNumber(tarch::multicore::NoDependency);
            """.format(construct_marker_name(self._task_name))
        if operation_name==ActionSet.OPERATION_DESTROY_CELL:
            return """
            tarch::multicore::releaseTaskNumber( fineGridCell{}.getTouchFirstNumber() );
            """.format(construct_marker_name(self._task_name))
        if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return """
            if ( 
              fineGridCell{}.getTouchFirstNumber()==tarch::multicore::NoDependency 
              and
              {}
            ) {{
              fineGridCell{}.setTouchFirstNumber( tarch::multicore::reserveTaskNumber() );
            }}
            if ( 
              fineGridCell{}.getTouchLastNumber()==tarch::multicore::NoDependency 
              and
              {}
            ) {{
              fineGridCell{}.setTouchLastNumber( tarch::multicore::reserveTaskNumber() );
            }}
            for (int i=0; i<TwoPowerD; i++) {{
              fineGridVertices{}(i).setAdjacentCellsTouchLastNumber(
                TwoPowerD-1-i,
                fineGridCell{}.getTouchLastNumber()
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

    def get_includes(self):
        return """
#include "tarch/multicore/Tasks.h"        
"""


class ClearTaskNumbersOnMesh(ActionSet):
    def __init__(self, task_name):
        self._task_name = task_name
        pass

    def get_body_of_operation(self, operation_name):
        """!
        
        Very simple operation which basically resets all data
        
        """
        if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            return """
            fineGridVertex{}.setTouchFirstNumber(tarch::multicore::NoDependency);
            fineGridVertex{}.setTouchLastNumber(tarch::multicore::NoDependency);
            for (int i=0; i<TwoPowerD; i++) fineGridVertex{}.setAdjacentCellsTouchLastNumber(i,tarch::multicore::NoDependency);
            """.format(construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name),
                       )
        if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return """
            fineGridCell{}.setTouchFirstNumber(tarch::multicore::NoDependency);
            fineGridCell{}.setTouchLastNumber(tarch::multicore::NoDependency);
            """.format(construct_marker_name(self._task_name),
                       construct_marker_name(self._task_name))
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n" 

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return """
#include "tarch/multicore/Tasks.h"        
"""
