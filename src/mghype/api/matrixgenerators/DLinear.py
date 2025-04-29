# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import numpy as np
import peano4
import mghype

from .MatrixGenerator import MatrixGenerator


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
               unknowns_per_node,
               ):

    super( DLinear, self ).__init__(dimensions,
                                    poly_degree = 1,
                                   )

    self.unknowns_per_node = unknowns_per_node
    
    # Use matrices for DG Interior Penalty for Poisson equation as we only need the cell-cell and mass matrices, which are identical from CG
    self.block_matrix = mghype.api.matrixgenerators.blockmatrix.DGPoisson2dPenaltyBlockMatrix(self.poly_degree)
    
  @property
  def _cell_dofs(self):
    """!
    @todo should this be 2**D or (p+1)**D? suppose it's not relevant, given this is a linear solver
    """
    return 2**(self.dimensions)
 
    
  def get_cell_identity_matrix(self):
    """!
        
    Return identity matrix and a 0 as the identity matrix has no intrinsic
    scaling, i.e. its scaling is @f$ h^0 @f$.
        
    """
    dim    = self._cell_dofs * self.unknowns_per_vertex_dof
    #           identity matrix.
    output = np.eye(dim,dim) * 2**(-self.dimensions)
    return output, 0

    
  def get_lumped_mass_matrix(self):
    """!

     Get this working for 2d. 
     
    What about 3d? If we select an index for the first dimension
    and then sum over the other two (ie call np.sum( massMatrix[i,] ))
    it will just add up all elements along the 2 other dimensions, as
    if we flattened it. Is this what we want?
    """
    #get mass matrix
    massMatrix = self.get_cell_mass_matrix()
    output     = np.zeros_like(massMatrix)
    for i in range( output.shape[0] ):
      output[i,i] = np.sum( massMatrix[i,] )
    return output
    
      
  def get_cell_mass_matrix(self):
    '''!
    
    Create a cell-cell mass matrix
    
    This matrix does not couple the individual unknowns per degree of freedom. 
    The resulting matrix has to be scaled by @f$ h^d @f$.

    Use the mass matrix generated for DG Interior Penalty for Poisson equation.

    '''
    output = self.block_matrix._RHS_CC[(1,1)]
    return [output], [self.dimensions]
        

  def get_cell_system_matrix_for_laplacian(self):
    '''!

    Create a cell-cell mass matrix
    
    In a finite element context, this matrix has to be 
    scaled with @f$ h^{d-2} @f$. Therefore, we retturn d-2 as second argument.

    Use the cell-cell matrix generated for DG Interior Penalty for Poisson equation.

    '''
    
    output = self.block_matrix._A_CC[(0,0)][(0,0)]
    return [output], [self.dimensions - 2]
