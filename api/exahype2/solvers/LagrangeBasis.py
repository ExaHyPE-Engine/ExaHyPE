# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from abc import abstractmethod


def render_tensor_1(tensor,entry_separator=",",vector_separator=("{","}")):
  """
  Renders a vector as one C++ initialiser list.
  :param tensor: list or numpy vector of numbers
  """
  result = "{} {}".format( vector_separator[0], tensor[0] )
  for elem in tensor[1:]:
    result += entry_separator
    result += str(elem)
  result += vector_separator[1]
  return result


class LagrangeBasis():
  """
  Abstract base class of the Lagrange bases of ExaHyPE 2.

  The class evaluates the basis through SciPy functors and holds no symbolic
  representation of it.
  """

  def __init__(self, polynomial_order, render_digits = 64):
    self._polynomial_order = polynomial_order
    self._render_digits    = render_digits

  @property
  def order(self):
    return self._polynomial_order

  @property
  def dofs_per_axis(self):
    return self._polynomial_order+1

  @abstractmethod
  def init_dictionary_with_default_parameters(self,dictionary):
    """
    Adds the entries of the basis to dictionary.
    """
    assert False, "A subclass implements init_dictionary_with_default_parameters."
