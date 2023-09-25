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

class MatrixGenerator:
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
  def getCellSystemMatrixForLaplacian(self):
    """!
    
    The resulting matrix has to be scaled by @f$ h^{d-1} @f$.

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
    
    """
    raise NotImplementedError
