# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.AlgorithmStep import AlgorithmStep
from swift2.actionsets.UpdateParticleMarker import UpdateParticleMarker

import peano4
import swift2


from .AMRLogic import add_dynamic_mesh_refinement_and_particle_resorting
from .ParticleSortingAndStorage import (
    ParticleSortingAndStorage,
    AddAdditionalDummySweep,
)
from .SpeciesStepOrdering import concatenate_steps_of_species
from .Utils import (
    does_one_species_require_gather,
    get_species_set,
    get_observer_prefix,
    StepType,
)
from .Sequential import (
    map_onto_separate_mesh_traversals,
    map_onto_separate_mesh_traversals_with_generic_mapper,
)
from .TaskGraph import add_clear_task_number_to_very_first_initialisation_step


def translate_one_step_of_species_into_mesh_traversal(
    step,
    current_species_set,
    mesh_traversals,
    alter_particle_position,
    use_continuous_particles_sequences,
    step_type: StepType,
    verbose,
):
    """!

    Actual translation of one step per particle species onto a task graph construction

    We basically forward to swift2.api.graphcompiler.TaskGraph.translate_one_step_of_species_into_mesh_traversal()
    but we set the flag synchronise_task_graph_after_each_mesh_sweep() to
    false. This does not mean that the arising task graph is bare of all
    synchronisation. It just means that we don't immediately synchronise
    after each step.

    All the wrappers have implicitly built in some synchronisation: A vertex
    issues a task with a number. If this number is already tied to an existing
    task, this creates an in-out dependency. However, we have three types of
    task numbers per species:

    - touch vertex first task numbers
    - touch cell first task numbers
    - touch vertex last task numbers

    Each observer will create vertex first-vertex first-cell-vertex last chains
    of tasks with an arbitrary number of first and last instances. The important thing
    to keep things correct therefore is that we establish vertex last-vertex first
    relations here which ensure that one chain does not mangle into the chain of the
    predecessor.

    This is the added thing here. Global syncs will automatically be added where
    they are necessary (cmp. translate_one_step_of_species_into_mesh_traversal()
    and the variable synchronise_after_each_mesh_traversal therein).

    SynchroniseVerticesWithPreviousMeshSweep


    - wrapper alles first
    - nix in touch cell last

    """
    swift2.api.graphcompiler.TaskGraph.translate_one_step_of_species_into_mesh_traversal(
        step,
        current_species_set,
        mesh_traversals,
        alter_particle_position,
        use_continuous_particles_sequences,
        step_type,
        verbose,
        synchronise_task_graph_after_each_mesh_sweep=False,
    )

    assert (
        len(mesh_traversals) > 0
    ), "at least one step has to be added after translate_one_step_of_species_into_mesh_traversal() has terminated"

    glue_two_mesh_sweeps_together = (
        swift2.api.actionsets.SynchroniseVerticesWithPreviousMeshSweep(
            current_species_set
        )
    )
    glue_two_mesh_sweeps_together.descend_invocation_order = (
        mesh_traversals[-1].lowest_descend_invocation_order() - 1
    )
    glue_two_mesh_sweeps_together.parallel = False
    mesh_traversals[-1].add_action_set(glue_two_mesh_sweeps_together)

    if verbose:
        print(
            "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} uses swift.api.actionsets.SynchroniseVerticesWithPreviousMeshSweep".format(
                len(mesh_traversals),
                step.name,
                current_species_set.particle_model.name,
            )
        )


def map_onto_separate_mesh_traversals_constructing_task_graph(
    sequence_of_steps,
    particle_sorting_and_storage: ParticleSortingAndStorage,
    step_type: StepType,
    verbose,
):
    """!

    Forward to map_onto_separate_mesh_traversals_with_generic_mapper().
    Consult implementation in TaskGraph.py for further information.

    The bespoke logic here is injected by the functor
    translate_one_step_of_species_into_mesh_traversal() pointing to
    another routine in this file.

    """
    return map_onto_separate_mesh_traversals_with_generic_mapper(
        sequence_of_steps,
        particle_sorting_and_storage,
        step_type,
        verbose,
        translate_one_step_of_species_into_mesh_traversal,
    )


# ====================================
# Public API functions
# ====================================
#
# These functions are made available through the __init__.py file. They are all
# "just" wrappers around the generic map_onto_separate_mesh_traversals()
# function calls, yet with different arguments.
#


def initialisation_steps_onto_multisweep_task_graph_multiscale_sort_scattered_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    result = map_onto_separate_mesh_traversals_constructing_task_graph(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )
    add_clear_task_number_to_very_first_initialisation_step(
        result, species_sets, verbose
    )
    return result


def initialisation_steps_onto_multisweep_task_graph_bucket_sort_scattered_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    result = map_onto_separate_mesh_traversals_constructing_task_graph(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )
    add_clear_task_number_to_very_first_initialisation_step(
        result, species_sets, verbose
    )
    return result


def initialisation_steps_onto_multisweep_task_graph_multiscale_sort_coalesced_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    result = map_onto_separate_mesh_traversals_constructing_task_graph(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )
    add_clear_task_number_to_very_first_initialisation_step(
        result, species_sets, verbose
    )
    return result


def initialisation_steps_onto_multisweep_task_graph_bucket_sort_coalesced_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    result = map_onto_separate_mesh_traversals_constructing_task_graph(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )
    add_clear_task_number_to_very_first_initialisation_step(
        result, species_sets, verbose
    )
    return result


def particle_steps_onto_multisweep_task_graph_multiscale_sort_scattered_memory(
    species_sets,
    verbose,
):
    """!

    For a sequential ordering, i.e. 1:1 mapping of steps per species onto
    traversals, it makes no real difference how we order the progress through
    the individual steps. So I just concatenate the steps per species.

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    return map_onto_separate_mesh_traversals_constructing_task_graph(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )


def particle_steps_onto_multisweep_task_graph_bucket_sort_scattered_memory(
    species_sets,
    verbose,
):
    """!

    For a sequential ordering, i.e. 1:1 mapping of steps per species onto
    traversals, it makes no real difference how we order the progress through
    the individual steps. So I just concatenate the steps per species.

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    return map_onto_separate_mesh_traversals_constructing_task_graph(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )


def particle_steps_onto_multisweep_task_graph_multiscale_sort_coalesced_memory(
    species_sets,
    verbose,
):
    """!

    For a sequential ordering, i.e. 1:1 mapping of steps per species onto
    traversals, it makes no real difference how we order the progress through
    the individual steps. So I just concatenate the steps per species.

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    return map_onto_separate_mesh_traversals_constructing_task_graph(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )


def particle_steps_onto_multisweep_task_graph_bucket_sort_coalesced_memory(
    species_sets,
    verbose,
):
    """!

    For a sequential ordering, i.e. 1:1 mapping of steps per species onto
    traversals, it makes no real difference how we order the progress through
    the individual steps. So I just concatenate the steps per species.

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    return map_onto_separate_mesh_traversals_constructing_task_graph(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )
