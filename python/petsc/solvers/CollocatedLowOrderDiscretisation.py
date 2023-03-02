# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import dastgen2

from .Solver       import Solver

from abc import abstractmethod


class CollocatedLowOrderDiscretisation(Solver):
  def __init__(self,
               number_of_unknowns):
    super( CollocatedLowOrderDiscretisation, self ).__init__()
    pass
