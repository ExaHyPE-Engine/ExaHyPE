# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
'''

here we put in a base class for generating 
matrices that we pipe into c++ code

'''
from abc import abstractmethod

import numpy as np
from numpy.polynomial.legendre  import leggauss
from scipy.interpolate import lagrange

from itertools import product

class MatrixGenerator:
  """!
  use this as a base class for generating matrices 
  that we pipe into the c++ code

  Every method marked abstract is intended to be
  overwritten in child class, but some will come
  with default behaviour.

  Warning - we didn't write the getFunctions in a
  functional way, and so eval integral cannot
  accept derivatives at this time

  """
  def __init__(self,
               dimensions,
               poly_degree
               ):
    self.Dimensions    = dimensions
    self.poly_degree   = poly_degree

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
      dim=self.Dimensions
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
      dim = self.Dimensions
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
      dim=self.Dimensions
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

    The index should be betweeen 0 and (self.poly_degree)^self.Dimensions

    We convert the index into a tuple (x,y,z) of indices, pull
    out appropriate polynomials and return a function object.
    This function object will expect dim arguments.
    """
    if dim == -1: 
      dim = self.Dimensions

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
      dim = self.Dimensions
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
      dim = self.Dimensions
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
    
    The resulting matrix has to be scaled by @f$ h^{d-1} @f$.

    Implement this in child class.

    """
    raise NotImplementedError
  
  @abstractmethod
  def get_cell_mass_matrix(self):
    """!
    
    A factor method to pre-assemble the mass matrix for a cell. This is
    a routine every matrix generator should provide. Obviously, some 
    schemes host function spaces on the faces and therefore might have
    mass matrices for the faces, too. Therefore, we call is cell mass
    matrix.

    The resulting matrix has to be scaled by @f$ h^d @f$.

    Implement this in child class.
    
    """
    raise NotImplementedError

