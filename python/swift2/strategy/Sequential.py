# This file is part of the SWIFT2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4


def serialise_and_map_onto_separate_mesh_traversals(species_sets, verbose=True):
  """
  
  Very simple compiler which takes each individual algorithmic step 
  per species and maps it onto one mesh traversal. We won't get any
  fancy parallelism besides the core domain decomposition. 
  
  As we work with a strictly synchronised way, we can ignore in and 
  outgoing dependencies which affect the global state. They are 
  implicitly handled.
  
  """
  mesh_traversals = []
  previous_step_has_altered_particle_positions = True
  
  for current_species_set in species_sets:
    for step in current_species_set.particle_model.algorithm_steps():
      use_action_set_with_neighbours = False
      use_action_set_alters_position = False
      if step.dependencies==AlgorithmStep.Dependencies.NEIGHBOURS or step.dependencies==AlgorithmStep.Dependencies.NEIGHBOURS_GLOBAL:
        use_action_set_with_neighbours = True
      if step.effect==AlgorithmStep.Effect.POSITION or step.effect==AlgorithmStep.Effect.POSITION_GLOBAL:
        use_action_set_alters_position = True

      new_step = peano4.solversteps.Step( "Step{}{}{}".format(current_species_set.particle_model.name,step.name,len(mesh_traversals)), False ) 
      if use_action_set_with_neighbours and (use_action_set_alters_position or previous_step_has_altered_particle_positions):
        if verbose:
          print( "GRAPH COMPILER DEBUG: step {} on particle {} uses Neighbours+Resort".format( step.name, current_species_set.particle_model.name ) )
        new_step.add_action_set(peano4.toolbox.particles.UpdateParticleGridAssociation(current_species_set),False)
        new_step.add_action_set(peano4.toolbox.particles.UpdateParallelState(current_species_set),False)
        #new_step.add_action_set(peano4.toolbox.particles.ParticleParticleInteraction(
        #    particle_set                                           = current_species_set,
        #    particle_particle_interaction_kernel                   = step.cell_kernel,
        #    touch_vertex_first_time_compute_particle_update_kernel = step.touch_vertex_first_time_kernel,
        #    touch_vertex_last_time_compute_particle_update_kernel  = step.touch_vertex_last_time_kernel,
        #    additional_includes                                    = step.includes,
        #    active_particle_set                                    = step.input_particles
        #  ),False)
      elif use_action_set_with_neighbours:
        if verbose:
          print( "GRAPH COMPILER DEBUG: step {} on particle {} uses Neighbours".format( step.name, current_species_set.particle_model.name ) )
        #new_step.add_action_set(peano4.toolbox.particles.ParticleParticleInteraction(
        #    particle_set                                           = current_species_set,
        #    particle_particle_interaction_kernel                   = step.cell_kernel,
        #    touch_vertex_first_time_compute_particle_update_kernel = step.touch_vertex_first_time_kernel,
        #    touch_vertex_last_time_compute_particle_update_kernel  = step.touch_vertex_last_time_kernel,
        #    additional_includes                                    = step.includes,
        #    active_particle_set                                    = step.input_particles
        #  ),False)
      elif use_action_set_alters_position or previous_step_has_altered_particle_positions:
        if verbose:
          print( "GRAPH COMPILER DEBUG: step {} on particle {} uses Cell-wise Access+Resort".format( step.name, current_species_set.particle_model.name ) )
        new_step.add_action_set(peano4.toolbox.particles.UpdateParticleGridAssociation(current_species_set),False)
        new_step.add_action_set(peano4.toolbox.particles.UpdateParallelState(current_species_set),False)
        new_step.add_action_set(peano4.toolbox.particles.ParticlePerCellUpdate(
            particle_set                                           = current_species_set,
            particle_cell_update_kernel                            = step.cell_kernel,
            touch_vertex_first_time_compute_particle_update_kernel = step.touch_vertex_first_time_kernel,
            touch_vertex_last_time_compute_particle_update_kernel  = step.touch_vertex_last_time_kernel,
            additional_includes                                    = step.includes,
            active_particle_set                                    = step.input_particles
          ),False)
      else:
        if verbose:
          print( "GRAPH COMPILER DEBUG: step {} on particle {} uses Cell-wise Access".format( step.name, current_species_set.particle_model.name ) )
        new_step.add_action_set(peano4.toolbox.particles.ParticlePerCellUpdate(
            particle_set                                           = current_species_set,
            particle_cell_update_kernel                            = step.cell_kernel,
            touch_vertex_first_time_compute_particle_update_kernel = step.touch_vertex_first_time_kernel,
            touch_vertex_last_time_compute_particle_update_kernel  = step.touch_vertex_last_time_kernel,
            additional_includes                                    = step.includes,
            active_particle_set                                    = step.input_particles
          ),False)
      mesh_traversals.append( new_step )
          
      previous_step_has_altered_particle_positions = use_action_set_alters_position
  return mesh_traversals
  
  
  
  