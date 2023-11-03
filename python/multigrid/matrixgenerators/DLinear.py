# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import numpy as np
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
 
    
  def get_cell_identity_matrix(self):
    dim    = self._cell_dofs * self.unknowns_per_vertex_dof
    #           identity matrix.
    output = np.eye(dim,dim) * 2**(-self.Dimensions)
    return output

      
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

    '''
    dim    = (self.poly_degree + 1) ** self.Dimensions
    output = np.zeros((dim,dim))

    for i in range(output.shape[0]):
      for j in range(output.shape[1]):
        output[i,j] = self.eval_integral(functions=[self.get_polynomial(i), self.get_polynomial(j)])
    
    #pad the output
    if self.unknowns_per_vertex_dof > 1:
      output = self.pad_matrix(output)

    return output

  def get_cell_system_matrix_for_laplacian(self):
    '''!

    Create a cell-cell mass matrix
    
    
    needs to be scaled 0.5*h, that is why we use factor=2.0 as evaluateIntegral2d() has coeff. 0.25 as default,
    so normalise by h

    '''
    # making it 2^D since this is number of vertices per cell
    dim    = self._cell_dofs
    output = np.zeros((dim,dim))

    for i in range(output.shape[0]):
      for j in range(output.shape[1]):
        output[i,j] += self.eval_integral(functions=[self.get_deriv(i, 0), self.get_deriv(j, 0)])
        output[i,j] += self.eval_integral(functions=[self.get_deriv(i, 1), self.get_deriv(j, 1)])
    
    #pad the output
    if self.unknowns_per_vertex_dof > 1:
      output = self.pad_matrix(output)

    return output

  def pad_matrix(self, matrix):
    """!
    This method pads a matrix by concatenating one copy of itself to the end
    for each unknown_per_vertex dof.

    @todo - Currently this just pads the same matrix out to each of the multiple dofs per node. Maybe provide option to not do this, eg just leave some of them empty
    """
    shape  = matrix.shape
    # make it bigger 
    output_shape  = [ self.unknowns_per_vertex_dof * s for s in shape ]
    output        = np.zeros( output_shape )

    for u in range( self.unknowns_per_vertex_dof ):
      output[ shape[0]*u: shape[0]*(1+u), shape[1]*u:shape[1]*(1+u) ] = matrix
    
    return output