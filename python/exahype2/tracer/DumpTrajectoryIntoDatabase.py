# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.toolbox.particles

import jinja2



class DumpTrajectoryIntoDatabase(peano4.solversteps.ActionSet):
  def __init__(self,particle_set,solver,delta_between_two_snapsots,filename,number_of_entries_between_two_db_flushes):
    """
     
    delta_between_two_snapshots: Float
     A particle is dumped if it has switched the domain partition or its position has
     changed more then delta_between_two_snapshots. You can 
     
    number_of_entries_between_two_db_flushes: Int
      Number of entries that we dump into the database before it is flushed the next
      time. Set it to max of an integer or zero to disable on-the-fly dumps. In this
      case, the database is only flushed when the simulation terminates.
     
    """
    self.d = {}
    self.d[ "PARTICLE" ]                 = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]      = particle_set.name
    self.d[ "DELTA" ]                    = delta_between_two_snapsots
    self.d[ "FILENAME" ]                 = filename
    self.d[ "SOLVER_NAME" ]              = solver._name
    self.d[ "SOLVER_INSTANCE" ]          = solver.get_name_of_global_instance()

    self.number_of_entries_between_two_db_flushes = number_of_entries_between_two_db_flushes

  __Template_TouchVertexFirstTime = jinja2.Template("""
  auto& localParticles = fineGridVertex{{PARTICLES_CONTAINER}};
  
  for (auto& p: localParticles) {
    _database.addParticleSnapshot( 
      p->getNumber(1) * peano4::parallel::Node::MaxSpacetreesPerRank + p->getNumber(0),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      p->getX(),
      p->getData().size(),
      p->getData().data()
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
    static toolbox::particles::TrajectoryDatabase  _database;
"""

  def get_static_initialisations(self,full_qualified_classname):
    return """
toolbox::particles::TrajectoryDatabase  """ + full_qualified_classname + """::_database( """ + str(self.number_of_entries_between_two_db_flushes) + """);
"""