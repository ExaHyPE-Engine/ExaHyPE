# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4
import dastgen2
import peano4.dastgen2

from abc import ABC


class LeapfrogFixedSearchRadius(Particle):
    """

    Leapfrog ODE integrator

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

    Study swift2.particle.ExplicitEulerFixedSearchRadius for further
    information including all parameter documentation.

    """

    def __init__(
        self,
        name,
        cfl_factor,
        initial_time_step_size,
        enter_cell_kernel="",
        touch_particles_of_set_first_time_kernel="",
        touch_particles_of_set_last_time_kernel="",
        particles_per_cell=0,
        min_h=0.005,
        max_h=0.3,
    ):
        super(LeapfrogFixedSearchRadius, self).__init__(
            name,
            particles_per_cell,
            min_h,
            max_h,
        )

        self.cfl_factor = cfl_factor
        self.initial_time_step_size = initial_time_step_size

        # Particle attributes
        self.velocity = peano4.dastgen2.Peano4DoubleArray("v", "Dimensions")
        self.accelerator = peano4.dastgen2.Peano4DoubleArray("a", "Dimensions")
        self.part_id = dastgen2.attributes.Integer("partid", initval="0")
        self.data.add_attribute(self.velocity)
        self.data.add_attribute(self.accelerator)
        self.data.add_attribute(self.part_id)

        self.__setup_algorithm_steps()

        self.enter_cell_kernel = enter_cell_kernel
        self.touch_particles_of_set_first_time_kernel = (
            touch_particles_of_set_first_time_kernel
        )
        self.touch_particles_of_set_last_time_kernel = (
            touch_particles_of_set_last_time_kernel
        )
        pass

    @property
    def touch_particles_of_set_first_time_kernel(self):
        return self._algorithm_steps["ForceCalculation"].touch_vertex_first_time_kernel

    @touch_particles_of_set_first_time_kernel.setter
    def touch_particles_of_set_first_time_kernel(self, value):
        self._algorithm_steps["ForceCalculation"].touch_vertex_first_time_kernel = value

    @property
    def touch_particles_of_set_last_time_kernel(self):
        return self._algorithm_steps["ForceCalculation"].touch_vertex_last_time_kernel

    @touch_particles_of_set_first_time_kernel.setter
    def touch_particles_of_set_last_time_kernel(self, value):
        self._algorithm_steps["ForceCalculation"].touch_vertex_last_time_kernel = value

    @property
    def cell_kernel(self):
        return self._algorithm_steps["ForceCalculation"].cell_kernel

    @cell_kernel.setter
    def cell_kernel(self, value):
        self._algorithm_steps["ForceCalculation"].cell_kernel = value

    def add_to_reduction(self, value):
        self._algorithm_steps[
            "ReduceGlobalQuantities"
        ].unprepare_traversal_kernel += value

    def __setup_algorithm_steps(self):
        """!

        Create a repository of algorithmic steps which are then
        ordered into the actual time stepping sequence.

        """

        PARTICLE = self.name
        CFL_FACTOR = self.cfl_factor
        TIME_STEP_SIZE = self.initial_time_step_size

        self._algorithm_steps = {
            "ForceCalculation": AlgorithmStep(
                name="ForceCalculation",
                dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
                effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
                touch_vertex_first_time_kernel="",
                cell_kernel="",
                touch_vertex_last_time_kernel="",
            ),
            "Kick": AlgorithmStep(
                name="Kick1",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
                touch_vertex_first_time_kernel=f"""::swift2::kernels::forAllParticles( marker, assignedParticles, ::swift2::timestepping::leapfrogKickWithGlobalTimeStepSize<globaldata::{PARTICLE}> );""",
                prepare_traversal_kernel=f"""::swift2::timestepping::computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity<globaldata::{PARTICLE}>({CFL_FACTOR},{TIME_STEP_SIZE});""",
                includes="""
                                         #include "swift2/timestepping/Leapfrog.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                """,
            ),
            "Drift": AlgorithmStep(
                name="Drift",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
                touch_vertex_first_time_kernel=f"""::swift2::kernels::forAllParticles( marker, assignedParticles, ::swift2::timestepping::resetMovedParticleMarker<globaldata::{PARTICLE}> );""",
                touch_vertex_last_time_kernel=f"""
                auto oldParticlePositions = ::toolbox::particles::assignmentchecks::recordParticlePositions(assignedParticles);
                ::swift2::kernels::forAllParticles( marker, assignedParticles, ::swift2::timestepping::leapfrogDriftWithGlobalTimeStepSize<globaldata::{PARTICLE}> );
                ::toolbox::particles::assignmentchecks::traceParticleMovements(assignedParticles, oldParticlePositions, _spacetreeId);
                """,
                includes="""
                                         #include "swift2/timestepping/Leapfrog.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                """,
            ),
            "ReduceVelocityAndDetermineTimeStepSize": AlgorithmStep(
                name="ReduceVelocityAndDetermineTimeStepSize",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
                prepare_traversal_kernel=f"""::swift2::timestepping::computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity<globaldata::{PARTICLE}>({CFL_FACTOR},{TIME_STEP_SIZE});""",
                includes="""
                                         #include "swift2/timestepping/Euler.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                """,
            ),
            "ReduceGlobalQuantities": AlgorithmStep(
                name="ReduceGlobalQuantities",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.ALTER_GLOBAL_STATE,
                touch_vertex_first_time_kernel=f"""::swift2::kernels::forAllParticles( marker, assignedParticles, ::swift2::statistics::reduceVelocityAndSearchRadius<globaldata::{PARTICLE}> );""",
                prepare_traversal_kernel=f"""
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
        }

    def algorithm_steps(self):
        """!

        Create algorithm steps behind leapfrog

        Leapfrog consists basically of four steps per particle. We first
        determine the force. Then we update the velocity by half a timestep and move
        the particle by a full timestep. Then the force is re-computed and the
        second half of the velocity update is done.
        Some variations of this KDK form re-arrange the steps executed per timestep
        to avoid a second force loop.

        """
        return [
            self._algorithm_steps["ForceCalculation"],
            self._algorithm_steps["Kick"],
            self._algorithm_steps["Drift"],
            self._algorithm_steps["ForceCalculation"],
            self._algorithm_steps["Kick"],
            self._algorithm_steps["ReduceGlobalQuantities"],
        ]

    def initialisation_steps(self):
        """!

        No particular initialisation required, but we reduce once, so we get
        the initial stats right before we jump into the time stepping.

        """
        return [
            self._algorithm_steps["ReduceVelocityAndDetermineTimeStepSize"],
        ]

    @property
    def readme_descriptor(self):
        return (
            super(LeapfrogFixedSearchRadius, self).readme_descriptor
            + """

  Time integrator: Leapfrog

  - search radius:                  fixed
  - CFL factor:                     """
            + str(self.cfl_factor)
            + """
  - dt_initial:                     """
            + str(self.initial_time_step_size)
            + """

    """
        )
