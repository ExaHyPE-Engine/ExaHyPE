# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4
import dastgen2
import peano4.dastgen2


class testDensityCalculation(Particle):
    """
    Test only the density calculation algorithmic step

    """

    def __init__(
        self,
        name,
        dimensions_hydro,
        cfl_factor,
        initial_time_step_size,
        particles_per_cell=5,
        min_h=0.6,
        max_h=0.5,
    ):
        super(testDensityCalculation, self).__init__(
            name,
            particles_per_cell,
            min_h,
            max_h,
        )

        # Simulation properties
        self._cfl_factor = cfl_factor
        self._initial_time_step_size = initial_time_step_size

        self.declare_sph_particle_attributes()

        # SPH parameters (default values)
        self._hydro_dimensions = dimensions_hydro

        self._eta_factor = 1.2348
        self._h_hydro_min = 1e-6
        self._h_hydro_max = 1.0 / 3.0 / 2  # Approximated value for coarsest grid level
        self._h_tolerance = 1e-4
        self._h_max_iterations = 10

        # Minimal SPH parameters
        self._alpha_av = 0.8
        self._beta_av = 3.0

        # now set these parameters as dastgen attributes
        self.set_parameters()

        pass

    def set_parameters(self):
        """
        This function translates "global" particle parameters which are
        constant throughout the simulation (like CFL factor, minimal time step
        size, viscosity parameters...) into dastgen attributes of the C++
        particle class.
        If you modify any of the attributes manually outside of the particle
        initialisation, e.g. by invoking

        ```
        particle = SPHLeapfrogFixedSearchRadius(initial_time_step_size=ABC, ...)
        particle.initial_time_step_size = XYZ
        ```

        you need to call this function manually so your changes propagate
        into the generated C++ files.
        """

        cfl_attr = dastgen2.attributes.Double(
            "cfl", is_const_static=True, initval=self._cfl_factor
        )
        self.data.add_or_replace_attribute(cfl_attr)

        initial_time_step_size_attr = dastgen2.attributes.Double(
            "initialTimeStepSize",
            is_const_static=True,
            initval=self._initial_time_step_size,
        )
        self.data.add_or_replace_attribute(initial_time_step_size_attr)

        #  if self._constant_time_step_size == True:
        #      adjustTimeStepSize = "false"
        #  else:
        #      adjustTimeStepSize = "true"
        #  adjust_time_step_size_attr = dastgen2.attributes.Boolean("adjustTimeStepSize", is_const_static=True, initval=adjustTimeStepSize)
        #  self.data.add_or_replace_attribute(adjust_time_step_size_attr)

        #  is_sph_attr = dastgen2.attributes.Boolean("isSPH", is_const_static=True, initval=self._isSPH_cpp)
        #  self.data.add_or_replace_attribute(is_sph_attr)

        hydro_dimensions_attr = dastgen2.attributes.Double(
            "hydroDimensions", is_const_static=True, initval=self._hydro_dimensions
        )
        self.data.add_or_replace_attribute(hydro_dimensions_attr)

        eta_factor_attr = dastgen2.attributes.Double(
            "etaFactor", is_const_static=True, initval=self._eta_factor
        )
        self.data.add_or_replace_attribute(eta_factor_attr)

        h_hydro_min_attr = dastgen2.attributes.Double(
            "smlMin", is_const_static=True, initval=self._h_hydro_min
        )
        self.data.add_or_replace_attribute(h_hydro_min_attr)

        h_hydro_max_attr = dastgen2.attributes.Double(
            "smlMax", is_const_static=True, initval=self._h_hydro_max
        )
        self.data.add_or_replace_attribute(h_hydro_max_attr)

        h_tolerance_attr = dastgen2.attributes.Double(
            "smlTolerance", is_const_static=True, initval=self._h_tolerance
        )
        self.data.add_or_replace_attribute(h_tolerance_attr)

        h_max_iterations_attr = dastgen2.attributes.Integer(
            "smlMaxIterations", is_const_static=True, initval=self._h_max_iterations
        )
        self.data.add_or_replace_attribute(h_max_iterations_attr)

        alpha_av_attr = dastgen2.attributes.Double(
            "alphaAV", is_const_static=True, initval=self._alpha_av
        )
        self.data.add_or_replace_attribute(alpha_av_attr)

        beta_av_attr = dastgen2.attributes.Double(
            "betaAV", is_const_static=True, initval=self._beta_av
        )
        self.data.add_or_replace_attribute(beta_av_attr)

        return

    def algorithm_steps(self):
        """

        Test only the density calculation
        """
        return [
            AlgorithmStep(
                name="DensityLoopWithConstantSearchRadius",
                dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
                effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
                touch_vertex_first_time_kernel="""
                                         ::swift2::kernels::prepareDensityCalculation( localParticles );
                                         """,
                cell_kernel="""
                                         ::swift2::kernels::computeDensityWithConstantSearchRadius(
                                           localParticles,
                                           activeParticles
                                         );
                                         """,
                touch_vertex_last_time_kernel="""
                                         ::swift2::kernels::endDensityCalculationWithConstantSearchRadius(localParticles);
                                         """,
                includes="""
                                         #include "Constants.h"
                                         #include "swift2/kernels/ParticleParticleInteraction.h"
                                         #include "swift2/kernels/ParticleSelfInteraction.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """,
            ),
            AlgorithmStep(
                name="ReduceGlobalQuantities",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.ALTER_GLOBAL_STATE,
                cell_kernel="""::swift2::statistics::reduceVelocityAndSearchRadius( localParticles );""",
                prepare_traversal_kernel="""
                                         globaldata::{PARTICLE}::getSpecies().clearSearchRadius();
                                         globaldata::{PARTICLE}::getSpecies().clearVelocity();
                                         """.replace(
                    "{PARTICLE}", self.name
                ),
                unprepare_traversal_kernel="""
                                         globaldata::{PARTICLE}::getSpecies().setTimeStamp( globaldata::{PARTICLE}::getSpecies().getMinTimeStamp() + globaldata::{PARTICLE}::getSpecies().getMinTimeStepSize(), false );
                                         ::swift2::statistics::reportSearchRadiusVTDt<globaldata::{PARTICLE}>( "{PARTICLE}" );
                                         """.replace(
                    "{PARTICLE}", self.name
                ),
                includes="""
                                         #include "Constants.h"
                                         #include "swift2/statistics/Reports.h"
                                         #include "swift2/statistics/Statistics.h"
                                         #include "swift2/kernels/kernel_hydro.h"
                                         #include "swift2/kernels/equation_of_state.h"
                                         """,
            ),
        ]

    def declare_sph_particle_attributes(self):
        """
        SPH Hydro particle attributes
        """

        # Baseline particle properties --------------------------------------------------------------
        self.mass = dastgen2.attributes.Double("mass")
        self.velocity = peano4.dastgen2.Peano4DoubleArray("v", "Dimensions")
        self.acceleration = peano4.dastgen2.Peano4DoubleArray("a", "Dimensions")
        self.data.add_attribute(self.mass)
        self.data.add_attribute(self.velocity)
        self.data.add_attribute(self.acceleration)

        # Basic SPH properties
        self.density = dastgen2.attributes.Double("density")
        self.pressure = dastgen2.attributes.Double("pressure")
        self.smoothL = dastgen2.attributes.Double("smoothingLength")
        self.u = dastgen2.attributes.Double("u")
        self.uDot = dastgen2.attributes.Double("uDot")
        self.data.add_attribute(self.density)
        self.data.add_attribute(self.pressure)
        self.data.add_attribute(self.smoothL)
        self.data.add_attribute(self.u)
        self.data.add_attribute(self.uDot)

        # Variable smoothing length scheme ---------------------------------------------
        self.f = dastgen2.attributes.Double("f")
        self.wcount_dh = dastgen2.attributes.Double("wcount_dh")
        self.rho_dh = dastgen2.attributes.Double("rho_dh")
        self.hleft = dastgen2.attributes.Double("hLeft")
        self.hright = dastgen2.attributes.Double("hRight")
        self.data.add_attribute(self.f)
        self.data.add_attribute(self.wcount_dh)
        self.data.add_attribute(self.rho_dh)
        self.data.add_attribute(self.hleft)
        self.data.add_attribute(self.hright)

        # Newton-Raphson iterations ----------------------------------------------------
        self.smoothL_prev = dastgen2.attributes.Double("smoothingLengthPrevious")
        self.wcount = dastgen2.attributes.Double("wcount")
        self.hDot = dastgen2.attributes.Double("hDot")
        self.residual = dastgen2.attributes.Double("residual")
        self.iteration_count = dastgen2.attributes.Integer("iterCount")
        self.has_converged = dastgen2.attributes.Boolean("hasConverged")
        self.data.add_attribute(self.smoothL_prev)
        self.data.add_attribute(self.wcount)
        self.data.add_attribute(self.hDot)
        self.data.add_attribute(self.residual)
        self.data.add_attribute(self.iteration_count)
        self.data.add_attribute(self.has_converged)

        # Artificial viscosity scheme -------------------------------------------------
        self.balsara = dastgen2.attributes.Double("balsara")
        self.rot_v = dastgen2.attributes.Double(
            "rot_v"
        )  # In 2D sims it only has 1 component (z)
        #    self.rot_v= peano4.dastgen2.Peano4DoubleArray("rot_v","Dimensions")
        self.div_v = dastgen2.attributes.Double("div_v")
        self.v_sig_AV = dastgen2.attributes.Double("v_sig_AV")
        self.soundSpeed = dastgen2.attributes.Double("soundSpeed")
        self.data.add_attribute(self.balsara)
        self.data.add_attribute(self.rot_v)
        self.data.add_attribute(self.div_v)
        self.data.add_attribute(self.v_sig_AV)
        self.data.add_attribute(self.soundSpeed)

        # Extended arrays for time integration consistency
        self.v_full = peano4.dastgen2.Peano4DoubleArray("v_full", "Dimensions")
        self.u_full = dastgen2.attributes.Double("u_full")
        self.data.add_attribute(self.v_full)
        self.data.add_attribute(self.u_full)

        # Extra ------------------------------------------------------------------------
        self.has_no_neighbours = dastgen2.attributes.Boolean("hasNoNeighbours")
        self.is_boundary_part = dastgen2.attributes.Boolean("isBoundaryParticle")
        self.Ncount = dastgen2.attributes.Integer("Ncount")  # integer number count
        self.part_id = dastgen2.attributes.Integer("partid")
        self.data.add_attribute(self.has_no_neighbours)
        self.data.add_attribute(self.is_boundary_part)
        self.data.add_attribute(self.Ncount)
        self.data.add_attribute(self.part_id)

    """
  Set the eta factor used by the SPH kernel to target a certain number
  of neighbour particles.
  The default value is eta = 1.235.
  """

    def set_eta_factor(self, etaFactor):
        self.etaFactor = etaFactor

    def initialisation_steps(self):
        """!

        No particular initialisation required

        """
        return []
