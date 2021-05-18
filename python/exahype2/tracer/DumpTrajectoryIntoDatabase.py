# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.toolbox.particles

import jinja2



class DumpTrajectoryIntoDatabase(peano4.solversteps.ActionSet):
  def __init__(self,particle_set,solver,delta_between_two_snapsots,filename):
    """
     
    delta_between_two_snapshots: Float
     A particle is dumped if it has switched the domain partition or its position has
     changed more then delta_between_two_snapshots. You can 
     
    """
    self.d = {}
    self.d[ "PARTICLE" ]                 = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]      = particle_set.name
    self.d[ "DELTA" ]        = delta_between_two_snapsots
    self.d[ "FILENAME" ]     = filename
    self.d[ "SOLVER_NAME" ]              = solver._name
    self.d[ "SOLVER_INSTANCE" ]          = solver.get_name_of_global_instance()


  __Template_TouchVertexFirstTime = jinja2.Template("""
  auto& localParticles = fineGridVertex{{PARTICLES_CONTAINER}};
  
  for (auto& p: localParticles) {
    _database.addParticleSnapshot( 
      p->getNumber(1) * peano4::parallel::Node::MaxSpacetreesPerRank + p->getNumber(0),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
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
  _database.setOutputFileName( "{{FILENAME}}" );
  _database.setDeltaBetweenTwoSnapsots( {{DELTA}} );
""")
    return template.render(**self.d)


  def get_includes(self):
    result = jinja2.Template( """
#include "toolbox/particles/TrajectoryDatabase.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "peano4/parallel/Node.h"
#include "repositories/SolverRepository.h"
""" )
    return result.render(**self.d)
    

  def get_attributes(self):
    return """
    toolbox::particles::TrajectoryDatabase  _database;
"""
