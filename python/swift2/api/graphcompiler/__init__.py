""" This file is part of the SWIFT 2 project. For conditions of distribution and 
 use, please see the copyright notice at www.peano-framework.org
"""


from .SpeciesStepOrdering import (
    concatenate_steps_of_species,
    interleave_steps_of_species,
)


from .Sequential import (
    particle_steps_onto_separate_mesh_traversals_insert_sort_sweeps_scattered_memory,
    particle_steps_onto_separate_mesh_traversals_sort_on_the_fly_scattered_memory,
    initialisation_steps_onto_separate_mesh_traversals_insert_sort_sweeps_scattered_memory,
    initialisation_steps_onto_separate_mesh_traversals_sort_on_the_fly_scattered_memory
)


#from .TaskTree import (
#    map_particle_steps_onto_task_tree,
#    map_particle_steps_onto_task_tree_insert_dummy_sweeps_and_vectorise,
#    map_initialisation_steps_onto_task_tree,
#    map_initialisation_steps_onto_task_tree_for_coalesced_memory_access,
#)


#from .TaskGraph import (
#    map_particle_steps_onto_task_graph,
#    map_particle_steps_onto_task_graph_insert_dummy_sweeps_and_vectorise,
#    map_initialisation_steps_onto_task_graph,
#    map_initialisation_steps_onto_task_graph_for_coalesced_memory_access,
#)
