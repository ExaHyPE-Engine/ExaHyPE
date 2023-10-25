# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum


class Optimisation(Enum):
    """ """

    NONE = 0
    INSERT_RESORT_SWEEPS_AND_VECTORISE = 1
    VECTORISE_WHENEVER_POSSIBLE = 2
