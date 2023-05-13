# This file is part of the SWIFT2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle      import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4

from abc import ABC


class ExplicitEulerFixedInteractionRadius(Particle):
  """!
  
Simple particle with fixed interaction radius subject to explicit Euler
  
Simple particle with a fixed interaction radius h which moves according
to an explicit Euler. By default, the Euler uses global time stepping, 
i.e. the combination of maximum velocity and minimal mesh size determines
the time step size of the subsequent time step. The term fixed means that
the interaction radius does not change throughout the time step. Nothing
stops you however to adopt the interaction radius in-between any two 
time steps. However, only change the radius in a step which is marked with
the AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS.


Besides the default variables x and h, the particle has the following properties:
  
    - a vector a which is the acceleration;
    - a vector v which is the velocity.

You can add further properties via
    
      myparticle.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("myFancyArray","Dimensions") )
 
in your code. Or you can create a subclass which adds additional fields 
after it has called the baseline constructor.
  
  
## Force calculation
  
 particle_particle_interaction_over_particle_sets_kernel is a C++ string which defines a force between two
 particles. It has access to three important objects:
 
 - localParticles is a container (list) over pointers to local particles
 - activeParticles is a container (list) over pointers to active particles
 - marker is a cell marker which identifies the current cell.
 
 Please consult the guidebook for a definition of local and active particles
 but take into account that the local particles always are a subset of the 
 active particles.
 
 Besides the actual particle-to-particle calculation, i.e. a force 
 calculation, users can also provide kernels that kick in when you touch 
 particles for the first time before you actually compute any 
 particle-particle interaction, and there is a plug-in point what you do 
 just once the particle-particle interaction has terminated. The latter 
 point is reached before we do the actual time stepping. In both plug-in
 points, you have a vertex marker which gives you the position of the 
 vertex to which a particular is associated, and you have the localParticles.
 This is a vector of pointers in this particular case.
 

 ## Attributes
 
 name: String
   To be in line with other conventions, I recommend you start with an 
   uppercase letter. This has to be a valid C++ identifier, i.e. don't 
   use any special symbols besides an underscore.
  


 @see peano4::datamanagement::CellMarker
 @see peano4::datamanagement::VertexMarker
  
  """
  def __init__(self, 
               name, 
               cfl_factor, 
               initial_time_step_size,
               particle_particle_interaction_over_particle_sets_kernel = "",
               touch_particles_of_set_first_time_kernel = "",
               touch_particles_of_set_last_time_kernel  = "",
               particles_per_cell = 0, # makes no sense, and should likely be forbidden
               min_h = 0.005,
               max_h = 0.3,
               ):
    super( ExplicitEulerFixedInteractionRadius, self ).__init__( name ,
                                                                 particles_per_cell,
                                                                 min_h,
                                                                 max_h,
                                                                )
    
    self.velocity    = peano4.dastgen2.Peano4DoubleArray("v","Dimensions")
    self.accelerator = peano4.dastgen2.Peano4DoubleArray("a","Dimensions")
    self.data.add_attribute( self.velocity )
    self.data.add_attribute( self.accelerator )
    
    self.cfl_factor                = cfl_factor
    self.initial_time_step_size    = initial_time_step_size
    
    self.particle_particle_interaction_over_particle_sets_kernel = particle_particle_interaction_over_particle_sets_kernel
    self.touch_particles_of_set_first_time_kernel                = touch_particles_of_set_first_time_kernel 
    self.touch_particles_of_set_last_time_kernel                 = touch_particles_of_set_last_time_kernel

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
        touch_vertex_first_time_kernel = self.touch_particles_of_set_first_time_kernel,
        cell_kernel                    = """
                                         ::swift2::kernels::genericInteraction<globaldata::{}>( 
                                           localParticles, 
                                           activeParticles,
                                           [&] (globaldata::{}* localParticle, const globaldata::{} * const globalParticle) -> void {{
                                             {}
                                           }}
                                         );
                                         """.format(self.name, self.name, self.name, self.particle_particle_interaction_over_particle_sets_kernel),
        touch_vertex_last_time_kernel  = self.touch_particles_of_set_last_time_kernel,
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
                                         globaldata::{PARTICLE}::getSpecies().allReduce();
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


  def initialisation_steps(self):
    """!
    
    No particular initialisation required
    
    """
    return [] 

