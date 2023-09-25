# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import numpy as np
from   numpy.polynomial.legendre  import leggauss
from   scipy.interpolate import lagrange

import peano4

from   .MatrixGenerator import MatrixGenerator



class DLinear(MatrixGenerator):
  '''!
  
  Simple matrix generator for continuous d-linear finite elements
  
  This factory method is there for convenience, i.e. users can take it to 
  pre-assemble local matrices and then to pipe it into the Peano 4 multigrid
  classes. Most of the stuff in here in hard-coded, so only well-suited for
  relatively primitive solvers. You might want to specialise these routines,
  switch to a more sophisticated matrix generator, or abandon the idea of a
  matrix generator alltogether.
    
  '''
  def __init__(self,
               dimensions,
               unknowns_per_vertex_dof,
               ):

    super( DLinear, self ).__init__(dimensions, 
                                   )
    self.unknowns_per_vertex_dof = unknowns_per_vertex_dof
    
  @property
  def _cell_dofs(self):
    return 2**(self.dimensions)
 
    
  def getCellIdentityMatrix(self):
    dim    = self._cell_dofs * self.unknowns_per_vertex_dof
    output = np.zeros((dim,dim))
    for i in range(dim):
      output[i,i] = 1.0
    return output

      
  def getCellMassMatrix(self):
    '''!
    
    Create a cell-cell mass matrix
    
    This matrix does not couple the individual unknowns per degree of freedom. 
    The resulting matrix has to be scaled by @f$ h^d @f$.

    '''
    dim    = self._cell_dofs * self.unknowns_per_vertex_dof
    output = np.zeros((dim,dim))
    for i in range( self._cell_dofs ):
      for j in range( self._cell_dofs ):
        for k in range( self.unknowns_per_cell_dof ):
          output[self.unknowns_per_cell_dof*i+k, self.unknowns_per_cell_dof*j+k] += self.evaluateIntegral2d(functions2d=[ self.getPoly2d(i), self.getPoly2d(j) ]) # phi_x-phi_x coupling 
    return output

  def getCellSystemMatrixForLaplacian(self):
    '''!

    Create a cell-cell mass matrix
    
    
    needs to be scaled 0.5*h, that is why we use factor=2.0 as evaluateIntegral2d() has coeff. 0.25 as default,
    so normalise by h
    '''
    dim    = self._cell_dofs * self.unknowns_per_vertex_dof
    output = np.zeros((dim,dim))
    for i in range( self._cell_dofs ):
      for j in range( self._cell_dofs ):
        for k in range( self.unknowns_per_vertex_dof     ):
          pass
          # They are all the same, but they don't have to be!
          # We should have different getDeriv2d, getPoly2d for different fields: u, phi_x, phi_y
          #output[3*i+1, 3*j] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,0), self.getPoly2d(j) ]) # phi_x-u coupling
          #output[3*i+2, 3*j] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,1), self.getPoly2d(j) ]) # phi_y-u coupling
          #output[3*i, 3*j+1] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,0), self.getPoly2d(j) ]) # u-phi_x coupling
          #output[3*i, 3*j+2] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,1), self.getPoly2d(j) ]) # u-phi_y coupling
    return output
  