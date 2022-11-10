# This file is part of the SWIFT2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle      import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4
import dastgen2
import peano4.dastgen2

class testDensityCalculation(Particle):
  """
  Test only the density calculation algorithmic step
  
  """
  def __init__(self, 
               name, 
               cfl_factor, 
               initial_time_step_size,
               ):
    super( testDensityCalculation , self ).__init__( name )
    
    # Simulation properties
    self.cfl_factor                = cfl_factor
    self.initial_time_step_size    = initial_time_step_size

    self.declare_sph_particle_attributes()

    # SPH parameters (default values)
    self.hydroDimensions = 1
    self.kernelSupportRadius = 1.936492
    self.etaFactor = 1.235

    # Minimal SPH parameters
    self.alphaAV = 0.8
    self.betaAV = 3.0

    pass


  def algorithm_steps(self):
    """
    
    Test only the density calculation
    """
    return [ 
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
                                           {},
                                           {}
                                         );
                                         """.format(self.name,
                                                    self.kernelSupportRadius,
                                                    self.hydroDimensions),

        touch_vertex_last_time_kernel  = """
                                         ::swift2::kernels::endDensityCalculationWithConstantSmoothingLength<globaldata::{}>(
                                             localParticles,
                                             {},
                                             {},
                                             {},
                                             {}
                                             );
                                         """.format(self.name,
                                                    self.kernelSupportRadius,
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


