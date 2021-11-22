# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


class InsertRandomParticlesIntoUnrefinedCells(ActionSet):

  def __init__(self,particle_set,average_distance_between_particles,initialisation_call,round_down = False, noise=True,additional_includes=""):
    """
      Insert particles

      :Attibutes:

      particle: ParticleSet
         I take this as particle set.
         
      average_distance_between_particles: Float
         Some positive floating point value.
         
      initialisation_call: String (C code)
         Within this code snippet, you have a variable particle which is a pointer to 
         your particle type. You can use it to set the particle attributes.

    """
    self.d = {}
    self.d[ "PARTICLE" ]                           = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]                = particle_set.name
    self.d[ "INITIALISATION_CALL" ]                = initialisation_call
    self.d[ "H" ]                        = average_distance_between_particles
    if noise:
      self.d[ "NOISE" ]                  = "true"
    else:
      self.d[ "NOISE" ]                  = "false"
    if round_down:
      self.d[ "ROUND_DOWN" ]                  = "true"
    else:
      self.d[ "ROUND_DOWN" ]                  = "false"
    self.additional_includes                       = additional_includes
    

  __Template_TouchCellFirstTime = jinja2.Template("""
  if ( not marker.hasBeenRefined() and not marker.willBeRefined() ) {
    auto newParticles = toolbox::particles::createEquallySpacedParticles<globaldata::{{PARTICLE}}>(
      {{H}},
      marker.x(),
      marker.h(),
      {{ROUND_DOWN}},
      {{NOISE}}
    );
    for (auto& particle: newParticles) {
      {{INITIALISATION_CALL}}
    }
    // just insert them; will be re-assigned then anyway
    fineGridVertices{{PARTICLES_CONTAINER}}(0).insert( fineGridVertices{{PARTICLES_CONTAINER}}(0).end(), newParticles.begin(), newParticles.end() );
  }
""")

  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCellFirstTime.render(**self.d)
    return result


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n"


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  def get_includes(self):
    result = jinja2.Template( """
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "toolbox/particles/ParticleFactory.h"
""" )
    return result.render(**self.d) + self.additional_includes


