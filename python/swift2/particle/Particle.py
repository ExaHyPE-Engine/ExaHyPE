# use, please see the copyright notice at www.peano-framework.org
import peano4.toolbox.particles

import dastgen2
from swift2.particle.AlgorithmStep import AlgorithmStep
from abc import abstractmethod
import copy


class Particle(peano4.toolbox.particles.Particle):
    """!

    Base class for any particle in the project

    You usually do not initialise this base class, but you pick one of the
    subclasses which define which time stepping scheme is realised by a
    particle. The time stepping scheme defines through which phases or steps
    each particle runs through per time step, while this generic base class
    has no steps at all.

    The type is a specialisation of Peano's particle as it is provided via the
    toolbox. In line with the docu there, you can use this particle and add
    further attributes via

        my_particle.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("v","Dimensions") )



    """

    class SpeciesAspect(object):
        """

        Every generated particle belongs to a species. I originally thought I'd
        use some kind of inheritance to model this, but actually it is easier to
        introduce a static field for the time being that stores the species
        information.

        """

        def __init__(self):
            pass

        def get_include(self):
            return """
#include "swift2/ParticleSpecies.h"
"""

        def set_model(self, data_model):
            self._data_model = data_model

        def get_attributes(self):
            return ""

        def get_method_declarations(self, full_qualified_name):
            return """
  static ::swift2::ParticleSpecies& getSpecies();
"""

        def get_implementation(self, full_qualified_name):
            return (
                """
::swift2::ParticleSpecies& """
                + full_qualified_name
                + """::getSpecies() {
  static ::swift2::ParticleSpecies species;
  return species;
}
"""
            )

    def __init__(
        self,
        name,
        particles_per_cell,
        min_h,
        max_h,
    ):
        """!

        Initialise the particle

        This is the baseclass for a particle, i.e. we only add the absolute minimum
        of information to a particle. As we inherit from the toolbox particle, we
        already have some attributes defined. These are the guys which are not
        explicitly visible from the code snippet below, i.e. they are introduced by
        the superclass constructor.

        Here's an overview of pre-defined attributes that each and every particle
        hosts:

        - A position which is a double vector with Dimension entries.
        - A search radius which describes the range with which other particles a
          particle might theoretically interact. The effective interaction might
          be smaller if a code decides to ignore potential interaction partners,
          i.e. this is an absolute maximum. The search radius will decide on which
          resolution level within the tree a particle is held.
        - A MoveState flag. This flag is essential. We use it later to label those
          particles that have been moved to avoid that we move particles multiple
          times.
        - A flag that indicates if a particle has been updated within a cell.
          Consult our code release papers: In short, a particle does not uniquely
          belong to one cell but can belong to many cells. We nevertheless want to
          update them only once.

        @see swift2.actionsets.UpdateParticleMarker

        """
        super(Particle, self).__init__(name)

        self.data.add_attribute(
            dastgen2.attributes.Enumeration(
                "MoveState", ["New", "NotMovedYet", "Moved"]
            )
        )
        self.data.add_attribute(dastgen2.attributes.Boolean("CellHasUpdatedParticle"))
        self.data.add_aspect(Particle.SpeciesAspect())

        if particles_per_cell <= 0:
            print(
                "WARNING: Particles per cell set to zero, so code might refine up to maximum depth even if there is only one particle"
            )
        self.particles_per_cell = particles_per_cell
        self.min_h = min_h
        self.max_h = max_h

        return

    @abstractmethod
    def algorithm_steps(self):
        """!

        Return sequence of algorithm steps that have to be called per time step

        """
        assert False, "Should be overwritten by subclass"
        return []

    @abstractmethod
    def initialisation_steps(self):
        """!

        Return sequence of algorithm steps that have to be performed throughout initialisation

        """
        assert False, "Should be overwritten by subclass"
        return []

    @property
    def readme_descriptor(self):
        """!

        Create default readme descriptor

        You might want to overwrite this for your particular particle
        species to get more detailed info the the README.md file generated
        by Peano.

        """
        return (
            """### Particle """
            + self.name
            + """

Particle properties:

- particles per cell:         """
            + str(self.particles_per_cell)
            + """
- h_min:                      """
            + str(self.min_h)
            + """
- h_max:                      """
            + str(self.max_h)
            + """
- attributes: """
            + str(len(self.data._attributes))
            + """

    """
        )

    def _dependency_checks_modify_steps(self, steplist, step_type_name):
        """
        This function actually modifies the AlgorithmSteps to contain the
        debug dependency checks. Is called by self.add_dependency_checks()

        Parameters
        ----------

        Steplist:       list
            list of AlgorithmStep objects to work on

        step_type_name: str
            name of type of steps we're dealing with. Either AlgorithmStep
            or InitStep.


        Returns
        -------

        steps: list
            the modified list of AlgorithmSteps, now including dependency checks
        """

        steps = copy.deepcopy(steplist)
        nsteps = len(steps)
        nstages = AlgorithmStep.SweepStage.STAGE_COUNT.value

        # add enum to particle for each AlgorithmStep
        step_name_list = [step.name for step in steps]
        step_name_list.append("count")  # add a count so we can loop over enums
        step_name_enum = dastgen2.attributes.Enumeration(
            step_type_name, step_name_list, ifdefs=["PeanoDebug > 0"]
        )
        self.data.add_attribute(step_name_enum)

        # add (flattened) array for flags
        step_progression = dastgen2.attributes.IntegerArray(
            step_type_name + "SweepStateProgression",
            nsteps * nstages,
            ifdefs=["PeanoDebug > 0"],
        )
        self.data.add_attribute(step_progression)

        check_template = """\n dependencyChecks::dependencyCheck{StepType}(localParticles,
                  globaldata::{NAME}::{StepType}::{0},
                  globaldata::{NAME}::SweepStage::{1}
                  );\n"""
        check_template = check_template.replace("{NAME}", self.name)
        check_template = check_template.replace("{StepType}", step_type_name)

        # First step is special: First reset all dependency values before doing anything else
        first_step = steps[0]
        if first_step.touch_vertex_first_time_kernel is None:
            first_step.touch_vertex_first_time_kernel = ""
        first_step.touch_vertex_first_time_kernel = (
            "\n  dependencyChecks::clearDependencyChecks"
            + step_type_name
            + "(localParticles);\n"
            + first_step.touch_vertex_last_time_kernel
        )

        for step in steps:
            if step.touch_vertex_first_time_kernel is None:
                step.touch_vertex_first_time_kernel = ""
            stage = AlgorithmStep.get_sweep_stage_name(
                AlgorithmStep.SweepStage.TOUCH_VERTEX_FIRST_TIME
            )
            step.touch_vertex_first_time_kernel += check_template.format(
                step.name, stage
            )

            if step.cell_kernel is None:
                step.cell_kernel = ""
            stage = AlgorithmStep.get_sweep_stage_name(
                AlgorithmStep.SweepStage.CELL_KERNEL
            )
            step.cell_kernel += check_template.format(step.name, stage)

            if step.touch_vertex_last_time_kernel is None:
                step.touch_vertex_last_time_kernel = ""
            stage = AlgorithmStep.get_sweep_stage_name(
                AlgorithmStep.SweepStage.TOUCH_VERTEX_LAST_TIME
            )
            step.touch_vertex_last_time_kernel += check_template.format(
                step.name, stage
            )

            if step.includes is None:
                step.includes = ""
            step.includes += """\n#include "swift2/dependencyChecks.h"\n"""

        return steps

    def add_dependency_checks(self):
        """

        Add dependency checks: We first create Enumerations representing each
        algorithm step defined for the Particle, as well as each sweep stage of
        any grid traversal.

        Then for each defined algorithm step, we add a call to the dependency
        check functions for each sweep stage (touch vertex first time, cell
        kernel, touch vertex last time...)

        The check consists of
        1) marking down which step in the full algorithm a particle has
        completed in the current simulation step
        2) verifying that each step and sweep stage before the one the particle
        currently finds itself in has been completed.

        This is done both for the main algorithm steps as well as for the
        initialisation steps individually.

        TODO: Move a level up to peano4.toolbox.particles.Particle ?
        """

        # First add enum to particle class for each sweep stage. These are identical
        # for both algorithm steps and initialisation steps.
        stage_name_list = [
            AlgorithmStep.get_sweep_stage_name(s) for s in AlgorithmStep.SweepStage
        ]
        stage_name_enum = dastgen2.attributes.Enumeration(
            "SweepStage", stage_name_list, ifdefs=["PeanoDebug > 0"]
        )
        self.data.add_attribute(stage_name_enum)

        # Add debug dependency checks to the actual algorithm steps:
        steplist = self.algorithm_steps()
        step_type_name = "AlgorithmStep"
        modified_steps = self._dependency_checks_modify_steps(steplist, step_type_name)
        self._algorithm_steps = modified_steps

        # Same but for initialisation steps
        steplist = self.initialisation_steps()
        step_type_name = "InitStep"
        modified_steps = self._dependency_checks_modify_steps(steplist, step_type_name)
        self._initialisation_steps = modified_steps
