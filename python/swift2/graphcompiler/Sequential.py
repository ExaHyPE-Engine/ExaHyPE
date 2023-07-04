# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.AlgorithmStep import AlgorithmStep
from swift2.actionsets.UpdateParticleMarker import UpdateParticleMarker

import peano4
import swift2


from .AMRLogic import add_dynamic_mesh_refinement_and_particle_resorting


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
        if verbose:
            print(
                "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} uses peano4.toolbox.particles.ParticleParticleInteraction_StackOfLists action set".format(
                    len(mesh_traversals),
                    step.name,
                    current_species_set.particle_model.name,
                )
            )
            
        if use_lists_to_construct_active_sets:
          action_set_particle_particle_interaction = peano4.toolbox.particles.ParticleParticleInteraction_StackOfLists(
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
    if step.effect == AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS:
        alter_particle_position.append(True)
    else:
        alter_particle_position.append(False)

    mesh_traversals.append(new_step)


def serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals(
    species_sets, use_lists_to_construct_active_sets, verbose=True
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


    """
    mesh_traversals = []
    alter_particle_position = []

    if verbose:
        print(
            "GRAPH COMPILER DEBUG [serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals]: start graph construction"
        )

    for current_species_set in species_sets:
        for step in current_species_set.particle_model.algorithm_steps():
            if use_lists_to_construct_active_sets and step.dependencies == AlgorithmStep.Dependencies.NEIGHBOURS and len(mesh_traversals)>0 and alter_particle_position[-1]:
              new_step = peano4.solversteps.Step(
                "DummyStep{}{}_sweep{}".format(
                  current_species_set.particle_model.name,
                  step.name,
                  len(mesh_traversals),
                ),
                add_user_defined_actions=False,
              )
              mesh_traversals.append(new_step)
              print(
                  "GRAPH COMPILER DEBUG [serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals]: step no {} ({}) on particle {} has to be dummy step to clean up sorting".format(
                    len(mesh_traversals),
                    step.name,
                    current_species_set.particle_model.name,
                )
            )
            
            translate_one_step_of_species_into_mesh_traversal(
                step                                = step,
                current_species_set                 = current_species_set,
                mesh_traversals                     = mesh_traversals,
                alter_particle_position             = alter_particle_position,
                use_lists_to_construct_active_sets  = use_lists_to_construct_active_sets,
                preamble                            = "Step",
                verbose                             = verbose,
            )

    add_dynamic_mesh_refinement_and_particle_resorting(
        mesh_traversals, alter_particle_position, species_sets, verbose
    )

    if verbose:
        print(
            "GRAPH COMPILER DEBUG [serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals]: finished graph construction"
        )

    return mesh_traversals


def serialise_initialisation_steps_and_map_onto_separate_mesh_traversals(
    species_sets, verbose=True
):
    """!

    Translate each initialisation step for each particle into a separate mesh sweep

    We always add one more sorting step after all of that

    The initialisation always uses sets to construct the active lists. 
    Performance should not be that much of an issue here, and we rather
    minimise the number of mesh traversals and stay generic.
    
    """
    mesh_traversals = []
    alter_particle_position = []

    verbose = True

    if verbose:
        print(
            "GRAPH COMPILER DEBUG [serialise_initialisation_steps_and_map_onto_separate_mesh_traversals]: start graph construction"
        )

    for current_species_set in species_sets:
        for step in current_species_set.particle_model.initialisation_steps():
            translate_one_step_of_species_into_mesh_traversal(
                step                     = step,
                current_species_set      = current_species_set,
                mesh_traversals          = mesh_traversals,
                alter_particle_position  = alter_particle_position,
                preamble                 = "Initialisation",
                verbose                  = verbose,
                use_lists_to_construct_active_sets=False
            )

    additional_initialisation_step = peano4.solversteps.Step(
        "InitialisationStartInitialSort", add_user_defined_actions=False
    )
    mesh_traversals.append(additional_initialisation_step)
    alter_particle_position.append(True)

    additional_initialisation_step = peano4.solversteps.Step(
        "InitialisationFinaliseInitialSort", add_user_defined_actions=False
    )
    mesh_traversals.append(additional_initialisation_step)
    alter_particle_position.append(True)

    if verbose:
        print(
            "GRAPH COMPILER DEBUG [serialise_initialisation_steps_and_map_onto_separate_mesh_traversals]: initial graph built with {} steps".format(
                len(mesh_traversals)
            )
        )

    add_dynamic_mesh_refinement_and_particle_resorting(
        mesh_traversals, alter_particle_position, species_sets, verbose
    )

    if verbose:
        print(
            "GRAPH COMPILER DEBUG [serialise_initialisation_steps_and_map_onto_separate_mesh_traversals]: finished graph construction with {} steps".format(
                len(mesh_traversals)
            )
        )

    return mesh_traversals
