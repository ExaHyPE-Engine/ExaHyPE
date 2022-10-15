# This file is part of the SWIFT2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.AlgorithmStep            import AlgorithmStep
from swift2.actionsets.UpdateParticleMarker   import UpdateParticleMarker


import peano4
import swift2


def serialise_and_map_onto_separate_mesh_traversals(species_sets, verbose=True):
  """
  
  Very simple compiler which takes each individual algorithmic step 
  per species and maps it onto one mesh traversal. We won't get any
  fancy parallelism besides the core domain decomposition. 
  
  As we work with a strictly synchronised way, we can ignore in and 
  outgoing dependencies which affect the global state. They are 
  implicitly handled.
  
  We get a sequence of tuples back. Each tuple has the following structure:
 
  - The first entry is a Peano mesh traversal (adapter) object which you can 
    hand over directly to Peano. 
  - The second entry is a string which should be called ahead of this step
    per 
  
  """
  mesh_traversals = []
  previous_step_has_altered_particle_positions = True
  
  for current_species_set in species_sets:
    for step in current_species_set.particle_model.algorithm_steps():
      new_step = peano4.solversteps.Step( "Step{}{}{}".format(current_species_set.particle_model.name,step.name,len(mesh_traversals)), False ) 
      if step.dependencies==AlgorithmStep.Dependencies.NEIGHBOURS and (step.effect==AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS or previous_step_has_altered_particle_positions):
        if verbose:
          print( "GRAPH COMPILER DEBUG: step {} on particle {} uses Neighbours+Resort".format( step.name, current_species_set.particle_model.name ) )
        new_step.add_action_set(UpdateParticleMarker(current_species_set),False)
        new_step.add_action_set(peano4.toolbox.particles.UpdateParticleGridAssociation(current_species_set),False)
        new_step.add_action_set(peano4.toolbox.particles.UpdateParallelState(current_species_set),False)
        new_step.add_action_set(peano4.toolbox.particles.ParticleParticleInteraction(
            particle_set                                           = current_species_set,
            particle_particle_interaction_kernel                   = step.cell_kernel,
            touch_vertex_first_time_compute_particle_update_kernel = step.touch_vertex_first_time_kernel,
            touch_vertex_last_time_compute_particle_update_kernel  = step.touch_vertex_last_time_kernel,
            prepare_traversal_kernel                               = step.prepare_traversal_kernel,
            unprepare_traversal_kernel                             = step.unprepare_traversal_kernel,
            additional_includes                                    = step.includes,
            active_particle_set                                    = step.input_particles
          ),False)
      elif step.dependencies==AlgorithmStep.Dependencies.NEIGHBOURS and step.effect==AlgorithmStep.Effect.ALTER_LOCAL_STATE:
        if verbose:
          print( "GRAPH COMPILER DEBUG: step {} on particle {} uses Neighbours".format( step.name, current_species_set.particle_model.name ) )
        new_step.add_action_set(UpdateParticleMarker(current_species_set),False)
        new_step.add_action_set(peano4.toolbox.particles.ParticleParticleInteraction(
            particle_set                                           = current_species_set,
            particle_particle_interaction_kernel                   = step.cell_kernel,
            touch_vertex_first_time_compute_particle_update_kernel = step.touch_vertex_first_time_kernel,
            touch_vertex_last_time_compute_particle_update_kernel  = step.touch_vertex_last_time_kernel,
            prepare_traversal_kernel                               = step.prepare_traversal_kernel,
            unprepare_traversal_kernel                             = step.unprepare_traversal_kernel,
            additional_includes                                    = step.includes,
            active_particle_set                                    = step.input_particles
          ),False)
      elif step.dependencies==AlgorithmStep.Dependencies.SELF and (step.effect==AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS or previous_step_has_altered_particle_positions):
        if verbose:
          print( "GRAPH COMPILER DEBUG: step {} on particle {} uses Cell-wise Access+Resort".format( step.name, current_species_set.particle_model.name ) )
        new_step.add_action_set(UpdateParticleMarker(current_species_set),False)
        new_step.add_action_set(peano4.toolbox.particles.UpdateParticleGridAssociation(current_species_set),False)
        new_step.add_action_set(peano4.toolbox.particles.UpdateParallelState(current_species_set),False)
        new_step.add_action_set(peano4.toolbox.particles.ParticlePerCellUpdate(
            particle_set                                           = current_species_set,
            particle_cell_update_kernel                            = step.cell_kernel,
            touch_vertex_first_time_compute_particle_update_kernel = step.touch_vertex_first_time_kernel,
            touch_vertex_last_time_compute_particle_update_kernel  = step.touch_vertex_last_time_kernel,
            prepare_traversal_kernel                               = step.prepare_traversal_kernel,
            unprepare_traversal_kernel                             = step.unprepare_traversal_kernel,
            additional_includes                                    = step.includes,
            active_particle_set                                    = step.input_particles
          ),False)
        # @todo Optimieren mit const falls nur global state
      elif step.dependencies==AlgorithmStep.Dependencies.SELF and (step.effect==AlgorithmStep.Effect.ALTER_LOCAL_STATE or step.effect==AlgorithmStep.Effect.ALTER_GLOBAL_STATE):
        if verbose:
          print( "GRAPH COMPILER DEBUG: step {} on particle {} uses Cell-wise Access".format( step.name, current_species_set.particle_model.name ) )
        new_step.add_action_set(UpdateParticleMarker(current_species_set),False)
        new_step.add_action_set(peano4.toolbox.particles.ParticlePerCellUpdate(
            particle_set                                           = current_species_set,
            particle_cell_update_kernel                            = step.cell_kernel,
            touch_vertex_first_time_compute_particle_update_kernel = step.touch_vertex_first_time_kernel,
            touch_vertex_last_time_compute_particle_update_kernel  = step.touch_vertex_last_time_kernel,
            prepare_traversal_kernel                               = step.prepare_traversal_kernel,
            unprepare_traversal_kernel                             = step.unprepare_traversal_kernel,
            additional_includes                                    = step.includes,
            active_particle_set                                    = step.input_particles
          ),False)
      else:
        assert False, "variant for step {} not handled".format( step )
      mesh_traversals.append( new_step )
          
      previous_step_has_altered_particle_positions = step.effect==AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS
  return mesh_traversals
  
  
  
  