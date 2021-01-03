import mpmath as mp
#import numpy as np
from sympy.integrals.quadrature import gauss_legendre, gauss_lobatto # quadrature_points by default in [-1,1]

from abc import abstractmethod

import re

PREC_DIGITS   = 256
RENDER_DIGITS = 64

mp.dps    = PREC_DIGITS
mp.pretty = False

class LagrangeBasis():

  def __init__(self,num_points):
    self.__num_points = num_points
    self.__max_poly_order = num_points - 1
    # quadrature_points, quadrature_weights
    self.__quadrature_points, self.__quadrature_weights = self._compute_quadrature_points_and_weights(num_points)
    # operators
    self.__mass_matrix                   = self.__compute_mass_matrix()
    self.__stiffness_matrix              = self.__compute_stiffness_matrix()
    self.__K1                            = self.__compute_K1()
    self.__inv_K1                        = mp.inverse(self.__K1).tolist()
    self.__basis_function_values_left,_  = self.__evaluate(mp.mpf(0.0))
    self.__basis_function_values_right,_ = self.__evaluate(mp.mpf(1.0))
    self.__dudx                          = self.__compute_discrete_derivative_operator()
    self.__equidistant_grid_projector    = self.__compute_equidistant_grid_projector()
    self.__fine_grid_projector           = [None]*3
    for j in range(0,3):
      self.__fine_grid_projector[j] = self.__compute_fine_grid_projector(j)
  def __str__(self):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    return "<{}.{} object>: {}".format(self.__class__.__module__,self.__class__.__name__,d)
  __repr__ = __str__
  
  # private methods
  def __evaluate(self,xi):
    """
    Evaluates a lagrange polynomial and its first derivative at a given point.
    
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
    as evaluation points.
    If we use Legendre nodes as support points, this is an exact evaluation if
    we use Lobatto nodes, this is an approximation.
  
    :note:

    :return: The (reference) element mass matrix. Exact for Gauss-Legrende nodes, inexact for Gauss-Lobatto (diagnoal lumping).
    """
    # init matrix with zeros
    MM = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]
   
    for i in range(0,self.__num_points):
      MM[i][i] = self.__quadrature_weights[i]
 
    #for i in range(0,self.__num_points):
    #  phi, _ = self.__evaluate(self.__quadrature_points[i])
    #  for k in range(0,self.__num_points):
    #    for l in range(0,self.__num_points):
    #      MM[k][l] += self.__quadrature_weights[i]*phi[k]*phi[l]
    return MM

  def __compute_stiffness_matrix(self):
    """
    Computes the (reference) element stiffness matrix for an approximation of order self.__max_poly_order.
    
    Computes:

    K_ij = <d/dx phi_i, phi_j>_[0,1] = w_j (d/dx phi_j) (x_i) 
    
    where phi_i,phi_j are Lagrange basis functions associated with
    N support points x_i and x_j within the unit-interval.
    They are polynomials of maximum order N-1.

    :note: Exact for both Lobatto and Legendre nodes due to the reduced degree in one scalar product operand.

    :return: The (reference) element stiffness matrix.
    """
    # init matrix with zero
    Kxi = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]
    
    for j in range(0,self.__num_points):
      phi, phi_xi = self.__evaluate(self.__quadrature_points[j])
      for i in range(0,self.__num_points):
        Kxi[i][j] = self.__quadrature_weights[j]*phi_xi[i]
       
    #for i in range(0,self.__num_points):
    #  phi, phi_xi = self.__evaluate(self.__quadrature_points[i])
    #  for k in range(0,self.__num_points):
    #    Kxi[k][i] = self.__quadrature_weights[i]*phi_xi[k]*phi[i] 
    return Kxi

  def __compute_K1(self):
    """
    Computes the difference between the reference element mass operator 
    evaluated at point xi=1.0 and the element stiffness matrix.
    
    :return: delta between the reference element mass operator at point xi=1.0 and the element stiffness matrix 
    """
    phi1, _ = self.__evaluate(1.0)
    Kxi = self.__stiffness_matrix    

    K1  = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]
    #FRm = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]

    for k in range(0, self.__num_points):
      for l in range(0, self.__num_points):
        #FRm[k][l] = phi1[k]*phi1[l]
        K1[k][l]  = phi1[k]*phi1[l] - Kxi[k][l]
    
    #K1_orig = np.subtract(FRm,self.__stiffness_matrix)
    #for k in range(0, self.__num_points):
    #  for l in range(0, self.__num_points):
    #    print(K1[k][l] - K1_orig[k][l])
   
    return K1    
      
  def __compute_discrete_derivative_operator(self):
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

    K_ij = <d/dx phi_i, phi_j>_[0,1] = w_j (d/dx phi_j) (x_i) 
 
    :return: transposed derivative operator

    :note: If you want to use this operator to compute the gradient of the solution,
    you need to use the transpose.
    """
    # matmul
    # @todo: get rid of this numpy dependency, as mass matrix is diagonal
    #dudx_orig  = np.dot(mp.inverse(self.__mass_matrix).tolist(),np.transpose(self.__stiffness_matrix))
    dudx = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]
    for i in range(0, self.__num_points):
      phi, phi_xi = self.__evaluate(self.__quadrature_points[i])
      for j in range(0, self.__num_points):
        dudx[i][j] = phi_xi[j]
        #print(dudx[i][j] - dudx_orig[i][j])
    return dudx

  def __compute_fine_grid_projector(self, j):
    """
    Transforms the degrees of freedom located on a coarse grid edge
    quadrature_points to degrees of freedoms located on quadrature_points of a fine grid edge.
    The difference in levels is 1.
    
    Let us denote by P the  fine grid projector (= equidistantGridProjector). The fine grid DoF 
    are computed according to:
    
    u^{fine;j}_i =  sum_{m} P^{j}_im u^{coarse}_m
     
    Args:
      j:
      subinterval index
    
    Returns:
       fineGridProjector:
        Operator to express polynomial function associated with original interval with basis functions associated with subinterval j
    """
    fineGridProjector = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)] # 4 x 4 for self.__max_poly_order=3
    
    for i in range(0, self.__num_points): # Eq. basis
      phi_i, _ = self.__evaluate((self.__quadrature_points[i]+j)/mp.mpf(3.0)) # comma after phi_i is important
      for m in range(0, self.__num_points): # DG basis
        fineGridProjector[m][i] = phi_i[m]
    return fineGridProjector

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
    equidistantGridProjector = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)] # 4 x 4 for self.__max_poly_order=3
    subxi = mp.linspace(mp.mpf(0.0), mp.mpf(1.0), self.__num_points)
    
    for i in range(0, self.__num_points): # Eq. basis
      phi_i, _ = self.__evaluate(subxi[i]) # comma after phi_i is important
      for m in range(0, self.__num_points): # DG basis
        equidistantGridProjector[m][i] = phi_i[m]
    return equidistantGridProjector

  # private class methods
  @classmethod
  def __make_initializer_list(cls,rendered_tensor):
    """
    :param rendered_tensor: (nested) list of str elements
    :return: C++ initializer list for tensor
    """
    def descend(current):
      if type(current) is list:
        result ="{"
        for element in current[:-1]:
          result += descend(element)
          result += ","
        result += descend(current[-1])
        result += "}"
        return result
      else:
        return current
    return descend(rendered_tensor)

  @classmethod
  def __render_tensor_1(cls,tensor):
    """
    Converts nested list or numpy matrix to nested list of strings.
    :param tensor: list or numpy vector storing  mpmath numbers
    """
    result = []
    for elem in tensor:
      result.append(mp.nstr(elem,n=RENDER_DIGITS))
    return result
  
  @classmethod
  def __render_tensor_2(cls,tensor):
    """
    Converts nested list or numpy matrix to nested list of strings.
    :param tensor: nested list or numpy matrix storing  mpmath numbers
    """
    result = []
    for row in tensor:
       resultRow = []
       for elem in row:
         resultRow.append(mp.nstr(elem,n=RENDER_DIGITS))
       result.append(resultRow)
    return result

  @classmethod
  def __render_tensor_3(cls,tensor):
    """
    Converts nested list or numpy matrix to nested list of strings.
    :param tensor: nested list or numpy matrix storing  mpmath numbers
    """
    result = []
    for r1 in tensor:
       tmp1 = []
       for r2 in r1:
          tmp2 = []
          for elem in r2:
            tmp2.append(mp.nstr(elem,n=RENDER_DIGITS))
          tmp1.append(tmp2)
       result.append(tmp1)
    return result

  # protected 
  @abstractmethod
  def _compute_quadrature_points_and_weights(self,num_points):
    """
    To be implemented by subclass.
    """
    assert False, "To be implemented by subclass."
  
  
  def _transform_and_sort_points_and_weights(self,quadrature_points,quadrature_weights):
    """
    Transform quadrature quadrature_points and quadrature_weights for interval [-1,1]
    to quadrature_points and quadrature_weights for interval [0,1].
    """
    transformed_quadrature_weights = []
    transformed_quadrature_points   = []
    # weight
    for i,w in enumerate(quadrature_weights):
      x = quadrature_points[i]
      wT = mp.mpf(w)/mp.mpf(2.0)
      xT = (mp.mpf(x)+mp.mpf(1.0))/mp.mpf(2.0)
      transformed_quadrature_weights.append(wT)
      transformed_quadrature_points.append(xT)
    # sort
    sorted_transformed_quadrature_points = sorted(transformed_quadrature_points)
    sorted_transformed_quadrature_weights = []
    for i,xs in enumerate(sorted_transformed_quadrature_points):
      n  = transformed_quadrature_points.index(xs) 
      ws = transformed_quadrature_weights[n]
      sorted_transformed_quadrature_weights.append(ws)
    return sorted_transformed_quadrature_points, sorted_transformed_quadrature_weights
  
  def _init_dictionary_with_default_parameters(self,d):
    def snake_to_camel(word):
      return ''.join(x.capitalize() or '_' for x in word.lower().split('_'))

    basisDeclarations = ""
    basisInitializers = ""
    for var in ["quadrature_points","quadrature_weights","basis_function_values_left","basis_function_values_right","dudx","mass_matrix","stiffness_matrix","inv_K1","equidistant_grid_projector","fine_grid_projector"]:
      var_key  = "_LagrangeBasis__" + var # prefix for privat variables of class LagrangeBasis
      var_name = snake_to_camel(var) # C++ name
      if var in ["quadrature_points","quadrature_weights","basis_function_values_left","basis_function_values_right"]:
          basisDeclarations += "const double {var_name}[{order}+1];\n".format(indent="  "*2,var_name=var_name,order=self.__max_poly_order)
          basisInitializers += "{var_name}{initializer},\n".format(var_name=var_name,\
              initializer=LagrangeBasis.__make_initializer_list(LagrangeBasis.__render_tensor_1(getattr(self,var_key))))
      elif var in ["fine_grid_projector"]:
          basisDeclarations += "const double {var_name}[3][{order}+1][{order}+1];\n".format(indent="  "*2,var_name=var_name,order=self.__max_poly_order)
          basisInitializers += "{var_name}{initializer},\n".format(var_name=var_name,\
              initializer=LagrangeBasis.__make_initializer_list(LagrangeBasis.__render_tensor_3(getattr(self,var_key))))
      else:
          basisDeclarations += "const double {var_name}[{order}+1][{order}+1];\n".format(indent="  "*2,var_name=var_name,order=self.__max_poly_order)
          basisInitializers += "{var_name}{initializer},\n".format(var_name=var_name,\
              initializer=LagrangeBasis.__make_initializer_list(LagrangeBasis.__render_tensor_2(getattr(self,var_key))))
    
    d["ORDER"]              = self.__max_poly_order
    d["BASIS_DECLARATIONS"] = basisDeclarations  
    d["BASIS_INITIALIZERS"] = basisInitializers  
   
  # public
  def quadrature_points(self,render=True):
    return LagrangeBasis.__render_tensor_1(self.__quadrature_points) if render else self.__quadrature_points
  
  def quadrature_weights(self,render=True):
    return LagrangeBasis.__render_tensor_1(self.__quadrature_weights) if render else self.__quadrature_weights
  

class GaussLegendreBasis(LagrangeBasis):
  def _compute_quadrature_points_and_weights(self,num_points):
    x,w = gauss_legendre(num_points,PREC_DIGITS) 
    return self._transform_and_sort_points_and_weights(x,w)

class GaussLobattoBasis(LagrangeBasis):
  def _compute_quadrature_points_and_weights(self,num_points):
    x,w = gauss_lobatto(num_points,PREC_DIGITS) 
    return self._transform_and_sort_points_and_weights(x,w)
