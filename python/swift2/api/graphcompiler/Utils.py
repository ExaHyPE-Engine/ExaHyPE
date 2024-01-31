# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum


class StepType(Enum):
    """ """

    TIME_STEP = 0
    INITIALISATION = 1


def get_observer_prefix(step_type: StepType):
    if step_type == StepType.TIME_STEP:
        return "Step"
    elif step_type == StepType.INITIALISATION:
        return "Init"
    else:
        assert False, "not valid switch"


def does_one_species_require_gather(sequence_of_steps):
    """!

    Look through a sequence of tuples of species and steps and look whether any
    species therein requires us to gather data.

    """
    some_particles_need_to_be_gathered = False
    for x in sequence_of_steps:
        current_species = x[0]
        some_particles_need_to_be_gathered = (
            some_particles_need_to_be_gathered
            or current_species.generator.gather_particles
        )
    return some_particles_need_to_be_gathered


def get_species_set(sequence_of_steps):
    """!

    Take a sequence of steps over multiple species and return the set of
    species used in this set.

    """
    species_sets = []
    for x in sequence_of_steps:
        current_species = x[0]
        if not current_species in species_sets:
            species_sets.append(current_species)
    return species_sets
