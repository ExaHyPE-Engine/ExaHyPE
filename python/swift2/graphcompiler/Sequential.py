# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.AlgorithmStep import AlgorithmStep
from swift2.actionsets.UpdateParticleMarker import UpdateParticleMarker

import peano4
import swift2


from enum import Enum


from .AMRLogic import (
    add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption,
)
from .AMRLogic import (
    add_dynamic_mesh_refinement_and_particle_resorting_minimise_resorting_and_add_gathers,
)


class Optimisation(Enum):
    """ """

    NONE = 0
    INSERT_RESORT_SWEEPS_AND_VECTORISE = 1


# VECTORISE_WHENEVER_POSSIBLE = 1


def translate_one_step_of_species_into_mesh_traversal(
    step,
    current_species_set,
    mesh_traversals,
    alter_particle_position,
    use_lists_to_construct_active_sets,
    preamble,
    verbose,
):
    """!

    Translate one step of species into mesh traversal

    Takes one step of the species and adds it to the list of steps. That is,
    you get one more entry within mesh_traversals. Further to that, the
    routine also appends a boolean arker to alter_particle_position. It
    shows if the mesh will change the particle position (or search radius)
    or not.

    This routine is a helper routine of serialise_initialisation_steps_and_map_onto_separate_mesh_traversals()
    for example. Note that the returned steps are not fully functionally.
    If a step alters the particles within the mesh, you still have to resort
    the particles. That's not done in the step added by this routine, i.e.
    these steps lack the actual particle sorting. Any sorting has to spread
    over two steps, so it makes no sense to add it here. Instead, we befill
    the alter_particle_position, and we expect the calling routine to
    supplement the actual particle sorting later on in a postprocessing
    step once all steps are assembled.

    """
    priority = 0

    new_step = peano4.solversteps.Step(
        "{}{}{}_sweep{}".format(
            preamble,
            current_species_set.particle_model.name,
            step.name,
            len(mesh_traversals),
        ),
        add_user_defined_actions=False,
    )

    action_set_update_marker = UpdateParticleMarker(current_species_set)
    action_set_update_marker.priority = new_step.highest_priority() + 1
    action_set_update_marker.parallel = True
    priority = priority + 1
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
                    "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} can use the peano4.toolbox.particles.ParticleParticleInteraction_StackOfListsOverStaticParticles action set and hence optimise aggressively".format(
                        len(mesh_traversals),
                        step.name,
                        current_species_set.particle_model.name,
                    )
                )
            action_set_particle_particle_interaction = peano4.toolbox.particles.ParticleParticleInteraction_StackOfListsOverStaticParticles(
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
                    "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} uses the peano4.toolbox.particles.ParticleParticleInteraction_Sets".format(
                        len(mesh_traversals),
                        step.name,
                        current_species_set.particle_model.name,
                    )
                )
            action_set_particle_particle_interaction = peano4.toolbox.particles.ParticleParticleInteraction_Sets(
                particle_set=current_species_set,
                particle_particle_interaction_kernel=step.cell_kernel,
                touch_vertex_first_time_compute_particle_update_kernel=step.touch_vertex_first_time_kernel,
                touch_vertex_last_time_compute_particle_update_kernel=step.touch_vertex_last_time_kernel,
                prepare_traversal_kernel=step.prepare_traversal_kernel,
                unprepare_traversal_kernel=step.unprepare_traversal_kernel,
                additional_includes=step.includes,
                active_particle_set=step.input_particles,
            )
        action_set_particle_particle_interaction.priority = (
            new_step.highest_priority() + 1
        )
        action_set_particle_particle_interaction.parallel = True
        priority = priority + 1
        new_step.add_action_set(action_set_particle_particle_interaction)
    elif step.dependencies == AlgorithmStep.Dependencies.SELF:
        if verbose:
            print(
                "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} uses peano4.toolbox.particles.ParticlePerCellUpdate action set".format(
                    len(mesh_traversals),
                    step.name,
                    current_species_set.particle_model.name,
                )
            )

        action_set_particle_update = peano4.toolbox.particles.ParticlePerCellUpdate(
            particle_set=current_species_set,
            particle_cell_update_kernel=step.cell_kernel,
            touch_vertex_first_time_compute_particle_update_kernel=step.touch_vertex_first_time_kernel,
            touch_vertex_last_time_compute_particle_update_kernel=step.touch_vertex_last_time_kernel,
            prepare_traversal_kernel=step.prepare_traversal_kernel,
            unprepare_traversal_kernel=step.unprepare_traversal_kernel,
            additional_includes=step.includes,
            active_particle_set=step.input_particles,
        )
        action_set_particle_update.priority = new_step.highest_priority() + 1
        action_set_particle_update.parallel = True
        priority = priority + 1
        new_step.add_action_set(action_set_particle_update)
    else:
        assert False, "step {} seems to have no action at all".format(step)

    #
    # As we have a 1:1 mapping, we can directly translate the effect into flags
    #
    if (
        step.effect
        == AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN
        or step.effect
        == AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_BUT_NEVER_RERUN
    ):
        alter_particle_position.append(True)
    else:
        alter_particle_position.append(False)

    mesh_traversals.append(new_step)


def serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals(
    species_sets,
    optimisation: Optimisation,
    use_initialisation_steps,
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

    ## Tree statistics

    We assume that there is a tree marker already. So our analysis
    will create it, but we assume that it is already out there, so
    we don't have to add it actually to the project.


    species_sets: [Species]

    optimisation: Optimisation,

    verbose: {True, False}

    """
    mesh_traversals = []
    alter_particle_position = []

    if verbose:
        print(
            "GRAPH COMPILER DEBUG [serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals]: start graph construction with optimisation {} (is initialisation={})".format(
                optimisation, use_initialisation_steps
            )
        )

    for current_species_set in species_sets:
        if verbose:
            print(
                "GRAPH COMPILER DEBUG [serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals]: handle species set {}".format(
                    current_species_set
                )
            )

        if use_initialisation_steps:
            per_species_set = current_species_set.particle_model.initialisation_steps()
            observer_preamble = "Init"
        else:
            per_species_set = current_species_set.particle_model.algorithm_steps()
            observer_preamble = "Step"

        if (
            optimisation == Optimisation.INSERT_RESORT_SWEEPS_AND_VECTORISE
            and not current_species_set.generator.gather_particles
        ):
            # suppress warning if we run in release version without verbose, as the
            # initialisation might always be off
            if not use_initialisation_steps and not verbose:
                print(
                    "GRAPH COMPILER DEBUG [serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals]: WARNING have been asked to vectorise for species {}, but species does not support gathered data structures. Vectorisation disabled".format(
                        current_species_set.particle_model.name
                    )
                )
            optimisation = Optimisation.NONE

        for step in per_species_set:
            if optimisation == Optimisation.NONE:
                use_lists_to_construct_active_sets = False
                add_additional_dummy_sweep = False
            # elif optimisation==Optimisation.VECTORISE_WHENEVER_POSSIBLE:
            #              use_lists_to_construct_active_sets = step.effect != AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN and step.effect != AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_BUT_NEVER_RERUN and len(alter_particle_position)>0 and not alter_particle_position[-1]
            #              add_additional_dummy_sweep        = False
            elif optimisation == Optimisation.INSERT_RESORT_SWEEPS_AND_VECTORISE:
                assert (
                    current_species_set.generator.gather_particles
                ), "cannot try to vectorise without gathered data structures"
                use_lists_to_construct_active_sets = (
                    step.effect
                    != AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN
                )
                add_additional_dummy_sweep = (
                    step.effect
                    == AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_BUT_NEVER_RERUN
                    or step.effect
                    == AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN
                )
            else:
                assert False, "option not supported"

            translate_one_step_of_species_into_mesh_traversal(
                step=step,
                current_species_set=current_species_set,
                mesh_traversals=mesh_traversals,
                alter_particle_position=alter_particle_position,
                use_lists_to_construct_active_sets=use_lists_to_construct_active_sets,
                preamble=observer_preamble,
                verbose=verbose,
            )

            if add_additional_dummy_sweep:
                if verbose:
                    print(
                        "GRAPH COMPILER DEBUG [serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals]: step no {} ({}) on particle {} has to be dummy step to clean up sorting".format(
                            len(mesh_traversals),
                            step.name,
                            current_species_set.particle_model.name,
                        )
                    )
                new_step = peano4.solversteps.Step(
                    "DummyStep{}{}_sweep{}".format(
                        current_species_set.particle_model.name,
                        step.name,
                        len(mesh_traversals),
                    ),
                    add_user_defined_actions=False,
                )
                mesh_traversals.append(new_step)
                alter_particle_position.append(True)

        if current_species_set.generator.gather_particles:
            add_dynamic_mesh_refinement_and_particle_resorting_minimise_resorting_and_add_gathers(
                mesh_traversals, alter_particle_position, species_sets, verbose
            )
        else:
            add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption(
                mesh_traversals, alter_particle_position, species_sets, verbose
            )

    if verbose:
        print(
            "GRAPH COMPILER DEBUG [serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals]: finished graph construction"
        )

    return mesh_traversals


def serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals_no_vec(
    species_sets,
    verbose,
):
    return serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals(
        species_sets=species_sets,
        optimisation=Optimisation.NONE,
        use_initialisation_steps=False,
        verbose=verbose,
    )


def serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals_insert_dummy_sweeps_and_vectorise(
    species_sets,
    verbose,
):
    return serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals(
        species_sets=species_sets,
        optimisation=Optimisation.INSERT_RESORT_SWEEPS_AND_VECTORISE,
        use_initialisation_steps=False,
        verbose=verbose,
    )


def serialise_initialisation_steps_and_map_onto_separate_mesh_traversals(
    species_sets,
    verbose,
):
    return serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals(
        species_sets=species_sets,
        optimisation=Optimisation.INSERT_RESORT_SWEEPS_AND_VECTORISE,
        use_initialisation_steps=True,
        verbose=verbose,
    )
