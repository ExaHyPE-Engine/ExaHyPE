# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer



class ParticleParticleInteraction(ActionSet):
  def __init__(self,particle_set):
    self._particle_set = particle_set
    self.d = {}
    self.d[ "PARTICLE" ]                 = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]      = particle_set.name


  __Template_TouchCellFirstTime = jinja2.Template("""
  for (int i=0; i<TwoPowerD; i++) {
    for (auto& p: fineGridVertices{{PARTICLES_CONTAINER}}(i) ) {
      bool append = marker.isContained( p->getX() );
      if (append) {
        _activeParticles.push_front( p );
      }
    }
  }
""")
  

  __Template_TouchCellLastTime = jinja2.Template("""
  for (int i=0; i<TwoPowerD; i++) {
    for (auto& p: fineGridVertices{{PARTICLES_CONTAINER}}(i) ) {
      _activeParticles.remove( p );
    }
  }
""")

  
  __Template_EndTraversal = jinja2.Template("""
  assertion( _activeParticles.empty() );
""")
  
  
  def get_body_of_operation(self,operation_name):
    result = "\n"
    #if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
    #  result = self.__Template_BeginTraversal.render(**self.d) 
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCellFirstTime.render(**self.d) 
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = self.__Template_TouchCellLastTime.render(**self.d) 
    if operation_name==ActionSet.OPERATION_END_TRAVERSAL:
      result = self.__Template_EndTraversal.render(**self.d) 
    return result


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False
  
    
  def get_includes(self):
    result = jinja2.Template( """
#include "tarch/multicore/Lock.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"

#include <forward_list>
""" )
    return result.render(**self.d)             


  def get_attributes(self):
    result = jinja2.Template( """
  std::forward_list< globaldata::{{PARTICLE}}* >  _activeParticles;
""")    
    return result.render(**self.d)             



  
