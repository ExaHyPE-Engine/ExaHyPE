# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org

from abc import abstractmethod

import numpy as np
from numpy.polynomial.legendre  import leggauss
from scipy.interpolate import lagrange

from itertools import product

class MatrixGenerator:
  """!
      
  Base class for generating matrices that we pipe into the C++ code

  Every method marked abstract is intended to be overwritten in child classes, but some will come
  with default behaviour. 

  """
  def __init__(self,
               dimensions,
               poly_degree,
               unknowns_per_node=1
               ):
    self.dimensions    = dimensions
    self.poly_degree   = poly_degree
    self.unknowns_per_node = unknowns_per_node

    self.polynomials1d = self.get_polynomial1d()
    self.derivatives1d = [ p.deriv() for p in self.polynomials1d ]
    self.points1d      = self.get_points1d()
    self.weights1d     = self.get_weights1d()

  @abstractmethod
  def get_polynomial1d(self):
    """!
    Method to create a list of 1D polynomials.
    
    We promote these to higher dimensions by
    taking tensor products. 
    
    By default, we will return Lagrange polynomials,
    but we can overwrite this in child class
    """
    polys = []

    '''
    take a linspace from -1 -> 1 as this is how
    gaussian quadrature points are defined. Note
    this may later require us to convert from 
    coordinates in [-1,1] to the global mesh
    coordinates.
    '''
    x = np.linspace(-1,1, self.poly_degree + 1)
    for j in range(self.poly_degree + 1):
      y = np.zeros_like(x)
      y[j] = 1.0
      polys.append( lagrange(x,y) )
    return polys

  @abstractmethod
  def get_points1d(self):
    """!
    Method to set the quadrature points, used for 
    integration.

    Provide a method here to produce Gauss-Legendre
    points, but this can be overwritten in child
    class.

    Slightly redundant use of the leggauss function
    here, but I thought it better to separate the 
    calculation of points and weights for ease of
    implementing in child class.
    """
    points, weights = leggauss( self.poly_degree + 1 )
    return points

  def get_integration_points_over_unit_interval(self):
    """!
        
    Most schemes define the integration point over the interval [-1,1]
    whereas MGHyPE often needs it on the unit interval. Therefore, I 
    offer this genereric recalibration routine.
    
    """
    shift   = -min(self.get_points1d())
    scaling = 1.0/(max(self.get_points1d())-min(self.get_points1d()))
    return [ (x+shift)*scaling for x in self.get_points1d() ]
  
  @abstractmethod
  def get_weights1d(self):
    """!
    Method to set the quadrature weights, used for 
    integration.

    Provide a method here to produce Gauss-Legendre
    weights, but this can be overwritten in child
    class.

    Slightly redundant use of the leggauss function
    here, but I thought it better to separate the 
    calculation of points and weights for ease of
    implementing in child class.
    """
    points, weights = leggauss( self.poly_degree + 1 )
    return weights

  def get_points_for_dimension(self, dim=-1):
    """!
    We promote the 1D quadrature points
    in into the dimensions of our problem
    by taking cartesian product. This works
    for any dimension and does not need to
    be implemented in the child class.

    We may need to produce arrays of points 
    for number of dimensions which is less 
    """
    if dim==-1:
      dim=self.dimensions
    #make array of length dim, where each 
    #entry is a copy of self.points1d
    points = [self.points1d for _ in range(dim)]

    '''!
    This is a bit of python magic. We unpack points, so we 
    are in effect passing Dimensions different arguments to 
    iterools.product. itertools.product then produces every
    possible combination, in tuple form. We then capture this
    in a list, and return it. Ie if we start with a list of 
    1d points: [p0,p1,p2,p3,...], we return, for Dimensions==3:
    [
      (p0,p0,p0),
      (p0,p0,p1),
      (p0,p0,p2),
      ...
    ]
    '''
    return [ *product( *points ) ]

  def get_weights_for_dimension(self, dim=-1):
    """!
    We promote the 1D quadrature weights
    in into the dimensions of our problem
    by taking cartesian product. This works
    for any dimension and does not need to
    be implemented in the child class.
    """
    if dim==-1:
      dim = self.dimensions
    #dummy function to return product of entries in array
    def arrayProduct(array):
      output = 1
      for el in array: output *= el
      return output

    #create an array of length dim, where each
    #each entry of the array is a copy of the 1d weights
    weights = [self.weights1d for _ in range(dim)]

    '''
    This time, given 1d weights [w0,w1,w2,...], we want to
    return:
    [
      w0*w0*w0,
      w0*w0*w1,
      w0*w0*w2,
      ...
    ]
    '''
    return [ arrayProduct(array) for array in product( *weights ) ]
  
  def convert_index_to_dim(self, index, dim=-1):
    """!
    This method helps with indexing. Typically,
    for polynomial degree p, we have (p+1) nodal
    points per coordinate axis, for a total of
    @f$ p^d @f$. This method converts an index
    in the range @f$ (0, ..., p^d) @f$ into a
    tuple of (x,y,z) coordinates.
    """
    #support converting index to any dimension. default to max
    if dim==-1:
      dim=self.dimensions
    assert index in range((self.poly_degree+1) ** dim), "index outside of range!"
    output = [0 for _ in range(dim)]
    for d in range(dim-1,-1,-1):
      pToD      = (self.poly_degree+1)**(d)
      output[d] = (index - index%pToD) // pToD
      index     = index % pToD
    return output
    
  def get_polynomial(self, index, dim=-1):
    """!
    Promote 1d polynomials to many dimensions
    by taking products.

    We pass in an index and a dimension, which will, by default
    be set to the number of dimensions of the problem.

    The index should be betweeen 0 and (self.poly_degree)^self.dimensions

    We convert the index into a tuple (x,y,z) of indices, pull
    out appropriate polynomials and return a function object.
    This function object will expect dim arguments.
    """
    if dim == -1: 
      dim = self.dimensions

    # the indices of the polynomials we wanna fetch
    # warning: will misfire if "index" not in correct range for dim
    indices =   self.convert_index_to_dim(index, dim)
    polys   = [ self.polynomials1d[i] for i in indices ]

    #define function object to return:
    def returnFunc(*args):
      '''
      The intention of this part is as follows:
      We have (for example) 3 arguments for a 3d function: [arg_x, arg_y, arg_z]
      We have correspondingly 3 polynomials, since we promote a 1d basis to a 3d
      basis by taking products: [poly_x, poly_y, poly_z].
      Then, we return poly_x(arg_x) * poly_y(arg_y) * poly_z(arg_z)

      The polys don't go out of scope.      
      '''
      assert(len(args)==dim), f"Supplied {len(args)} args to a {dim}d function!"
      out = 1
      for i, arg in enumerate(args):
        # dummy evaluation of i'th polynomial
        out *= polys[i](arg)
      return out

    #return this function object
    return returnFunc

  def get_deriv(self, index, dimForDeriv, dim=-1):
    """!
    dimForDeriv is the singular dimension that we wanna take a polyderiv in

    Behaves the same as get_polynomial, except in that in the specified 
    dimension, we take a derivative rather than the poly itself
    """
    if dim == -1: 
      dim = self.dimensions
    assert dimForDeriv in range(dim), f"dimForDeriv={dimForDeriv}, dim={dim}"


    # the indices of the polynomials we wanna fetch
    # warning: will misfire if "index" not in correct range for dim
    indices = self.convert_index_to_dim(index, dim)
    polys   = []
    for i, index in enumerate(indices):
      if i == dimForDeriv:
        # get poly deriv instead
        polys.append( self.derivatives1d[index] )
      else:
        polys.append( self.polynomials1d[index] )
    
    #define function object to return:
    def returnFunc(*args):
      assert(len(args)==dim), f"Supplied {len(args)} args to a {dim}d function!"
      out = 1
      for i, arg in enumerate(args):
        # dummy evaluation of i'th polynomial
        out *= polys[i](arg)
      return out

    #return this function object
    return returnFunc

  def eval_integral(self, functions, dim=-1, factor=1):
    if dim == -1:
      dim = self.dimensions
    points  = self.get_points_for_dimension(dim)
    weights = self.get_weights_for_dimension(dim)
    output  = 0

    for p, w in zip(points, weights):
      localEval  = 1
      for func in functions:
        # do the following for each function we pass in
        # need to unpack p since this function will expect
        # 3 arguments for 3d, etc...
        localEval *= func( *p )
      # aggregate
      output += w * localEval
    return output * factor

  @abstractmethod
  def get_cell_system_matrix_for_laplacian(self):
    """!
     
    Construct the Laplacian
    
    Create the Laplacian. This routine represents the fact
    that the Laplacian is kind of the evergreen of many 
    PDEs, and we therefore provide a factory method for 
    this one always.
    
    These factory methods all return two lists. The first
    one is a list of matrices @f$ A_1, A_2, ... @f$ defined relative to the unit
    cube or square, respectively. The second list has the
    same length as the first list, and stores integer indices
    @f$ k_1, k_2, k_3, ... @f$. Throughout the assemly, you
    should then apply the local matrix
    @f$ A = h^{k_1} A_1 + h^{k_2} A_2 + ... @f$. The mass 
    matrix typically is only one matrix scaled with d, i.e.
    we return a list hosting only one matrix as first argument
    and the second list only hosts the integer self.dimensions.

    Implement this in child class.
    
    @return [Matrix] x [Int]
      The two lists have to have the same number of entries.

    """
    raise NotImplementedError
  
  @abstractmethod
  def get_cell_mass_matrix(self):
    """!
    Factory for cell mass matrix.
    
    Consult get_cell_system_matrix_for_laplacian() for an overview
    of the result types. As the mass matrix is typically fairly
    simple (it does not host any derivative), and as we work within
    a finite element mindset, this routine returns a list with one
    matrix as first argument, and then a list with the integer
    self.dimensions.
    
    """
    raise NotImplementedError

  @abstractmethod
  def get_cell_identity_matrix(self):
    """!
    
    Construct identity for this particular equation system
    
    Not that this is identity construction has to take the number of
    unknowns into account as well as the number of degrees of freedom
    hosted within a cell.
        
    """
    raise NotImplementedError
