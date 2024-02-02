# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4

from abc import ABC


class ExplicitEulerFixedSearchRadius(Particle):
    """!

    Simple particle with fixed search radius subject to explicit Euler

    Simple particle with a fixed search radius H which moves according
    to an explicit Euler. By default, the Euler uses global time stepping,
    i.e. the combination of maximum velocity and minimal mesh size determines
    the time step size of the subsequent time step. The term fixed means that
    the search radius does not change throughout the time step. Nothing
    stops you however to adopt the search radius in-between any two
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

    There is a dedicated algorithmic step to do all the force calculation. This
    step can be tailored to your needs by setting the strings

    - enter_cell_kernel
    - touch_particles_of_set_first_time_kernel
    - touch_particles_of_set_last_time_kernel

    Each of these is unset (None) by default, but you can make it hold any
    reasonable C++ string. The Swift2ish way to use them is ot use the
    iterators from ParticleSetIterators.h to run over the relevant particles.

    For the two vertex kernels, this would look similar to

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    touch_vertex_first_time_kernel = "::swift2::kernels::forAllParticles( marker, assignedParticles, myFunctor );"
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    For the cell, it resembles

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ::swift2::kernels::forAllParticlePairs( marker, localParticles, activeParticles, myFunction);
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    The documentation of the actual operators ParticleBinaryOperator and
    ParticleUnaryOperatorOnVertex for more documentation.


     ## Attributes

     name: String
       To be in line with other conventions, I recommend you start with an
       uppercase letter. This has to be a valid C++ identifier, i.e. don't
       use any special symbols besides an underscore.





    ## Attributes

    name: String
      To be in line with other conventions, I recommend you start with an
      uppercase letter. This has to be a valid C++ identifier, i.e. don't
      use any special symbols besides an underscore.

    @see peano4::datamanagement::CellMarker
    @see peano4::datamanagement::VertexMarker


     @see peano4::datamanagement::CellMarker
     @see peano4::datamanagement::VertexMarker

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
        super(ExplicitEulerFixedSearchRadius, self).__init__(
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
      return self._algorithm_steps[ "ForceCalculation" ].touch_vertex_first_time_kernel

    @touch_particles_of_set_first_time_kernel.setter
    def touch_particles_of_set_first_time_kernel(self, value):
      self._algorithm_steps[ "ForceCalculation" ].touch_vertex_first_time_kernel = value
    
    @property 
    def touch_particles_of_set_last_time_kernel(self):
      return self._algorithm_steps[ "ForceCalculation" ].touch_vertex_last_time_kernel

    @touch_particles_of_set_first_time_kernel.setter
    def touch_particles_of_set_last_time_kernel(self, value):
      self._algorithm_steps[ "ForceCalculation" ].touch_vertex_last_time_kernel = value
    
    @property 
    def cell_kernel(self):
      return self._algorithm_steps[ "ForceCalculation" ].cell_kernel

    @cell_kernel.setter
    def cell_kernel(self, value):
        self._algorithm_steps[ "ForceCalculation" ].cell_kernel = value
    
    def add_to_reduction(self, value):
      self._algorithm_steps["ReduceGlobalQuantities"].unprepare_traversal_kernel += value

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
            "UpdatePositionAndVelocity": AlgorithmStep(
                name="UpdatePositionAndVelocity",
                dependencies=AlgorithmStep.Dependencies.SELF,
                effect=AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
                touch_vertex_first_time_kernel=f"""::swift2::kernels::forAllParticles( marker, assignedParticles, ::swift2::timestepping::resetMovedParticleMarker<globaldata::{PARTICLE}> );""",
                touch_vertex_last_time_kernel=f"""
                auto oldParticlePositions = ::toolbox::particles::assignmentchecks::recordParticlePositions(assignedParticles);
                ::swift2::kernels::forAllParticles(marker, assignedParticles, ::swift2::timestepping::computeExplicitEulerWithGlobalTimeStepSize<globaldata::{PARTICLE}> );
                ::toolbox::particles::assignmentchecks::traceParticleMovements(assignedParticles, oldParticlePositions, _spacetreeId);
                """,
                prepare_traversal_kernel=f"""::swift2::timestepping::computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity<globaldata::{PARTICLE}>({CFL_FACTOR},{TIME_STEP_SIZE});""",
                includes="""
                                         #include "swift2/timestepping/Euler.h"
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
        """

        The explicit Euler basically consists of two steps per particle. We first
        determine the force. For this, we need access to the neighbours. The step
        solely alters the individual particle's state. In the next algorithm step,
        we need this state, as well as global data (the admissible time step size)
        to update position and velocity. We also determine the CFL condition here.
        So we

        """
        return [
            self._algorithm_steps["ForceCalculation"],
            self._algorithm_steps["UpdatePositionAndVelocity"],
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
            super(ExplicitEulerFixedSearchRadius, self).readme_descriptor
            + """

  Time integrator: Explicit Euler

  - search radius:                  fixed
  - CFL factor:                     """
            + str(self.cfl_factor)
            + """
  - dt_initial:                     """
            + str(self.initial_time_step_size)
            + """

    """
        )
