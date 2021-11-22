# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer



class InsertParticlesAlongCartesianMesh(ActionSet):
  def __init__(self,particle_set,h=-1,round_down = False, noise=True):
    """
=
    Very simple particle seed which inserts particles into every unrefined
    cell. Logically, the particles align along a Cartesian mesh with mesh 
    size h even though I insert at least one particle per cell.

    particle_set: ParticleSet
 
    h: Float
     Initial spacing of particles. See C++ functions in toolbox/particles/ParticleFactory
     for an explanation of the parameters.

    """
    #self._particle_set = particle_set
    #self._h            = h
    self.d = {}
    self.d[ "PARTICLE" ]                 = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]      = particle_set.name
    self.d[ "H" ]                        = h
    if noise:
      self.d[ "NOISE" ]                  = "true"
    else:
      self.d[ "NOISE" ]                  = "false"
    if round_down:
      self.d[ "ROUND_DOWN" ]                  = "true"
    else:
      self.d[ "ROUND_DOWN" ]                  = "false"


  __Template_TouchCellFirstTime = jinja2.Template("""
  if ( not marker.willBeRefined() ) {
    auto newParticles = toolbox::particles::createEquallySpacedParticles<globaldata::{{PARTICLE}}>({{H}},marker,{{ROUND_DOWN}},{{NOISE}});
    for (auto& p: newParticles) {
      p->setNumber(0,_spacetreeId);
      p->setNumber(1,_particleNumberOnThisTree);
      _particleNumberOnThisTree++;
    }
    // just insert them; will be re-assigned then anyway
    fineGridVertices{{PARTICLES_CONTAINER}}(0).insert( fineGridVertex{{PARTICLES_CONTAINER}}.end(), newParticles.begin(), newParticles.end() );
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
