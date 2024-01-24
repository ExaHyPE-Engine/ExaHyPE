# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum


class Optimisation(Enum):
    """ """

    NONE = 0
    INSERT_RESORT_SWEEPS_AND_VECTORISE = 1
    SORT_ON_THE_FLY = 2


class AddAdditionalDummySweep(Enum):
    NO = 0
    BEFORE = 1
    AFTER = 2
