# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum


class Storage(Enum):
    CallStack = 0
    Heap = 1
    SmartPointers = 2

