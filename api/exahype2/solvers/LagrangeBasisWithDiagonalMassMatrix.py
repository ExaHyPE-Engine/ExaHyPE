# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from abc import abstractmethod
from scipy.special import roots_sh_legendre

from .LagrangeBasis import LagrangeBasis
from .LagrangeBasis import render_tensor_1
from .LagrangeBasis import render_tensor_2
from .LagrangeBasis import render_tensor_3


class LagrangeBasisWithDiagonalMassMatrix(LagrangeBasis):
  r"""

  Lagrange basis with a diagonal mass matrix.

  This abstract class is a factory for the vectors and matrices that we need
  within DG solvers. Never use it directly, but always a subclass. As with all
  basis classes, the most relevant routine for a user is
  init_dictionary_with_default_parameters(), which fills a map with replacement
  rules, i.e. with the precomputed matrices and vectors. The class assumes that
  the mass matrix is diagonal. That holds exactly for Gauss-Legendre sampling
  points only, yet it is also the right starting point if you want to work with
  lumped matrices.

  Further to the initialisation, some client classes read quadrature_points and
  quadrature_weights. The exahype2.solvers.rkdg.RungeKuttaDG solver for example
  uses them to initialise the mapping of the plotter properly.

  ## Generated vectors and matrices

  The documentation of the individual fields lives in class attributes of their
  own. That way it shows up in the doxygen documentation and can be piped into
  the generated C++ code.

  - QuadraturePoints1d
  - MassMatrixDiagonal1d
  - StiffnessOperator
  - RestrictionMatrix1d
  - InterpolationMatrix1d

  ## Realisation

  The class relies on SciPy alone.

  """

  def __init__(self,polynomial_order):
    super(LagrangeBasisWithDiagonalMassMatrix,self).__init__(polynomial_order)
    self._cached_basis_entries  = {}
    self._cached_basis_declarations = {}

  def __str__(self):
    d = {}
    self.init_dictionary_with_default_parameters(d,True)
    return "<{}.{} object>: {}".format(self.__class__.__module__,self.__class__.__name__,d)

  __repr__ = __str__

  QuadraturePoints1dDocumentation = r"""
  The quadrature points are an array of size [p+1] if p is the order of the
  chosen polynomial. The double array defines the spacing of the quadrature points
  over the unit interval. See the helper routine exahype2::dg::getQuadraturePoint()
  for an example of how the points identify the actual location in space of the
  quadrature points.

  As we work in a tensor product world, the 2d and 3d quadrature points can be
  simply constructed by multiplying the respective coordinate system indices.
"""

  QuadratureWeights1dDocumentation = r"""
  The quadrature weights are an array of size [p+1] if p is the order of the
  chosen polynomial. The double array defines the integral over the polynomial
  over the unit interval. We use Lagrangian bases here, i.e. only one polynomial
  of the basis is 1 in a quadrature point, while all the others are 0. However,
  the fact that it is 1 in that point does not tell us anything about the
  integral over this particular polynomial. That is the weight associated with
  it.

  As we work in a tensor product world, the 2d and 3d weights can be
  simply constructed by multiplying the respective coordinate system indices.
"""

  MassMatrix1dDocumentation = r"""
  Computes the (reference) element mass matrix' diagonal. The mass matrix is defined
  via

  @f$  \int _{(0,1)}(\phi _i, \phi _j) dx, i,j = 1 ... N > 0 @f$

  where @f$ \phi _i,\phi _j @f$ are Lagrange basis functions associated with
  the quadrature within the unit-interval. They are polynomials of maximum order N-1.

  For the diagonal or lumped setting handled by this class, the diagonal entries
  coincide with the quadrature weights. This is exact for Legendre nodes and the
  intended lumped approximation for Lobatto-type nodes.

  This is a 1d mass matrix: if we have a polynomial of order p, we have
  @f$ (p+1)^d @f$ dofs in total per cell. The matrix has only the dimensions
  @f$ (p+1) \times (p+1) @f$ so it effectively only models how the dofs along one
  horizontal or vertical line within the cell interact. This is sufficient
  if we know that we have a tensor product approach of the shape functions and a
  diagonal mass matrix.

  This is an operation which maps nodal data into a weak space, i.e. the outcome has
  to be scaled with h to be correct. It stems from a 1d integral. If you want to
  compute the real mass matrix output, you have to multiply the outcome along each
  coordinate axis.
"""

  StiffnessOperator1dDocumentation = r"""
  The stiffness operator is a [p+1]x[p+1] matrix which is constructed in
  __compute_stiffness_operator().

  The matrix holds

  @f$  K_{ij} = <\partial _x \phi _i, \phi _j> = w_j (\partial _x \phi _i) (x_j) @f$

  where @f$ \phi _i, \phi _j @f$ are Lagrange basis functions associated with
  the support (quadrature) points @f$ x_i @f$ and @f$ x_j @f$ within the unit
  interval.

  If you have a vector of p+1 entries, it defines a polynomial (the function f) over the unit
  interval. If you multiply this vector with @f$ K @f$, you get the outcome of

  @f$ \int _{[0,1]} \Big( \partial _x f, \phi _j \Big) dx @f$

  for the different tests @f$ \phi _j @f$. If your interval is not the unit
  one but has side length @f$ h @f$, you have to multiply the outcome with
  @f$ h @f$ as you integrate. At the same time, the derivative however has to be
  rescaled with @f$ h^{-1} @f$ and the two h-recalibrations thus cancel out.
  If you have a 2d or 3d function space, you still have to multiply with these
  shape functions (see discussion around mass matrix), and you will get an
  additional @f$ h^{d-1} @f$ scaling of the outcome.

  The outcome is exact for both Lobatto and Legendre nodes due to the reduced
  degree in one scalar product operand.
"""

  BasisFunctionValuesLeft1dDocumentation = r"""
  This is a vector of p+1 entries which tells you for a 1d interval
  what the function on the left side is. Assume all Lagrange polynomials
  are defined over the unit interval. The first entry in this vector
  tells you what the leftmost polynomial's value is at x=0. The second
  entry gives the value of the second polynomial. If you have a
  vector of p+1 shape function weights, you can compute the scalar
  product of this vector and this weight vector, and you know what
  the solution at the point x=0 is.

  If the rightmost value, i.e. the value at x=1 is of interest, the order
  within this vector has to be inverted.

  This is a nodal 1d operation, i.e. there are no integrals or similar
  involved. We take the real 1d polynomial from the shape function.
"""

  DerivativeOperator1dDocumentation = r"""
    Matrix which is given the p+1 weights of the shape function
    along a 1d line. It then returns the p+1 gradients at the integration
    points.

    This is a point-wise operator, i.e. it works directly with the weights
    of the shape function in the function space. It does not involve any
    integral. Furthermore, it is exact for a given Lagrange polynomial.
"""

  InterpolationMatrix1dDocumentation = r"""
    This is essentially a @f$ 3 \cdot (order+1)^d-1 @f$ matrix split up
    into a three-dimensional tensor. The page about DG AMR in the file
    exahype2/dg/Riemann.h documents its content.
"""

  RestrictionMatrix1dDocumentation = r"""
    This is essentially a @f$ 3 \cdot (order+1)^d-1 @f$ matrix split up
    into a three-dimensional tensor. The page about DG AMR in the file
    exahype2/dg/Riemann.h documents its content.
"""

  def __compute_mass_matrix_diagonal(self):
    r"""

    Reference mass matrix diagonal.

    The description lives in the class documentation, as doxygen does not
    extract documentation from private routines in Python.

    """
    return list(self.quadrature_weights)

  def __compute_stiffness_operator(self):
    r"""

    Compute the (reference) element stiffness matrix for the configured polynomial order.

    @see StiffnessOperator1dDocumentation which is also used to dump documentation
      into generated C++ code.

    """
    dofs_per_axis      = self.dofs_per_axis
    quadrature_points  = self.quadrature_points
    quadrature_weights = self.quadrature_weights
    stiffness_matrix   = [ [0.0 for _ in range(dofs_per_axis)] for _ in range(dofs_per_axis) ]

    for row in range(dofs_per_axis):
      for col in range(dofs_per_axis):
        stiffness_matrix[row][col] = quadrature_weights[col] * self.derivative1d(quadrature_points[col],row)

    return stiffness_matrix

  def __compute_interpolation_matrix(self):
    r"""

    Compute the interpolation matrix.

    The matrix prepares the halo layer data for the fine grid points at a
    refinement boundary. It is a 1d mapping only, i.e. it serves the "surface"
    of a 2d domain, and has a size of 3Dof \times Dof.
    Its elements are \phi_j(x^f_i), i.e. the value of the coarse grid basis
    functions at a fine grid point. In practice we multiply this matrix with the
    vector of coarse grid weights (u^c_i):

    \phi_j(x^f_i)*u^c_i=u^c(x^f_i)

    u^c(x) is the original solution on the coarse grid, so the result is the
    coarse solution evaluated at the fine grid positions.

    We can also use the same matrix to interpolate along the normal direction.

    """
    if self._polynomial_order == 0:
      return [[[1.0]],[[1.0]],[[1.0]]]
    else:
      dofs_per_axis     = self.dofs_per_axis
      quadrature_points = self.quadrature_points
      interp_matrix     = [ [[0.0 for _ in range(dofs_per_axis)] for _ in range(dofs_per_axis)] for _ in range(3) ]
      fine_grid_points  = [0.0 for _ in range(3*dofs_per_axis)]
      for index in range(dofs_per_axis):
        fine_grid_points[index]                   = quadrature_points[index] / 3.0
        fine_grid_points[index +   dofs_per_axis] = quadrature_points[index] / 3.0 + 1.0 / 3.0
        fine_grid_points[index + 2*dofs_per_axis] = quadrature_points[index] / 3.0 + 2.0 / 3.0
      for block in range(3):
        for row in range(dofs_per_axis):
          for col in range(dofs_per_axis):
            interp_matrix[block][row][col] = self.value1d(fine_grid_points[row + block*dofs_per_axis],col)

      return interp_matrix

  def __compute_restriction_matrix(self):
    r"""

    Compute the restriction matrix.

    The matrix prepares the halo layer data for the coarse cells at a refinement
    boundary, and its formulation follows the interpolation scheme. It is a 1d
    mapping only, i.e. it serves the "surface" of a 2d domain, and has a size of
    Dof \times 3Dof.

    We can also use the same matrix along the normal direction.

    """
    if self._polynomial_order == 0:
      return [[[1.0/3.0]],[[1.0/3.0]],[[1.0/3.0]]]
    else:
      dofs_per_axis      = self.dofs_per_axis
      quadrature_points  = self.quadrature_points
      restriction_matrix = [ [[0.0 for _ in range(dofs_per_axis)] for _ in range(dofs_per_axis)] for _ in range(3) ]
      fine_grid_points   = [0.0 for _ in range(3*dofs_per_axis)]
      for index in range(dofs_per_axis):
        fine_grid_points[index]                   = quadrature_points[index] / 3.0
        fine_grid_points[index +   dofs_per_axis] = quadrature_points[index] / 3.0 + 1.0 / 3.0
        fine_grid_points[index + 2*dofs_per_axis] = quadrature_points[index] / 3.0 + 2.0 / 3.0
      for block in range(3):
        for row in range(dofs_per_axis):
          for col in range(dofs_per_axis):
            restriction_matrix[block][row][col] = self.value1d(fine_grid_points[col + block*dofs_per_axis],row) / 3.0

      return restriction_matrix

  def __compute_derivative_operator(self):
    r"""

    @see DerivativeOperator1dDocumentation which is also used to dump documentation
      into generated C++ code.

    """
    dofs_per_axis     = self.dofs_per_axis
    quadrature_points = self.quadrature_points
    dQdx              = [ [0.0 for _ in range(dofs_per_axis)] for _ in range(dofs_per_axis) ]

    for row in range(dofs_per_axis):
      for col in range(dofs_per_axis):
        dQdx[row][col] = self.derivative1d(quadrature_points[row], col)

    return dQdx

  def __compute_basis_function_values_left(self):
    r"""

    Compute the value of every basis function at the left end of the interval.

    @see BasisFunctionValuesLeft1dDocumentation which defines what this routine
      does and is also used to inject documentation into the generated C++ code.

    """
    return [ self.value1d(0.0, number) for number in range(self.dofs_per_axis) ]

  def _get_cached_basis_entry(self, key, compute):
    if key not in self._cached_basis_entries:
      self._cached_basis_entries[key] = compute()
    return self._cached_basis_entries[key]

  @staticmethod
  def __render_basis_declaration(documentation, declaration):
    return "/**\n{}*/\n{}\n\n".format(documentation, declaration)

  def init_dictionary_with_default_parameters(self,dictionary,use_multidimensional_arrays):
    dictionary["ORDER"]               = self.order
    if use_multidimensional_arrays not in self._cached_basis_declarations:
      basis_declarations = ""

      basis_declarations += self.__render_basis_declaration(
        self.QuadraturePoints1dDocumentation,
        "static constexpr double QuadraturePoints1d[] = {};".format( render_tensor_1(self.quadrature_points) )
      )

      basis_declarations += self.__render_basis_declaration(
        self.QuadratureWeights1dDocumentation,
        "static constexpr double QuadratureWeights1d[] = {};".format( render_tensor_1(self.quadrature_weights) )
      )

      basis_declarations += self.__render_basis_declaration(
        self.MassMatrix1dDocumentation,
        "static constexpr double MassMatrixDiagonal1d[] = {};".format(
          render_tensor_1(
            self._get_cached_basis_entry("mass_matrix_diagonal", self.__compute_mass_matrix_diagonal)
          )
        )
      )

      stiffness_operator = self._get_cached_basis_entry("stiffness_operator", self.__compute_stiffness_operator)
      if use_multidimensional_arrays:
        stiffness_declaration = "static constexpr double StiffnessOperator1d[][] = {};".format(
          render_tensor_2(stiffness_operator,use_multidimensional_arrays)
        )
      else:
        stiffness_declaration = "static constexpr double StiffnessOperator1d[] = {};".format(
          render_tensor_2(stiffness_operator,use_multidimensional_arrays)
        )
      basis_declarations += self.__render_basis_declaration(self.StiffnessOperator1dDocumentation, stiffness_declaration)

      basis_declarations += self.__render_basis_declaration(
        self.BasisFunctionValuesLeft1dDocumentation,
        "static constexpr double BasisFunctionValuesLeft1d[] = {};".format(
          render_tensor_1(
            self._get_cached_basis_entry("basis_function_values_left", self.__compute_basis_function_values_left)
          )
        )
      )

      derivative_operator = self._get_cached_basis_entry("derivative_operator", self.__compute_derivative_operator)
      if use_multidimensional_arrays:
        derivative_declaration = "static constexpr double DerivativeOperator1d[][] = {};".format(
          render_tensor_2(derivative_operator,use_multidimensional_arrays)
        )
      else:
        derivative_declaration = "static constexpr double DerivativeOperator1d[] = {};".format(
          render_tensor_2(derivative_operator,use_multidimensional_arrays)
        )
      basis_declarations += self.__render_basis_declaration(self.DerivativeOperator1dDocumentation, derivative_declaration)

      restriction_matrix = self._get_cached_basis_entry("restriction_matrix", self.__compute_restriction_matrix)
      if use_multidimensional_arrays:
        restriction_declaration = "static constexpr double RestrictionMatrix1d[][][] = {};".format(
          render_tensor_3(restriction_matrix,use_multidimensional_arrays)
        )
      else:
        restriction_declaration = "static constexpr double RestrictionMatrix1d[] = {};".format(
          render_tensor_3(restriction_matrix,use_multidimensional_arrays)
        )
      basis_declarations += self.__render_basis_declaration(self.RestrictionMatrix1dDocumentation, restriction_declaration)

      interpolation_matrix = self._get_cached_basis_entry("interpolation_matrix", self.__compute_interpolation_matrix)
      if use_multidimensional_arrays:
        interpolation_declaration = "static constexpr double InterpolationMatrix1d[][][] = {};".format(
          render_tensor_3(interpolation_matrix,use_multidimensional_arrays)
        )
      else:
        interpolation_declaration = "static constexpr double InterpolationMatrix1d[] = {};".format(
          render_tensor_3(interpolation_matrix,use_multidimensional_arrays)
        )
      basis_declarations += self.__render_basis_declaration(self.InterpolationMatrix1dDocumentation, interpolation_declaration)

      self._cached_basis_declarations[use_multidimensional_arrays] = basis_declarations

    dictionary["BASIS_DECLARATIONS"] = self._cached_basis_declarations[use_multidimensional_arrays]
    dictionary["BASIS_INITIALIZERS"] = ""

  @property
  @abstractmethod
  def quadrature_points(self):
    assert False, "To be implemented by a subclass"

  @property
  @abstractmethod
  def quadrature_weights(self):
    assert False, "To be implemented by a subclass"

  def value1d(self,x,number):
    r"""

    Return the value of the shape function ``number`` at x.

    The routine evaluates one single shape function. derivative1d() provides the
    matching derivative.

    """
    quadrature_points = self.quadrature_points
    enumerator        = 1.0
    for i in range(0,self.dofs_per_axis):
      if i!=number:
        enumerator  *= (x-quadrature_points[i])

    denominator = 1.0
    for i in range(0,self.dofs_per_axis):
      if i!=number:
        denominator *= (quadrature_points[number]-quadrature_points[i])

    return enumerator/denominator

  def derivative1d(self,x,number):
    r"""

    Return the derivative of the shape function ``number`` at x.

    u = sum_i u_i phi_i

    => d/dx u = sum_j (d/dx phi_j) u_j

    To construct the gradient field, we make the ansatz:

    grad u = sum_i g_i phi_i

    =>

    g_i = ( d/dx u, phi_i )_[0,1] / (phi_i,phi_i)_[0,1] = sum_j (d/dx phi_j, phi_i)_[0,1] / w_i

        = w_i (d/dx phi_j) (x_i) / w_i = (d/dx phi_j) (x_i)

        = DUDX^T

    where DUDX is the operator computed by this function:

    DUDX_ij = (d/dx phi_i) (x_j)

    It can be further written as

    DUDX_ij = 1/w_i * K^T_ij

    where the stiffness matrix K is defined as

    K_ij = <d/dx phi_i, phi_j>_[0,1] = w_j (d/dx phi_i) (x_j)

    :return: derivative of the numbered shape function at x

    :note: __compute_derivative_operator() assembles DUDX from these values. If
    you want to use that operator to compute the gradient of the solution, you
    need to use its transpose.

    """

    quadrature_points = self.quadrature_points
    result = 0.0
    for l in range(0,self.dofs_per_axis):
      if l!=number:
        enumerator  = 1.0
        denominator = 1.0/(quadrature_points[number]-quadrature_points[l])
        for m in range(0,self.dofs_per_axis):
          if m!=number and m!=l:
            enumerator  *= x-quadrature_points[m]
            denominator *= 1.0/(quadrature_points[number]-quadrature_points[m])
        result += enumerator*denominator
    return result


class GaussLegendreBasis(LagrangeBasisWithDiagonalMassMatrix):
  r"""

  Gauss-Legendre basis, which by construction yields diagonal mass matrices.

  The construction relies solely on SciPy, as only the numerical values are of
  interest here and the standard SciPy accuracy is sufficient. Implementations
  based on SymPy could exploit the special structure behind Gauss-Legendre, yet
  sticking to a single Python package keeps the dependencies small.

  """

  def __init__(self,polynomial_order):
    super(GaussLegendreBasis,self).__init__(polynomial_order)
    self._quadrature_points, self._quadrature_weights = roots_sh_legendre(self._polynomial_order+1)

  @property
  def quadrature_points(self):
    r"""

    The roots of the shifted Legendre polynomial.

    They already live on the unit interval and need no rescaling.
    roots_sh_legendre() takes the number of points rather than the polynomial
    order, which is why it is called with the order plus one.

    https://en.wikipedia.org/wiki/Legendre_polynomials#Shifted_Legendre_polynomials

    """
    return self._quadrature_points

  @property
  def quadrature_weights(self):
    r"""

    The Gauss-Legendre weights belonging to quadrature_points.

    They come from the same shifted-Legendre call, so they are scaled to the
    unit interval and sum to one.

    """
    return self._quadrature_weights
