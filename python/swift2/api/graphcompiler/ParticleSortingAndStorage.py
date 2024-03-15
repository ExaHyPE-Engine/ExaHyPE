# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum


class ParticleSortingAndStorage(Enum):
    """ """

    SORT_WITH_LIFTS_AND_DROPS_SCATTERED_PARTICLE_STORAGE = 0
    SORT_ON_THE_FLY_SCATTERED_PARTICLE_STORAGE = 1
    SORT_WITH_LIFTS_AND_DROPS_COALESCED_PARTICLE_STORAGE = 2
    SORT_ON_THE_FLY_COALESCED_PARTICLE_STORAGE = 3


# Wor ist das scattering?

class AddAdditionalDummySweep(Enum):
    NO = 0
    BEFORE = 1
    AFTER = 2
    BEFORE_AND_SCATTER_DATA = 3 # todo implement
    AFTER_AND_SCATTER_DATA = 4


