# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from abc import abstractmethod

import mpmath as mp
from sympy.integrals.quadrature import gauss_legendre, gauss_lobatto

from .LagrangeBasis import LagrangeBasis

PREC_DIGITS = 256
mp.dps = PREC_DIGITS
mp.pretty = False


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

  BasisFunctionValuesRight1DDocumentation = r"""
  Stores each Lagrange polynomial's value at x=1.
"""

  InversePicardOperator1DDocumentation = r"""
  Stores the inverse of @f$ \phi_i(1) \phi_j(1) - K_{ij} @f$, the temporal operator of the
  ADER-DG predictor, packed row major.
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

  def __compute_basis_function_values_right(self):
    return [ self.value1D(1.0, number) for number in range(self.dofs_per_axis) ]

  def __compute_inverse_picard_operator(self):
    dofs_per_axis = self.dofs_per_axis
    values_right  = self.basis_function_values_right
    stiffness     = self.stiffness_operator
    picard_operator = mp.matrix(dofs_per_axis, dofs_per_axis)
    for row in range(dofs_per_axis):
      for col in range(dofs_per_axis):
        picard_operator[row, col] = values_right[row] * values_right[col] - stiffness[row][col]
    return mp.inverse(picard_operator).tolist()

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

  @property
  def basis_function_values_right(self):
    return self._get_cached_basis_entry("basis_function_values_right", self.__compute_basis_function_values_right)

  @property
  def inverse_picard_operator(self):
    return self._get_cached_basis_entry("inverse_picard_operator", self.__compute_inverse_picard_operator)

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


class MultiprecisionLagrangeBasis(LagrangeBasisWithDiagonalMassMatrix):
  """Computes the quadrature of the unit interval with 256 digits and sorts its points."""

  def __init__(self, number_of_points):
    super().__init__(number_of_points - 1)
    (
      self.__quadrature_points,
      self.__quadrature_weights,
    ) = self._compute_quadrature_points_and_weights(number_of_points)

  @abstractmethod
  def _compute_quadrature_points_and_weights(self, number_of_points):
    """Computes quadrature points and weights over the unit interval."""
    raise NotImplementedError

  def _transform_and_sort_points_and_weights(self, quadrature_points, quadrature_weights):
    """Maps quadrature data from [-1, 1] to [0, 1] and sorts it by ascending point."""
    points_and_weights = []
    for point, weight in zip(quadrature_points, quadrature_weights):
      transformed_point  = (mp.mpf(point) + mp.mpf(1.0)) / mp.mpf(2.0)
      transformed_weight = mp.mpf(weight) / mp.mpf(2.0)
      points_and_weights.append((transformed_point, transformed_weight))

    sorted_points_and_weights = sorted(points_and_weights)
    sorted_points, sorted_weights = zip(*sorted_points_and_weights)
    return list(sorted_points), list(sorted_weights)

  @property
  def quadrature_points(self):
    return self.__quadrature_points

  @property
  def quadrature_weights(self):
    return self.__quadrature_weights


class GaussLegendreBasis(MultiprecisionLagrangeBasis):
  def _compute_quadrature_points_and_weights(self, number_of_points):
    quadrature_points, quadrature_weights = gauss_legendre(number_of_points, PREC_DIGITS)
    return self._transform_and_sort_points_and_weights(quadrature_points, quadrature_weights)

  def __str__(self):
    return "GaussLegendreBasis"


class GaussLobattoBasis(MultiprecisionLagrangeBasis):
  def _compute_quadrature_points_and_weights(self, number_of_points):
    quadrature_points, quadrature_weights = gauss_lobatto(number_of_points, PREC_DIGITS)
    return self._transform_and_sort_points_and_weights(quadrature_points, quadrature_weights)

  def __str__(self):
    return "GaussLobattoBasis"
