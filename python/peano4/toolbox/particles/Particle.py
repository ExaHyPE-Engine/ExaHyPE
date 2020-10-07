# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.datamodel.DaStGen2

from peano4.datamodel.DaStGen2 import DaStGen2

import dastgen2


class Particle(DaStGen2):
  """
  
    Represent a single particle. This is a DaStGen2 wrapper, i.e. I 
    define a DaStGen object and add some particular fields that I always 
    need to administer the particles.
    
    If you use these particles, please do not add them to your use 
    definitions of the actions/observers. For pidt, you need a second
    ParticleSet and this one is used by the observers.
    
    You have to add the particle to your project though via 
    
    my_project.add_global_object
    
  
    :Attributes:
    
    name: String
      Name of the particle. Has to be a valid C++ class name.
  
  """
  def __init__(self,name):
    peano4.datamodel.DaStGen2.__init__( self, name )
    
    self.name = name
    self._position_identifier = "x"
    
    self.data.add_attribute( peano4.dastgen2.Peano4DoubleArray(self._position_identifier,"Dimensions") )
    self.data.add_attribute( dastgen2.attributes.Enumeration("MoveState",["New","NotMovedYet","Moved"]) )
