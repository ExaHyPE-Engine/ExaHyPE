# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.AlgorithmStep import AlgorithmStep
from swift2.actionsets.UpdateParticleMarker import UpdateParticleMarker

import peano4
import swift2


from .AMRLogic import (
    add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption,
)
from .AMRLogic import (
    add_dynamic_mesh_refinement_and_particle_resorting_minimise_resorting_and_add_gathers,
)
from .Optimisation import Optimisation
from .SpeciesStepOrdering import concatenate_steps_of_species
from .Utils import does_one_species_require_gather, get_species_set, get_observer_prefix, StepType


DefaultIncludes = """
#include "tarch/multicore/Tasks.h"                
#include "swift2/VertexTask.h"                
                """


def construct_touch_vertex_first_time_call(current_species_set,
                                           touch_vertex_first_time_kernel):
    if touch_vertex_first_time_kernel==None:
        return ""
    else:
        return """
      const int taskNumberToBeUsed = fineGridVertex""" + peano4.toolbox.TaskNumbersOnMesh.construct_marker_name(current_species_set.name) + """.getTouchFirstNumber();
      
      if ( taskNumberToBeUsed!=tarch::multicore::NoDependency ) {
        auto* newTask = new ::swift2::VertexTask< decltype(localParticles) >(
          localParticles,
          activeParticles,
          [=](decltype(localParticles)& localParticles, decltype(localParticles)& activeParticles)->void {
            """ + touch_vertex_first_time_kernel + """
          }
        );
      
        tarch::multicore::spawnTaskWithDependencies( newTask, tarch::multicore::NoDependency, taskNumberToBeUsed );
      }
      else {
        """ + touch_vertex_first_time_kernel + """
      }
    """


def construct_touch_cell_first_time_call(current_species_set,
                                         touch_cell_first_time_kernel):
    if touch_cell_first_time_kernel==None:
        return ""
    else:
        return """
      const int taskNumberToBeUsed = fineGridCell""" + peano4.toolbox.TaskNumbersOnMesh.construct_marker_name(current_species_set.name) + """.getTouchLastNumber();
      
      std::vector<int> adjacentVertexTask(TwoPowerD);
      for (int i=0; i<TwoPowerD; i++) {
        adjacentVertexTask[i] = fineGridVertices""" + peano4.toolbox.TaskNumbersOnMesh.construct_marker_name(current_species_set.name) + """(i).getTouchFirstNumber();
      }

      if ( taskNumberToBeUsed!=tarch::multicore::NoDependency ) {        
        auto* newTask = new ::swift2::VertexTask< decltype(localParticles) >(
          localParticles,
          activeParticles,
          [=](decltype(localParticles)& localParticles, decltype(localParticles)& activeParticles)->void {
            """ + touch_cell_first_time_kernel + """
          }
        );
      
        tarch::multicore::spawnTaskWithDependencies( newTask, adjacentVertexTask, taskNumberToBeUsed );
      }
      else {
        tarch::multicore::waitForTasks( adjacentVertexTask );
        """ + touch_cell_first_time_kernel + """
      }
    """


def construct_touch_vertex_last_time_call(current_species_set,
                                          touch_vertex_last_time_kernel):
    result = """
    tarch::multicore::waitForTasks( fineGridVertex""" + peano4.toolbox.TaskNumbersOnMesh.construct_marker_name(current_species_set.name) + """.getAdjacentCellsTouchLastNumber() );
    """

    if touch_vertex_last_time_kernel!=None:
      result += touch_vertex_last_time_kernel + """
    """
    
    return result


#
# @todo Here for final SWIFT-like task graph construction, i.e. left on purpose
#
#def construct_touch_vertex_last_time_call(current_species_set,
#                                          touch_vertex_last_time_kernel):
#    if touch_vertex_last_time_kernel==None:
#        return ""
#    else:
#        return """
#      const int taskNumberToBeUsed = fineGridVertex""" + peano4.toolbox.TaskNumbersOnMesh.construct_marker_name(current_species_set.name) + """.getTouchLastNumber();
#      
#      if ( taskNumberToBeUsed!=tarch::multicore::NoDependency ) {
#        auto* newTask = new ::swift2::VertexTask< decltype(localParticles) >(
#          localParticles,
#          activeParticles,
#          [=](decltype(localParticles)& localParticles, decltype(localParticles)& activeParticles)->void {
#            """ + touch_vertex_last_time_kernel + """
#          }
#        );
#      
#        tarch::multicore::spawnTaskWithDependencies( 
#          newTask, 
#          fineGridVertex""" + peano4.toolbox.TaskNumbersOnMesh.construct_marker_name(current_species_set.name) + """.getAdjacentCellsTouchLastNumber(),
#          tarch::multicore::NoDependency
#        );
#      }
#      else {
#        tarch::multicore::waitForTasks( fineGridVertex""" + peano4.toolbox.TaskNumbersOnMesh.construct_marker_name(current_species_set.name) + """.getAdjacentCellsTouchLastNumber() );
#        """ + touch_vertex_last_time_kernel + """
#      }
#    """


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
    
    We create a new step and then add various different action sets depending 
    on the context and behaviour. Once befilled, the new step is appended to
    mesh_traversals.
    
    1. If this is an initialisation sequence, then I clear all the task 
       markers. This is to ensure that they do not hold any garbage. I can 
       clear the set as often as I want. It is just important that I clear
       it at least once per particle species.
    2. Update the particle markers (analysis of particles per cell). This is
       something each and every step in Peano does.
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

    #
    # Erase task markers throughout initialisation
    #
    if step_type==StepType.INITIALISATION:
        if verbose:
            print(
                "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} is initialisation step and hence uses the peano4.toolbox.ClearTaskNumbersOnMesh action set".format(
                    len(mesh_traversals),
                    step.name,
                    current_species_set.particle_model.name,
                )
            )
        action_set_erase_task_markers          = peano4.toolbox.ClearTaskNumbersOnMesh(current_species_set.name)
        action_set_erase_task_markers.descend_invocation_order = new_step.lowest_descend_invocation_order() - 1
        action_set_erase_task_markers.parallel = True
        new_step.add_action_set(action_set_erase_task_markers)


    action_set_update_marker = UpdateParticleMarker(current_species_set)
    action_set_update_marker.descend_invocation_order = new_step.highest_descend_invocation_order() + 1
    action_set_update_marker.parallel = True
    new_step.add_action_set(action_set_update_marker)

    #
    # Pick computational kernel
    #
    if step.dependencies == AlgorithmStep.Dependencies.NEIGHBOURS:
        if use_lists_to_construct_active_sets:
            if verbose:
                print(
                    "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} uses the peano4.toolbox.particles.ParticleParticleInteraction_StackOfListsOverStaticParticles action set and hence optimise aggressively".format(
                        len(mesh_traversals),
                        step.name,
                        current_species_set.particle_model.name,
                    )
                )
            action_set_particle_particle_interaction = peano4.toolbox.particles.ParticleParticleInteraction_StackOfListsOverStaticParticles(
                particle_set=current_species_set,
                particle_particle_interaction_kernel=construct_touch_cell_first_time_call(step.cell_kernel),
                touch_vertex_first_time_compute_particle_update_kernel=construct_touch_vertex_first_time_call(current_species_set,step.touch_vertex_first_time_kernel),
                touch_vertex_last_time_compute_particle_update_kernel=construct_touch_vertex_last_time_call(step.touch_vertex_last_time_kernel),
                prepare_traversal_kernel=step.prepare_traversal_kernel,
                unprepare_traversal_kernel=step.unprepare_traversal_kernel,
                additional_includes=step.includes + DefaultIncludes,
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
                particle_particle_interaction_kernel=construct_touch_cell_first_time_call(current_species_set,step.cell_kernel),
                touch_vertex_first_time_compute_particle_update_kernel=construct_touch_vertex_first_time_call(current_species_set,step.touch_vertex_first_time_kernel),
                touch_vertex_last_time_compute_particle_update_kernel=construct_touch_vertex_last_time_call(current_species_set,step.touch_vertex_last_time_kernel),
                prepare_traversal_kernel=step.prepare_traversal_kernel,
                unprepare_traversal_kernel=step.unprepare_traversal_kernel,
                additional_includes=step.includes + DefaultIncludes,
                active_particle_set=step.input_particles,
            )
        action_set_particle_particle_interaction.descend_invocation_order = (
            new_step.highest_descend_invocation_order() + 1
        )
        action_set_particle_particle_interaction.parallel = True
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
            particle_cell_update_kernel=construct_touch_cell_first_time_call(current_species_set,step.cell_kernel),
            touch_vertex_first_time_compute_particle_update_kernel=construct_touch_vertex_first_time_call(current_species_set,step.touch_vertex_first_time_kernel),
            touch_vertex_last_time_compute_particle_update_kernel=construct_touch_vertex_last_time_call(current_species_set,step.touch_vertex_last_time_kernel),
            prepare_traversal_kernel=step.prepare_traversal_kernel,
            unprepare_traversal_kernel=step.unprepare_traversal_kernel,
            additional_includes=step.includes + DefaultIncludes,
            active_particle_set=step.input_particles,
        )
        action_set_particle_update.descend_invocation_order = new_step.highest_descend_invocation_order() + 1
        action_set_particle_update.parallel = True
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
        if verbose:
            print(
                "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} alters particle state and hence uses the peano4.toolbox.ClearTaskNumbersOnMesh action set".format(
                    len(mesh_traversals),
                    step.name,
                    current_species_set.particle_model.name,
                )
            )
        alter_particle_position.append(True)
        action_set_erase_task_markers          = peano4.toolbox.ClearTaskNumbersOnMesh(current_species_set.name)
        action_set_erase_task_markers.descend_invocation_order = new_step.lowest_descend_invocation_order() - 1
        action_set_erase_task_markers.parallel = True
        new_step.add_action_set(action_set_erase_task_markers)
    else:
        if verbose:
            print(
                "GRAPH COMPILER DEBUG [translate_one_step_of_species_into_mesh_traversal]: step no {} ({}) on particle {} alters particle state and hence uses the peano4.toolbox.AssignTaskNumbersToMesh action set".format(
                    len(mesh_traversals),
                    step.name,
                    current_species_set.particle_model.name,
                )
            )
        alter_particle_position.append(False)
        action_set_erase_task_markers          = peano4.toolbox.AssignTaskNumbersToMesh(current_species_set.name,
                                                                                        touch_cell_first_time_assignment_predicate = "false",
                                                                                        touch_vertex_last_time_assignment_predicate = "not marker.isAdjacentToParallelDomainBoundary()"
                                                                                        )
        action_set_erase_task_markers.descend_invocation_order = new_step.lowest_descend_invocation_order() - 1
        action_set_erase_task_markers.parallel = True
        new_step.add_action_set(action_set_erase_task_markers)

    mesh_traversals.append(new_step)


def map_onto_separate_mesh_traversals_constructing_task_tree(
    sequence_of_steps,
    optimisation: Optimisation,
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

    @param optimisation: Optimisation,

    @param observer_preamble: String
      Usually people pass in "Init" or "Step" here to distinguish observers for
      the initialisation from the actual particle update steps.

    @param verbose: {True, False}

    @return mesh_traversals

    """
    mesh_traversals = []
    alter_particle_position = []

    if verbose:
        print(
            "GRAPH COMPILER DEBUG [map_onto_separate_mesh_traversals_constructing_task_tree]: start graph construction with optimisation {} and step type {}".format(
                optimisation, step_type
            )
        )

    for current_entry_from_total_sequence_of_steps in sequence_of_steps:
        current_species_set = current_entry_from_total_sequence_of_steps[0]
        current_step_of_species = current_entry_from_total_sequence_of_steps[1]

        assert (
            optimisation == Optimisation.NONE
            or current_species_set.generator.gather_particles
        ), "GRAPH COMPILER DEBUG [map_onto_separate_mesh_traversals_constructing_task_tree]: ERROR have been asked to vectorise for species {}, but species does not support gathered data structures".format(
            current_species_set.particle_model.name
        )
        if optimisation == Optimisation.NONE:
            use_lists_to_construct_active_sets = False
            add_additional_dummy_sweep = False
        # @todo Wieder rein
        # elif optimisation==Optimisation.VECTORISE_WHENEVER_POSSIBLE:
        #              use_lists_to_construct_active_sets = step.effect != AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN and step.effect != AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_BUT_NEVER_RERUN and len(alter_particle_position)>0 and not alter_particle_position[-1]
        #              add_additional_dummy_sweep        = False
        elif optimisation == Optimisation.INSERT_RESORT_SWEEPS_AND_VECTORISE:
            use_lists_to_construct_active_sets = True
            add_additional_dummy_sweep = (
                current_step_of_species.effect
                == AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_BUT_NEVER_RERUN
                or current_step_of_species.effect
                == AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN
            )
        else:
            assert False, "option not supported"

        translate_one_step_of_species_into_mesh_traversal(
            step=current_step_of_species,
            current_species_set=current_species_set,
            mesh_traversals=mesh_traversals,
            alter_particle_position=alter_particle_position,
            use_lists_to_construct_active_sets=use_lists_to_construct_active_sets,
            step_type=step_type,
            verbose=verbose,
        )

        if add_additional_dummy_sweep:
            if verbose:
                print(
                    "GRAPH COMPILER DEBUG [map_onto_separate_mesh_traversals_constructing_task_tree]: step no {} ({}) on particle {} has to be dummy step to clean up sorting".format(
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
            mesh_traversals.append(new_step)
            alter_particle_position.append(True)
            
    if step_type==StepType.INITIALISATION:
        alter_particle_position[0] = True

    if does_one_species_require_gather(sequence_of_steps):
        add_dynamic_mesh_refinement_and_particle_resorting_minimise_resorting_and_add_gathers(
            mesh_traversals,
            alter_particle_position,
            get_species_set(sequence_of_steps),
            verbose,
        )
    else:
        add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption(
            mesh_traversals,
            alter_particle_position,
            get_species_set(sequence_of_steps),
            verbose,
        )

    if verbose:
        print(
            "GRAPH COMPILER DEBUG [map_onto_separate_mesh_traversals_constructing_task_tree]: finished graph construction"
        )

    return mesh_traversals


def map_particle_steps_onto_task_tree(
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
        optimisation=Optimisation.NONE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )


def map_particle_steps_onto_task_tree_insert_dummy_sweeps_and_vectorise(
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
        optimisation=Optimisation.INSERT_RESORT_SWEEPS_AND_VECTORISE,
        step_type=StepType.TIME_STEP,
        verbose=verbose,
    )


def map_initialisation_steps_onto_task_tree(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    return map_onto_separate_mesh_traversals_constructing_task_tree(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        optimisation=Optimisation.NONE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )


def map_initialisation_steps_onto_task_tree_for_coalesced_memory_access(
    species_sets,
    verbose,
):
    """!

    @see map_onto_separate_mesh_traversals()

    @param concatenate_steps_of_species: [Species]
      Sequence of particle species to be handled.

    @param verbose: {True, False}

    """
    return map_onto_separate_mesh_traversals_constructing_task_tree(
        sequence_of_steps=concatenate_steps_of_species(species_sets, True),
        optimisation=Optimisation.INSERT_RESORT_SWEEPS_AND_VECTORISE,
        step_type=StepType.INITIALISATION,
        verbose=verbose,
    )
