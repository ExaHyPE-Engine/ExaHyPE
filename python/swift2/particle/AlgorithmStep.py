# This file is part of the SWIFT2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from enum import Enum

from abc import abstractmethod


class AlgorithmStep( object ):
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
    SELF_GLOBAL        = 20
    NEIGHBOURS_GLOBAL  = 21,
    
    
  class Effect( Enum ):
    SELF               = 0,
    POSITION           = 1,
    GLOBAL             = 2
    POSITION_GLOBAL    = 21
    
    
  def __init__(self, 
               name, 
               dependencies: Dependencies, 
               effect: Effect,
               cell_kernel                    = None,
               touch_vertex_first_time_kernel = None, 
               touch_vertex_last_time_kernel  = None, 
               input_particles                = None,
               includes                       = ""
               ):
    """
    
    The algorithmic step description is a meta data object, i.e. it only
    holds properties.

    ## Parameters/attributes
    
    cell_kernel: C++ code (string)
      Kernel invocation what happens in a cell. Set to None if nothing
      is to be done per cell.
    
    touch_vertex_first_time_kernel: C++ code (string)
      Kernel invocation what happens when we read a vertex and its particles
      for the first time. Set to None if nothing is to be done.
        
    touch_vertex_last_time_kernel: C++ code (string)
      Kernel invocation what happens when we read a vertex and its particles
      for the last time. Set to None if nothing is to be done.
     
    input_particles: ParticleSet
      Switch to None in most cases. This fragment gives you the opportunity
      to couple the particles of one step with another particle species. If
      you hand in None, then we assume that the algorithm step updates the
      particles as input function from the particles.
      
    includes: String
      Additional include statements
      
    """
    self.name         = name
    self.dependencies = dependencies
    self.effect       = effect
    
    self.cell_kernel                    = cell_kernel
    self.touch_vertex_first_time_kernel = touch_vertex_first_time_kernel
    self.touch_vertex_last_time_kernel  = touch_vertex_last_time_kernel 
    
    self.input_particles = input_particles
    self.includes        = includes
 

  def __str__(self):
    return "({},{},{})".format( self.name, self.dependencies, self.effect )


