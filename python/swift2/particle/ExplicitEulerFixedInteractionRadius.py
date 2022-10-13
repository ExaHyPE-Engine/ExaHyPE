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
  def __init__(self, name, cfl_factor, initial_time_step_size):
    super( ExplicitEulerFixedInteractionRadius, self ).__init__( name )
    
    self.velocity    = peano4.dastgen2.Peano4DoubleArray("v","Dimensions")
    self.accelerator = peano4.dastgen2.Peano4DoubleArray("a","Dimensions")
    self.data.add_attribute( self.velocity )
    self.data.add_attribute( self.accelerator )
    
    self.cfl_factor             = cfl_factor
    self.initial_time_step_size = initial_time_step_size
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

#In dieser Denkweise sollte es hier einen globalen Algorithmen-Schritt geben, der sich dann aber in das globale Event einklinkt.
#Dazu macht es Sinn, pro Species eine Subklasse zu erzeugen.
#Mir ist nicht klar, ob ich evtl komplett die _GLOBAL Varianten eliminieren kann    
    
    return [ 
      AlgorithmStep( 
        name                           = "ForceCalculation",          
        dependencies                   = AlgorithmStep.Dependencies.NEIGHBOURS, 
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE, 
        prepare_traversal_kernel       = """globaldata::{}::startTimeStep();""".format(self.name),
      ),
      AlgorithmStep( 
        name         = "UpdatePositionAndVelocity", 
        dependencies = AlgorithmStep.Dependencies.SELF,              
        effect       = AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
        touch_vertex_first_time_kernel = """::swift2::resetMovedParticleMarker(localParticles);""",        
#        cell_kernel                    = """::swift2::computeExplicitEulerWithGlobalTimeStepSize( localParticles );""",
        prepare_traversal_kernel       = """::swift2::computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity<globaldata::{}>({},{});""".format(self.name, self.cfl_factor,self.initial_time_step_size),
        includes = """
#include "swift2/TimeStepping.h"
"""              
      ),
      AlgorithmStep( 
        name         = "ReduceGlobalQuantities", 
        dependencies = AlgorithmStep.Dependencies.SELF,              
        effect       = AlgorithmStep.Effect.ALTER_GLOBAL_STATE,
        #clear?
        #touch_vertex_first_time_kernel = """::swift2::resetMovedParticleMarker(localParticles);""",        
        cell_kernel                    = """::swift2::reduceVelocityAndCutOffRadius( localParticles );""",
        prepare_traversal_kernel     = """
globaldata::{PARTICLE}::getSpecies().clearCutOffRadius();
globaldata::{PARTICLE}::getSpecies().clearVelocity();
""".replace( "{PARTICLE}", self.name ),
        unprepare_traversal_kernel     = """
globaldata::{PARTICLE}::getSpecies().setTimeStamp( globaldata::{PARTICLE}::getSpecies().getMinTimeStamp() + globaldata::{PARTICLE}::getSpecies().getMinTimeStepSize(), false );
logInfo("finishTimeStep()", "{PARTICLE}: cut-off-radius_{min}=" << globaldata::{PARTICLE}::getSpecies().getMinCutOffRadius() );
logInfo("finishTimeStep()", "{PARTICLE}: cut-off-radius_{max}=" << globaldata::{PARTICLE}::getSpecies().getMaxCutOffRadius() );
logInfo("finishTimeStep()", "{PARTICLE}: v_{min}=" << globaldata::{PARTICLE}::getSpecies().getMinVelocity() );
logInfo("finishTimeStep()", "{PARTICLE}: v_{max}=" << globaldata::{PARTICLE}::getSpecies().getMaxVelocity() );
logInfo("finishTimeStep()", "{PARTICLE}: t=" << globaldata::{PARTICLE}::getSpecies().getMinTimeStamp() );
logInfo("finishTimeStep()", "{PARTICLE}: dt=" << globaldata::{PARTICLE}::getSpecies().getMinTimeStepSize() );
""".replace( "{PARTICLE}", self.name ),
        includes = """
#include "swift2/TimeStepping.h"
#include "repositories/GlobalState.h"
"""              
      ),
    ]

#logInfo( "finishTimeStep()", "reportCutOffRadius();""".format( self.name ),
#logInfo("finishTimeStep()", "{{PARTICLE}}: t_{min}=" << globaldata::{{PARTICLE}}::getSpecies().getMinTimeStamp() );
#logInfo("finishTimeStep()", "{{PARTICLE}}: t_{max}=" << globaldata::{{PARTICLE}}::getSpecies().getMaxTimeStamp() );
#logInfo("finishTimeStep()", "{{PARTICLE}}: dt_{min}=" << globaldata::{{PARTICLE}}::getSpecies().getMinTimeStepSize() );
#logInfo("finishTimeStep()", "{{PARTICLE}}: dt_{max}=" << globaldata::{{PARTICLE}}::getSpecies().getMaxTimeStepSize() );
