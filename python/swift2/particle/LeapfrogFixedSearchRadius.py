# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4
import dastgen2
import peano4.dastgen2

from abc import ABC
import copy


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

    @param add_partid: Whether to add a particle ID.
    Normally, that is done in the Particle super class if PeanoDebug > 0.
    For certain tests however, a particle ID is required even in release mode.
    So switch this on if you need it and are building release mode.

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
        add_partid=False,
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
        self.data.add_attribute(self.velocity)
        self.data.add_attribute(self.accelerator)

        if add_partid:
            self.partid = dastgen2.attributes.Integer("partid", initval="0")
            self.data.add_or_replace_attribute(self.partid)

        self._setup_algorithm_steps_dict()
        self._setup_algorithm_steps()
        self._setup_initialisation_steps()

        self.enter_cell_kernel = enter_cell_kernel
        self.touch_particles_of_set_first_time_kernel = (
            touch_particles_of_set_first_time_kernel
        )
        self.touch_particles_of_set_last_time_kernel = (
            touch_particles_of_set_last_time_kernel
        )

        # Raises errors for 1D periodic BC test... leave it for now.
        #  self._add_dependency_checks()

        return

    @property
    def touch_particles_of_set_first_time_kernel(self):
        return self._algorithm_steps_dict[
            "ForceCalculation"
        ].touch_vertex_first_time_kernel

    @touch_particles_of_set_first_time_kernel.setter
    def touch_particles_of_set_first_time_kernel(self, value):
        self._algorithm_steps_dict[
            "ForceCalculation"
        ].touch_vertex_first_time_kernel = value

    @property
    def touch_particles_of_set_last_time_kernel(self):
        return self._algorithm_steps_dict[
            "ForceCalculation"
        ].touch_vertex_last_time_kernel

    @touch_particles_of_set_last_time_kernel.setter
    def touch_particles_of_set_last_time_kernel(self, value):
        self._algorithm_steps_dict[
            "ForceCalculation"
        ].touch_vertex_last_time_kernel = value

    @property
    def cell_kernel(self):
        return self._algorithm_steps_dict["ForceCalculation"].cell_kernel

    @cell_kernel.setter
    def cell_kernel(self, value):
        self._algorithm_steps_dict["ForceCalculation"].cell_kernel = value

    def add_to_reduction(self, value):
        self._algorithm_steps_dict[
            "ReduceGlobalQuantities"
        ].unprepare_traversal_kernel += value

    def algorithm_steps(self):
        # function needs to exist for parent class
        return self._algorithm_steps

    def initialisation_steps(self):
        # function needs to exist for parent class
        return self._initialisation_steps

    def _setup_algorithm_steps_dict(self):
        """!

        Create a repository of algorithmic steps which are then
        ordered into the actual time stepping sequence.

        """

        PARTICLE = self.name
        CFL_FACTOR = self.cfl_factor
        TIME_STEP_SIZE = self.initial_time_step_size

        self._algorithm_steps_dict = {
            "ForceCalculation": AlgorithmStep(
                name="ForceCalculation",
                dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
                effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
                touch_vertex_first_time_kernel="",
                cell_kernel="",
                touch_vertex_last_time_kernel="",
            ),
            "Kick1": AlgorithmStep(
                name="Kick1",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
                touch_vertex_first_time_kernel=f"""
                    // Template in python/swift2/particle/LeapfrogFixedSearchRadius.py
                    ::swift2::kernels::forAllParticles(marker,
                                                       assignedParticles,
                                                       numberOfCoalescedAssignedParticles,
                                                       ::swift2::timestepping::leapfrogKickWithGlobalTimeStepSize<globaldata::{PARTICLE}>,
                                                       ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::{PARTICLE}>);
                    // end template
                    """,
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
                touch_vertex_first_time_kernel=f"""
                    // Template in python/swift2/particle/LeapfrogFixedSearchRadius.py
                    ::swift2::kernels::forAllParticles(
                        marker,
                        assignedParticles,
                        numberOfCoalescedAssignedParticles,
                        ::swift2::timestepping::resetMovedParticleMarker<globaldata::{PARTICLE}>,
                        ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::{PARTICLE}>);
                    // end template
                    """,
                touch_vertex_last_time_kernel=f"""
                // Template in python/swift2/particle/LeapfrogFixedSearchRadius.py
                auto oldParticlePositions = ::toolbox::particles::assignmentchecks::recordParticlePositions(assignedParticles);

                ::swift2::kernels::forAllParticles(
                                                    marker,
                                                    assignedParticles,
                                                    numberOfCoalescedAssignedParticles,
                                                    ::swift2::timestepping::leapfrogDriftWithGlobalTimeStepSize<globaldata::{PARTICLE}>,
                                                    ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::{PARTICLE}>
                                                    );

                ::toolbox::particles::assignmentchecks::traceParticleMovements(assignedParticles, oldParticlePositions, marker.x(), marker.h(), _spacetreeId);
                // end template
                """,
                includes="""
                                         #include "swift2/timestepping/Leapfrog.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                """,
                cell_kernel_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep_FutureStagesMayUpdate",
                touch_vertex_first_time_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_FutureStagesMayUpdate",
                touch_vertex_last_time_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce",
            ),
            "Kick2": AlgorithmStep(
                name="Kick2",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
                touch_vertex_first_time_kernel=f"""
                    // Template in python/swift2/particle/LeapfrogFixedSearchRadius.py
                    ::swift2::kernels::forAllParticles(
                        marker,
                        assignedParticles,
                        numberOfCoalescedAssignedParticles,
                        ::swift2::timestepping::resetMovedParticleMarker<globaldata::{PARTICLE}>,
                        ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::{PARTICLE}>);

                    ::swift2::kernels::forAllParticles(marker,
                                                       assignedParticles,
                                                       numberOfCoalescedAssignedParticles,
                                                       ::swift2::timestepping::leapfrogKickWithGlobalTimeStepSize<globaldata::{PARTICLE}>,
                                                       ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::{PARTICLE}>);
                    // end template
                    """,
                includes="""
                                         #include "swift2/timestepping/Leapfrog.h"
                                         #include "swift2/timestepping/GlobalTimeStepping.h"
                                         #include "swift2/timestepping/TimeStepping.h"
                """,
                touch_vertex_first_time_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep",
                cell_kernel_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep",
            ),
            "ReduceVelocityAndDetermineTimeStepSize": AlgorithmStep(
                name="ReduceVelocityAndDetermineTimeStepSize",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.ALTER_GLOBAL_STATE,
                prepare_traversal_kernel=f"""
                    // Template in python/swift2/particle/LeapfrogFixedSearchRadius.py
                    ::swift2::timestepping::computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity<globaldata::{PARTICLE}>({CFL_FACTOR},{TIME_STEP_SIZE});
                    // end template
                    """,
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
                touch_vertex_first_time_kernel=f"""
                    // Template in python/swift2/particle/LeapfrogFixedSearchRadius.py
                    ::swift2::kernels::forAllParticles(
                        marker,
                        assignedParticles,
                        numberOfCoalescedAssignedParticles,
                        ::swift2::statistics::reduceVelocityAndSearchRadius<globaldata::{PARTICLE}>,
                        ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::{PARTICLE}>);
                    // end template
                    """,
                prepare_traversal_kernel=f"""
                    // Template in python/swift2/particle/LeapfrogFixedSearchRadius.py
                    globaldata::{PARTICLE}::getSpecies().clearSearchRadius();
                    globaldata::{PARTICLE}::getSpecies().clearVelocity();
                    // end template
                    """,
                unprepare_traversal_kernel=f"""
                    // Template in python/swift2/particle/LeapfrogFixedSearchRadius.py
                    globaldata::{PARTICLE}::getSpecies().allReduce();
                    globaldata::{PARTICLE}::getSpecies().setTimeStamp( globaldata::{PARTICLE}::getSpecies().getMinTimeStamp() + globaldata::{PARTICLE}::getSpecies().getMinTimeStepSize(), false );
                    ::swift2::statistics::reportSearchRadiusVTDt<globaldata::{PARTICLE}>( "{PARTICLE}" );
                    // end template
                    """,
                includes="""
                    #include "swift2/statistics/Reports.h"
                    #include "swift2/statistics/Statistics.h"
                """,
            ),
            "FirstInitParticle": AlgorithmStep(
                name="FirstInitParticle",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
                touch_vertex_last_time_kernel="",
                includes="",
                cell_kernel_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep_FutureStagesMayUpdate",
                touch_vertex_first_time_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_FutureStagesMayUpdate",
                touch_vertex_last_time_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce",
            ),
        }

    def _setup_algorithm_steps(self):
        """!

        Create algorithm steps behind leapfrog

        Leapfrog consists basically of four steps per particle. We first
        determine the force. Then we update the velocity by half a timestep and move
        the particle by a full timestep. Then the force is re-computed and the
        second half of the velocity update is done.
        Some variations of this KDK form re-arrange the steps executed per timestep
        to avoid a second force loop.

        """

        steps = [
            self._algorithm_steps_dict["ForceCalculation"],
            self._algorithm_steps_dict["ReduceVelocityAndDetermineTimeStepSize"],
            self._algorithm_steps_dict["Kick1"],
            self._algorithm_steps_dict["Drift"],
            self._algorithm_steps_dict["Kick2"],
            self._algorithm_steps_dict["ReduceGlobalQuantities"],
        ]
        # make a copy of the steps, so if you modify them later (e.g. by
        # adding dependency checks), it won't modify the steps in the dict.
        self._algorithm_steps = [copy.deepcopy(i) for i in steps]
        return

    def _setup_initialisation_steps(self):
        """!

        Technically nothing really required, but

        """

        steps = [self._algorithm_steps_dict["FirstInitParticle"]]
        self._initialisation_steps = [copy.deepcopy(i) for i in steps]

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
