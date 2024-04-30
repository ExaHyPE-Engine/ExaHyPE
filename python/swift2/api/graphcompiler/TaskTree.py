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




def translate_one_step_of_species_into_mesh_traversal(
    step,
    current_species_set,
    mesh_traversals,
    alter_particle_position,
    use_lists_to_construct_active_sets,
    step_type: StepType,
    verbose,
    construct_tree_task_graph = True
):
    """!

    Translate one step of species into mesh traversal

    We create a new step and then add various different action sets depending
    on the context and behaviour. Once befilled, the new step is appended to
    mesh_traversals.

    1. If this is an initialisation sequence, then I clear all the task
       markers. This is to ensure that they do not hold any garbage. I can
       clear the set as often as I want. It is just important that I clear
       it at least once per particle species.
    2. Update the task markers. This is something each and every step in Peano 
       does. As we inject the update into each and every action set, we become
       independent of the AMR and when it is actually realised.
    3. Pick computational kernel. This is where the actual computation per step
       is done. We have two fundamentally different kernels: one keeps track of
       (multiscale) neighbours, while the other just updates the particles
       within the octant (cell).
    4. If an action set alters the position or cut-off of a particle, then we
       append True to alter_particle_position. Otherwise, we append False.

       Here's where the action set differs completely from the Sequential graph
       compiler. If we change a position, we erase all the task indices from
       the tree.

    In general, we never assign any index to touchCellLastTime(), as Swift does
    not only one type of plug-in points. This is actually touchCellFirstTime()
    in the realisation, so one might be tempted to mask out touchCellLastTime()
    and only use task indices for touchCellFirstTime(). However, we have to
    plug into touchVertexLastTime(), too, and this one has to know about events
    within the adjacent cells. The action set AssignTaskNumbersToMesh memorises
    the touchCellLastTime() indices within the vertices, to we use the numbers
    from touchCellLastTime() but actually plug into touchCellFirstTime().


    @param current_species_set: Particle
      You get a specific algorithmic step as input. The current_species_set
      highlights to which this step originally belonged to.

    @param mesh_traversals: [peano4.solversteps.Step]
      Inout parameter. We append a new step to this sequence.

    @param alter_particle_position: [Bool]
      Inout parameter. We append a new marker to this sequence.

     
    ## Use it to construct task graphs
    
    The routine is not expected to have an argument construct_tree_task_graph 
    in Sequential. But I want to use the routines from Sequential. So I give 
    this value a default value. This way, I can reuse the routine in TaskGraph,
    where I simply redefine the default.
      
    """
    new_step = peano4.solversteps.Step(
        "{}{}{}_sweep{}".format(
            get_observer_prefix(step_type),
            current_species_set.particle_model.name,
            step.name,
            len(mesh_traversals),
        ),
        add_user_defined_actions=False,
    )

    action_set_update_marker = UpdateParticleMarker(particle_set = current_species_set,
                                                    automatically_set_marker_in_leave_cell=False,
                                                    make_reset_separate_task=True,
                                                    )
    action_set_update_marker.descend_invocation_order = (
        new_step.highest_descend_invocation_order() + 1
    )
    action_set_update_marker.parallel = True
    new_step.add_action_set(action_set_update_marker)
    
    #
    # Need to know this for the action sets later down the line, as they have
    # to know if the tasks have to be waited for immediately.
    #
    current_step_alters_particle_position = (
        step.effect== AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN
        or 
        step.effect == AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS
    )

    #
    # We have to synchronise after each traversal if we work with a 
    # tree or alter the global state
    #
    synchronise_after_each_mesh_traversal = (
        step.effect== AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN
        or 
        step.effect == AlgorithmStep.Effect.ALTER_GLOBAL_STATE
        or 
        step.effect == AlgorithmStep.Effect.ALTER_GLOBAL_STATE_AND_MIGHT_RERUN
        or 
        construct_tree_task_graph
    )

    #
    # Pick computational kernel
    # =========================
    # This is where the actual computation per step is done. We have two
    # fundamentally different kernels: one keeps track of (multiscale)
    # neighbours, while the other just updates the particles within the
    # octant (cell).
    #
    if step.dependencies == AlgorithmStep.Dependencies.NEIGHBOURS:
        if use_lists_to_construct_active_sets:
            if verbose:
                print(
                    "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} can use the peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles action set and hence optimise aggressively".format(
                        len(mesh_traversals),
                        step.name,
                        current_species_set.particle_model.name,
                    )
                )
            action_set_particle_particle_interaction = swift2.api.actionsets.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles(
                particle_set=current_species_set,
                particle_particle_interaction_kernel=step.cell_kernel,
                touch_vertex_first_time_compute_particle_update_kernel=step.touch_vertex_first_time_kernel,
                touch_vertex_last_time_compute_particle_update_kernel=step.touch_vertex_last_time_kernel,
                prepare_traversal_kernel=step.prepare_traversal_kernel,
                unprepare_traversal_kernel=step.unprepare_traversal_kernel,
                additional_includes=step.includes,
                active_particle_set=step.input_particles,
                alter_particle_position_or_global_state=current_step_alters_particle_position,
                wait_for_all_tasks_to_finish_after_traversal=synchronise_after_each_mesh_traversal,
            )
        else:
            if verbose:
                print(
                    "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} uses the peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_Sets".format(
                        len(mesh_traversals),
                        step.name,
                        current_species_set.particle_model.name,
                    )
                )
            action_set_particle_particle_interaction = swift2.api.actionsets.UpdateParticle_MultiLevelInteraction_Sets(
                particle_set=current_species_set,
                particle_particle_interaction_kernel=step.cell_kernel,
                touch_vertex_first_time_compute_particle_update_kernel=step.touch_vertex_first_time_kernel,
                touch_vertex_last_time_compute_particle_update_kernel=step.touch_vertex_last_time_kernel,
                prepare_traversal_kernel=step.prepare_traversal_kernel,
                unprepare_traversal_kernel=step.unprepare_traversal_kernel,
                additional_includes=step.includes,
                active_particle_set=step.input_particles,
                alter_particle_position_or_global_state=current_step_alters_particle_position,
                wait_for_all_tasks_to_finish_after_traversal=synchronise_after_each_mesh_traversal,
            )
        action_set_particle_particle_interaction.descend_invocation_order = (
            new_step.highest_descend_invocation_order() + 1
        )
        action_set_particle_particle_interaction.parallel = True
        new_step.add_action_set(action_set_particle_particle_interaction)
    elif step.dependencies == AlgorithmStep.Dependencies.SELF:
        if use_lists_to_construct_active_sets:
            if verbose:
                print(
                    "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} uses peano4.toolbox.particles.UpdateParticle_SingleLevelInteraction_ContiguousParticles action set".format(
                        len(mesh_traversals),
                        step.name,
                        current_species_set.particle_model.name,
                    )
                )

            action_set_particle_update = swift2.api.actionsets.UpdateParticle_SingleLevelInteraction_ContiguousParticles(
                particle_set=current_species_set,
                particle_cell_update_kernel=step.cell_kernel,
                touch_vertex_first_time_compute_particle_update_kernel=step.touch_vertex_first_time_kernel,
                touch_vertex_last_time_compute_particle_update_kernel=step.touch_vertex_last_time_kernel,
                prepare_traversal_kernel=step.prepare_traversal_kernel,
                unprepare_traversal_kernel=step.unprepare_traversal_kernel,
                additional_includes=step.includes,
                active_particle_set=step.input_particles,
                current_step_alters_particle_position=current_step_alters_particle_position,
                wait_for_all_tasks_to_finish_after_traversal=synchronise_after_each_mesh_traversal,
            )
            action_set_particle_update.descend_invocation_order = (
                new_step.highest_descend_invocation_order() + 1
            )
            action_set_particle_update.parallel = True
            new_step.add_action_set(action_set_particle_update)
        else:
            if verbose:
                print(
                    "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} uses peano4.toolbox.particles.UpdateParticle_SingleLevelInteraction action set".format(
                        len(mesh_traversals),
                        step.name,
                        current_species_set.particle_model.name,
                    )
                )

            action_set_particle_update = swift2.api.actionsets.UpdateParticle_SingleLevelInteraction(
                particle_set=current_species_set,
                particle_cell_update_kernel=step.cell_kernel,
                touch_vertex_first_time_compute_particle_update_kernel=step.touch_vertex_first_time_kernel,
                touch_vertex_last_time_compute_particle_update_kernel=step.touch_vertex_last_time_kernel,
                prepare_traversal_kernel=step.prepare_traversal_kernel,
                unprepare_traversal_kernel=step.unprepare_traversal_kernel,
                additional_includes=step.includes,
                active_particle_set=step.input_particles,
                current_step_alters_particle_position=current_step_alters_particle_position,
                wait_for_all_tasks_to_finish_after_traversal=synchronise_after_each_mesh_traversal,
            )
            action_set_particle_update.descend_invocation_order = (
                new_step.highest_descend_invocation_order() + 1
            )
            action_set_particle_update.parallel = True
            new_step.add_action_set(action_set_particle_update)
    else:
        assert False, "step {} seems to have no action at all".format(step)

    #
    # As we have a 1:1 mapping, we can directly translate the effect into flags
    #
    if (current_step_alters_particle_position):
        alter_particle_position.append(True)
    else:
        alter_particle_position.append(False)

    #
    # Now add the task number management. First step erases all indices, as it 
    # might be the first time stepping step overall. Other than that, all steps
    # plug into creational and destroy events.
    #
    action_set_assign_task_markers = peano4.toolbox.api.AssignNumbersToMesh(
        #current_species_set.particle_model.name,
        current_species_set.name,
    )
    action_set_assign_task_markers.descend_invocation_order = (
        new_step.lowest_descend_invocation_order() - 1
    )
    action_set_assign_task_markers.parallel = True
    new_step.add_action_set(action_set_assign_task_markers)
    if verbose:
        print(
            "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} is added peano4.toolbox.api.AssignNumbersToMesh".format(
                len(mesh_traversals),
                step.name,
                current_species_set.particle_model.name,
            )
        )

    mesh_traversals.append(new_step)


def map_onto_separate_mesh_traversals_constructing_task_tree(
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


def add_clear_task_number_to_initialisation_steps(algorithm_steps, 
                                                  species_sets, 
                                                  verbose):
    """!
    
    Take set of mesh run throughs (algorithm steps) and add clear task number action set
    
    Usually called on initialisation routines.
    
    """

    for step in algorithm_steps:
      for current_species_set in species_sets:
        action_set_erase_task_markers = peano4.toolbox.api.ClearNumbersOnMesh(
            current_species_set.name,
        )
        action_set_erase_task_markers.descend_invocation_order = (
            step.highest_descend_invocation_order() + 1
        )
        action_set_erase_task_markers.parallel = True
        step.add_action_set(action_set_erase_task_markers)
        if verbose:
            print(
                "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} on particle {} clears task numbers/markers and hence uses the peano4.toolbox.api.ClearNumbersOnMesh action set".format(
                    step.name,
                    current_species_set.particle_model.name,
                )
            )


def initialisation_steps_onto_task_tree_multiscale_sort_scattered_memory(
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


# ====================================
# Public API functions
# ====================================
#
# These functions are made available through the __init__.py file. They are all
# "just" wrappers around the generic map_onto_separate_mesh_traversals() 
# function calls, yet with different arguments.
#
def initialisation_steps_onto_task_tree_bucket_sort_scattered_memory(
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


def initialisation_steps_onto_task_tree_multiscale_sort_coalesced_memory(
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


def initialisation_steps_onto_task_tree_bucket_sort_coalesced_memory(
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


def particle_steps_onto_task_tree_multiscale_sort_scattered_memory(
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
    return map_onto_separate_mesh_traversals_constructing_task_tree(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )


def particle_steps_onto_task_tree_bucket_sort_scattered_memory(
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
    return map_onto_separate_mesh_traversals_constructing_task_tree(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )



def particle_steps_onto_task_tree_multiscale_sort_coalesced_memory(
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
    return map_onto_separate_mesh_traversals_constructing_task_tree(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )


def particle_steps_onto_task_tree_bucket_sort_coalesced_memory(
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
    return map_onto_separate_mesh_traversals_constructing_task_tree(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )

