""" This file is part of the SWIFT 2 project. For conditions of distribution and 
 use, please see the copyright notice at www.peano-framework.org
"""


from .SpeciesStepOrdering import (
    concatenate_steps_of_species,
    interleave_steps_of_species,
)


from .Sequential import (
    map_particle_steps_onto_separate_mesh_traversals,
    map_particle_steps_onto_separate_mesh_traversals_insert_dummy_sweeps_and_vectorise,
    map_initialisation_steps_onto_separate_mesh_traversals,
    map_initialisation_steps_onto_separate_mesh_traversals_for_coalesced_memory_access,
)


from .TaskTree import (
    map_particle_steps_onto_task_tree,
    map_particle_steps_onto_task_tree_insert_dummy_sweeps_and_vectorise,
    map_initialisation_steps_onto_task_tree,
    map_initialisation_steps_onto_task_tree_for_coalesced_memory_access
)
