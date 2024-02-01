# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4

from abc import ABC


class ExplicitEulerDynamicSearchRadius(Particle):
    """

    Extension of the explicit Euler to support dynamic search radius
    adoption. We try to keep a constant number of particles.


    ## Attributes

    name: String
      To be in line with other conventions, I recommend you start with an
      uppercase letter. This has to be a valid C++ identifier, i.e. don't
      use any special symbols besides an underscore.


    @see swift2::kernels::adoptSearchRadiusAndTriggerRerun()

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
        super(ExplicitEulerDynamicSearchRadius, self).__init__(
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

        PARTICLE = self.name
        CFL_FACTOR = self.cfl_factor
        TIME_STEP_SIZE = self.initial_time_step_size
        NR_INTERACTION_PARTNERS = self.number_of_interaction_partners

        return [
            AlgorithmStep(
                name="DensityCalculation",
                dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
                effect=AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
                touch_vertex_first_time_kernel="",
                cell_kernel=f"""
                                         ::swift2::kernels::adoptSearchRadiusAndTriggerRerun<globaldata::{PARTICLE}>(
                                           localParticles,
                                           activeParticles,
                                           {NR_INTERACTION_PARTNERS}
                                         );
                """,
                touch_vertex_last_time_kernel="",
                prepare_traversal_kernel=f"globaldata::{PARTICLE}::getSpecies().clearRerunPreviousGridSweepFlag();",
                unprepare_traversal_kernel=f"globaldata::{PARTICLE}::getSpecies().allReduce();",
                includes="""
                                         #include "swift2/kernels/ParticleSearchRadiusCalculation.h"
                """,
            ),
            AlgorithmStep(
                name="ForceCalculation",
                dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
                effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
                touch_vertex_first_time_kernel=self.touch_particles_of_set_first_time_kernel,
                cell_kernel=f"""
                                         ::swift2::kernels::genericInteraction<globaldata::{PARTICLE}>(
                                           localParticles,
                                           activeParticles,
                                           [&] (globaldata::{PARTICLE}* localParticle, const globaldata::{PARTICLE} * const globalParticle) -> void {{
                                             {self.particle_particle_interaction_over_particle_sets_kernel}
                                           }}
                                         );
                """,
                touch_vertex_last_time_kernel=self.touch_particles_of_set_last_time_kernel,
                includes="""
                                         #include "swift2/kernels/ParticleParticleInteraction.h"
                """,
            ),
            AlgorithmStep(
                name="UpdatePositionAndVelocity",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
                touch_vertex_first_time_kernel="""::swift2::timestepping::resetMovedParticleMarker(assignedParticles);""",
                touch_vertex_last_time_kernel="""::swift2::timestepping::computeExplicitEulerWithGlobalTimeStepSize( assignedParticles );""",
                prepare_traversal_kernel=f"""::swift2::timestepping::computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity<globaldata::{PARTICLE}>({CFL_FACTOR},{TIME_STEP_SIZE});""",
                includes="""
                                         #include "swift2/timestepping/Euler.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
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
                """,
                unprepare_traversal_kernel=f"""
                                         globaldata::{PARTICLE}::getSpecies().allReduce();
                                         globaldata::{PARTICLE}::getSpecies().setTimeStamp( globaldata::{PARTICLE}::getSpecies().getMinTimeStamp() + globaldata::{PARTICLE}::getSpecies().getMinTimeStepSize(), false );
                                         ::swift2::statistics::reportSearchRadiusVTDt<globaldata::{PARTICLE}>( "{PARTICLE}" );
                """,
                includes="""
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
