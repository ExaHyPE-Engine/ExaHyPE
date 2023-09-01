'''

here we put in a base class for generating 
matrices that we pipe into c++ code

'''
from abc import abstractmethod

import numpy as np
from numpy.polynomial.legendre  import leggauss
from scipy.interpolate import lagrange

class MatrixGeneratorBase:
  """!
  use this as a base class for generating matrices 
  that we pipe into the c++ code

  at present, only works for 2d
  """
  def __init__(self,
               dimensions,
               poly_degree,
               unknowns_per_cell_dof,
               unknowns_per_face_dof):
    self.dimensions  = dimensions
    self.poly_degree = poly_degree
    self.unknowns_per_cell_dof = unknowns_per_cell_dof
    self.unknowns_per_face_dof = unknowns_per_face_dof

    self.celldofs    = ( self.poly_degree + 1 ) ** (self.dimensions)
    self.facedofs    = ( self.poly_degree + 1 ) ** (self.dimensions - 1)

    self.polynomials = self.set_polynomials()
    self.points,\
    self.weights     = leggauss( self.poly_degree + 1 )
    self.weights2d   = [ w1*w2  for w1 in self.weights for w2 in self.weights]
    self.points2d    = [(p1,p2) for p1 in self.points  for p2 in self.points ]
    #2d face normal for   0       1       2      3
    self.faceNormals = [ [-1,0], [0,-1], [1,0], [0,1] ]

  @staticmethod
  def convert1dPointTo2d(faceNo, point1d):
    '''
    dummy function to convert 1d quadpoint on a face to a
    2d quadpoint, for use in 2d basis functions
    
     _ 3 
    |   |2
   0|_ _| 
       1

    '''
    directions = [-1,-1,1,1]
    if faceNo % 2 == 0: #evens. ie 0 and 2
      return [directions[faceNo], point1d]
    else:
      return [point1d, directions[faceNo]]

  def set_polynomials(self):
    polys = []
    x = np.linspace(-1,1, self.poly_degree + 1)
    for j in range(self.poly_degree + 1):
      y = np.zeros_like(x)
      y[j] = 1.0
      polys.append( lagrange(x,y) )
    return polys

  def convertIndexTo2d(self, index):
    x = index %  ( self.poly_degree + 1 )
    y = index // ( self.poly_degree + 1 )
    return [x,y]

  def getPoly1d(self,index):
    '''
    returns the 1d poly at the given index.
    you should treat the returned object like a function!
    '''
    assert 0 <= index <= self.poly_degree
    return self.polynomials[index]
 
  def getPoly2d(self,index):
    '''
    returns the 2d poly a the given index
    '''
    assert 0 <= index <= self.celldofs

    #convert to 2d
    indexX, indexY = self.convertIndexTo2d(index)
    
    #get 1d polys for each
    polyX, polyY   = self.getPoly1d(indexX), self.getPoly1d(indexY)

    #return a lambda that evaluates the product of these polynomials 
    #at a given point
    return lambda x,y: polyX(x) * polyY(y)

  def getDeriv1d(self, index):
    return self.polynomials[index].deriv()
  
  def getDeriv2d(self,index,dimension):
    assert dimension in [0,1]
    #convert to 2d
    indexX, indexY = self.convertIndexTo2d(index)
    if   dimension == 0: #want to take deriv in x direction
      polyX, polyY = self.getDeriv1d(indexX), self.getPoly1d(indexY)
    elif dimension == 1:
      polyX, polyY = self.getPoly1d(indexX),  self.getDeriv1d(indexY)
    return lambda x,y : polyX(x) * polyY(y)
    
  def evaluateIntegral1d(self, faceNo, factor=1, functions1d=[], functions2d=[]):
    output = 0
    for p, w in zip(self.points, self.weights):
      #evaluate all the 1d functions at this point
      eval1d = 1
      for function1d in functions1d: 
        eval1d *= function1d(p)
      #same for 2d functions
      eval2d = 1
      point2d = self.convert1dPointTo2d(faceNo, p)
      for function2d in functions2d:
        eval2d *= function2d(*point2d)
      #aggregate into the output variable
      output += 0.5 * w * eval1d * eval2d

    #return the output, scaled by the factor we pass in
    return output * factor

  def evaluateIntegral2d(self, factor=1, functions2d=[]):
    output = 0
    for p, w in zip(self.points2d, self.weights2d):
      eval2d = 1
      #get the product of all these functions we passed
      #at this point
      for function2d in functions2d:
        #we unpack with *p because functions2d is lambda that 
        #requires two arguments
        eval2d *= function2d(*p)
      
      #aggregate
      output += 0.25 * w * eval2d
    return output * factor

  @abstractmethod
  def getCellCellSystemMatrix(self):
    raise NotImplementedError
  
  @abstractmethod
  def getCellCellMassMatrix(self):
    raise NotImplementedError

  @abstractmethod
  def getCellToFaceMatrix(self):
    '''
    project cell values onto faces
    '''
    raise NotImplementedError

  @abstractmethod
  def getFaceFaceMatrix(self):
    '''
    glue together solution across faces
    '''
    raise NotImplementedError
