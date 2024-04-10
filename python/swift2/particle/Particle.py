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

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        my_particle.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("v","Dimensions") )
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



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

    DependencyChecks_Attribute_Prefix = "dependencyChecks"
    DependencyChecks_Ifdefs = ["PeanoDebug > 0"]

    def _dependency_checks_modify_steps(
        self,
        steplist,
        step_type_name,
        peano4_event_enum,
    ):
        """!

        Add dependency checks as well as mesh consistency checks to the algorithm steps

        This function actually modifies the AlgorithmSteps to contain the
        debug dependency checks. Is called by self.add_dependency_checks()
        and uses the validation routines from swift2::dependencychecks.

        The routine is used to either add checks for the init phase or the
        actual time stepping. Which one is meant is identified through
        step_type_name. You can obviously call the routine twice: once for
        init and once for the time stepping. Later on, you might want to
        add further steps.

        Besides the name of interest, we also require a list of algorithmic
        steps through which the code runs through.


        ## Default settings

        If the user does not define bespoke default settings, this routine adds
        default ones. These are set through instances of swift::dependencychecks::Invariant.

        - For vertices, this is touch-at-most-once-mask-out-otherwise-all-previous-steps-update-at-least-once.
        - For cells, this is also touch-at-most-once-mask-out-otherwise-all-previous-steps-update-at-least-once.

        ## Additional attributes

        First, the particle is added two counters which count up how often a
        particle is touched or masked out.

        @param Steplist:       list
            list of AlgorithmStep objects to work on

        @param step_type_name: str
            name of type of steps we're dealing with. Either AlgorithmStep
            or InitStep.

        @param peano4_event_enum: list
            list of enums of peano4 events within a single mesh traversal
            (e.g. touch vertex first time, cell kernel, ...)

        @return steps: list
            the modified list of AlgorithmSteps, now including dependency checks

        """
        steps = copy.deepcopy(steplist)
        nsteps = len(steps)
        nevents = AlgorithmStep.PeanoEventUsedBySwift.EVENT_COUNT.value

        # add enum to particle for each AlgorithmStep. This enum tracks in
        # which algorithmic step a particle currently is. See
        # DependencyChecks.h.
        step_name_list = [step.name for step in steps]
        step_name_list.append("count")  # add a count so we can loop over enums
        step_name_enum = dastgen2.attributes.Enumeration(
            self.DependencyChecks_Attribute_Prefix + step_type_name + "LastUpdated",
            step_name_list,
            ifdefs=self.DependencyChecks_Ifdefs,
        )
        self.data.add_attribute(step_name_enum)

        # add (flattened) array for flags
        number_of_updates = dastgen2.attributes.IntegerArray(
            self.DependencyChecks_Attribute_Prefix + step_type_name + "Updates",
            nsteps * nevents,
            ifdefs=self.DependencyChecks_Ifdefs,
        )
        self.data.add_attribute(number_of_updates)

        number_of_mask_outs = dastgen2.attributes.IntegerArray(
            self.DependencyChecks_Attribute_Prefix + step_type_name + "MaskOuts",
            nsteps * nevents,
            ifdefs=self.DependencyChecks_Ifdefs,
        )
        self.data.add_attribute(number_of_mask_outs)

        # String template for dependency check call
        check_template_cell_operation = """
  ::swift2::dependencychecks::check{STEP_TYPE}(
                  ::swift2::dependencychecks::Invariant::{INVARIANT},
                  localParticles,
                  activeParticles,
                  marker,
                  globaldata::{NAME}::DependencyChecks{STEP_TYPE}LastUpdated::{STEP_NAME},
                  globaldata::{NAME}::{PEANO4_EVENT_TYPE}::{EVENT},
                  {CHECK_FUNCTION},
                  _spacetreeId
  );
"""

        check_template_vertex_operation = """
  ::swift2::dependencychecks::check{STEP_TYPE}(
                  ::swift2::dependencychecks::Invariant::{INVARIANT},
                  assignedParticles,
                  marker,
                  globaldata::{NAME}::DependencyChecks{STEP_TYPE}LastUpdated::{STEP_NAME},
                  globaldata::{NAME}::{PEANO4_EVENT_TYPE}::{EVENT},
                  {CHECK_FUNCTION},
                  _spacetreeId
  );
"""

        mark_template_cell_operation = """
  ::swift2::dependencychecks::mark{STEP_TYPE}(
                  localParticles,
                  activeParticles,
                  marker,
                  globaldata::{NAME}::DependencyChecks{STEP_TYPE}LastUpdated::{STEP_NAME},
                  globaldata::{NAME}::{PEANO4_EVENT_TYPE}::{EVENT},
                  ::swift2::dependencychecks::Invariant::{INVARIANT},
                  {CHECK_FUNCTION},
                  _spacetreeId
  );
"""

        mark_template_vertex_operation = """
  ::swift2::dependencychecks::mark{STEP_TYPE}(
                  assignedParticles,
                  marker,
                  globaldata::{NAME}::DependencyChecks{STEP_TYPE}LastUpdated::{STEP_NAME},
                  globaldata::{NAME}::{PEANO4_EVENT_TYPE}::{EVENT},
                  ::swift2::dependencychecks::Invariant::{INVARIANT},
                  {CHECK_FUNCTION},
                  _spacetreeId
  );
"""

        check_template_vertex_operation_local_particles_after_sweep = """
  ::swift2::dependencychecks::checkParticlesAssignedToVertexInTouchLastTime{STEP_TYPE}(
    ::swift2::dependencychecks::Invariant::{INVARIANT},
    assignedParticles,
    globaldata::{NAME}::DependencyChecks{STEP_TYPE}LastUpdated::{STEP_NAME},
                  _spacetreeId
  );
"""

        for i, step in enumerate(steps):
            d = {
                "NAME": self.name,
                "STEP_TYPE": step_type_name,
                "STEP_NAME": step.name,
                "PEANO4_EVENT_TYPE": peano4_event_enum.get_accessor_name(),
                "CHECK_FUNCTION": None,
                "EVENT": None,
                "INVARIANT": None,
            }

            step.prepare_traversal_kernel = (
                """
                toolbox::particles::assignmentchecks::startMeshSweep( "{}" );
                """.format(
                    step.name
                )
                + step.prepare_traversal_kernel
            )

            if (
                step.touch_vertex_first_time_dependency_policy == None
                and AlgorithmStep.may_trigger_rerun(step.effect)
            ):
                d[
                    "INVARIANT"
                ] = "TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun"
            elif step.touch_vertex_first_time_dependency_policy == None:
                d[
                    "INVARIANT"
                ] = "TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce"
            else:
                d["INVARIANT"] = step.touch_vertex_first_time_dependency_policy

            # TOUCH VERTEX FIRST TIME
            # -----------------------
            d["EVENT"] = AlgorithmStep.get_event_name(
                AlgorithmStep.PeanoEventUsedBySwift.TOUCH_VERTEX_FIRST_TIME
            )
            d[
                "CHECK_FUNCTION"
            ] = "::swift2::kernels::localParticleCanBeUpdatedInVertexKernel<globaldata::{}>".format(
                self.name
            )

            if step.touch_vertex_first_time_kernel is None:
                step.touch_vertex_first_time_kernel = ""

            if i == 0:
                # Only exception is first AlgorithmStep: There, first re-set
                # all counters
                step.touch_vertex_first_time_kernel = (
                    "\n  ::swift2::dependencychecks::clearDependencyChecks"
                    + step_type_name
                    + "(assignedParticles);\n"
                    + mark_template_vertex_operation.format(**d)
                    + step.touch_vertex_first_time_kernel
                )
            else:
                step.touch_vertex_first_time_kernel = (
                    mark_template_vertex_operation.format(**d)
                    + step.touch_vertex_first_time_kernel
                )

            # Finally, add the dependency check *after* the actual function call
            step.touch_vertex_first_time_kernel += (
                check_template_vertex_operation.format(**d)
            )

            # CELL_KERNEL
            # -----------
            d["EVENT"] = AlgorithmStep.get_event_name(
                AlgorithmStep.PeanoEventUsedBySwift.CELL_KERNEL
            )
            d[
                "CHECK_FUNCTION"
            ] = "::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<globaldata::{}>".format(
                self.name
            )

            if (
                step.cell_kernel_dependency_policy == None
                and AlgorithmStep.may_trigger_rerun(step.effect)
            ):
                d[
                    "INVARIANT"
                ] = "TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun"
            elif step.cell_kernel_dependency_policy == None:
                d[
                    "INVARIANT"
                ] = "TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce"
            else:
                d["INVARIANT"] = step.cell_kernel_dependency_policy

            if step.cell_kernel is None:
                step.cell_kernel = ""

            step.cell_kernel = (
                mark_template_cell_operation.format(**d) + step.cell_kernel
            )

            # Now add the dependency check *after* the actual function call
            step.cell_kernel += check_template_cell_operation.format(**d)

            # TOUCH VERTEX LAST TIME
            # ----------------------
            d["EVENT"] = AlgorithmStep.get_event_name(
                AlgorithmStep.PeanoEventUsedBySwift.TOUCH_VERTEX_LAST_TIME
            )
            d[
                "CHECK_FUNCTION"
            ] = "::swift2::kernels::localParticleCanBeUpdatedInVertexKernel<globaldata::{}>".format(
                self.name
            )

            if (
                step.touch_vertex_last_time_dependency_policy == None
                and AlgorithmStep.may_trigger_rerun(step.effect)
            ):
                d[
                    "INVARIANT"
                ] = "TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun"
            elif step.touch_vertex_last_time_dependency_policy == None:
                d[
                    "INVARIANT"
                ] = "TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce"
            else:
                d["INVARIANT"] = step.touch_vertex_last_time_dependency_policy

            if step.touch_vertex_last_time_kernel is None:
                step.touch_vertex_last_time_kernel = ""

            step.touch_vertex_last_time_kernel = (
                mark_template_vertex_operation.format(**d)
                + step.touch_vertex_last_time_kernel
            )

            # Now add the dependency check *after* the actual function call
            step.touch_vertex_last_time_kernel += (
                check_template_vertex_operation.format(**d)
            )

            # touch vertex last time is special: Add additional dependency check
            # which makes sure all activeParticles have been local at some point
            # during this algorithm step
            step.touch_vertex_last_time_kernel += (
                check_template_vertex_operation_local_particles_after_sweep.format(**d)
            )

        return steps

    def _add_dependency_checks(self):
        """!

        Add dependency (particle consistency) checks

        This routine should be called in the subclass directly after you have
        befilled self._algorithm_steps and self._initialisation_steps. It is
        safe to add this stuff to your particle model all the time - the
        actual checks kick in if and only if you run in non-release modes.
        However, you might want to call the function only in these cases to
        ensure that no (empty) C++ code is generated and the compiler can
        optimise more aggressively.

        ## Realisation

        We first create Enumerations representing each
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


        """

        ifdefs = ["PeanoDebug > 0"]

        # First add enum to particle class for each sweep stage. These are identical
        # for both algorithm steps and initialisation steps.
        peano4_event_enum = dastgen2.attributes.Enumeration(
            self.DependencyChecks_Attribute_Prefix + "PeanoEventUsedBySwift",
            [
                AlgorithmStep.get_event_name(s)
                for s in AlgorithmStep.PeanoEventUsedBySwift
            ],
            ifdefs=ifdefs,
        )
        self.data.add_attribute(peano4_event_enum)

        # Add debug dependency checks to the actual algorithm steps:
        steplist = self.algorithm_steps()
        step_type_name = "AlgorithmStep"
        modified_steps = self._dependency_checks_modify_steps(
            steplist, step_type_name, peano4_event_enum
        )
        self._algorithm_steps = modified_steps

        # Same but for initialisation steps
        steplist = self.initialisation_steps()
        step_type_name = "InitStep"
        modified_steps = self._dependency_checks_modify_steps(
            steplist, step_type_name, peano4_event_enum
        )
        self._initialisation_steps = modified_steps
