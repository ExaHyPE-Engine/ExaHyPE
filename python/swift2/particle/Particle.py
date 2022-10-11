# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4.toolbox.particles

import dastgen2


class Particle(peano4.toolbox.particles.Particle):
  """
  
  Base class for any particle in the project
    
  """
  
  
  class SpeciesAspect(object):
    """
    
    Every generated particle belongs to a species. I originally thought I'd
    use some kind of inheritance to model this, but actually it is easier to
    introduce a static field for the time being that stores the species 
    information.
    
    """
    def __init__(self):
      pass


    def get_include(self):
      return """
#include "swift2/ParticleSpecies.h"
"""


    def set_model(self,data_model):
      self._data_model = data_model

  
    def get_attributes(self):
      return """
//  static ::swift2::ParticleSpecies _species;
"""

    
    def get_method_declarations(self,full_qualified_name):
      return """
  static ::swift2::ParticleSpecies& getSpecies();
"""
          

    def get_implementation(self,full_qualified_name):
      return """
::swift2::ParticleSpecies& """ + full_qualified_name + """::getSpecies() {
  static ::swift2::ParticleSpecies species;
  return species;
}
"""
    
    
  def __init__(self, name):
    super( Particle, self ).__init__( name )
    self.data.add_attribute( dastgen2.attributes.Enumeration("MoveState",["New","NotMovedYet","Moved"]) )
    self.data.add_aspect( Particle.SpeciesAspect() )
    pass


  def algorithm_steps(self):
    """
    
    Return sequence of algorithm steps
    
    """
    assert False, "Should be overwritten by subclass"
    return []
