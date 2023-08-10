# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.AlgorithmStep import AlgorithmStep
from swift2.actionsets.UpdateParticleMarker import UpdateParticleMarker

import peano4
import swift2


from .Sequential import translate_one_step_of_species_into_mesh_traversal
from .AMRLogic import add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption
from .AMRLogic import add_dynamic_mesh_refinement_and_particle_resorting_minimise_resorting_and_add_gathers


def serialise_initialisation_steps_and_map_onto_separate_mesh_traversals(species_sets, 
                                                                         verbose,
                                                                         ):
    """!

    Translate each initialisation step for each particle into a separate mesh sweep
    
    At this point, we assume that all particles are inserted into the mesh. In 
    line with the particle toolbox documentation, we can assume that this 
    insertion is consistent, i.e. particles are dropped into the right 
    location. They are properly sorted. However, the initialisation might alter
    particle attributes, and it might also trigger some mesh adaption. 
    Therefore, we have to add some additional sorting steps after we've 
    initialised the particles.

    The initialisation is extremely similar to serialise_algorithmic_steps_and_map_onto_separate_mesh_traversals().
    Different to this one, we ignore the optimisation however and instead 
    always work with the list implementation to build up the active and local 
    lists. The list implementation is provides additional information to the 
    actual compute kernels such as "how many particles have been added due to
    one vertex". This information is only available if the mesh-particle 
    association remains stationary - which is the case here by definition. 
    Therefore, we can use the more sophisticated particle management and thus
    support a wider range of compute kernels.
   
    species_sets: [Species]
                         
    verbose: {True, False}
    
    """
    mesh_traversals = []
    alter_particle_position = []

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
                use_lists_to_construct_active_sets = True,
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
            "GRAPH COMPILER DEBUG [serialise_initialisation_steps_and_map_onto_separate_mesh_traversals]: finished graph construction with {} steps".format(
                len(mesh_traversals)
            )
        )

    return mesh_traversals
