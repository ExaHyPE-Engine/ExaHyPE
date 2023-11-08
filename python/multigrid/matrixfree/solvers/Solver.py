import peano4
import dastgen2
from abc import abstractmethod

class Solver:
  """!

  Abstract base class for matrix free solvers.

  Much of the implementation is yet to be done. 

  """

  def __init__(self, 
               name,
               min_h,
               max_h):
    self._name = name
    self.min_h = min_h
    self.max_h = max_h

  def typename(self):
    return self._name;