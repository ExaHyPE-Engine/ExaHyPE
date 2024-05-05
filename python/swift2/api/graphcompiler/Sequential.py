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


def translate_one_step_of_species_into_mesh_traversal(
    step,
    current_species_set,
    mesh_traversals,
    alter_particle_position,
    use_lists_to_construct_active_sets,
    step_type: StepType,
    verbose,
):
    """!

    Translate one step of species into mesh traversal

    Takes one step of the species and adds it to the list of steps. That is,
    you get one more entry within mesh_traversals. Further to that, the
    routine also appends a boolean arker to alter_particle_position. It
    shows if the mesh will change the particle position (or search radius)
    or not.

    This routine is a helper routine of map_initialisation_steps_onto_separate_mesh_traversals()
    for example. Note that the returned steps are not fully functionally.
    If a step alters the particles within the mesh, you still have to resort
    the particles. That's not done in the step added by this routine, i.e.
    these steps lack the actual particle sorting. Any sorting has to spread
    over two steps, so it makes no sense to add it here. Instead, we befill
    the alter_particle_position, and we expect the calling routine to
    supplement the actual particle sorting later on in a postprocessing
    step once all steps are assembled.


    @param current_species_set: Particle
      You get a specific algorithmic step as input. The current_species_set
      highlights to which this step originally belonged to.

    @param mesh_traversals: [peano4.solversteps.Step]
      Inout parameter. We append a new step to this sequence.

    @param alter_particle_position: [Bool]
      Inout parameter. We append a new marker to this sequence.

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
                                                    automatically_set_marker_in_leave_cell=True,
                                                    make_reset_separate_task=False,
                                                    )

    action_set_update_marker.descend_invocation_order = (
        new_step.highest_descend_invocation_order() + 1
    )
    action_set_update_marker.parallel = True
    new_step.add_action_set(action_set_update_marker)

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
            action_set_particle_particle_interaction = peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles(
                particle_set=current_species_set,
                particle_particle_interaction_kernel=step.cell_kernel,
                touch_vertex_first_time_compute_particle_update_kernel=step.touch_vertex_first_time_kernel,
                touch_vertex_last_time_compute_particle_update_kernel=step.touch_vertex_last_time_kernel,
                prepare_traversal_kernel=step.prepare_traversal_kernel,
                unprepare_traversal_kernel=step.unprepare_traversal_kernel,
                additional_includes=step.includes,
                active_particle_set=step.input_particles,
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
            action_set_particle_particle_interaction = peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_Sets(
                particle_set=current_species_set,
                particle_particle_interaction_kernel=step.cell_kernel,
                touch_vertex_first_time_compute_particle_update_kernel=step.touch_vertex_first_time_kernel,
                touch_vertex_last_time_compute_particle_update_kernel=step.touch_vertex_last_time_kernel,
                prepare_traversal_kernel=step.prepare_traversal_kernel,
                unprepare_traversal_kernel=step.unprepare_traversal_kernel,
                additional_includes=step.includes,
                active_particle_set=step.input_particles,
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

            action_set_particle_update = peano4.toolbox.particles.UpdateParticle_SingleLevelInteraction_ContiguousParticles(
                particle_set=current_species_set,
                particle_cell_update_kernel=step.cell_kernel,
                touch_vertex_first_time_compute_particle_update_kernel=step.touch_vertex_first_time_kernel,
                touch_vertex_last_time_compute_particle_update_kernel=step.touch_vertex_last_time_kernel,
                prepare_traversal_kernel=step.prepare_traversal_kernel,
                unprepare_traversal_kernel=step.unprepare_traversal_kernel,
                additional_includes=step.includes,
                active_particle_set=step.input_particles,
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

            action_set_particle_update = peano4.toolbox.particles.UpdateParticle_SingleLevelInteraction(
                particle_set=current_species_set,
                particle_cell_update_kernel=step.cell_kernel,
                touch_vertex_first_time_compute_particle_update_kernel=step.touch_vertex_first_time_kernel,
                touch_vertex_last_time_compute_particle_update_kernel=step.touch_vertex_last_time_kernel,
                prepare_traversal_kernel=step.prepare_traversal_kernel,
                unprepare_traversal_kernel=step.unprepare_traversal_kernel,
                additional_includes=step.includes,
                active_particle_set=step.input_particles,
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
    if (
        step.effect== AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN
        or 
        step.effect == AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS
    ):
        alter_particle_position.append(True)
    else:
        alter_particle_position.append(False)

    mesh_traversals.append(new_step)


def map_onto_separate_mesh_traversals_with_generic_mapper(
    sequence_of_steps,
    particle_sorting_and_storage: ParticleSortingAndStorage,
    step_type: StepType,
    verbose,
    mapper
):
    """!
    
    Generic mapper of separate steps of separate particles onto mesh traversals
    
    expects a mapper that then defines how to actually map the species plus
    step combination.
    
    """
    mesh_traversals = []
    alter_particle_position = []

    if verbose:
        print(
            "GRAPH COMPILER DEBUG [map_onto_separate_mesh_traversals]: start graph construction with particle sorting {} and step type {}".format(
                particle_sorting_and_storage, step_type
            )
        )

    for current_entry_from_total_sequence_of_steps in sequence_of_steps:
        current_species_set     = current_entry_from_total_sequence_of_steps[0]
        current_step_of_species = current_entry_from_total_sequence_of_steps[1]

        if (
            particle_sorting_and_storage == ParticleSortingAndStorage.SORT_ON_THE_FLY_SCATTERED_PARTICLE_STORAGE
        ):
            use_lists_to_construct_active_sets = False
            add_additional_dummy_sweep = AddAdditionalDummySweep.NO
        elif (
            particle_sorting_and_storage == ParticleSortingAndStorage.SORT_ON_THE_FLY_COALESCED_PARTICLE_STORAGE
        ):
            use_lists_to_construct_active_sets = True
            add_additional_dummy_sweep = AddAdditionalDummySweep.NO
        elif (
            particle_sorting_and_storage == ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_SCATTERED_PARTICLE_STORAGE
            and len(alter_particle_position) > 0
            and alter_particle_position[-1]
        ):
            use_lists_to_construct_active_sets = False
            add_additional_dummy_sweep = AddAdditionalDummySweep.BEFORE
        elif (
            particle_sorting_and_storage == ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_SCATTERED_PARTICLE_STORAGE
        ):
            use_lists_to_construct_active_sets = False
            add_additional_dummy_sweep = AddAdditionalDummySweep.NO
        elif (
            particle_sorting_and_storage == ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_COALESCED_PARTICLE_STORAGE
            and len(alter_particle_position) > 0
            and alter_particle_position[-1]
        ):
            use_lists_to_construct_active_sets = True
            add_additional_dummy_sweep = AddAdditionalDummySweep.BEFORE
        elif (
            particle_sorting_and_storage == ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_COALESCED_PARTICLE_STORAGE
        ):
            use_lists_to_construct_active_sets = True
            add_additional_dummy_sweep = AddAdditionalDummySweep.NO
        else:
           assert False, "variant {} x {} not supported".format(particle_sorting_and_storage,alter_particle_position)

        if add_additional_dummy_sweep == AddAdditionalDummySweep.BEFORE:
            if verbose:
                print(
                    "GRAPH COMPILER DEBUG [map_onto_separate_mesh_traversals]: step no {} ({}) on particle {} has to be dummy step to clean up sorting. Will not alter particle position itself".format(
                        len(mesh_traversals),
                        current_step_of_species.name,
                        current_species_set.particle_model.name,
                    )
                )
            new_step = peano4.solversteps.Step(
                "DummyStep{}{}_sweep{}".format(
                    current_species_set.particle_model.name,
                    current_step_of_species.name,
                    len(mesh_traversals),
                ),
                add_user_defined_actions=False,
            )
            new_step.add_action_set(swift2.actionsets.DummyStep())
            mesh_traversals.append(new_step)
            alter_particle_position.append(False)

        mapper(
            step=current_step_of_species,
            current_species_set=current_species_set,
            mesh_traversals=mesh_traversals,
            alter_particle_position=alter_particle_position,
            use_lists_to_construct_active_sets=use_lists_to_construct_active_sets,
            step_type=step_type,
            verbose=verbose,
        )

        if add_additional_dummy_sweep == AddAdditionalDummySweep.AFTER:
            if verbose:
                print(
                    "GRAPH COMPILER DEBUG [map_onto_separate_mesh_traversals]: step no {} ({}) on particle {} has to be dummy step to clean up sorting. Will not alter the particle position itself".format(
                        len(mesh_traversals),
                        current_step_of_species.name,
                        current_species_set.particle_model.name,
                    )
                )
            new_step = peano4.solversteps.Step(
                "DummyStep{}{}_sweep{}".format(
                    current_species_set.particle_model.name,
                    current_step_of_species.name,
                    len(mesh_traversals),
                ),
                add_user_defined_actions=False,
            )
            new_step.add_action_set(swift2.actionsets.DummyStep())
            mesh_traversals.append(new_step)
            alter_particle_position.append(False)

    if step_type == StepType.INITIALISATION and len(alter_particle_position) > 0:
        # When we insert particles initially, they will be scattered. So it is
        # important that we immediately gather them. We cannot flag step -1 as
        # one that changes teh particle position, but we can present to the
        # graph compiler that the last step will change the ordering. The graph
        # compiler does not distinguish initialisation from time stepping and
        # hence thinks that the sweep prior to the first sweep has changed the
        # particle order. Which is exactly what we want. We also flag the first
        # sweep as change, as the initial insertion indeed might be buggy.
        alter_particle_position[0] = True
        alter_particle_position[-1] = True
        

    if (
        particle_sorting_and_storage==ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_SCATTERED_PARTICLE_STORAGE
        or
        particle_sorting_and_storage==ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_COALESCED_PARTICLE_STORAGE
    ):
        sort_with_lifts_and_drops = True
    elif (
        particle_sorting_and_storage==ParticleSortingAndStorage.SORT_ON_THE_FLY_SCATTERED_PARTICLE_STORAGE
        or
        particle_sorting_and_storage==ParticleSortingAndStorage.SORT_ON_THE_FLY_COALESCED_PARTICLE_STORAGE
    ):
        sort_with_lifts_and_drops = False
    else:
        assert False, "variant {} not implemented".format(particle_sorting_and_storage)


    add_dynamic_mesh_refinement_and_particle_resorting(
        mesh_traversals,
        alter_particle_position,
        get_species_set(sequence_of_steps),
        sort_with_lifts_and_drops=sort_with_lifts_and_drops,
        verbose=verbose,
    )
    
    if verbose:
        print(
            "GRAPH COMPILER DEBUG [map_onto_separate_mesh_traversals]: finished graph construction"
        )

    return mesh_traversals
    
    
    
def map_onto_separate_mesh_traversals(
    sequence_of_steps,
    particle_sorting_and_storage: ParticleSortingAndStorage,
    step_type: StepType,
    verbose,
):
    """!

    Simpest graph compiler mapping each step per particle onto one mesh traversal

    Very simple compiler which takes each individual algorithmic step
    per species and maps it onto one mesh traversal. We won't get any
    fancy parallelism besides the core domain decomposition.

    As we work with a strictly synchronised way, we can ignore in and
    outgoing dependencies which affect the global state. They are
    implicitly handled.

    It is important to recognise the inter-dependencies between steps:

    We do not have to care about the plotting: It always resorts the
    particles, and it always re-analyses which particles are local for
    a tree. Consequently, the plotting will throw away local particle
    copies if particles have meanwhile left the domain. We may assume
    that all particles are in the right place after the plotting. We
    also may assume that the initialisation inserts particles only at
    the right point. The realisation of this behaviour can be found in
    Project.generate_Peano4_project().

    Due to the observations above, we can conclude that particles might
    have moved in the first step of the algorithmic sequence if and
    only if the last step has changed a position. Otherwise, we know
    that all is in place. As we run through the species one by one, we
    also have to make this reasoning one by one.

    ## Initialisation

    The initialisation is no different to the time stepping besides two
    places:

    1. We have to use the initialisation steps rather than the
       algorithm steps.
    2. We have to insert an additional dummy sweep initially. After
       users have inserted their particles, they are scattered all
       over the place. If the first initialisation phase then relies
       on continuous AoS data, it will crash.

    The latter "tweak" obviously can be ignored whenever we work without
    coalesced memory.

    ## Tree statistics

    We assume that there is a tree marker already. So our analysis
    will create it, but we assume that it is already out there, so
    we don't have to add it actually to the project.

    ## Adaptive mesh refinement

    If the mesh changes, we have to resort. In the second step of the
    sorting, particles might still be dropped. Throughout drops, we may
    not evaluate particle-particle interactions, as the mesh-particle
    topology continues to change, so we'll evaluate stuff twice. Therefore,
    we have to insert dummy steps. This is not a problem when we bucket
    sort.


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

def initialisation_steps_onto_separate_mesh_traversals_multiscale_sort_scattered_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    return map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )


def initialisation_steps_onto_separate_mesh_traversals_bucket_sort_scattered_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    return map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )


def initialisation_steps_onto_separate_mesh_traversals_multiscale_sort_coalesced_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    return map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )


def initialisation_steps_onto_separate_mesh_traversals_bucket_sort_coalesced_memory(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    return map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )


def particle_steps_onto_separate_mesh_traversals_multiscale_sort_scattered_memory(
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
    return map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )


def particle_steps_onto_separate_mesh_traversals_bucket_sort_scattered_memory(
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
    return map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_SCATTERED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )



def particle_steps_onto_separate_mesh_traversals_multiscale_sort_coalesced_memory(
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
    return map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_WITH_LIFTS_AND_DROPS_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )


def particle_steps_onto_separate_mesh_traversals_bucket_sort_coalesced_memory(
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
    return map_onto_separate_mesh_traversals(
        sequence_of_steps=concatenate_steps_of_species(species_sets, False),
        particle_sorting_and_storage=ParticleSortingAndStorage.SORT_ON_THE_FLY_COALESCED_PARTICLE_STORAGE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )

