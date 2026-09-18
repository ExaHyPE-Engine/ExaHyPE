# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import mpmath as mp
# gauss_legendre and gauss_lobatto return their points in [-1,1].
from sympy.integrals.quadrature import gauss_legendre, gauss_lobatto

from abc import abstractmethod

PREC_DIGITS   = 256
RENDER_DIGITS = 64

mp.dps    = PREC_DIGITS
mp.pretty = False


class LagrangeBasis:
  """
  Lagrange basis that produces the vectors and matrices the DG solvers need.

  GaussLegendreBasis and GaussLobattoBasis provide the quadrature. A user reads
  quadrature_points() and quadrature_weights() and calls
  init_dictionary_with_default_parameters() to fill a map with replacement rules.
  """

  def __init__(self,num_points):
    self.__num_points = num_points
    self.__max_poly_order = num_points - 1
    self.__quadrature_points, self.__quadrature_weights = self._compute_quadrature_points_and_weights(num_points)
    self.__barycentric_weights             = self.__compute_barycentric_weights()
    self.__mass_matrix                     = self.__compute_mass_matrix()
    self.__stiffness_operator              = self.__compute_stiffness_operator()
    self.__K1                              = self.__compute_K1()
    self.__inverted_predictor_lhs_operator = mp.inverse(self.__K1).tolist()
    self.__basis_function_values_left,_    = self.__evaluate(mp.mpf(0.0))
    self.__basis_function_values_right,_   = self.__evaluate(mp.mpf(1.0))
    self.__derivative_operator    = self.__compute_derivative_operator()
    self.__equidistant_grid_projector      = self.__compute_equidistant_grid_projector()

    # The tracer action sets read order.
    self.order = num_points-1

  def __str__(self):
    d = {}
    self.init_dictionary_with_default_parameters(d)
    return "<{}.{} object>: {}".format(self.__class__.__module__,self.__class__.__name__,d)
  __repr__ = __str__

  def __evaluate(self,xi):
    """
    Evaluates the Lagrange polynomials and their first derivatives at one point.

    Args:
       xi:
        The reference element point the basis functions are evaluated at.
        Here, xi refers to the greek letter that is often used as a reference element coordinate.
    Returns:
       phi:
        Basis function values.
       phi_xi:
        First derivatives of the basis functions.
    """
    phi  = [mp.mpf(1.)]*(self.__num_points)
    phi_xi = [mp.mpf(0.)]*(self.__num_points)
    xin = self.__quadrature_points
    for m in range(0,self.__num_points):
      for j in range(0,self.__num_points):
        if j == m:
          continue
        phi[m] = phi[m]*(xi-xin[j])/(xin[m]-xin[j])
      for i in range(0,self.__num_points):
        if i == m:
          continue
        tmp = 1.;
        for j in range(0,self.__num_points):
          if j == i:
            continue
          if j == m:
            continue
          tmp = tmp*(xi-xin[j])/(xin[m]-xin[j])
        phi_xi[m] += tmp/(xin[m]-xin[i])
    return phi, phi_xi

  def __compute_barycentric_weights(self):
    """
    See Eq. 3.2 in
    https://people.maths.ox.ac.uk/trefethen/barycentric.pdf
    """
    result = [0.0]*self.__num_points
    for j,xj in enumerate(self.__quadrature_points):
      divisor = 1.0
      for k,xk in enumerate(self.__quadrature_points):
        if j != k:
          divisor *= (xj-xk)
      result[j] = 1.0/divisor
    return result

  def __compute_mass_matrix(self):
    """
    Computes the (reference) element mass matrix for an approximation of
    order self.__max_poly_order.

    We evaluate the scalar product:

    (phi_i, phi_j)_[0,1], i,j = 1 ... N > 0

    where phi_i,phi_j are Lagrange basis functions associated with
    N support points x_i and x_j within the unit-interval.
    They are polynomials of maximum order N-1.

    We evaluate the scalar product via a degree N Gauss quadrature rule that uses basis function support points
    as evaluation points. The evaluation is exact for Legendre nodes and an
    approximation for Lobatto nodes.

    :return: The (reference) element mass matrix. It is exact for Gauss-Legendre nodes and a diagonal lumping for Gauss-Lobatto nodes.
    """
    MM = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]

    for i in range(0,self.__num_points):
      MM[i][i] = self.__quadrature_weights[i]

    return MM

  def __compute_stiffness_operator(self):
    """
    Computes the (reference) element stiffness matrix for an approximation of order self.__max_poly_order.

    Computes:

    K_ij = <d/dx phi_i, phi_j>_[0,1] = w_j (d/dx phi_i) (x_j)

    where phi_i,phi_j are Lagrange basis functions associated with
    N support points x_i and x_j within the unit-interval.
    They are polynomials of maximum order N-1.

    :note: Exact for both Lobatto and Legendre nodes due to the reduced degree in one scalar product operand.

    :return: The (reference) element stiffness matrix.
    """
    Kxi = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]

    for j in range(0,self.__num_points):
      phi, phi_xi = self.__evaluate(self.__quadrature_points[j])
      for i in range(0,self.__num_points):
        Kxi[i][j] = self.__quadrature_weights[j]*phi_xi[i]

    return Kxi

  def __compute_K1(self):
    """
    Computes the difference between the reference element mass operator
    evaluated at point xi=1.0 and the element stiffness matrix.

    :return: delta between the reference element mass operator at point xi=1.0 and the element stiffness matrix
    """
    phi1, _ = self.__evaluate(1.0)
    Kxi = self.__stiffness_operator

    K1 = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]

    for k in range(0, self.__num_points):
      for j in range(0, self.__num_points):
        K1[k][j] = phi1[k]*phi1[j] - Kxi[k][j]

    return K1

  def __compute_derivative_operator(self):
    """
    Computes basis function derivatives at each support point.
    Transposes the resulting operator.

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

    :return: transposed derivative operator

    :note: If you want to use this operator to compute the gradient of the solution,
    you need to use the transpose.
    """
    dudx = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]
    for i in range(0, self.__num_points):
      phi, phi_xi = self.__evaluate(self.__quadrature_points[i])
      for j in range(0, self.__num_points):
        dudx[i][j] = phi_xi[j]
    return dudx

  def __compute_equidistant_grid_projector(self):
    """
    Transforms the degrees of freedom located at non-equidistant Lagrange support points
    quadrature_points to degrees of freedoms located at quadrature_points of an equidistant grid over (0,1).

    Let us denote by P the  projection operator (= equidistantGridProjector). The equidistant DoF
    are computed according to:

    u^eq_i =  sum_{m} P_im u^DG_m

    Returns:
       equidistantGridProjector:
        The corresponding degrees of freedom located at quadrature_points of an equidistant grid over (0,1).
    """
    equidistantGridProjector = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]
    subxi = mp.linspace(mp.mpf(0.0), mp.mpf(1.0), self.__num_points)

    for i in range(0, self.__num_points): # Equidistant basis
      phi_i, _ = self.__evaluate(subxi[i])
      for m in range(0, self.__num_points): # DG basis
        equidistantGridProjector[m][i] = phi_i[m]
    return equidistantGridProjector

  @staticmethod
  def __make_initializer_list(rendered_tensor):
    """
    Renders a tensor as a C++ initialiser list.

    Jinja2 resolves the resulting string repeatedly, so a nested list writes
    "{ {" with a space where C++ nests two braces. Two adjacent braces would
    form a jinja2 token.

    :param rendered_tensor: (nested) list of str elements
    :return: C++ initialiser list of the tensor
    """
    def descend(current):
      if type(current) is list:
        result =" {"
        for element in current[:-1]:
          result += descend(element)
          result += ","
        result += descend(current[-1])
        result += "} "
        return result
      else:
        return current
    return descend(rendered_tensor)

  @staticmethod
  def __render_tensor_1(tensor):
    """
    Converts a vector into a list of strings.
    :param tensor: list or numpy vector of mpmath numbers
    """
    result = []
    for elem in tensor:
      result.append(mp.nstr(elem,n=RENDER_DIGITS))
    return result

  @staticmethod
  def __render_tensor_2(tensor,multiDimArray=True):
    """
    Converts a matrix into a nested list of strings, or into one flat list when
    multiDimArray is not set.
    :param tensor: nested list or numpy matrix of mpmath numbers
    """
    result = []
    for row in tensor:
      resultRow = []
      for elem in row:
        resultRow.append(mp.nstr(elem,n=RENDER_DIGITS))
      if multiDimArray:
        result.append(resultRow)
      else:
        result += resultRow
    return result

  @abstractmethod
  def _compute_quadrature_points_and_weights(self,num_points):
    """
    Returns the quadrature points and weights of num_points nodes. A subclass
    implements it.
    """
    assert False, "A subclass implements _compute_quadrature_points_and_weights."

  def _transform_and_sort_points_and_weights(self,quadrature_points,quadrature_weights):
    """
    Maps the quadrature points and weights of the interval [-1,1] onto the
    interval [0,1] and sorts them by ascending point.
    """
    transformed_quadrature_weights = []
    transformed_quadrature_points   = []
    for i,w in enumerate(quadrature_weights):
      x = quadrature_points[i]
      wT = mp.mpf(w)/mp.mpf(2.0)
      xT = (mp.mpf(x)+mp.mpf(1.0))/mp.mpf(2.0)
      transformed_quadrature_weights.append(wT)
      transformed_quadrature_points.append(xT)
    sorted_transformed_quadrature_points = sorted(transformed_quadrature_points)
    sorted_transformed_quadrature_weights = []
    for i,xs in enumerate(sorted_transformed_quadrature_points):
      n  = transformed_quadrature_points.index(xs)
      ws = transformed_quadrature_weights[n]
      sorted_transformed_quadrature_weights.append(ws)
    return sorted_transformed_quadrature_points, sorted_transformed_quadrature_weights

  def init_dictionary_with_default_parameters(self,d,multiDimArrays=False):
    def snake_to_camel(word):
      return ''.join(x.capitalize() or '_' for x in word.lower().split('_'))

    basisDeclarations = ""
    basisInitializers = ""
    for var in ["quadrature_points","quadrature_weights","barycentric_weights","basis_function_values_left","basis_function_values_right","derivative_operator","mass_matrix","stiffness_operator","inverted_predictor_lhs_operator","equidistant_grid_projector"]:
      var_key  = "_LagrangeBasis__" + var # Python mangles the private members of LagrangeBasis with this prefix.
      var_name = snake_to_camel(var) # C++ name
      if var == "quadrature_points":
        var_name = "QuadraturePoints1D"
      if var in ["quadrature_points","quadrature_weights","barycentric_weights","basis_function_values_left","basis_function_values_right"]:
          basisDeclarations += "const double {var_name}[{order}+1];\n".format(var_name=var_name,order=self.__max_poly_order)
          basisInitializers += "{var_name}{initializer},\n".format(var_name=var_name,\
              initializer=LagrangeBasis.__make_initializer_list(LagrangeBasis.__render_tensor_1(getattr(self,var_key))))
      else:
          if multiDimArrays:
            basisDeclarations += "const double {var_name}[{order}+1][{order}+1];\n".format(var_name=var_name,order=self.__max_poly_order)
          else:
            basisDeclarations += "const double {var_name}[({order}+1)*({order}+1)];\n".format(var_name=var_name,order=self.__max_poly_order)
          basisInitializers += "{var_name}{initializer},\n".format(var_name=var_name,\
              initializer=LagrangeBasis.__make_initializer_list(LagrangeBasis.__render_tensor_2(getattr(self,var_key),multiDimArrays)))

    d["ORDER"]              = self.__max_poly_order
    d["BASIS_DECLARATIONS"] = basisDeclarations
    d["BASIS_INITIALIZERS"] = basisInitializers

  def quadrature_points(self,render=True):
    return LagrangeBasis.__render_tensor_1(self.__quadrature_points) if render else self.__quadrature_points

  def quadrature_weights(self,render=True):
    return LagrangeBasis.__render_tensor_1(self.__quadrature_weights) if render else self.__quadrature_weights


class GaussLegendreBasis(LagrangeBasis):
  def _compute_quadrature_points_and_weights(self,num_points):
    x,w = gauss_legendre(num_points,PREC_DIGITS)
    return self._transform_and_sort_points_and_weights(x,w)

  def __str__(self):
    return "GaussLegendreBasis"


class GaussLobattoBasis(LagrangeBasis):
  def _compute_quadrature_points_and_weights(self,num_points):
    x,w = gauss_lobatto(num_points,PREC_DIGITS)
    return self._transform_and_sort_points_and_weights(x,w)

  def __str__(self):
    return "GaussLobattoBasis"
