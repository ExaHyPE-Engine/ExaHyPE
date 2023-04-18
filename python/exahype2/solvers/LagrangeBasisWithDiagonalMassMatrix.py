# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org


#import mpmath as mp
#import numpy as np
#from sympy.integrals.quadrature import gauss_legendre, gauss_lobatto # quadrature_points by default in [-1,1]


import scipy
from scipy.integrate import quad

from abc import abstractmethod

from .LagrangeBasis import LagrangeBasis
from .LagrangeBasis import render_tensor_1
from .LagrangeBasis import render_tensor_2
from .LagrangeBasis import render_tensor_3

from numpy import number




class LagrangeBasisWithDiagonalMassMatrix(LagrangeBasis):
  """

  Lagrange Basis with a diagonal mass matrix
  
  
  Class representing an abstract Lagrange basis which is used as a factory 
  mechanism to produce vectors and matrices that we need within DG solvers.
  Never use this class directly, but always a subclass. As with all basis 
  classes, the most relevant 
  routine for a user is init_dictionary_with_default_parameters() which is 
  a routine to befill a map with replacement rules, i.e. with the precomputed
  matrices and vectors. The class assumes that the mass matrix is a diagonal 
  matrix. 
  This is not true in general, i.e. holds only for Gauss-Legendre sampling
  points, but you might want to work with lumped matrices, and then this 
  class is the right starting point.
  
  Further to the initialisation, some using classes use quadrature_points_and_weights().
  The exahype2.solvers.rkdg.RungeKuttaDG solver for example uses it to 
  initialise the mapping of the plotter properly.
  
  
  ## Generated vectors and matrices
  
  I decided to "outsource" the documentation of the individual fields to 
  class attributes of their own. This way, they are visible both in the 
  doxygen documentation and I can pipe them into the generated C++ code.
   
  - QuadraturePoints1d
  - MassMatrixDiagonal1d
  - StiffnessOperator
  - RestrictionMatrix1d
  - InterpolationMatrix1d

  
  ## Realisation
  
  The first version of this utility class has been written by Dominic E.
  Charrier. It relied on NumPy and SymPy in several places. The current
  version is stripped down and solely uses SciPy.
  
  
  """
  
  def __init__(self,polynomial_order):
    super(LagrangeBasisWithDiagonalMassMatrix,self).__init__(polynomial_order)
      
      
  def __str__(self):
    d = {}
    self.init_dictionary_with_default_parameters(d,True)
    return "<{}.{} object>: {}".format(self.__class__.__module__,self.__class__.__name__,d)


  __repr__ = __str__
  

  QuadraturePoints1dDocumentation = """
  The quadrature points are an array of size [p+1] if p is the order of the 
  chosen polyomial. The double array defines the spacing of the quadrature points 
  over the unit interval. See the helper routine exahype2::dg::getQuadraturePoint()
  for an example how the points are used to identify the actual location in 
  space of the quadrature points.
  
  As we work in a tensor product world, the 2d and 3d quadrature points can be 
  simply constructed by multiplying the respective coordinate system indices.
"""
  

  QuadratureWeights1dDocumentation = """
  The quadrature weights are an array of size [p+1] if p is the order of the 
  chosen polyomial. The double array defines the integral over the polynomial
  over the unit interval. We use Lagrangian bases here, i.e. only one polynomial
  of the basis is 1 in a quadrature point, while all the others are 0. However,
  the fact that it is 1 in the point, does not tell us anything about the 
  integral over this particular polynomial. That's the weight associated with 
  it.
  
  As we work in a tensor product world, the 2d and 3d weights can be 
  simply constructed by multiplying the respective coordinate system indices.
"""


  MassMatrix1dDocumentation = """
  Computes the (reference) element mass matrix' diagonal. The mass matrix is defined
  via

  @f$  \int _{(0,1)}(\phi _i, \phi _j) dx, i,j = 1 ... N > 0 @f$

  where @f$ \phi _i,\phi _j @f$ are Lagrange basis functions associated with
  the quadrature within the unit-interval. They are polynomials of maximum order N-1.

  Our implementation is quite some overkill, as I assemble the whole mass matrix 
  and then extract only the diagonal.  If we use Legendre nodes as support points, 
  this diagonal is an exact evaluation, i.e. the mass matrix is diagonal. If
  we use Lobatto nodes, it is an approximation.
  
  Please note that this is a 1d mass matrix: If we have a polynomial of order p,
  we have @f$ (p+1)^d @f$ dofs in total per cell. My matrix has only the dimensions
  @f$ (p+1) \times (p+1) @f$ so it effectively only models how the dofs along one 
  horizontal or vertical line within the cell do interact. This is totally sufficient
  if we know that we have a tensor product approach of the shape functions and a 
  diagonal mass matrix.

  @todo I don't know if we need renormalisation for Lobatto nodes, i.e. if we 
  should ensure that the row sum is preserved
  
  This is an operation which maps nodal data into a weak space, i.e. the outcome has
  to be scaled with h to be correct. It stems from a 1d integral. If you want to 
  compute the real mass matrix output, you have to multiply the outcome along each
  coordinate axis.
"""
    
     
  StiffnessOperator1dDocumentation = """
  The stiffness operator is a [p+1]x[p+1] matrix which is constructed in
  __compute_stiffness_operator().

  The matrix holds 
  
  @f$  K_{ij} = <\partial _x \phi _i, \phi _j> = w_j (\partial _x \phi _j) (x_i) @f$
    
  where @f$ \phi _i, \phi _j @f$ are Lagrange basis functions associated with
  the support (quadrature) points @f$ x_i @f$ and @f$ x_j @f$ within the unit
  interval.
 
  If you have a vector of p+1 entries, it defines a polynomial (aka function f) over the unit
  interval. If you multiply this vector if @f$ K @f$, you get the outcome of
  
  @f$ \int _{[0.1]} \Big( \partial _x f, \phi _j \Big) dx @f$
  
  for the different tests @f$ \phi _j @f$. If your interval is not the unit 
  one but has side length $h$, you have to multiply the outcome with $h$ as 
  you integrate. At the same time, the derivative however has to be rescaled
  with @f$ h^{-1} @f$ and the two h-recalibrations thus cancel out.
  If you have a 2d or 3d function space, you still have to multiple with these 
  shape functions (see discussion around mass matrix), and you will get an 
  additional @f$ h^{d-1} @f$ scaling of the outcome.
  
  We note that the outcome is exact for both Lobatto and Legendre nodes due to 
  the reduced degree in one scalar product operand.
"""

    
  BasisFunctionValuesLeft1dDocumentation = """
  This is a vector of p+1 entries which tells you for a 1d interval
  what the function on the left side is. Assume all Lagrange polynomials
  are defined over the unit interval. The first entry in this matrix 
  tells you what the leftmost polynomial's value is at x=0. The second
  entry discusses the impact of the second polynomial. If you have a 
  vector of p+1 shape function weights, you can compute the scalar
  product of this vector and this weight vector, and you'll know what 
  the solution at the point x=0 is.
  
  If the rightmost value, i.e. the value at x=1 is of interest, the order
  within this vector has to be inverted.
  
  This is a nodal 1d operation, i.e. there are no integrals or similar 
  involved. We take the real 1d polyomial from the shape function.
"""
    
  DerivativeOperator1dDocumentation = """
    Matrix which is given the p+1 weights of the shape function
    along a 1d line. It then returns the p+1 gradients at the integration 
    points.
    
    This is a point-wise operator, i.e. it works directly with the weights
    of the shape function in the function space. It does not involve any
    integral et al. Furthermore, it is exact given a certain Lagrange 
    polynomial. 
"""    

  InterpolationMatrix1dDocumentation = """
    This is essentially a @f$ 3 \cdot (order+1)^d-1 @f$ matrix split up 
    into a three-dimensional tensor. The documentation/content of this
    tensor can be found on the page about DG AMR which is held in exahype
    within the file exahype2/dg/Riemann.h.
"""      

  RestrictionMatrix1dDocumentation = """
    This is essentially a @f$ 3 \cdot (order+1)^d-1 @f$ matrix split up 
    into a three-dimensional tensor. The documentation/content of this
    tensor can be found on the page about DG AMR which is held in exahype
    within the file exahype2/dg/Riemann.h.
"""      
    
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
     
     
  def __compute_mass_matrix_diagonal(self):
    """
    
    Reference mass matrix. See class documentation. I moved the docu there,
    as doxygen does not extract documentation from private routines in Python.
         
    """
    mass_matrix = [ [0.0 for _ in range(0,self.dofs_per_axis)] for _ in range(0,self.dofs_per_axis) ]
    
    for row in range(0,self.dofs_per_axis):
      for col in range(0,self.dofs_per_axis):
        ans,err = quad(lambda x: self.value1d(x,row) * self.value1d(x,col),0,1)
        mass_matrix[row][col] = ans

    return [ mass_matrix[i][i] for i in range(0,self.dofs_per_axis) ]


  def __compute_stiffness_operator(self):
    """
    
    Computes the (reference) element stiffness matrix for an approximation of order self.__max_poly_order.
    
    @see StiffnessOperator1dDocumentation which is also used to dump documentation
      into generated C++ code.
    
    """
    stiffness_matrix = [ [0.0 for _ in range(0,self.dofs_per_axis)] for _ in range(0,self.dofs_per_axis) ]
    
    for row in range(0,self.dofs_per_axis):
      for col in range(0,self.dofs_per_axis):
        ans,err = quad(lambda x: self.derivative1d(x,row) * self.value1d(x,col),0,1)
        stiffness_matrix[row][col] = ans

    return stiffness_matrix
  

  def __compute_interpolation_matrix(self):
    """

    compute the interpolation matrix, which used to prepare halo layer data for fine grid points at the refinement boundary.

    The matrix is only 1d mapping, i.e. used for "surface" of a 2D domain and have a size of 3Dof \times Dof.
    Its elements are \phi_j(x^f_i), i.e. the value of basis functions for coarse grid on fine grid point.
    In practive we multiply this matrix to vector of weight in coarse grid (u^c_i):

    \phi_j(x^f_i)*u^c_i=u^c(x^f_i)

    u^c(x) is the original solution on coarse grid. Thus the result is actually the evaluation of coarse solution on fine grid position

    We can also use the same matrix to interpolate along the normal direction.
 
    """
    if self._polynomial_order == 0:
      return [[[1.0]],[[1.0]],[[1.0]]]
    else:
      interp_matrix = [ [0.0 for _ in range(0,self.dofs_per_axis)] for _ in range(0,3*self.dofs_per_axis) ]
      interp_matrix = [ [[0.0 for _ in range(0,self.dofs_per_axis)] for _ in range(0,self.dofs_per_axis) ] for _ in range(0,3) ]
      fine_grid_point = [0.0 for _ in range(0,3*self.dofs_per_axis) ]  
      for index in range(0,self.dofs_per_axis):
        fine_grid_point[index                     ]=self.quadrature_points[index]/3.0
        fine_grid_point[index+  self.dofs_per_axis]=self.quadrature_points[index]/3.0+1.0/3.0
        fine_grid_point[index+2*self.dofs_per_axis]=self.quadrature_points[index]/3.0+2.0/3.0
      for block in range(0, 3):
        for row in range(0, self.dofs_per_axis):
          for col in range(0, self.dofs_per_axis):
            interp_matrix[block][row][col]=self.value1d(fine_grid_point[row],col)

      return interp_matrix 


  def __compute_restriction_matrix(self):
    """

    compute the restriction matrix, which used to prepare halo layer data for coarse cells at the refinement boundary.
    the matrix formulation depends on the interpolation scheme.

    The matrix is only 1d mapping, i.e. used for "surface" of a 2D domain and have a size of Dof \times 3Dof.

    We can also use the same matrix to interpolate along the normal direction. 


    """
    if self._polynomial_order == 0:
      return [[[1.0/3.0]],[[1.0/3.0]],[[1.0/3.0]]]
    else:
      restriction_matrix = [ [[0.0 for _ in range(0,self.dofs_per_axis)] for _ in range(0,self.dofs_per_axis) ] for _ in range(0,3) ]
      fine_grid_point = [0.0 for _ in range(0,3*self.dofs_per_axis) ]  
      for index in range(0,self.dofs_per_axis):
        fine_grid_point[index                     ]=self.quadrature_points[index]/3.0
        fine_grid_point[index+  self.dofs_per_axis]=self.quadrature_points[index]/3.0+1.0/3.0
        fine_grid_point[index+2*self.dofs_per_axis]=self.quadrature_points[index]/3.0+2.0/3.0
      for block in range(0, 3):
        for row in range(0, self.dofs_per_axis):
          for col in range(0, self.dofs_per_axis):
            restriction_matrix[block][row][col]=self.value1d(fine_grid_point[col],row)/3.0

      return restriction_matrix   


  def __compute_K1(self):
    """
    
    @todo Not yet updated atm but seems also not to be used anywhere
    
    Computes the difference between the reference element mass operator 
    evaluated at point xi=1.0 and the element stiffness matrix.
    
    :return: delta between the reference element mass operator at point xi=1.0 and the element stiffness matrix 
    """
    phi1, _ = self.__evaluate(1.0)
    Kxi = self.__stiffness_operator    

    K1  = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]
    #FRm = [[mp.mpf(0) for _ in range(self.__num_points)] for _ in range(self.__num_points)]

    for k in range(0, self.__num_points):
      for l in range(0, self.__num_points):
        #FRm[k][l] = phi1[k]*phi1[l]
        K1[k][l]  = phi1[k]*phi1[l] - Kxi[k][l]
    
    #K1_orig = np.subtract(FRm,self.__stiffness_operator)
    #for k in range(0, self.__num_points):
    #  for l in range(0, self.__num_points):
    #    print(K1[k][l] - K1_orig[k][l])
   
    return K1    
      
      
  def __compute_derivative_operator(self):
    """

    @see DerivativeOperator1dDocumentation which is also used to dump documentation
      into generated C++ code.

    """
    dQdx = [ [0.0 for _ in range(0,self.dofs_per_axis)] for _ in range(0,self.dofs_per_axis) ]
    
    for row in range(0,self.dofs_per_axis):
      for col in range(0,self.dofs_per_axis):
        dQdx[row][col] = self.derivative1d( self.quadrature_points[row], col)

    return dQdx
  

  def __compute_fine_grid_projector(self, j):
    """
    
    @todo Definitely useful, but not used atm
    
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
  
  
  def __compute_basis_function_values_left(self):
    """
    
    Compute per basis function what the very left value of the polynomial would be
    
    @see BasisFunctionValuesLeft1dDocumentation which defines what this routine
      does and is also used to inject documentation into the generated C++ code.
    
    """
    return [ self.value1d(0.0, number) for number in range(self.dofs_per_axis) ]  
    
      
  def __compute_equidistant_grid_projector(self):
    """
    
    @todo Not used atm, but definitely useful
    
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
  
  
  def init_dictionary_with_default_parameters(self,dictionary,use_multidimensional_arrays):
    def snake_to_camel(word):
      return ''.join(x.capitalize() or '_' for x in word.lower().split('_'))

    basisDeclarations = ""
    basisInitializers = ""
    
    dictionary["ORDER"]               = self.order
    dictionary["BASIS_DECLARATIONS"]  = ""
    dictionary["BASIS_INITIALIZERS"]  = ""
    
    dictionary["BASIS_DECLARATIONS"] += "/**\n"
    dictionary["BASIS_DECLARATIONS"] += self.QuadraturePoints1dDocumentation
    dictionary["BASIS_DECLARATIONS"] += "*/\n"
    #dictionary["BASIS_DECLARATIONS"] += "static const double QuadraturePoints1d[DGOrder+1];\n\n"
    #dictionary["BASIS_INITIALIZERS"] += "             QuadraturePoints1d{},\n".format( render_tensor_1(self.quadrature_points) )
    dictionary["BASIS_DECLARATIONS"] += "static constexpr double QuadraturePoints1d[] = {};\n\n".format( render_tensor_1(self.quadrature_points) )
    
    dictionary["BASIS_DECLARATIONS"] += "/**\n"
    dictionary["BASIS_DECLARATIONS"] += self.QuadratureWeights1dDocumentation
    dictionary["BASIS_DECLARATIONS"] += "*/\n"
    #dictionary["BASIS_DECLARATIONS"] += "const double QuadratureWeights1d[DGOrder+1];\n\n"
    #dictionary["BASIS_INITIALIZERS"] += "             QuadratureWeights1d{},\n".format( render_tensor_1(self.quadrature_weights) )
    dictionary["BASIS_DECLARATIONS"] += "static constexpr double QuadratureWeights1d[] = {};\n\n".format( render_tensor_1(self.quadrature_weights) )

    dictionary["BASIS_DECLARATIONS"] += "/**\n"
    dictionary["BASIS_DECLARATIONS"] += self.MassMatrix1dDocumentation
    dictionary["BASIS_DECLARATIONS"] += "*/\n"
    #dictionary["BASIS_DECLARATIONS"] += "const double MassMatrixDiagonal1d[DGOrder+1];\n\n"
    #dictionary["BASIS_INITIALIZERS"] += "             MassMatrixDiagonal1d{},\n".format( render_tensor_1(self.__compute_mass_matrix_diagonal()) )
    dictionary["BASIS_DECLARATIONS"] += "static constexpr double MassMatrixDiagonal1d[] = {};\n\n".format( render_tensor_1(self.__compute_mass_matrix_diagonal()) )

    dictionary["BASIS_DECLARATIONS"] += "/**\n"
    dictionary["BASIS_DECLARATIONS"] += self.StiffnessOperator1dDocumentation
    dictionary["BASIS_DECLARATIONS"] += "*/\n"
    if use_multidimensional_arrays:        
      #dictionary["BASIS_DECLARATIONS"] += "const double StiffnessOperator1d[DGOrder+1][DGOrder+1];\n\n"
      #dictionary["BASIS_INITIALIZERS"] += "             StiffnessOperator1d{},\n".format( render_tensor_2(self.__compute_stiffness_operator(),use_multidimensional_arrays) )
      dictionary["BASIS_DECLARATIONS"] += "static constexpr double StiffnessOperator1d[][] = {};\n\n".format( render_tensor_2(self.__compute_stiffness_operator(),use_multidimensional_arrays) )
    else:
      #dictionary["BASIS_DECLARATIONS"] += "const double StiffnessOperator1d[(DGOrder+1)*(DGOrder+1)];\n\n"
      #dictionary["BASIS_INITIALIZERS"] += "             StiffnessOperator1d{},\n".format( render_tensor_2(self.__compute_stiffness_operator(),use_multidimensional_arrays) )
      dictionary["BASIS_DECLARATIONS"] += "static constexpr double StiffnessOperator1d[] = {};\n\n".format( render_tensor_2(self.__compute_stiffness_operator(),use_multidimensional_arrays) )

    dictionary["BASIS_DECLARATIONS"] += "/**\n"
    dictionary["BASIS_DECLARATIONS"] += self.BasisFunctionValuesLeft1dDocumentation
    dictionary["BASIS_DECLARATIONS"] += "*/\n"
    dictionary["BASIS_DECLARATIONS"] += "static constexpr double BasisFunctionValuesLeft1d[] = {};\n\n".format( render_tensor_1(self.__compute_basis_function_values_left()) )

    dictionary["BASIS_DECLARATIONS"] += "/**\n"
    dictionary["BASIS_DECLARATIONS"] += self.DerivativeOperator1dDocumentation
    dictionary["BASIS_DECLARATIONS"] += "*/\n"
    if use_multidimensional_arrays:        
      dictionary["BASIS_DECLARATIONS"] += "static constexpr double DerivativeOperator1d[][] = {};\n\n".format( render_tensor_2(self.__compute_derivative_operator(),use_multidimensional_arrays) )
    else:
      dictionary["BASIS_DECLARATIONS"] += "static constexpr double DerivativeOperator1d[] = {};\n\n".format( render_tensor_2(self.__compute_derivative_operator(),use_multidimensional_arrays) )

    dictionary["BASIS_DECLARATIONS"] += "/**\n"
    dictionary["BASIS_DECLARATIONS"] += self.RestrictionMatrix1dDocumentation
    dictionary["BASIS_DECLARATIONS"] += "*/\n"
    if use_multidimensional_arrays:        
      dictionary["BASIS_DECLARATIONS"] += "static constexpr double RestrictionMatrix1d[][][] = {};\n\n".format( render_tensor_3(self.__compute_restriction_matrix(),use_multidimensional_arrays) )
    else:
      dictionary["BASIS_DECLARATIONS"] += "static constexpr double RestrictionMatrix1d[] = {};\n\n".format( render_tensor_3(self.__compute_restriction_matrix(),use_multidimensional_arrays) )

    dictionary["BASIS_DECLARATIONS"] += "/**\n"
    dictionary["BASIS_DECLARATIONS"] += self.InterpolationMatrix1dDocumentation
    dictionary["BASIS_DECLARATIONS"] += "*/\n"
    if use_multidimensional_arrays:        
      dictionary["BASIS_DECLARATIONS"] += "static constexpr double InterpolationMatrix1d[][][] = {};\n\n".format( render_tensor_3(self.__compute_interpolation_matrix(),use_multidimensional_arrays) )
    else:
      dictionary["BASIS_DECLARATIONS"] += "static constexpr double InterpolationMatrix1d[] = {};\n\n".format( render_tensor_3(self.__compute_interpolation_matrix(),use_multidimensional_arrays) )


  @property
  @abstractmethod
  def quadrature_points(self):
    assert False, "to be implemented by subclass"


  @property
  @abstractmethod
  def quadrature_weights(self):
    assert False, "to be implemented by subclass"


  def value1d(self,x,number):
    """
    
    Return the numberth shape function's value at x
    
    This equals Dominic's old evaluate function, but that one evaluated all the 
    polynomials in one rush, whereas this one only looks at shape function number
    number. Also, Dominic's version did both the value and the derivative. I 
    split this one up into two version.
    
    """  
    enumerator  = 1.0
    for i in range(0,self.dofs_per_axis):
      if i!=number:
        enumerator  *= (x-self.quadrature_points[i])
        
    denominator = 1.0
    for i in range(0,self.dofs_per_axis):
      if i!=number:
        denominator *= (self.quadrature_points[number]-self.quadrature_points[i])
        
    return enumerator/denominator


  def derivative1d(self,x,number):
    """
    
    Return the numberth shape function's derivative at x
    
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

    result = 0.0
    for l in range(0,self.dofs_per_axis):
      if l!=number:
        enumerator  = 1.0
        denominator = 1.0/(self.quadrature_points[number]-self.quadrature_points[l])
        for m in range(0,self.dofs_per_axis):
          if m!=number and m!=l:
            enumerator  *= x-self.quadrature_points[m]
            denominator *= 1.0/(self.quadrature_points[number]-self.quadrature_points[m])
        result += enumerator*denominator
    return result
  
from scipy.special import legendre
from scipy.special import roots_sh_legendre

class GaussLegendreBasis(LagrangeBasisWithDiagonalMassMatrix):
  """
  
  The Gauss-Legendre Basis is by construction the only basis which 
  yields diagonal mass matrices. I rely solely on SciPy here to 
  construct these functions, as I'm interested in numerical values
  only, and as the normal SciPy accuracy is sufficient. 
  
  There might be more elegant implementations using SymPy which 
  exploit the special structure behind Gauss-Legendre. But I prefer
  here to stick to one Python package only.
  
  """
  
  def __init__(self,polynomial_order):
    super(GaussLegendreBasis,self).__init__(polynomial_order)  
    
  
  @property
  def quadrature_points(self):
    """
    
    It seems to be a little bit weird that I have to use the polynomial
    order plus one here, but you have to read the docu to understand 
    that we are evaluating the polynomial of a certain order to get the 
    roots. So the argument has to be equal to the number of quadrature
    points that I wannt have.
    
    I originally used the roots_legendre and then did recalibrate the 
    result manually. Today, SciPy directly offers a shifted Legendre
    function which I can use directly. So I don't need an expression like
    
    
          return [ x/2.0+0.5 for x in roots_on_minus_one_one_interval ]


    anymore. The nicest definition of the shifted Legendre shape functions
    can, as so often, be found on Wikipedia:
    
    https://en.wikipedia.org/wiki/Legendre_polynomials#Shifted_Legendre_polynomials
    
    """
    roots, weights = scipy.special.roots_sh_legendre(self._polynomial_order+1)
    return roots    
    
  
  @property
  def quadrature_weights(self):
    """
    
    It seems to be a little bit weird that I have to use the polynomial
    order plus one here, but you have to read the docu to understand 
    that we are evaluating the polynomial of a certain order to get the 
    roots. So the argument has to be equal to the number of quadrature
    points that I wannt have.
    
    I originally used the roots_legendre and then did recalibrate the 
    result manually. Today, SciPy directly offers a shifted Legendre
    function which I can use directly. So I don't need an expression like
    
    
          return [ x/2.0+0.5 for x in roots_on_minus_one_one_interval ]


    anymore. The nicest definition of the shifted Legendre shape functions
    can, as so often, be found on Wikipedia:
    
    https://en.wikipedia.org/wiki/Legendre_polynomials#Shifted_Legendre_polynomials
    
    """
    roots, weights = scipy.special.roots_sh_legendre(self._polynomial_order+1)
    return weights



class GaussLobattoBasisWithLumpedDiagonalBasis(LagrangeBasisWithDiagonalMassMatrix):
  """

  The Gauss-Lobatto quadrature points do not yield a diagonal mass
  matrix. They yield a full matrix. However, there are codes which 
  ignore this fact and lump the basis. Mathematically, this is 
  equivalent to the Gauss-Lobatto quadrature because this leads to a diagonal matrix that is easy to invert
  
  """
  
  def __init__(self,polynomial_order):
    super(GaussLobattoBasisWithLumpedDiagonalBasis,self).__init__(polynomial_order)  
    
    
  @property
  def quadrature_points(self):
    assert False, "to be implemented by subclass"
    raise Exception( "check implementation. See class comment")
    x,w = quadrature.gauss_lobatto(self.dofs_per_axis,PREC_DIGITS) 
    return self._transform_and_sort_points_and_weights(x,w)

