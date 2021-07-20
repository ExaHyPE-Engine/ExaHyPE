# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer



class InsertParticlesbyCoor(ActionSet):
  def __init__(self,particle_set,N=2,coor_s=[[0,0,0],[0,0,0]]):
    """
=

    particle_set: ParticleSet
 
    N: number of particles

    coor_s: coordinates for the particle 

    """

    self.d = {}
    self.d[ "PARTICLE" ]                 = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]      = particle_set.name
    self.d[ "N" ]                        = N
    self._coor_s			 = coor_s

  __Template_TouchVertexFirstTime = jinja2.Template("""
  for(int i=0;i<{{N}};i++){
    if ( not marker.isRefined() and 
    (marker.x()(0)-marker.h()(0)/2.0) < coor_s[i][0] and (marker.x()(0)+marker.h()(0)/2.0) > coor_s[i][0] and
    (marker.x()(1)-marker.h()(1)/2.0) < coor_s[i][1] and (marker.x()(1)+marker.h()(1)/2.0) > coor_s[i][1] and
    (marker.x()(2)-marker.h()(2)/2.0) < coor_s[i][2] and (marker.x()(2)+marker.h()(2)/2.0) > coor_s[i][2] 
    )
    {
      globaldata::{{PARTICLE}}* newParticle = new globaldata::{{PARTICLE}}();
      newParticle->setNumber(0, _spacetreeId);
      newParticle->setNumber(1, _particleNumberOnThisTree);
      //std::cout<<_particleNumberOnThisTree<<std::endl;
      toolbox::particles::init(*newParticle,{coor_s[i][0],coor_s[i][1],coor_s[i][2]},0.0);
      _particleNumberOnThisTree++;
      fineGridVertex{{PARTICLES_CONTAINER}}.push_back( newParticle );
      //std::cout<<coor_s[i][0]<<std::endl;
    }
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

  def Read_In_Coor(self):
    result=""
    for i in range(self.d["N"]):
      result+= "  coor_s["+str(i)+"][0]="+str(self._coor_s[i][0])+";"
      result+= " coor_s["+str(i)+"][1]="+str(self._coor_s[i][1])+";"
      result+= " coor_s["+str(i)+"][2]="+str(self._coor_s[i][2])+";"
      result+= "\n"
    return result

  def get_constructor_body(self):
    return f"""
  _particleNumberOnThisTree = 0;
  _spacetreeId              = treeNumber;
{self.Read_In_Coor()}
"""


  def get_attributes(self):
     return f"""
  int _particleNumberOnThisTree;
  int _spacetreeId;
  double coor_s["""+str(self.d["N"])+"""][3];
"""
#    result = jinja2.Template( """
#  std::forward_list< globaldata::{{PARTICLE}}* >  _activeParticles;
#""")
#    return result.render(**self.d)
