# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .Particle import Particle
from .Particle import AlgorithmStepDescriptor


class ExplicitEulerFixedInteractionRadius(Particle):
  """
  
  Base class for any particle in the project
    
  """
  def __init__(self, name):
    super( Particle, self ).__init__( name )
    pass


  def algorithm_steps(self):
    """
    
    The explicit Euler basically consists of two steps per particle. We first 
    determine 
    
    """
    return [ 
      ( AlgorithmStepDescriptor( 
          name="ForceCalculation",          
          dependencies = AlgorithmStepDescriptor.Dependencies.NEIGHBOURS_GLOBAL, 
          change_position_or_interaction_radius = False
        ), "" 
      ),
      ( AlgorithmStepDescriptor( 
          name="UpdatePositionAndVelocity", 
          dependencies = AlgorithmStepDescriptor.Dependencies.SELF,              
          change_position_or_interaction_radius = False
        ), "" 
      ),
    ]
