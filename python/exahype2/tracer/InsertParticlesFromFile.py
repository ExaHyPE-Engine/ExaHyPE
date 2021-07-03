# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer

import numpy as np

class InsertParticlesFromFile(ActionSet):
  def __init__(self, particle_set, filename ):
    """

    particle_set: ParticleSet
 
    filename: String
      Name of the sample point table, currently we have: t-design (948 points), Gauss_Legendre_quadrature (800 points)

    """

    self.d = {}
    self.d[ "PARTICLE" ]                 = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]      = particle_set.name
    self._filename                       = filename


  __Template_TouchVertexFirstTime = jinja2.Template("""
  if (not marker.isRefined()) {
    tarch::multicore::Lock lock( _semaphore );
  
    std::list< tarch::la::Vector<Dimensions,double> > coords = _fileReader.getParticlesWithinVoxel(marker.x(), marker.h());  
    for (auto& p: coords) {
      globaldata::{{PARTICLE}}* newParticle = new globaldata::{{PARTICLE}}();
      newParticle->setNumber(0, _spacetreeId);
      newParticle->setNumber(1, _particleNumberOnThisTree);
      newParticle->setX(p);                 // position
      newParticle->setCutOffRadius(0.0);    // not used with tracers, but we have to set it. Otherwise the vis (Paraview) will crash
      _particleNumberOnThisTree++;
      fineGridVertex{{PARTICLES_CONTAINER}}.push_back( newParticle );
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
#include "toolbox/particles/FileReader.h"
#include <fstream>
#include <string>
#include <vector>
""" )
    return result.render(**self.d)


  def get_static_initialisations(self,full_qualified_classname):
    return """
tarch::multicore::BooleanSemaphore """ + full_qualified_classname + """::_semaphore;
"""


  def get_constructor_body(self):
    return """
  _particleNumberOnThisTree = 0;
  _spacetreeId              = treeNumber;
  _fileReader.readDatFile( \"""" + self._filename + """\" );
"""


  def get_destructor_body(self):
    return """
  _fileReader.clear();
"""


  def get_attributes(self):
     return """
  static tarch::multicore::BooleanSemaphore _semaphore;
     
  int                            _particleNumberOnThisTree;
  int                            _spacetreeId;
  toolbox::particles::FileReader _fileReader;
"""

