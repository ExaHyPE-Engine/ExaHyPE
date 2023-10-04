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

  """
  def __init__(self,
               dimensions,
               poly_degree=1
               ):
    self.Dimensions    = dimensions
    self.poly_degree   = poly_degree

    '''
    The next three attributes are set here, and 
    are promoted to 2/3 dimensions by calling
    getPolynomialsForDimension, for instance.
    Each comes with a default argument of 
    self.Dimensions, but this can be modified if
    a lower dimension of polynomial is required
    '''
    self.polynomials1d = self.getPolynomial1d()
    self.points1d      = self.getPoints1d()
    self.weights1d     = self.getWeights1d()

  @abstractmethod
  def getPolynomial1d(self):
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
  def getPoints1d(self):
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
  def getWeights1d(self):
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

  def getPointsForDimension(self, dim=self.Dimensions):
    """!
    We promote the 1D quadrature points
    in into the dimensions of our problem
    by taking cartesian product. This works
    for any dimension and does not need to
    be implemented in the child class.

    We may need to produce arrays of points 
    for number of dimensions which is less 
    """
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

  def getWeightsForDimension(self, dim=self.Dimensions):
    """!
    We promote the 1D quadrature weights
    in into the dimensions of our problem
    by taking cartesian product. This works
    for any dimension and does not need to
    be implemented in the child class.
    """
    #dummy function to return product of entries in array
    def arrayProduct(array):
      output = 1
      for el in array: output *= el
      return array

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
  
  def getPolynomial(self, index):
    """!
    Promote 1d polynomials to many dimensions
    by taking products. 
    """
    raise NotImplementedError

  @abstractmethod
  def getCellSystemMatrixForLaplacian(self):
    """!
    
    The resulting matrix has to be scaled by @f$ h^{d-1} @f$.

    Implement this in child class.

    """
    raise NotImplementedError
  
  @abstractmethod
  def getCellMassMatrix(self):
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
