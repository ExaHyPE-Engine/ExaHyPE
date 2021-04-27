# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum


class CompileMode(Enum):
  """
  
    The compile mode is required in my get_library() routines. It is also used in the 
    Makefile template.
  
  """
  Debug   = 0
  Trace   = 1
  Asserts = 2
  Release = 3
  Stats   = 4
