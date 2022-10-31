# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4.toolbox.particles

from enum import Enum

from abc import abstractmethod


class AlgorithmStepDescriptor( object ):
  """
  
  Defines the meta data around one algorithmic step per particle
  
  A sequence of these steps describes the particle lifecycle per time step. It 
  is Peano's/SWIFT's responsibility to arrange these steps in grid sweeps and 
  to identify concurrency.  
  
  """
    
  class Dependencies( Enum ):
    """
    
    An algorithmic step for one particle can depend on different 
    pre-conditions:
    
    - It can only depend on its own previous step (SELF);
    - It can depend on the neighbours within a particle's h-radius 
      (NEIGHBOURS);
    - It can depend upon a global synchronisation of the previous step,
      i.e. it can can require that the previous step has terminated on all the
      particles of this kind;
      
    """
    SELF               = 0,
    NEIGHBOURS         = 1,
    GLOBAL             = 2,
    NEIGHBOURS_GLOBAL  = 21
    
    
    
  def __init__(self, name, dependencies: Dependencies, change_position_or_interaction_radius ):
    """
    
    The algorithmic step description is a meta data object, i.e. it only
    holds properties.

    """
    self.name         = name
    self.dependencies = dependencies
    self.change_position_or_interaction_radius = change_position_or_interaction_radius



class Particle(peano4.toolbox.particles.Particle):
  """
  
  Base class for any particle in the project
    
  """
  def __init__(self, name):
    super( Particle, self ).__init__( name )
    self.data.add_attribute( dastgen2.attributes.Enumeration("MoveState",["New","NotMovedYet","Moved"]) )
    pass


  def algorithm_steps(self):
    """
    
    Return tuples of algorithm descriptors and method invocations (plain C++
    strings). 
    
    """
    assert False, "Should be overwritten by subclass"
    return []
