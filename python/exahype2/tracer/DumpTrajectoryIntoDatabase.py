# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.toolbox.particles

import jinja2



class DumpTrajectoryIntoDatabase(peano4.solversteps.ActionSet):
  def __init__(self,particle_set,delta_between_two_snapsots,filename):
    """
     
    delta_between_two_snapshots: Float
     A particle is dumped if it has switched the domain partition or its position has
     changed more then delta_between_two_snapshots. You can 
     
    """
    self.d = {}
    self.d[ "PARTICLE_SET" ] = particle_set.name
    self.d[ "DELTA" ]        = delta_between_two_snapsots
    self.d[ "FILENAME" ]     = filename


  __Template_TouchVertexFirstTime = jinja2.Template("""
  auto& localParticles = fineGridVertex{{PARTICLE_SET}};
  
  for (auto& p: localParticles) {
    _database.addParticleSnapshot( 
      p->getNumber(1) * peano4::parallel::Node::MaxSpacetreesPerRank + p->getNumber(0),
      p->getX(),
      p->getData()
    );
  };
""")
  

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = self.__Template_TouchVertexFirstTime.render(**self.d)
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
    

  def user_should_modify_template(self):
    return False


  def get_constructor_body(self):
    template = jinja2.Template("""
  _database.setOutputFileName( {{FILENAME}} );
  _database.setDeltaBetweenTwoSnapsots( {{DELTA}} );
""")
    return template.render(**self.d)


  def get_includes(self):
    return """
#include "toolbox/particles/TrajectoryDatabase.h"
"""

  def get_includes(self):
    return """
  toolbox::particles::TrajectoryDatabase  _database;
"""
