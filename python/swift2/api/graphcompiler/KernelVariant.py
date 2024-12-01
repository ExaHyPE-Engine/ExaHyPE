# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum


class KernelVariant(Enum):
    """ """

    NO_OPTIMISATION = 0
    VECTORISE = 1
    VECTORISE_WITH_DISTANCE_EVALUATION_PREAMBLE = 2
    OFFLOAD_TO_GPU = 3


