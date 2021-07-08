# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer



class InsertParticlesbyCoor(ActionSet):
  def __init__(self,particle_set,p1=[0,0,0],p2=[0,0,0]):
    """
=

    particle_set: ParticleSet
 
    p1,p2: Float
     coordinates for the particle 

    """
    #self._particle_set = particle_set
    #self._h            = h
    self.d = {}
    self.d[ "PARTICLE" ]                 = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]      = particle_set.name
    self.d[ "P1" ]                       = p1
    self.d[ "P2" ]                       = p2


  __Template_TouchVertexFirstTime = jinja2.Template("""
  if ( not marker.isRefined() and 
  (marker.x()(0)-marker.h()(0)/2.0) < {{P1[0]}} and (marker.x()(0)+marker.h()(0)/2.0) > {{P1[0]}} and
  (marker.x()(1)-marker.h()(1)/2.0) < {{P1[1]}} and (marker.x()(1)+marker.h()(1)/2.0) > {{P1[1]}} and
  (marker.x()(2)-marker.h()(2)/2.0) < {{P1[2]}} and (marker.x()(2)+marker.h()(2)/2.0) > {{P1[2]}} 
  )
  {
    globaldata::{{PARTICLE}}* p = new globaldata::{{PARTICLE}}();
    p->setNumber(0, _spacetreeId);
    p->setNumber(1, _particleNumberOnThisTree);
    p->setX(0, {{P1[0]}} );		// x coordinate
    p->setX(1, {{P1[1]}} );		// y coordinate
    p->setX(2, {{P1[2]}} );		// z coordinate
    p->setCutOffRadius(0.0); 
    _particleNumberOnThisTree++;
    fineGridVertex{{PARTICLES_CONTAINER}}.push_back( p );
  }
  if ( not marker.isRefined() and 
  (marker.x()(0)-marker.h()(0)/2.0) < {{P2[0]}} and (marker.x()(0)+marker.h()(0)/2.0) > {{P2[0]}} and
  (marker.x()(1)-marker.h()(1)/2.0) < {{P2[1]}} and (marker.x()(1)+marker.h()(1)/2.0) > {{P2[1]}} and
  (marker.x()(2)-marker.h()(2)/2.0) < {{P2[2]}} and (marker.x()(2)+marker.h()(2)/2.0) > {{P2[2]}} 
  )
  {
    globaldata::{{PARTICLE}}* p2 = new globaldata::{{PARTICLE}}();
    p2->setNumber(0, _spacetreeId);
    p2->setNumber(1, _particleNumberOnThisTree);

    toolbox::particles::init(*newParticle, { {{P2[0]}},{{P2[1]}},{{P2[2]}} },0.0);

    _particleNumberOnThisTree++;
    fineGridVertex{{PARTICLES_CONTAINER}}.push_back( p2 );
  }
""")

  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = self.__Template_TouchVertexFirstTime.render(**self.d)
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
#include "toolbox/particles/ParticleFactory.h"
""" )
    return result.render(**self.d)


  def get_constructor_body(self):
    return """
  _particleNumberOnThisTree = 0;
  _spacetreeId              = treeNumber;
"""


  def get_attributes(self):
     return """
  int _particleNumberOnThisTree;
  int _spacetreeId;
"""
#    result = jinja2.Template( """
#  std::forward_list< globaldata::{{PARTICLE}}* >  _activeParticles;
#""")
#    return result.render(**self.d)
