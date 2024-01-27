# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum


class ParticleSortingAndStorage(Enum):
    """ """

    INSERT_RESORT_SWEEPS_SCATTERED_PARTICLE_STORAGE = 0
    SORT_ON_THE_FLY = 1


class AddAdditionalDummySweep(Enum):
    NO = 0
    BEFORE = 1
    AFTER = 2
    BEFORE_AND_SCATTER_DATA = 3 # todo implement
    AFTER_AND_SCATTER_DATA = 4


