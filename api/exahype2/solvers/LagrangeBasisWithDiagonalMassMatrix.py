# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from abc import abstractmethod
from scipy.special import roots_sh_legendre

from .LagrangeBasis import LagrangeBasis


class LagrangeBasisWithDiagonalMassMatrix(LagrangeBasis):
  """Computes reference operators from a subclass's quadrature.

  The mass matrix is diagonal for Gauss-Legendre nodes and lumped for other bases.
  """

  def __init__(self,polynomial_order):
    super(LagrangeBasisWithDiagonalMassMatrix,self).__init__(polynomial_order)
    self._cached_basis_entries = {}

  def __str__(self):
    d = {}
    self.init_dictionary_with_default_parameters(d)
    return "<{}.{} object>: {}".format(self.__class__.__module__,self.__class__.__name__,d)

  __repr__ = __str__

  QuadraturePoints1DDocumentation = r"""
  Stores the nodal coordinates over the unit interval.
"""

  QuadratureWeights1DDocumentation = r"""
  Stores the integral of each Lagrange polynomial over the unit interval.
"""

  MassMatrix1DDocumentation = r"""
  Stores the reference mass diagonal as quadrature weights.
  The diagonal is exact for Legendre nodes and lumped for Lobatto nodes.
"""

  StiffnessOperator1DDocumentation = r"""
  Stores @f$ K_{ij} = w_j \, \partial_x \phi_i(x_j) @f$ over the unit interval.
  Multiplication by nodal values of f gives @f$ \int_0^1 \partial_x \phi_i \, f \, dx @f$.
"""

  BasisFunctionValuesLeft1DDocumentation = r"""
  Stores each Lagrange polynomial's value at x=0.
  Reversing the array gives the values at x=1 for symmetric node sets.
"""

  DerivativeOperator1DDocumentation = r"""
  Maps nodal polynomial values to their derivatives on the unit interval.
"""

  def __compute_mass_matrix_diagonal(self):
    return list(self.quadrature_weights)

  def __compute_stiffness_operator(self):
    dofs_per_axis      = self.dofs_per_axis
    quadrature_points  = self.quadrature_points
    quadrature_weights = self.quadrature_weights
    stiffness_matrix   = [ [0.0 for _ in range(dofs_per_axis)] for _ in range(dofs_per_axis) ]

    for row in range(dofs_per_axis):
      for col in range(dofs_per_axis):
        stiffness_matrix[row][col] = quadrature_weights[col] * self.derivative1D(quadrature_points[col],row)

    return stiffness_matrix

  def __compute_derivative_operator(self):
    dofs_per_axis     = self.dofs_per_axis
    quadrature_points = self.quadrature_points
    dQdx              = [ [0.0 for _ in range(dofs_per_axis)] for _ in range(dofs_per_axis) ]

    for row in range(dofs_per_axis):
      for col in range(dofs_per_axis):
        dQdx[row][col] = self.derivative1D(quadrature_points[row], col)

    return dQdx

  def __compute_basis_function_values_left(self):
    return [ self.value1D(0.0, number) for number in range(self.dofs_per_axis) ]

  def _get_cached_basis_entry(self, key, compute):
    if key not in self._cached_basis_entries:
      self._cached_basis_entries[key] = compute()
    return self._cached_basis_entries[key]

  @property
  def mass_matrix_diagonal(self):
    return self._get_cached_basis_entry("mass_matrix_diagonal", self.__compute_mass_matrix_diagonal)

  @property
  def stiffness_operator(self):
    return self._get_cached_basis_entry("stiffness_operator", self.__compute_stiffness_operator)

  @property
  def basis_function_values_left(self):
    return self._get_cached_basis_entry("basis_function_values_left", self.__compute_basis_function_values_left)

  @property
  def derivative_operator(self):
    return self._get_cached_basis_entry("derivative_operator", self.__compute_derivative_operator)

  def init_dictionary_with_default_parameters(self,dictionary):
    dictionary["ORDER"] = self.order

  @property
  @abstractmethod
  def quadrature_points(self):
    assert False, "A subclass implements this property."

  @property
  @abstractmethod
  def quadrature_weights(self):
    assert False, "A subclass implements this property."

  def value1D(self,x,number):
    """Returns the value of shape function ``number`` at x."""
    quadrature_points = self.quadrature_points
    numerator         = 1.0
    for i in range(0,self.dofs_per_axis):
      if i!=number:
        numerator  *= (x-quadrature_points[i])

    denominator = 1.0
    for i in range(0,self.dofs_per_axis):
      if i!=number:
        denominator *= (quadrature_points[number]-quadrature_points[i])

    return numerator/denominator

  def derivative1D(self,x,number):
    """Returns the derivative of shape function ``number`` at x."""
    quadrature_points = self.quadrature_points
    result = 0.0
    for j in range(0,self.dofs_per_axis):
      if j!=number:
        numerator   = 1.0
        denominator = 1.0/(quadrature_points[number]-quadrature_points[j])
        for m in range(0,self.dofs_per_axis):
          if m!=number and m!=j:
            numerator   *= x-quadrature_points[m]
            denominator *= 1.0/(quadrature_points[number]-quadrature_points[m])
        result += numerator*denominator
    return result


class GaussLegendreBasis(LagrangeBasisWithDiagonalMassMatrix):
  """Computes a Gauss-Legendre basis with a diagonal mass matrix."""

  def __init__(self,polynomial_order):
    super(GaussLegendreBasis,self).__init__(polynomial_order)
    self._quadrature_points, self._quadrature_weights = roots_sh_legendre(self._polynomial_order+1)

  @property
  def quadrature_points(self):
    """Returns the shifted Legendre roots on the unit interval."""
    return self._quadrature_points

  @property
  def quadrature_weights(self):
    """Returns the Gauss-Legendre weights scaled to the unit interval."""
    return self._quadrature_weights
