# This file is part of the SWIFT2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.AlgorithmStep            import AlgorithmStep
from swift2.actionsets.UpdateParticleMarker   import UpdateParticleMarker

import peano4
import swift2


from swift2.actionsets.AMRLogic import add_mesh_management_optimise_mesh



def serialise_and_map_onto_separate_mesh_traversals(species_sets, 
                                                    verbose=True):
  """
  
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
  
  for current_species_set in species_sets:
    for step in current_species_set.particle_model.algorithm_steps():
      new_step                = peano4.solversteps.Step( "Step{}{}_sweep{}".format(current_species_set.particle_model.name,step.name,len(mesh_traversals)), add_user_defined_actions=False ) 

      new_step.add_action_set(UpdateParticleMarker(current_species_set), append_at_end=True)

      #
      # Pick computational kernel
      # =========================
      # This is where the actual computation per step is done. We have two 
      # fundamentally different kernels: one keeps track of (multiscale) 
      # neighbours, while the other just updates the particles within the
      # octant (cell).
      #
      if step.dependencies==AlgorithmStep.Dependencies.NEIGHBOURS:
        if verbose:
          print( "GRAPH COMPILER DEBUG [serialise_and_map_onto_separate_mesh_traversals]: step {} on particle {} uses peano4.toolbox.particles.ParticleParticleInteraction action set".format( step.name, current_species_set.particle_model.name ) )
        new_step.add_action_set(peano4.toolbox.particles.ParticleParticleInteraction(
            particle_set                                           = current_species_set,
            particle_particle_interaction_kernel                   = step.cell_kernel,
            touch_vertex_first_time_compute_particle_update_kernel = step.touch_vertex_first_time_kernel,
            touch_vertex_last_time_compute_particle_update_kernel  = step.touch_vertex_last_time_kernel,
            prepare_traversal_kernel                               = step.prepare_traversal_kernel,
            unprepare_traversal_kernel                             = step.unprepare_traversal_kernel,
            additional_includes                                    = step.includes,
            active_particle_set                                    = step.input_particles
          ), append_at_end=True)
      elif step.dependencies==AlgorithmStep.Dependencies.SELF:
        if verbose:
          print( "GRAPH COMPILER DEBUG [serialise_and_map_onto_separate_mesh_traversals]: step {} on particle {} uses peano4.toolbox.particles.ParticlePerCellUpdate action set".format( step.name, current_species_set.particle_model.name ) )
        new_step.add_action_set(peano4.toolbox.particles.ParticlePerCellUpdate(
            particle_set                                           = current_species_set,
            particle_cell_update_kernel                            = step.cell_kernel,
            touch_vertex_first_time_compute_particle_update_kernel = step.touch_vertex_first_time_kernel,
            touch_vertex_last_time_compute_particle_update_kernel  = step.touch_vertex_last_time_kernel,
            prepare_traversal_kernel                               = step.prepare_traversal_kernel,
            unprepare_traversal_kernel                             = step.unprepare_traversal_kernel,
            additional_includes                                    = step.includes,
            active_particle_set                                    = step.input_particles
          ), append_at_end=True)
      else:
        assert False, "step {} seems to have no action at all".format( step )
        
      #
      # As we have a 1:1 mapping, we can directly translate the effect into flags
      #
      if step.effect==AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS:
        alter_particle_position.append( True )
      else:
        alter_particle_position.append( False )

        
      mesh_traversals.append( new_step )
          
  add_mesh_management_optimise_mesh( mesh_traversals, 
                                     alter_particle_position, 
                                     species_sets,
                                     verbose )
  return mesh_traversals
  
  
  
  
