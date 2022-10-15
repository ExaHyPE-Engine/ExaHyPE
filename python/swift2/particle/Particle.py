# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4.toolbox.particles

import dastgen2


class Particle(peano4.toolbox.particles.Particle):
  """
  
  Base class for any particle in the project
  
  You usually do not initialise this base class, but you pick one of the 
  subclasses which define which time stepping scheme is realised by a 
  particle. The time stepping scheme defines through which phases or steps
  each particle runs through per time step, while this generic base class
  has no steps at all.
  
  The type is a specialisation of Peano's particle as it is provided via the 
  toolbox. In line with the docu there, you can use this particle and add
  further attributes via

      my_particle.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("v","Dimensions") )

  
    
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
