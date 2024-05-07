# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.AlgorithmStep import AlgorithmStep
from swift2.actionsets.UpdateParticleMarker import UpdateParticleMarker

import peano4
import swift2


from .AMRLogic import add_dynamic_mesh_refinement_and_particle_resorting
from .ParticleSortingAndStorage import (
   ParticleSortingAndStorage,
   AddAdditionalDummySweep
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
    map_onto_separate_mesh_traversals_with_generic_mapper
)
from .TaskTree import (
    add_clear_task_number_to_initialisation_steps
)


def translate_one_step_of_species_into_mesh_traversal(
    step,
    current_species_set,
    mesh_traversals,
    alter_particle_position,
    use_lists_to_construct_active_sets,
    step_type: StepType,
    verbose,
):
    return swift2.api.graphcompiler.TaskTree.translate_one_step_of_species_into_mesh_traversal(
        step,
        current_species_set,
        mesh_traversals,
        alter_particle_position,
        use_lists_to_construct_active_sets,
        step_type,
        verbose,
        construct_tree_task_graph = False
    )


def map_onto_separate_mesh_traversals_constructing_task_graph(
    sequence_of_steps,
    particle_sorting_and_storage: ParticleSortingAndStorage,
    step_type: StepType,
    verbose,
):
    """!

    Simpest graph compiler constructing a task tree

    This graph compiler is very very similar to Sequential.map_onto_separate_mesh_traversals().
    It runs over the tree and generates one task per touchVertexFirstTime(),
    touchCellFirstTime() and touchVertexLastTime(). We end up with a
    task graph that resembles an overlay of a task tree and the graph
    belonging to a Cartesian mesh with a vertex-cell-vertex update
    sequence.

    @image html documentation/Doxygen/Swift/task-graph-tree.png
    
    Whenever we spawn a task, we first generate a new task id. This is the
    number of this task which we store within the task and which the task
    has to free eventually. We also store this task within the associated
    mesh entity (vertex or cell) and hence can use the tree as a lookup
    mechanism of the task dependencies. That is, the indices within the
    tree are exclusively used for lookups and not for any semantics.


    @param sequence_of_steps: [(Species,Step)]
          Sequence of tuples each consisting of one species and one step. Typically
          constructed through functions within SpeciesStepOrdering.py.

    @param particle_sorting: ParticleSorting

    @param observer_preamble: String
          Usually people pass in "Init" or "Step" here to distinguish observers for
          the initialisation from the actual particle update steps.

    @param verbose: {True, False}

    @return mesh_traversals

    """
    return map_onto_separate_mesh_traversals_with_generic_mapper(
        sequence_of_steps,
        particle_sorting_and_storage,
        step_type,
        verbose,
        translate_one_step_of_species_into_mesh_traversal
    )


# ====================================
# Public API functions
# ====================================
#
# These functions are made available through the __init__.py file. They are all
# "just" wrappers around the generic map_onto_separate_mesh_traversals() 
# function calls, yet with different arguments.
#

def initialisation_steps_onto_task_graph_multiscale_sort_scattered_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    result = map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )
    add_clear_task_number_to_initialisation_steps(result, species_sets, verbose)
    return result


def initialisation_steps_onto_task_graph_bucket_sort_scattered_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    result = map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )
    add_clear_task_number_to_initialisation_steps(result, species_sets, verbose)
    return result


def initialisation_steps_onto_task_graph_multiscale_sort_coalesced_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    result = map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )
    add_clear_task_number_to_initialisation_steps(result, species_sets, verbose)
    return result


def initialisation_steps_onto_task_graph_bucket_sort_coalesced_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    result = map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )
    add_clear_task_number_to_initialisation_steps(result, species_sets, verbose)
    return result


def particle_steps_onto_task_graph_multiscale_sort_scattered_memory(
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


def particle_steps_onto_task_graph_bucket_sort_scattered_memory(
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



def particle_steps_onto_task_graph_multiscale_sort_coalesced_memory(
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


def particle_steps_onto_task_graph_bucket_sort_coalesced_memory(
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

