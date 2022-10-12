# This file is part of the SWIFT2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle      import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4

class ExplicitEulerFixedInteractionRadius(Particle):
  """
  
  Simple particle with a fixed interaction radius h which moves according
  to an explicit Euler. By default, the Euler uses global time stepping, 
  i.e. the combination of maximum velocity and minimal mesh size determines
  the time step size of the subsequent time step. Besides the default 
  variables x and h, the particle has the following properties:
  
    - a vector a which is the acceleration;
    - a vector v which is the velocity.
    
  You can add further properties via
    
         myparticle.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("myFancyArray","Dimensions") )
 
  in your code. Or you can create a subclass which adds additional fields 
  after it has called the baseline constructor.
  
  """
  def __init__(self, name):
    super( ExplicitEulerFixedInteractionRadius, self ).__init__( name )
    
    self.velocity    = peano4.dastgen2.Peano4DoubleArray("v","Dimensions")
    self.accelerator = peano4.dastgen2.Peano4DoubleArray("a","Dimensions")
    self.data.add_attribute( self.velocity )
    self.data.add_attribute( self.accelerator )
    pass


  def algorithm_steps(self):
    """
    
    The explicit Euler basically consists of two steps per particle. We first 
    determine the force. For this, we need access to the neighbours. The step
    solely alters the individual particle's state. In the next algorithm step,
    we need this state, as well as global data (the admissible time step size)
    to update position and velocity. We also determine the CFL condition here.
    So we 
    
    """
    return [ 
      AlgorithmStep( 
        name         = "ForceCalculation",          
        dependencies = AlgorithmStep.Dependencies.NEIGHBOURS, 
        effect       = AlgorithmStep.Effect.SELF, 
      ),
      AlgorithmStep( 
        name         = "UpdatePositionAndVelocity", 
        dependencies = AlgorithmStep.Dependencies.SELF_GLOBAL,              
        effect       = AlgorithmStep.Effect.POSITION_GLOBAL,
        touch_vertex_first_time_kernel = """
::swift2::resetParticleMarker(
  localParticles
);
""",        
        cell_kernel  = """
::swift2::computeExplicitEulerWithGlobalTimeStepSizeAndReduceNewMaximalVelocity(
  localParticles
);
        """,
        includes = """
#include "swift2/TimeStepping.h"
"""              
      ),
    ]
