# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


def concatenate_steps_of_species(
    species_sets,
    use_initialisation_steps=False,
):
    """!

    Take the steps per species and concatenate them

    So we first have all the steps of the first species, then all the steps of
    the second species, and so forth.

    @return [(Species,Step)]
      The first entry is the species itself, the second its step. The third 
      entry is the number of the species. All species are enumerationed starting
      with 0. This index is one way to distinguish them.

    """
    result = []
    for current_species_set in species_sets:
        if use_initialisation_steps:
            per_species_set = current_species_set.particle_model.initialisation_steps()
        else:
            per_species_set = current_species_set.particle_model.algorithm_steps()
        for step in per_species_set:
            result.append(
                (current_species_set, step)
            )
    return result


def interleave_steps_of_species(
    species_sets,
    use_initialisation_steps=False,
):
    """!

    Interleave the steps of the species

    Start with the first step of the first species, then the first step of the
    second species, and so forth.

    """

    if use_initialisation_steps:
        unassigned_steps = [
            current_species_set.particle_model.initialisation_steps()
            for current_species_set in species_sets
        ]
    else:
        unassigned_steps = [
            current_species_set.particle_model.algorithm_steps()
            for current_species_set in species_sets
        ]

    unassigned_steps = [0 for x in current_species_set]
    result = []
    added_entry = True
    while added_entry:
        added_entry = False
        for current_species_set in species_sets:
            current_species_set_number = species_sets.index(current_species_set)
            if use_initialisation_steps:
                per_species_set = (
                    current_species_set.particle_model.initialisation_steps()
                )
            else:
                per_species_set = current_species_set.particle_model.algorithm_steps()
            if active_step[current_species_set_number] < len(per_species_set):
                result.append(
                    current_species,
                    per_species_set[active_step[current_species_set_number]]
                )
                active_step[current_species_set_number] += 1
                added_entry = True
    return result
