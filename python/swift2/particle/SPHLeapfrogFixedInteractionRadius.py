# This file is part of the SWIFT2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle      import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4
import dastgen2
import peano4.dastgen2

class SPHLeapfrogFixedInteractionRadius(Particle):
  """

  Same as LeapfrogFixedInteractionRadius but augmented for full SPH.
  Implements the Minimal SPH scheme.
  
  Simple particle with a fixed interaction radius h which moves according
  to leapfrog KDK scheme. 
  By default, it uses global time stepping, 
  i.e. the combination of maximum velocity and minimal mesh size determines
  the time step size of the subsequent time step. Besides the default 
  variables x and h, the particle has the following properties:
  
    - a vector a which is the acceleration;
    - a vector v which is the velocity.
    
  You can add further properties via
    
         myparticle.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("myFancyArray","Dimensions") )
 
  in your code. Or you can create a subclass which adds additional fields 
  after it has called the baseline constructor.

  You will need to add further properties for any SPH project.
  
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
  
  @see peano4::datamanagement::CellMarker
  @see peano4::datamanagement::VertexMarker
  
  """
  def __init__(self, 
               name, 
               dimensions_hydro,
               cfl_factor, 
               initial_time_step_size,
               constant_time_step_size,
               swift_project_namespace,
               sph_flavour = 'minimal',
               is_sph = True,
               particles_per_cell = 0, # makes no sense, and should likely be forbidden
               min_h = 0.3,
               max_h = 0.3,
               ):
    super( SPHLeapfrogFixedInteractionRadius , self ).__init__( name,
                                                                particles_per_cell,
                                                                min_h,
                                                                max_h,
                                                               )
    
    # Simulation properties
    self.cfl_factor                = cfl_factor
    self.initial_time_step_size    = initial_time_step_size

    # Whether we want constant dt
    if(constant_time_step_size==True): adjustTimeStepSize='true'
    else: adjustTimeStepSize='false'
    self.adjustTimeStepSize = adjustTimeStepSize

    # SPH check Boolean for Cpp kernels
    if(is_sph==True): isSPH='true'
    else: isSPH='false'
    self.is_sph = isSPH
                    
    self.sph_flavour = sph_flavour
    self.declare_sph_particle_attributes()

    # SPH parameters (default values)
    self.hydroDimensions = dimensions_hydro
    self.etaFactor = 1.235
    self.hTolerance = 1e-4
    self.hHydroMin = 1e-6
    self.hHydroMax = 0.2

    # Minimal SPH parameters
    self.alphaAV = 0.8
    self.betaAV = 3.0

    # Project namespace to have consistent use of exported Constants in kernels
    # Not used right now.
    self.swift_project_namespace = swift_project_namespace
    
    pass


  def algorithm_steps(self):
    """
    
    Leapfrog consists basically of four steps per particle. We first 
    determine the force. Then we update the velocity by half a timestep and move
    the particle by a full timestep. Then the force is re-computed and the
    second half of the velocity update is done.
    Some variations of this KDK form re-arrange the steps executed per timestep
    to avoid a second force loop.
    """
    return [ 
      AlgorithmStep( 
        name                           = "FlagBoundaryParticles",          
        dependencies                   = AlgorithmStep.Dependencies.SELF, 
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE, 
        touch_vertex_first_time_kernel = """
                                         """,
        touch_vertex_last_time_kernel  =
                    """::swift2::kernels::flagBoundaryParticles<globaldata::{}>(
                        localParticles );""".format(self.name),
 
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/kernels/ParticleParticleInteraction.h"
                                         #include "swift2/kernels/ParticleSelfInteraction.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """,
      ),
      AlgorithmStep( 
        name                           = "DensityLoopWithConstantSmoothingLengthInitial",          
        dependencies                   = AlgorithmStep.Dependencies.NEIGHBOURS, 
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE, 
        touch_vertex_first_time_kernel = """
                                         ::swift2::kernels::prepareDensityCalculation<globaldata::{}>( localParticles );
                                         """.format(self.name),

        cell_kernel                    = """
                                         ::swift2::kernels::computeDensity<globaldata::{}>( 
                                           localParticles, 
                                           activeParticles,
                                           {}
                                         );
                                         """.format(self.name,
                                                    self.hydroDimensions),

        touch_vertex_last_time_kernel  = """
                                         ::swift2::kernels::endDensityCalculationWithConstantSmoothingLength<globaldata::{}>(
                                             localParticles,
                                             {},
                                             {},
                                             {}
                                             );
                                         """.format(self.name,
                                                    self.hydroDimensions,
                                                    self.etaFactor,
                                                    self.alphaAV),
 
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/kernels/ParticleParticleInteraction.h"
                                         #include "swift2/kernels/ParticleSelfInteraction.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """,
      ),
      AlgorithmStep( 
        name                           = "ForceLoopInitial",          
        dependencies                   = AlgorithmStep.Dependencies.NEIGHBOURS, 
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE, 
        touch_vertex_first_time_kernel = """
                                         ::swift2::kernels::prepareHydroForceCalculation<globaldata::{}>( localParticles );
                                         """.format(self.name),

        cell_kernel                    = """
                                         ::swift2::kernels::computeHydroForce<globaldata::{}>( 
                                           localParticles, 
                                           activeParticles,
                                           {},
                                           {}
                                         );
                                         """.format(self.name,
                                                    self.hydroDimensions,
                                                    self.betaAV),

        touch_vertex_last_time_kernel  = """
                                         ::swift2::kernels::endHydroForceCalculation<globaldata::{}>(
                                             localParticles,
                                             {}
                                             );
                                         """.format(self.name,
                                                    self.hydroDimensions),
 
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/kernels/ParticleParticleInteraction.h"
                                         #include "swift2/kernels/ParticleSelfInteraction.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """,
      ),
      AlgorithmStep( 
        name                           = "Kick1", 
        dependencies                   = AlgorithmStep.Dependencies.SELF,              
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_first_time_kernel = """::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
        touch_vertex_last_time_kernel  =
                    """::swift2::timestepping::computeLeapfrogKickWithGlobalTimeStepSize(
                        localParticles, 1, {} );""".format(self.is_sph),
        prepare_traversal_kernel       =
                    """::swift2::timestepping::computeCFLTimeStepSizeSPH<globaldata::{}>({},{},{});""".format(self.name,
                                                                                                           self.cfl_factor,self.initial_time_step_size,
                                                                                                           self.adjustTimeStepSize),
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/timestepping/Leapfrog.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """
      ),
      AlgorithmStep( 
        name                           = "KickExtra1", 
        dependencies                   = AlgorithmStep.Dependencies.SELF,              
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_first_time_kernel = """::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
        touch_vertex_last_time_kernel  =
                    """::swift2::timestepping::computeLeapfrogKickExtraWithGlobalTimeStepSize(
                        localParticles );""",
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/timestepping/Leapfrog.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """
      ),
      AlgorithmStep( 
        name                           = "Drift", 
        dependencies                   = AlgorithmStep.Dependencies.SELF,              
        effect                         = AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
        touch_vertex_first_time_kernel = """::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
        touch_vertex_last_time_kernel  = """::swift2::timestepping::computeLeapfrogDriftWithGlobalTimeStepSize( localParticles );""",
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/timestepping/Leapfrog.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """
      ),
      AlgorithmStep( 
        name                           = "PredictHydro", 
        dependencies                   = AlgorithmStep.Dependencies.SELF,              
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_first_time_kernel = """::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
        touch_vertex_last_time_kernel  =
                    """::swift2::timestepping::computePredictHydro(
                        localParticles, {} );""".format(self.hydroDimensions),
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/timestepping/Leapfrog.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """
      ),
      AlgorithmStep( 
        name                           = "DensityLoopWithConstantSmoothingLength",          
        dependencies                   = AlgorithmStep.Dependencies.NEIGHBOURS, 
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE, 
        touch_vertex_first_time_kernel = """
                                         ::swift2::kernels::prepareDensityCalculation<globaldata::{}>( localParticles );
                                         """.format(self.name),

        cell_kernel                    = """
                                         ::swift2::kernels::computeDensity<globaldata::{}>( 
                                           localParticles, 
                                           activeParticles,
                                           {}
                                         );
                                         """.format(self.name,
                                                    self.hydroDimensions),

        touch_vertex_last_time_kernel  = """
                                         ::swift2::kernels::endDensityCalculationWithConstantSmoothingLength<globaldata::{}>(
                                             localParticles,
                                             {},
                                             {},
                                             {}
                                             );
                                         """.format(self.name,
                                                    self.hydroDimensions,
                                                    self.etaFactor,
                                                    self.alphaAV),
 
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/kernels/ParticleParticleInteraction.h"
                                         #include "swift2/kernels/ParticleSelfInteraction.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """,
      ),
      AlgorithmStep( 
        name                           = "ForceLoop",          
        dependencies                   = AlgorithmStep.Dependencies.NEIGHBOURS, 
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE, 
        touch_vertex_first_time_kernel = """
                                         ::swift2::kernels::prepareHydroForceCalculation<globaldata::{}>( localParticles );
                                         """.format(self.name),

        cell_kernel                    = """
                                         ::swift2::kernels::computeHydroForce<globaldata::{}>( 
                                           localParticles, 
                                           activeParticles,
                                           {},
                                           {}
                                         );
                                         """.format(self.name,
                                                    self.hydroDimensions,
                                                    self.betaAV),

        touch_vertex_last_time_kernel  = """
                                         ::swift2::kernels::endHydroForceCalculation<globaldata::{}>(
                                             localParticles,
                                             {}
                                             );
                                         """.format(self.name,
                                                    self.hydroDimensions),
 
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/kernels/ParticleParticleInteraction.h"
                                         #include "swift2/kernels/ParticleSelfInteraction.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """,
      ),
      AlgorithmStep( 
        name                           = "Kick2", 
        dependencies                   = AlgorithmStep.Dependencies.SELF,              
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_first_time_kernel = """::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
        touch_vertex_last_time_kernel  =
                    """::swift2::timestepping::computeLeapfrogKickWithGlobalTimeStepSize(
                        localParticles, 2, {} );""".format(self.is_sph),
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/timestepping/Leapfrog.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """
      ),
      AlgorithmStep( 
        name                           = "KickExtra2", 
        dependencies                   = AlgorithmStep.Dependencies.SELF,              
        effect                         = AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_first_time_kernel = """::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
        touch_vertex_last_time_kernel  =
                    """::swift2::timestepping::computeLeapfrogKickExtraWithGlobalTimeStepSize(
                        localParticles );""",
        includes                       = """
                                         #include "Constants.h"
                                         #include "swift2/timestepping/Leapfrog.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """
      ),
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
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """              
      ),
    ]

  def declare_sph_particle_attributes(self):
    """
    SPH Hydro particle attributes
    """

    # Baseline particle properties --------------------------------------------------------------
    self.mass             = dastgen2.attributes.Double("mass")
    self.velocity         = peano4.dastgen2.Peano4DoubleArray("v","Dimensions")
    self.acceleration     = peano4.dastgen2.Peano4DoubleArray("a","Dimensions")
    self.data.add_attribute( self.mass )
    self.data.add_attribute( self.velocity )
    self.data.add_attribute( self.acceleration )

    # Basic SPH properties
    self.density = dastgen2.attributes.Double("density")
    self.pressure= dastgen2.attributes.Double("pressure")
    self.smoothL = dastgen2.attributes.Double("smoothingLength")
    self.u       = dastgen2.attributes.Double("u")
    self.uDot    = dastgen2.attributes.Double("uDot")
    self.data.add_attribute( self.density )
    self.data.add_attribute( self.pressure )
    self.data.add_attribute( self.smoothL )
    self.data.add_attribute( self.u )
    self.data.add_attribute( self.uDot )

    # Variable smoothing length scheme ---------------------------------------------
    self.f        = dastgen2.attributes.Double("f")   
    self.wcount_dh= dastgen2.attributes.Double("wcount_dh")
    self.rho_dh   = dastgen2.attributes.Double("rho_dh")
    self.hleft    = dastgen2.attributes.Double("hLeft")
    self.hright   = dastgen2.attributes.Double("hRight")
    self.Omega    = dastgen2.attributes.Double("Omega")        # PHANTOM SPH (~1/f)
    self.data.add_attribute(self.f)
    self.data.add_attribute(self.wcount_dh)
    self.data.add_attribute(self.rho_dh)
    self.data.add_attribute(self.hleft)
    self.data.add_attribute(self.hright)
    self.data.add_attribute(self.Omega)

    # Newton-Raphson iterations ----------------------------------------------------
    self.smoothL_prev   = dastgen2.attributes.Double("smoothingLengthPrevious")
    self.wcount         = dastgen2.attributes.Double("wcount")
    self.hDot           = dastgen2.attributes.Double("hDot")
    self.residual       = dastgen2.attributes.Double("residual")
    self.iteration_count= dastgen2.attributes.Integer("iterCount")
    self.has_converged  = dastgen2.attributes.Boolean("hasConverged")
    self.data.add_attribute(self.smoothL_prev)
    self.data.add_attribute(self.wcount)
    self.data.add_attribute(self.hDot)
    self.data.add_attribute(self.residual)
    self.data.add_attribute(self.iteration_count)
    self.data.add_attribute(self.has_converged)

    # Artificial viscosity scheme -------------------------------------------------
    self.balsara   = dastgen2.attributes.Double("balsara")
    self.rot_v     = dastgen2.attributes.Double("rot_v")    # In 2D sims it only has 1 component (z)
#    self.rot_v= peano4.dastgen2.Peano4DoubleArray("rot_v","Dimensions")
    self.div_v     = dastgen2.attributes.Double("div_v")
    self.v_sig_AV  = dastgen2.attributes.Double("v_sig_AV")
    self.soundSpeed= dastgen2.attributes.Double("soundSpeed")
    self.data.add_attribute(self.balsara)
    self.data.add_attribute(self.rot_v)
    self.data.add_attribute(self.div_v)
    self.data.add_attribute(self.v_sig_AV)
    self.data.add_attribute(self.soundSpeed)

    # Extended arrays for time integration consistency
    self.v_full = peano4.dastgen2.Peano4DoubleArray("v_full","Dimensions")
    self.u_full = dastgen2.attributes.Double("u_full")
    self.data.add_attribute(self.v_full)
    self.data.add_attribute(self.u_full)

    # Extra ------------------------------------------------------------------------
    self.has_no_neighbours= dastgen2.attributes.Boolean("hasNoNeighbours")
    self.is_boundary_part = dastgen2.attributes.Boolean("isBoundaryParticle")
    self.time             = dastgen2.attributes.Double("time")
    self.Ncount           = dastgen2.attributes.Integer("Ncount")                         # integer number count
    self.part_id          = dastgen2.attributes.Integer("partid")
    self.data.add_attribute(self.has_no_neighbours)
    self.data.add_attribute(self.is_boundary_part)
    self.data.add_attribute(self.time)
    self.data.add_attribute(self.Ncount)
    self.data.add_attribute(self.part_id)


  def set_eta_factor(self, etaFactor):
    self.etaFactor = etaFactor

  def set_h_hydro_limits(self, hHydroMin, hHydroMax):
    self.hHydroMin = hHydroMin
    self.hHydroMax = hHydroMax

  """
  Transform namespace into cpp format. Could be used to append namespace to
  constants in kernels.
  """
  def get_cpp_namespace_from_project_namespace(self):
    namespace = ""
    for entry in self.swift_project_namespace:
      namespace = namespace + "::" + entry

    namespace = namespace + "::" 
    print(namespace)

