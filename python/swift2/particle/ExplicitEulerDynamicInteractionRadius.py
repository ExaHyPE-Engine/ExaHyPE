# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4

from abc import ABC


class ExplicitEulerDynamicInteractionRadius(Particle):
    """

    Extension of the explicit Euler to support dynamic cut-off radius
    adoption. We try to keep a constant number of particles.


    ## Attributes

    name: String
      To be in line with other conventions, I recommend you start with an
      uppercase letter. This has to be a valid C++ identifier, i.e. don't
      use any special symbols besides an underscore.


    @see swift2::kernels::adoptInteractionRadiusAndTriggerRerun()

    """

    def __init__(
        self,
        name,
        cfl_factor,
        initial_time_step_size,
        particle_particle_interaction_over_particle_sets_kernel="",
        touch_particles_of_set_first_time_kernel="",
        touch_particles_of_set_last_time_kernel="",
        number_of_interaction_partners=64,
        particles_per_cell=0,  # makes no sense, and should likely be forbidden
        min_h=0.005,
        max_h=0.3,
    ):
        super(ExplicitEulerDynamicInteractionRadius, self).__init__(
            name,
            particles_per_cell,
            min_h,
            max_h,
        )

        self.velocity = peano4.dastgen2.Peano4DoubleArray("v", "Dimensions")
        self.accelerator = peano4.dastgen2.Peano4DoubleArray("a", "Dimensions")
        self.data.add_attribute(self.velocity)
        self.data.add_attribute(self.accelerator)

        self.cfl_factor = cfl_factor
        self.initial_time_step_size = initial_time_step_size

        self.particle_particle_interaction_over_particle_sets_kernel = (
            particle_particle_interaction_over_particle_sets_kernel
        )
        self.touch_particles_of_set_first_time_kernel = (
            touch_particles_of_set_first_time_kernel
        )
        self.touch_particles_of_set_last_time_kernel = (
            touch_particles_of_set_last_time_kernel
        )
        self.number_of_interaction_partners = number_of_interaction_partners

        pass

    def algorithm_steps(self):
        """

        The explicit Euler basically consists of two steps per particle. We first
        determine the force. For this, we need access to the neighbours. The step
        solely alters the individual particle's state. In the next algorithm step,
        we need this state, as well as global data (the admissible time step size)
        to update position and velocity. We also determine the CFL condition here.

        """
        return [
            AlgorithmStep(
                name="DensityCalculation",
                dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
                effect=AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
                touch_vertex_first_time_kernel="",
                cell_kernel="""
                                         ::swift2::kernels::adoptInteractionRadiusAndTriggerRerun<globaldata::{}>( 
                                           localParticles, 
                                           activeParticles,
                                           {}
                                         );
                                         """.format(
                    self.name, self.number_of_interaction_partners
                ),
                touch_vertex_last_time_kernel="",
                prepare_traversal_kernel="globaldata::{}::getSpecies().clearRerunPreviousGridSweepFlag();".format(
                    self.name
                ),
                unprepare_traversal_kernel="globaldata::{}::getSpecies().allReduce();".format(
                    self.name
                ),
                includes="""
                                         #include "Constants.h"
                                         #include "swift2/kernels/ParticleCutOffRadiusCalculation.h"
                                         """,
            ),
            AlgorithmStep(
                name="ForceCalculation",
                dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
                effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
                touch_vertex_first_time_kernel=self.touch_particles_of_set_first_time_kernel,
                cell_kernel="""
                                         ::swift2::kernels::genericInteraction<globaldata::{}>( 
                                           localParticles, 
                                           activeParticles,
                                           [&] (globaldata::{}* localParticle, const globaldata::{} * const globalParticle) -> void {{
                                             {}
                                           }}
                                         );
                                         """.format(
                    self.name,
                    self.name,
                    self.name,
                    self.particle_particle_interaction_over_particle_sets_kernel,
                ),
                touch_vertex_last_time_kernel=self.touch_particles_of_set_last_time_kernel,
                includes="""
                                         #include "Constants.h"
                                         #include "swift2/kernels/ParticleParticleInteraction.h"
                                         """,
            ),
            AlgorithmStep(
                name="UpdatePositionAndVelocity",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
                touch_vertex_first_time_kernel="""::swift2::timestepping::resetMovedParticleMarker(localParticles);""",
                touch_vertex_last_time_kernel="""::swift2::timestepping::computeExplicitEulerWithGlobalTimeStepSize( localParticles );""",
                prepare_traversal_kernel="""::swift2::timestepping::computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity<globaldata::{}>({},{});""".format(
                    self.name, self.cfl_factor, self.initial_time_step_size
                ),
                includes="""
                                         #include "Constants.h"
                                         #include "swift2/timestepping/Euler.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                                         """,
            ),
            AlgorithmStep(
                name="ReduceGlobalQuantities",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.ALTER_GLOBAL_STATE,
                cell_kernel="""::swift2::statistics::reduceVelocityAndCutOffRadius( localParticles );""",
                prepare_traversal_kernel="""
                                         globaldata::{PARTICLE}::getSpecies().clearCutOffRadius();
                                         globaldata::{PARTICLE}::getSpecies().clearVelocity();
                                         """.replace(
                    "{PARTICLE}", self.name
                ),
                unprepare_traversal_kernel="""
                                         globaldata::{PARTICLE}::getSpecies().allReduce();
                                         globaldata::{PARTICLE}::getSpecies().setTimeStamp( globaldata::{PARTICLE}::getSpecies().getMinTimeStamp() + globaldata::{PARTICLE}::getSpecies().getMinTimeStepSize(), false );
                                         ::swift2::statistics::reportCutOffRadiusVTDt<globaldata::{PARTICLE}>( "{PARTICLE}" );
                                         """.replace(
                    "{PARTICLE}", self.name
                ),
                includes="""
                                         #include "Constants.h"
                                         #include "swift2/statistics/Reports.h"
                                         #include "swift2/statistics/Statistics.h"
                                         """,
            ),
        ]

    def initialisation_steps(self):
        """!

        No particular initialisation required

        """
        return []

    # @property
    # def readme_descriptor(self):
    #  assert False, "not yet written"
    #  return """ I will au aufgerufen werden"""
