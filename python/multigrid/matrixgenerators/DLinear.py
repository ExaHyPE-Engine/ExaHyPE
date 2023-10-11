# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import numpy as np
from   numpy.polynomial.legendre  import leggauss
from   scipy.interpolate import lagrange

import peano4

from   MatrixGenerator import MatrixGenerator



class DLinear(MatrixGenerator):
  '''!
  
  Simple matrix generator for continuous d-linear finite elements
  
  This factory method is there for convenience, i.e. users can take it to 
  pre-assemble local matrices and then to pipe it into the Peano 4 multigrid
  classes. Most of the stuff in here in hard-coded, so only well-suited for
  relatively primitive solvers. You might want to specialise these routines,
  switch to a more sophisticated matrix generator, or abandon the idea of a
  matrix generator altogether.
    
  '''
  def __init__(self,
               dimensions,
               poly_degree=1,
               unknowns_per_vertex_dof=1,
               ):

    super( DLinear, self ).__init__(dimensions,
                                    poly_degree,
                                   )
    self.unknowns_per_vertex_dof = unknowns_per_vertex_dof
    
  @property
  def _cell_dofs(self):
    """!
    @todo should this be 2**D or (p+1)**D? suppose it's not relevant, given this is a linear solver
    """
    return 2**(self.Dimensions)
 
    
  def getCellIdentityMatrix(self):
    dim    = self._cell_dofs * self.unknowns_per_vertex_dof
    #           identity matrix.
    output = np.eye(dim,dim) * 2**(-self.Dimensions)
    return output

      
  def getLumpedMassMatrix(self):
    """!
    Get this working for 2d. 
    What about 3d? If we select an index for the first dimension
    and then sum over the other two (ie call np.sum( massMatrix[i,] ))
    it will just add up all elements along the 2 other dimensions, as
    if we flattened it. Is this what we want?
    """
    #get mass matrix
    massMatrix = self.getCellMassMatrix()
    output     = np.zeros_like(massMatrix)
    for i in range( output.shape[0] ):
      output[i,i] = np.sum( massMatrix[i,] )
    return output
    
      
  def getCellMassMatrix(self):
    '''!
    
    Create a cell-cell mass matrix
    
    This matrix does not couple the individual unknowns per degree of freedom. 
    The resulting matrix has to be scaled by @f$ h^d @f$.

    '''
    dim    = (self.poly_degree + 1) ** self.Dimensions
    output = np.zeros((dim,dim))

    for i in range(output.shape[0]):
      for j in range(output.shape[1]):
        output[i,j] = self.evalIntegral(functions=[self.getPolynomial(i), self.getPolynomial(j)])
    
    #pad the output
    if self.unknowns_per_vertex_dof > 1:
      output = self.padMatrixForMoreThanOneDofPerNode(output)

    return output

  def getCellSystemMatrixForLaplacian(self):
    '''!

    Create a cell-cell mass matrix
    
    
    needs to be scaled 0.5*h, that is why we use factor=2.0 as evaluateIntegral2d() has coeff. 0.25 as default,
    so normalise by h

    '''
    dim    = (self.poly_degree + 1) ** self.Dimensions
    output = np.zeros((dim,dim))

    for i in range(output.shape[0]):
      for j in range(output.shape[1]):
        output[i,j] += self.evalIntegral(functions=[self.getDeriv(i, 0), self.getDeriv(j, 0)])
        output[i,j] += self.evalIntegral(functions=[self.getDeriv(i, 1), self.getDeriv(j, 1)])
    
    #pad the output
    if self.unknowns_per_vertex_dof > 1:
      output = self.padMatrixForMoreThanOneDofPerNode(output)

    return output

  def padMatrixForMoreThanOneDofPerNode(self, matrix):
    """!
    @todo - Currently this just pads the same matrix out to each of the multiple dofs per node. 
    """
    shape = matrix.shape
    # scale up each entry
    upv    = self.unknowns_per_vertex_dof #alias
    shape  = [s * upv for s in shape]
    output = np.zeros(shape)
    # pad!
    for i in range(matrix.shape[0]):
        # now we want to loop over the rows within the node
      for dof in range(upv):
        # get the row we want from the matrix passed in
        row = matrix[i]

        #make a dummy row to fill out
        newRow = np.zeros( matrix.shape[0] * upv )

        # we want to populate every upv'th entry, starting with dof
        for d in range(row.shape[0]):
          newRow[ upv*d + dof ] = row[d]
        #place this into the output
        output[ upv*i + dof ] = newRow
    return output
