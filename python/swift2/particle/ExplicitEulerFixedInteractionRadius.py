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
  
  
  ## Force calculation
  
  particle_acceleration is a C++ string which defines a force.
  
  """
  def __init__(self, 
               name, 
               cfl_factor, 
               initial_time_step_size,
               particle_acceleration = ""):
    super( ExplicitEulerFixedInteractionRadius, self ).__init__( name )
    
    self.velocity    = peano4.dastgen2.Peano4DoubleArray("v","Dimensions")
    self.accelerator = peano4.dastgen2.Peano4DoubleArray("a","Dimensions")
    self.data.add_attribute( self.velocity )
    self.data.add_attribute( self.accelerator )
    
    self.cfl_factor                = cfl_factor
    self.initial_time_step_size    = initial_time_step_size
    
    self.particle_acceleration     = particle_acceleration
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
        name                           = "ForceCalculation",          
        dependencies                   = AlgorithmStep.Dependencies.NEIGHBOURS, 
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE, 
        cell_kernel                    = """
                                         ::swift2::kernels::genericInteraction<globaldata::{}>( 
                                           localParticles, 
                                           activeParticles,
                                           [&] (globaldata::{}* localParticle, const globaldata::{} const * globalParticle) -> void {{
                                             {}
                                           }}
                                         );
                                         """.format(self.name, self.name, self.name, self.particle_acceleration),
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/kernels/ParticleParticleInteraction.h"
                                         """,
      ),
      AlgorithmStep( 
        name                           = "UpdatePositionAndVelocity", 
        dependencies                   = AlgorithmStep.Dependencies.SELF,              
        effect                         = AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
        touch_vertex_first_time_kernel = """::swift2::timestepping::resetMovedParticleMarker(localParticles);""",       
        #
        # Machen wir das nicht im touchLast?
        # 
        touch_vertex_last_time_kernel  = """::swift2::timestepping::computeExplicitEulerWithGlobalTimeStepSize( localParticles );""",
        prepare_traversal_kernel       = """::swift2::timestepping::computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity<globaldata::{}>({},{});""".format(self.name, self.cfl_factor,self.initial_time_step_size),
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/timestepping/Euler.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                                         """),
      AlgorithmStep( 
        name                           = "ReduceGlobalQuantities", 
        dependencies                   = AlgorithmStep.Dependencies.SELF,              
        effect                         = AlgorithmStep.Effect.ALTER_GLOBAL_STATE,
        cell_kernel                    = """::swift2::statistics::reduceVelocityAndCutOffRadius( localParticles );""",
        prepare_traversal_kernel       = """
                                         globaldata::{PARTICLE}::getSpecies().clearCutOffRadius();
                                         globaldata::{PARTICLE}::getSpecies().clearVelocity();
                                         """.replace( "{PARTICLE}", self.name ),
        unprepare_traversal_kernel     = """
                                         globaldata::{PARTICLE}::getSpecies().setTimeStamp( globaldata::{PARTICLE}::getSpecies().getMinTimeStamp() + globaldata::{PARTICLE}::getSpecies().getMinTimeStepSize(), false );
                                         ::swift2::statistics::reportCutOffRadiusVTDt<globaldata::{PARTICLE}>( "{PARTICLE}" );
                                         """.replace( "{PARTICLE}", self.name ),
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/statistics/Reports.h"
                                         #include "swift2/statistics/Statistics.h"
                                         """              
      ),
    ]

