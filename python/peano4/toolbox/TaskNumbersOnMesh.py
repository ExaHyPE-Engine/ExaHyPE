# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2
import dastgen2
import peano4


TaskMarkerIdentifier = "TaskMarker"



def construct_marker_name(task_name):
    return task_name + "TaskNumber"



def create_marker(task_name):
    """!
    
    Create marker for tasking
    
    This marker can be used for vertices and cells. It does not really matter.
    We need two entries, as any user might plug into 
    touchVertexFirstTime() and touchVertexLastTime() or the corresponding cell
    variants.
    
    If you want to use the action sets from this file, you have to run create
    markers for the vertices and the cells, and you have to add them to the 
    mesh:
    
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    cell_marker_for_tasks   = peano4.toolbox.TaskNumbersOnMesh.create_marker(current_species_set.name)
    vertex_marker_for_tasks = peano4.toolbox.TaskNumbersOnMesh.create_marker(current_species_set.name)
 
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
    def __init__(self, task_name):
        self._task_name = task_name
        pass



    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n" 

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False


class ClearTaskNumbersOnMesh(ActionSet):
    def __init__(self, task_name):
        self._task_name = task_name
        pass




    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n" 

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False
