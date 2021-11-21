# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer



class ParticleTreeAnalysis(ActionSet):
  """
  
    This action set implements an analysed tree grammar
  
    The grammar goes down to the finest level and writes into each 
    leaf cell of the tree the following data:
    
    - Number of particles within this cell
    - The flag ParentOfRefinedCell is set to false
    
    We then run through the tree bottom-up and set these two flags
    on each spacetree cell along the following rules:
    
    - Number of particles of a refined cell in the spacetree equals
      the number of all the children cells plus the local particles,
      i.e. those particles stored on the present level.
    - ParentOfRefinedCell becomes true if one of the children is 
      refined. If none of the children is refined, the flag is false.
  
    I use the analysed tree data to determine if a tree's has to be
    refined further as it hosts too many particles. I also use it to 
    steer if a tree has to be coarsened if there are not enough 
    particles. Finally, I use the flag ParentOfRefinedCell to ensure 
    that we always remove at most one level from the tree in one 
    tree sweep, as I only erase refined tree nodes for which 
    ParentOfRefinedCell does not hold.
    
    
    
    :: Usage ::  
    
    I need a particle set to work properly. You also have to
    invoke add the tree cell properties to the tree and tell
    every observer that invokes this particle set to handle 
    the tree marker:
    
    
    my_Peano_project = ...
    
    my_tree_analysis = ParticleTreeAnalysis(...)
    my_Peano_project.add_cell(my_tree_analysis.cell_marker)
    


    my_algorithmic_step = peano4.solversteps.Step( ... )
    my_algorithmic_step.use_cell(my_tree_analysis.cell_marker)

    
  """
  def __init__(self,particle_set):
    self._cell_data_name = particle_set.name + "CellStatistics"
    self.cell_marker     = peano4.datamodel.DaStGen2( self._cell_data_name )
    
    self.cell_marker.data.add_attribute( dastgen2.attributes.Integer("NumberOfParticles") )
    self.cell_marker.data.add_attribute( dastgen2.attributes.Boolean("ParentOfRefinedCell") )
    
    self.d = {
      "CELL_DATA_NAME":    self._cell_data_name,
      "PARTICLE_SET_NAME": particle_set.name,
      "PARTICLE_NAME":     particle_set.particle_model.name
    }
    pass


  __Template_TouchCellFirstTime = jinja2.Template( """
  fineGridCell{{CELL_DATA_NAME}}.setNumberOfParticles(0);
  fineGridCell{{CELL_DATA_NAME}}.setParentOfRefinedCell(false);
""")

             
  __Template_TouchCellLastTime = jinja2.Template("""
  if (marker.hasBeenRefined()) {
    coarseGridCell{{CELL_DATA_NAME}}.setParentOfRefinedCell(true);
  }
  else {
    int count = 0;
    for (int i=0; i<TwoPowerD; i++) {
      count += fineGridVertices{{PARTICLE_SET_NAME}}(i).size();
    }
    fineGridCell{{CELL_DATA_NAME}}.setNumberOfParticles(count);
  }
  
  coarseGridCell{{CELL_DATA_NAME}}.setNumberOfParticles(
    coarseGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() 
    +
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles()
  );
""")


  def get_constructor_body(self):
    return ""

    
  def get_destructor_body(self):
    return ""


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  def get_attributes(self):
    return ""


  __Template_Includes = jinja2.Template("""
#include "../vertexdata/{{PARTICLE_SET_NAME}}.h"
#include "../globaldata/{{PARTICLE_NAME}}.h"
""")


  def get_includes(self):
    return self.__Template_Includes.render(**self.d)             


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCellFirstTime.render(**self.d)             
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = self.__Template_TouchCellLastTime.render(**self.d)             
    return result

