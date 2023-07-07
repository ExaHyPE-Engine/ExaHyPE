# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4
import dastgen2
import peano4.dastgen2

from abc import ABC


class SPHLeapfrogFixedSearchRadius(Particle):
    """!

    SPH particle with fixed search radius

    Same as LeapfrogFixedSearchRadius but augmented for full SPH.
    Implements the Minimal SPH scheme.

    Simple particle with a fixed search radius h which moves according
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

    ## Precision

    By default, all particle attributes are modelled as doubles or arrays of
    doubles. This might be overly precise. You can alter the precision through
    our LLVM compiler modifications if you invoke the following commands on a
    particle species:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~
    particle.mass.valid_mantissa_bits         = args.mantissa_size
    particle.velocity.valid_mantissa_bits     = args.mantissa_size
    particle.acceleration.valid_mantissa_bits = args.mantissa_size
    particle.density.valid_mantissa_bits      = args.mantissa_size
    particle.pressure.valid_mantissa_bits     = args.mantissa_size
    particle.smoothL.valid_mantissa_bits      = args.mantissa_size
    particle.u.valid_mantissa_bits            = args.mantissa_size
    particle.uDot.valid_mantissa_bits         = args.mantissa_size
    particle.f.valid_mantissa_bits            = args.mantissa_size
    particle.wcount_dh.valid_mantissa_bits    = args.mantissa_size
    particle.rho_dh.valid_mantissa_bits       = args.mantissa_size
    particle.hleft.valid_mantissa_bits        = args.mantissa_size
    particle.hright.valid_mantissa_bits       = args.mantissa_size
    particle.smoothL_prev.valid_mantissa_bits = args.mantissa_size
    particle.wcount.valid_mantissa_bits       = args.mantissa_size
    particle.hDot.valid_mantissa_bits         = args.mantissa_size
    particle.residual.valid_mantissa_bits     = args.mantissa_size
    particle.balsara.valid_mantissa_bits      = args.mantissa_size
    particle.rot_v.valid_mantissa_bits        = args.mantissa_size
    particle.div_v.valid_mantissa_bits        = args.mantissa_size
    particle.v_sig_AV.valid_mantissa_bits     = args.mantissa_size
    particle.soundSpeed.valid_mantissa_bits   = args.mantissa_size
    particle.v_full.valid_mantissa_bits       = args.mantissa_size
    particle.u_full.valid_mantissa_bits       = args.mantissa_size
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    This is an example. You can also add ony a few of these fields, and picking
    a value for args.mantissa_size is obviously up to you. We refrain from
    illustrating that you can also alter the precision of the position and
    maximum search radius of a particle this way. This is on purpose: We have
    made bad experience with such modifications.


    ## Attributes

    name: String
    To be in line with other conventions, I recommend you start with an
    uppercase letter. This has to be a valid C++ identifier, i.e. don't
    use any special symbols besides an underscore.

    @see peano4::datamanagement::CellMarker
    @see peano4::datamanagement::VertexMarker

    """

    def __init__(
        self,
        name,
        dimensions_hydro=2,
        cfl_factor=0.1,
        initial_time_step_size=1e-4,
        constant_time_step_size=True,
        swift_project_namespace="SPH",
        particles_per_cell=0,  # makes no sense, and should likely be forbidden
        min_h=0.3,
        max_h=0.3,
        use_optimised_coalesced_memory_access_kernels = False
    ):
        super(SPHLeapfrogFixedSearchRadius, self).__init__(
            name,
            particles_per_cell,
            min_h,
            max_h,
        )

        # Initialize default parameters.
        # ---------------------------------------

        # This is an SPH scheme.
        self._is_sph = True
        self._isSPH_cpp = "true"
        # Hardcoded this for now until we can switch flavours.
        self._sph_flavour = "minimal"

        # Simulation properties
        self._cfl_factor = cfl_factor
        self._initial_time_step_size = initial_time_step_size
        self._constant_time_step_size = constant_time_step_size
        self._use_optimised_coalesced_memory_access_kernels = use_optimised_coalesced_memory_access_kernels

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

        # Project namespace to have consistent use of exported Constants in kernels
        # Not used right now.
        self._swift_project_namespace = swift_project_namespace

        # now set these parameters as dastgen attributes
        self.set_parameters()

        # Construct particle properties
        # --------------------------------

        # Baseline particle properties -------------------------------------------------
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

        # Artificial viscosity scheme --------------------------------------------------
        self.balsara = dastgen2.attributes.Double("balsara")
        if self._hydro_dimensions == 3:
            self.rot_v = peano4.dastgen2.Peano4DoubleArray("rot_v", "Dimensions")
        else:  # Curl is a scalar in lower dimensions
            self.rot_v = dastgen2.attributes.Double(
                "rot_v"
            )  # In 2D sims it only has 1 component (z)
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

        # Algorithm Steps ---------------------------------------------------------------
        self.algorithm_steps_dict = {}
        self._setup_algorithm_steps_dict()

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

        if self._constant_time_step_size:
            adjustTimeStepSize = "false"
        else:
            adjustTimeStepSize = "true"
        adjust_time_step_size_attr = dastgen2.attributes.Boolean(
            "adjustTimeStepSize", is_const_static=True, initval=adjustTimeStepSize
        )
        self.data.add_or_replace_attribute(adjust_time_step_size_attr)

        is_sph_attr = dastgen2.attributes.Boolean(
            "isSPH", is_const_static=True, initval=self._isSPH_cpp
        )
        self.data.add_or_replace_attribute(is_sph_attr)

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

    def _setup_algorithm_steps_dict(self):
        """
        Sets up a dict of the algorithm steps for this scheme
        so they can be accessed at various points.
        """

        flagBoundaryParticlesStep = AlgorithmStep(
            name="FlagBoundaryParticles",
            dependencies=AlgorithmStep.Dependencies.SELF,
            effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
            touch_vertex_first_time_kernel="""
                                       """,
            touch_vertex_last_time_kernel="""::swift2::kernels::flagBoundaryParticles(localParticles, {0}, {1}, {2});""".format(
                "::".join(self._swift_project_namespace) + "::HYDRO_PART_NUMBER",
                "::".join(self._swift_project_namespace) + "::DomainSize",
                "::".join(self._swift_project_namespace) + "::DomainOffset",
            ),
            includes="""
                                       #include "Constants.h"
                                       #include "swift2/kernels/ParticleParticleInteraction.h"
                                       #include "swift2/kernels/ParticleSelfInteraction.h"
                                       #include "swift2/kernels/kernel_hydro.h"
                                       #include "swift2/kernels/equation_of_state.h"
                                       """,
        )
        self.algorithm_steps_dict["FlagBoundaryParticles"] = flagBoundaryParticlesStep

        kick1 = AlgorithmStep(
            name="Kick1",
            dependencies=AlgorithmStep.Dependencies.SELF,
            effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
            touch_vertex_first_time_kernel="""::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
            touch_vertex_last_time_kernel="""::swift2::timestepping::computeLeapfrogKickWithGlobalTimeStepSize(
                      localParticles, 1, {} );""".format(
                self._isSPH_cpp
            ),
            prepare_traversal_kernel="""::swift2::timestepping::computeCFLTimeStepSizeSPH<globaldata::{}>();""".format(
                self.name,
            ),
            includes="""
                                       #include "Constants.h"
                                       #include "swift2/timestepping/Leapfrog.h"
                                       #include "swift2/timestepping/GlobalTimeStepping.h"
                                       #include "swift2/timestepping/TimeStepping.h"
                                       #include "swift2/kernels/kernel_hydro.h"
                                       #include "swift2/kernels/equation_of_state.h"
                                       """,
        )
        self.algorithm_steps_dict["Kick1"] = kick1

        kick1Extra = AlgorithmStep(
            name="Kick1Extra",
            dependencies=AlgorithmStep.Dependencies.SELF,
            effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
            touch_vertex_first_time_kernel="""::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
            touch_vertex_last_time_kernel="""::swift2::timestepping::computeLeapfrogKickExtraWithGlobalTimeStepSize(
                      localParticles );""",
            includes="""
                                       #include "Constants.h"
                                       #include "swift2/timestepping/Leapfrog.h"
                                       #include "swift2/timestepping/GlobalTimeStepping.h"
                                       #include "swift2/timestepping/TimeStepping.h"
                                       #include "swift2/kernels/kernel_hydro.h"
                                       #include "swift2/kernels/equation_of_state.h"
                                       """,
        )
        self.algorithm_steps_dict["Kick1Extra"] = kick1Extra

        drift = AlgorithmStep(
            name="Drift",
            dependencies=AlgorithmStep.Dependencies.SELF,
            effect=AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
            touch_vertex_first_time_kernel="""::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
            touch_vertex_last_time_kernel="""::swift2::timestepping::computeLeapfrogDriftWithGlobalTimeStepSize( localParticles );""",
            includes="""
                                       #include "Constants.h"
                                       #include "swift2/timestepping/Leapfrog.h"
                                       #include "swift2/timestepping/GlobalTimeStepping.h"
                                       #include "swift2/timestepping/TimeStepping.h"
                                       #include "swift2/kernels/kernel_hydro.h"
                                       #include "swift2/kernels/equation_of_state.h"
                                       """,
        )
        self.algorithm_steps_dict["Drift"] = drift

        predictHydro = AlgorithmStep(
            name="PredictHydro",
            dependencies=AlgorithmStep.Dependencies.SELF,
            effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
            touch_vertex_first_time_kernel="""::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
            touch_vertex_last_time_kernel="""::swift2::timestepping::computePredictHydro(
                      localParticles, {} );""",
            includes="""
                                       #include "Constants.h"
                                       #include "swift2/timestepping/Leapfrog.h"
                                       #include "swift2/timestepping/GlobalTimeStepping.h"
                                       #include "swift2/timestepping/TimeStepping.h"
                                       #include "swift2/kernels/kernel_hydro.h"
                                       #include "swift2/kernels/equation_of_state.h"
                                       """,
        )
        self.algorithm_steps_dict["PredictHydro"] = predictHydro

        densityWithConstantSearchRadius = AlgorithmStep(
            name="DensityLoopWithConstantSearchRadius",
            dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
            effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
            touch_vertex_first_time_kernel="""
                                       ::swift2::kernels::prepareDensityCalculation( localParticles );
                                       """,
            cell_kernel="""
                                       ::swift2::kernels::computeDensityWithConstantSearchRadius(
                                         localParticles, activeParticles
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
        )
        self.algorithm_steps_dict["Density"] = densityWithConstantSearchRadius

        if self._use_optimised_coalesced_memory_access_kernels:
          force = AlgorithmStep(
            name="ForceLoop",
            dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
            effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
            touch_vertex_first_time_kernel="""
                                       ::swift2::kernels::prepareHydroForceCalculation(localParticles);
                                       """,
            cell_kernel="""
                                       ::toolbox::particles::ensureThatParticleListsEncodeContinuousMemoryChunks(
                                         activeParticles,
                                         _numberOfActiveParticlesPerVertexAdded
                                       );
                                       ::toolbox::particles::ensureThatParticleListsEncodeContinuousMemoryChunks(
                                         particlesAssociatedWithLocalVertices,
                                         numberOfParticlesPerLocalVertex
                                       );
                                       ::swift2::kernels::computeHydroForce(
                                         marker,
                                         particlesAssociatedWithLocalVertices,
                                         numberOfParticlesPerLocalVertex,
                                         activeParticles,
                                         _numberOfActiveParticlesPerVertexAdded
                                       );
                                       """,
            touch_vertex_last_time_kernel="""
                                       ::swift2::kernels::endHydroForceCalculation(
                                           localParticles);
                                       """,
            includes="""
                                       #include "Constants.h"
                                       #include "swift2/kernels/ParticleParticleInteraction.h"
                                       #include "swift2/kernels/ParticleSelfInteraction.h"
                                       #include "swift2/kernels/kernel_hydro.h"
                                       #include "swift2/kernels/equation_of_state.h"
                                       #include "toolbox/particles/ParticleListValidation.h"
                                       """,
          )
        else:
          force = AlgorithmStep(
            name="ForceLoop",
            dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
            effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
            touch_vertex_first_time_kernel="""
                                       ::swift2::kernels::prepareHydroForceCalculation(localParticles);
                                       """,
            cell_kernel="""
                                       ::swift2::kernels::computeHydroForce(localParticles, activeParticles);
                                       """,
            touch_vertex_last_time_kernel="""
                                       ::swift2::kernels::endHydroForceCalculation(
                                           localParticles);
                                       """,
            includes="""
                                       #include "Constants.h"
                                       #include "swift2/kernels/ParticleParticleInteraction.h"
                                       #include "swift2/kernels/ParticleSelfInteraction.h"
                                       #include "swift2/kernels/kernel_hydro.h"
                                       #include "swift2/kernels/equation_of_state.h"
                                       """,
          )
        self.algorithm_steps_dict["Force"] = force

        kick2 = AlgorithmStep(
            name="Kick2",
            dependencies=AlgorithmStep.Dependencies.SELF,
            effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
            touch_vertex_first_time_kernel="""::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
            touch_vertex_last_time_kernel="""::swift2::timestepping::computeLeapfrogKickWithGlobalTimeStepSize(
                      localParticles, 2, {} );""".format(
                self._isSPH_cpp
            ),
            includes="""
                                       #include "Constants.h"
                                       #include "swift2/timestepping/Leapfrog.h"
                                       #include "swift2/timestepping/GlobalTimeStepping.h"
                                       #include "swift2/timestepping/TimeStepping.h"
                                       #include "swift2/kernels/kernel_hydro.h"
                                       #include "swift2/kernels/equation_of_state.h"
                                       """,
        )
        self.algorithm_steps_dict["Kick2"] = kick2

        kick2Extra = AlgorithmStep(
            name="KickExtra2",
            dependencies=AlgorithmStep.Dependencies.SELF,
            effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
            touch_vertex_first_time_kernel="""::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
            touch_vertex_last_time_kernel="""::swift2::timestepping::computeLeapfrogKickExtraWithGlobalTimeStepSize(localParticles);""",
            includes="""
                                       #include "Constants.h"
                                       #include "swift2/timestepping/Leapfrog.h"
                                       #include "swift2/timestepping/GlobalTimeStepping.h"
                                       #include "swift2/timestepping/TimeStepping.h"
                                       #include "swift2/kernels/kernel_hydro.h"
                                       #include "swift2/kernels/equation_of_state.h"
                                       """,
        )
        self.algorithm_steps_dict["Kick2Extra"] = kick2Extra

        reduceGlobals = AlgorithmStep(
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
        )
        self.algorithm_steps_dict["ReduceGlobalQuantities"] = reduceGlobals

        return

    def algorithm_steps(self):
        """!

        Return algorithm steps: A list of AlgorithmStep objects to be executed in
        that order.

        Leapfrog consists basically of four steps per particle. We first
        determine the force. Then we update the velocity by half a timestep and move
        the particle by a full timestep. Then the force is re-computed and the
        second half of the velocity update is done.
        Some variations of this KDK form re-arrange the steps executed per timestep
        to avoid a second force loop.

        Definitions of the algorithm steps stored in the algorithm_steps_dict are
        placed in _setup_algorithm_steps_dict(self). The dict is set up during
        instantiation of this class.

        """

        return [
            self.algorithm_steps_dict["FlagBoundaryParticles"],
            self.algorithm_steps_dict["Kick1"],
            self.algorithm_steps_dict["Kick1Extra"],
            self.algorithm_steps_dict["Drift"],
            self.algorithm_steps_dict["PredictHydro"],
            self.algorithm_steps_dict["Density"],
            self.algorithm_steps_dict["Force"],
            self.algorithm_steps_dict["Kick2"],
            self.algorithm_steps_dict["Kick2Extra"],
            self.algorithm_steps_dict["ReduceGlobalQuantities"],
        ]

    @property
    def hydro_dimensions(self):
        """
        Forbid users to modify hydro dimensions on-the-fly.
        """
        return self._hydro_dimensions

    @hydro_dimensions.setter
    def hydro_dimensions(self, hydro_dimensions):
        raise ValueError(
            "Can't change hydro_dimensions now. Modify it while instantiating the class."
        )

    @property
    def eta_factor(self):
        """

        Set the eta factor used by the SPH kernel to target a certain number of
        neighbour particles.
        The default value is eta = 1.2348.

        """
        return self._eta_factor

    @eta_factor.setter
    def eta_factor(self, eta_factor):
        if eta_factor <= 0:
            raise ValueError("SPH eta factor value not valid.")
        self._eta_factor = eta_factor

    @property
    def alpha_av(self):
        """

        Viscosity parameters of the Minimal SPH model. Default SWIFT values are
        alpha_av=0.8
        and beta_av=3.

        """
        return self._alpha_av

    @alpha_av.setter
    def alpha_av(self, alpha_av):
        if alpha_av <= 0:
            raise ValueError("Value for the alpha_av viscosiy parameter not valid.")
        self._alpha_av = alpha_av

    @property
    def beta_av(self):
        return self._alpha_av

    @beta_av.setter
    def beta_av(self, beta_av):
        if beta_av <= 0:
            raise ValueError("Value for the beta_av viscosiy parameter not valid.")
        self._beta_av = beta_av

    @property
    def h_hydro_min(self):
        """

        Set the limits allowed for the SPH smoothing length. Notice that the max value
        is bounded by Peano's search radius as h_max = R_search_max = max_grid_size /
        gamma_k, where gamma_k is the SPH kernel factor that relates h and H.
        Typically gamma_k = 2, so h_max = max_grid_size / 2.

        """
        return self._h_hydro_min

    @h_hydro_min.setter
    def h_hydro_min(self, h_hydro_min):
        if h_hydro_min > self._h_hydro_max:
            raise ValueError("Min value of smoothing length larger than max allowed.")
        self._h_hydro_min = h_hydro_min

    @property
    def h_hydro_max(self):
        return self._h_hydro_max

    @h_hydro_max.setter
    def h_hydro_max(self, h_hydro_max):
        if h_hydro_max < self._h_hydro_min:
            raise ValueError("max value of smoothing length smaller than min allowed.")
        self._h_hydro_max = h_hydro_max

    @property
    def h_tolerance(self):
        """

        Tolerance for Newton-Raphson convergence criterion.

        """
        return self._h_tolerance

    @h_tolerance.setter
    def h_tolerance(self, h_tolerance):
        if h_tolerance > 1:
            raise ValueError("Value of h_tolerance cannot be larger than 1.")
        self._h_tolerance = h_tolerance

    @property
    def h_max_iterations(self):
        """
        Max number of iterations to adapt the SPH smoothing length
        """
        return self._h_max_iterations

    @h_max_iterations.setter
    def h_max_iterations(self, h_max_iterations):
        if h_max_iterations > 50:
            raise ValueError("Value of h_max_iterations is too large.")
        self._h_max_iterations = h_max_iterations

    @property
    def mantissa_size(self):
        """

        Set the mantissa size of doubles and Peano double arrays if
        we want to use reduced precission via Clang annotations. As a reference,
        floats have mantissa size = 23.

        """
        return self._mantissa_size

    @mantissa_size.setter
    def mantissa_size(self, mantissa_size):
        if mantissa_size < 0:
            raise ValueError("Mantissa size has to be larger than 0.")
        self._mantissa_size = mantissa_size

    def get_cpp_namespace_from_project_namespace(self):
        """

        Transform namespace into cpp format. Could be used to append namespace to
        constants in kernels (not used currently).

        """
        namespace = "::".join(self._swift_project_namespace) + "::"

        print(namespace)

    def initialisation_steps(self):
        return [
            self.algorithm_steps_dict["Density"],
        ]

    @property
    def readme_descriptor(self):
        return (
            super(SPHLeapfrogFixedSearchRadius, self).readme_descriptor
            + """

  Time integrator: SPH Leapfrog

  - search radius:                  fixed
  - CFL factor:                     """
            + str(self._cfl_factor)
            + """
  - dt_initial:                     """
            + str(self._initial_time_step_size)
            + """

    """
        )
