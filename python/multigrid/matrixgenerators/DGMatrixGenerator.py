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

class DGMatrixGenerator:
  """!
  use this as a base class for generating matrices 
  that we pipe into the c++ code

  at present, only works for 2d
  """
  def __init__(self,
               dimensions,
               ):
    self.dimensions  = dimensions
      


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
