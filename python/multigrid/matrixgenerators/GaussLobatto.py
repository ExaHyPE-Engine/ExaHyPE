# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import numpy as np
from   numpy.polynomial.legendre  import leggauss
from   scipy.interpolate import lagrange
from abc import abstractmethod

import peano4

from .MatrixGenerator import MatrixGenerator
#from .BlockMatrix.block_matrix_dgpoisson2d import DGPoisson2dBlockMatrix
from .BlockMatrix.block_matrix_dgpoisson2dpenalty import DGPoisson2dPenaltyBlockMatrix

class GaussLobatto(MatrixGenerator):
  '''
  
  Simple matrix generator assuming that we use a Gauss Lobatto unknown layout
  
  If you use this class, you assuem that a cell hosts @f$ (p+1)^d @f$
  nodes carrying Legendre polynomials. As you use a Gauss Lobatto layout,
  the "outer" nodes will end up on the cell faces. Each node can hold a
  fixed number of unknowns. If you multiply the number of nodes per cell
  with the corresponding number of unknowns, you get the degrees of freedom
  within a global assembly matrix which correspond to volumetric data. 
  
  '''
  def __init__(self,
               Dimensions,
               poly_degree,
               unknowns_per_cell_node=1,
               unknowns_per_face_node=1,
               use_unit_interval=False):

    super( GaussLobatto, self ).__init__(Dimensions,
                                         poly_degree
                                         )
    self.unknowns_per_cell_node = unknowns_per_cell_node
    self.unknowns_per_face_node = unknowns_per_face_node

    self.cell_nodes    = ( self.poly_degree + 1 ) ** (self.Dimensions)
    self.face_nodes    = ( self.poly_degree + 1 ) ** (self.Dimensions - 1)

    self.use_unit_interval = use_unit_interval

    if use_unit_interval:
      self.points1d    = self.get_points_()

    r_press = self.poly_degree # polynomial order for p
    self.block_matrix = DGPoisson2dPenaltyBlockMatrix(r_press)
    #self.block_matrix = DGPoisson2dBlockMatrix(r_press)

  def get_points1d(self):
    """!
    Overwriting the method in base class. Let's produce Gauss-Lobatto
    points. Standard behaviour is Legendre points using built-in stuff
    from scipy.
    We borrow the method from the wikipedia page
    """
    # let p be the poly degree. Then p+1 is the number of points we aim for
    # So we gather the n'th polynomial Pn, then return [-1,1], as well as the roots
    # of the derivative of Pn
    output = [-1] #we'll append 1 at the end
    if self.poly_degree > 1: # no need to do anything for degree 1
      n      = self.poly_degree
      # next few lines are just what needs to be done for numpy
      coeffs = [0 for _ in range(n+1)]
      coeffs[n] = 1
      poly   = np.polynomial.legendre.Legendre(coeffs)
      #get the roots
      roots  = poly.deriv().roots().tolist()
      # append
      output += roots
    #clip the weird rounding errors
    for i in range(len(output)):
      if abs(output[i])<1E-10:
        output[i]=0
    output.append(1)
    return output

  def get_weights1d(self):
    """!
    Overwrites the method in the base class. Method
    is borrowed from wikipedia
    """
    # number of points
    n = self.poly_degree + 1
    firstWeight = 2 / (n*(n-1))

    # need to get P_{n-1}
    coeffs = [0 for _ in range(n)]
    coeffs[n-1] = 1
    poly   = np.polynomial.legendre.Legendre(coeffs)

    output = [firstWeight]
    for p in self.get_points1d()[1:-1]:
      weight = firstWeight / poly(p)**2
      output.append(weight)

    output.append(firstWeight)
    return output

  def get_weights1d_legendre(self):
    return super( GaussLobatto, self ).get_weights1d()

  def get_points1d_legendre(self):
    return super( GaussLobatto, self ).get_points1d()

  
  def get_points_unit_interval(self):
    scale = lambda x: 0.5*(x+1)
    return [scale(x) for x in self.points1d]

  def eval_integral(self, functions, dim=-1, factor=1):
    # if we shrank the interval, then we need to pass factor = 0.5 along
    if self.use_unit_interval:
      factor = 0.5
    else:
      factor = 1
    # no need to re-implement
    return super( GaussLobatto, self ).eval_integral(functions, dim, factor)

  def get_cell_identity_matrix(self):
    dim    = self.cell_nodes * self.unknowns_per_cell_node
    output = np.eye(dim,dim)# * 2**(-self.Dimensions)
    return output
      
  def get_cell_mass_matrix(self):
    '''!
    
    Create a cell-cell mass matrix
    
    This matrix does not couple the individual unknowns per degree of freedom. 
    The resulting matrix has to be scaled by @f$ h^d @f$.

    '''
    # dim    = self.cell_nodes * self.unknowns_per_cell_node
    # output = np.zeros((dim,dim))
    # for i in range( self.cell_nodes ):
    #   for j in range( self.cell_nodes ):
    #     for k in range( self.unknowns_per_cell_node ):
    #       output[self.unknowns_per_cell_node*i+k, self.unknowns_per_cell_node*j+k] += self.eval_integral(functions=[ self.get_polynomial(i), self.get_polynomial(j) ]) 
    
    output = self.block_matrix._RHS_CC[(1,1)]
    return output

  def get_cell_system_matrix_for_laplacian(self):
    '''Create a cell-cell mass matrix for $\nabla p \cdot \nabla v$
    '''
    # dim    = self.cell_nodes * self.unknowns_per_cell_node
    # output = np.zeros((dim,dim))
    # for i in range( self.cell_nodes ):
    #   for j in range( self.cell_nodes ):
    #     for k in range( self.unknowns_per_cell_node ):
    #       pass
    #       # We should have different getDeriv2d, getPoly2d for different fields: u, phi_x, phi_y
    #       #output[3*i+1, 3*j] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,0), self.getPoly2d(j) ]) # phi_x-u coupling
    #       #output[3*i+2, 3*j] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,1), self.getPoly2d(j) ]) # phi_y-u coupling
    #       #output[3*i, 3*j+1] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,0), self.getPoly2d(j) ]) # u-phi_x coupling
    #       #output[3*i, 3*j+2] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,1), self.getPoly2d(j) ]) # u-phi_y coupling
    return self.block_matrix._A_CC[(0,0)][(0,0)]
  
  def get_face_to_cell_matrix(self):

    """Return the cel-to-face matrix A_p^qf
    """
    # SCALING: scaled by h
    A_CF_q_left   = self.block_matrix._A_CF[(0,2)]["left"][(0,1)]
    A_CF_q_bottom = self.block_matrix._A_CF[(0,2)]["bottom"][(1,0)]
    A_CF_q_right  = self.block_matrix._A_CF[(0,2)]["right"][(0,1)]
    A_CF_q_top    = self.block_matrix._A_CF[(0,2)]["top"][(1,0)]

    # SCALING:
    # !!! A_{C<-F} has penalty components of different scaling!  
    # For now, we just sum them up into one matrix with scaling (0,0)
    # Need to work out how to separate them in the solver.
    A_CF_u_left   = self.block_matrix._A_CF[(0,5)]["left"][(0,0)] # + block_matrix._A_CF[(0,5)]["left"][(0,1)]
    A_CF_u_bottom = self.block_matrix._A_CF[(0,5)]["bottom"][(0,0)] # + block_matrix._A_CF[(0,5)]["bottom"][(1,0)]
    A_CF_u_right  = self.block_matrix._A_CF[(0,5)]["right"][(0,0)] # + block_matrix._A_CF[(0,5)]["right"][(0,1)]
    A_CF_u_top    = self.block_matrix._A_CF[(0,5)]["top"][(0,0)] # + block_matrix._A_CF[(0,5)]["top"][(1,0)]

    A_CF_left = np.concatenate((A_CF_q_left, A_CF_u_left), axis=1)
    A_CF_bottom = np.concatenate((A_CF_q_bottom, A_CF_u_bottom), axis=1)
    A_CF_right = np.concatenate((A_CF_q_right, A_CF_u_right), axis=1)
    A_CF_top = np.concatenate((A_CF_q_top, A_CF_u_top), axis=1)

    A_CF_result = np.concatenate((A_CF_left, A_CF_bottom, A_CF_right, A_CF_top), axis=1)
    
    return A_CF_result
  
    # SCALING: Consider our case with 4x16 matrix:
    #
    #    by h        by h        by 1        by 1        by h        by h        by 1        by 1        by h        by h        by 1        by 1        by h        by h        by 1        by 1      
    # [[ 0.33333333  0.16666667  0.33333333  0.16666667 -0.33333333 -0.16666667  0.33333333  0.16666667  0.          0.         -0.33333333 -0.16666667  0.          0.         -0.33333333 -0.16666667]
    #  [ 0.          0.         -0.33333333 -0.16666667 -0.16666667 -0.33333333  0.16666667  0.33333333 -0.33333333 -0.16666667  0.33333333  0.16666667  0.          0.         -0.16666667 -0.33333333]
    #  [ 0.16666667  0.33333333  0.16666667  0.33333333  0.          0.         -0.33333333 -0.16666667  0.          0.         -0.16666667 -0.33333333  0.33333333  0.16666667  0.33333333  0.16666667]
    #  [ 0.          0.         -0.16666667 -0.33333333  0.          0.         -0.16666667 -0.33333333 -0.16666667 -0.33333333  0.16666667  0.33333333  0.16666667  0.33333333  0.16666667  0.33333333]]
  
    #faceDim = self.face_nodes * self.unknowns_per_face_node * 2 * self.Dimensions * 2 # * 2 at the end since we are glueing together 2 faces
    #cellDim = self.cell_nodes * self.unknowns_per_cell_node
    # 
    #return np.zeros((faceDim,cellDim))
  
  def get_cell_to_face_matrix(self):
    """Return the projection operator
    """
    ### Compute the normal gradient projections
    A_qp_u_left = self.block_matrix._A_FC[(0,0)]["left"][(0,0)]
    A_qp_u_bottom = self.block_matrix._A_FC[(0,0)]["bottom"][(0,0)]
    A_qm_u_right = self.block_matrix._A_FC[(1,0)]["right"][(0,0)]
    A_qm_u_top = self.block_matrix._A_FC[(1,0)]["top"][(0,0)]

    M_qp_qp_left   = self.block_matrix._A_FF[(0,0)]["left"][(0,1)]   # These are now the same;
    M_qp_qp_bottom = self.block_matrix._A_FF[(0,0)]["bottom"][(1,0)] # keep for generality
    M_qm_qm_right = self.block_matrix._A_FF[(1,1)]["right"][(0,1)]   #
    M_qm_qm_top  = self.block_matrix._A_FF[(1,1)]["top"][(1,0)]      #

    # SCALING: All P_q matrices are scaled with power -1 (i.e. 1/h)
    P_qp_u_left = -np.dot(np.linalg.inv(M_qp_qp_left), A_qp_u_left)
    P_qp_u_bottom = -np.dot(np.linalg.inv(M_qp_qp_bottom), A_qp_u_bottom)
    P_qm_u_right = -np.dot(np.linalg.inv(M_qm_qm_right), A_qm_u_right)
    P_qm_u_top = -np.dot(np.linalg.inv(M_qm_qm_top), A_qm_u_top)

    ### Compute the solution projections
    A_up_u_left = self.block_matrix._A_FC[(3,0)]["left"][(0,1)]
    A_up_u_bottom = self.block_matrix._A_FC[(3,0)]["bottom"][(1,0)]
    A_um_u_right = self.block_matrix._A_FC[(4,0)]["right"][(0,1)]
    A_um_u_top = self.block_matrix._A_FC[(4,0)]["top"][(1,0)]

    M_up_up_left   = self.block_matrix._A_FF[(3,3)]["left"][(0,1)]   # These are now the same;
    M_up_up_bottom = self.block_matrix._A_FF[(3,3)]["bottom"][(1,0)] # keep for generality
    M_um_um_right = self.block_matrix._A_FF[(4,4)]["right"][(0,1)]   #
    M_um_um_top  = self.block_matrix._A_FF[(4,4)]["top"][(1,0)]      #

    # SCALING: All P_u matrices are scaled with power 0
    P_up_u_left = -np.dot(np.linalg.inv(M_up_up_left), A_up_u_left)
    P_up_u_bottom = -np.dot(np.linalg.inv(M_up_up_bottom), A_up_u_bottom)
    P_um_u_right = -np.dot(np.linalg.inv(M_um_um_right), A_um_u_right)
    P_um_u_top = -np.dot(np.linalg.inv(M_um_um_top), A_um_u_top)

    # Components of the resulting matrix  
    # SCALING: Resulting matrix is made by concatenating P_q matrices and P_u matrices in the following order
    P_left = np.concatenate((P_qp_u_left, P_up_u_left), axis=0)
    P_bottom = np.concatenate((P_qp_u_bottom, P_up_u_bottom), axis=0)
    P_right = np.concatenate((P_qm_u_right, P_um_u_right), axis=0)
    P_top = np.concatenate((P_qm_u_top, P_um_u_top), axis=0)

    # SCALING: As a result, in the output matrix first two rows should be scaled by 1/h, following 2 rows -- by 1, following two rows -- by 1/h, etc. 
    return np.concatenate((P_left, P_bottom, P_right, P_top), axis=0)
  
    # SCALING: Consider our case with 16x4 matrix:
    # [[ 1. -1. -0. -0.]  -- scale by 1/h
    #  [-0. -0.  1. -1.]  -- scale by 1/h
    #  [-1. -0. -0. -0.]  -- scale by 1
    #  [-0. -0. -1. -0.]  -- scale by 1
    #  [ 1. -0. -1. -0.]  -- scale by 1/h
    #  [-0.  1. -0. -1.]  -- scale by 1/h
    #  [-1. -0. -0. -0.]  -- scale by 1
    #  [-0. -1. -0. -0.]  -- scale by 1
    #  [-1.  1. -0. -0.]  -- scale by 1/h
    #  [-0. -0. -1.  1.]  -- scale by 1/h
    #  [-0.  1. -0. -0.]  -- scale by 1
    #  [-0. -0. -0.  1.]  -- scale by 1
    #  [-1. -0.  1. -0.]  -- scale by 1/h
    #  [-0. -1. -0.  1.]  -- scale by 1/h
    #  [-0. -0.  1. -0.]  -- scale by 1
    #  [-0. -0. -0.  1.]] -- scale by 1


  def get_averaging_riemann_solver(self):
    """!
    
    Simple averaging Riemann solver
    
    We know that this solver is extremely diffusive, but it makes sense to 
    provide this one as a simple debugging mechanism. In line with our
    @ref page_multigrid_numerics_Disontinuous_Galerkin "DG description", it
    is clear that the dimension of the result matrix is @f$ dof \times 2dof @f$
    where dof is the number of degrees of freedom projected onto a face.
    
    Please note that unknowns_per_face_node=@f$ 3 \cdot dof @f$ in the equation
    above.
    
    """
    dim = self.face_nodes * self.unknowns_per_face_node * 2 # * 2 at the end since we are glueing together 2 faces
    return np.eye(dim,dim)

  def get_face_face_riemann_problem_matrix(self):
    # modified averaging matrix. set a = b = 0 for original matrix
    a = 1/2  * 0
    b = 1/16 * 0
    return np.array([0.5,  0,  b,  0,0.5,  0, -b,  0,
                       0,0.5,  0,  b,  0,0.5,  0, -b,
                       a,  0,0.5,  0, -a,  0,0.5,  0,
                       0,  a,  0,0.5,  0, -a,  0,0.5]).reshape(4,8)
  
  # def get_face_to_face_matrix(self):
  #   """ Face-face coupling block.
  #   I provide here the entire block includind identities and zeros, not only the averaging part,
  #   because later we may want to
  #   1) use more general definition of the flux,
  #   2) keep the face-face mass matrices, resulting from L-2 projections, in this block.
  #   """
  #   F_qf_qp = self.block_matrix._A_FF[(2,0)]["left"][(0,0)]
  #   F_qf_qm = self.block_matrix._A_FF[(2,1)]["left"][(0,0)]
  #   F_qf_qf = self.block_matrix._A_FF[(2,2)]["left"][(0,0)]
  #   Eye = np.eye(np.shape(F_qf_qf)[0])
  #   Zeros = np.zeros(np.shape(F_qf_qf))

  #   # Let it be like this for a while...
  #   # We can later import zero and identity blocks of the right shape form BlockMatrix 
  #   # or decide that we don't need them at all and only supply the flux-defining block (last line) 
  #   return np.concatenate((np.concatenate((Eye,     Zeros,   Zeros), axis=1),
  #                          np.concatenate((Zeros,   Eye,     Zeros), axis=1),
  #                          np.concatenate((F_qf_qp, F_qf_qm, F_qf_qf), axis=1)), axis=0)
  
  #   # dim = self.face_nodes * self.unknowns_per_face_node * 2 # * 2 at the end since we are glueing together 2 faces
  #   # return np.eye(dim,dim)


'''
New class to mimic the one above
'''
class GLMatrixFree(GaussLobatto):
  def __init__(self,
               Dimensions,
               poly_degree,
               unknowns_per_cell_node=1,
               solutions_per_face_node=2,   # number of solutions   we hold per node. Default to 2 since we have u and q
               projections_per_face_node=2, # number of projections we hold per node. Default to 2 since we have u and q
               use_unit_interval=False):
    super( GLMatrixFree, self ).__init__(Dimensions,
                                         poly_degree,
                                         unknowns_per_cell_node
                                         )
    self._solutions_per_face_node   = solutions_per_face_node
    self._projections_per_face_node = projections_per_face_node
    
  def get_A_tilde(self):
    return [self.get_cell_identity_matrix()], [0]

  def get_cell_from_face_matrix(self):
    '''
    @Alex the shape of this should be:
    unknowns_per_cell_node * ( self.poly_degree + 1 ) ** self.Dimensions rows (= 4)
    solutions_per_face_node * ( self.poly_degree + 1 ) ** (self.Dimensions-1) * 2 *Dimensions cols (= 16)
    
    solutions_per_face_node counts u and q
    multiply further by number of nodes per face
    multiply further by number of faces

    '''
    
    # SCALING: scaled by h
    A_CF_q_left   = self.block_matrix._A_CF[(0,2)]["left"][(0,1)]
    A_CF_q_bottom = self.block_matrix._A_CF[(0,2)]["bottom"][(1,0)]
    A_CF_q_right  = self.block_matrix._A_CF[(0,2)]["right"][(0,1)]
    A_CF_q_top    = self.block_matrix._A_CF[(0,2)]["top"][(1,0)]

    # SCALING:
    # !!! A_{C<-F} has penalty components of different scaling!  
    # For now, we just sum them up into one matrix with scaling (0,0)
    # Need to work out how to separate them in the solver.
    A_CF_u_left   = self.block_matrix._A_CF[(0,5)]["left"][(0,0)] # + block_matrix._A_CF[(0,5)]["left"][(0,1)]
    A_CF_u_bottom = self.block_matrix._A_CF[(0,5)]["bottom"][(0,0)] # + block_matrix._A_CF[(0,5)]["bottom"][(1,0)]
    A_CF_u_right  = self.block_matrix._A_CF[(0,5)]["right"][(0,0)] # + block_matrix._A_CF[(0,5)]["right"][(0,1)]
    A_CF_u_top    = self.block_matrix._A_CF[(0,5)]["top"][(0,0)] # + block_matrix._A_CF[(0,5)]["top"][(1,0)]

    # A_CF_left = np.concatenate((A_CF_q_left, A_CF_u_left), axis=1)
    # A_CF_bottom = np.concatenate((A_CF_q_bottom, A_CF_u_bottom), axis=1)
    # A_CF_right = np.concatenate((A_CF_q_right, A_CF_u_right), axis=1)
    # A_CF_top = np.concatenate((A_CF_q_top, A_CF_u_top), axis=1)

    # A_CF_result = np.concatenate((A_CF_left, A_CF_bottom, A_CF_right, A_CF_top), axis=1)
    
    # return A_CF_result

    # Zeros = np.zeros((self.unknowns_per_cell_node * ( self.poly_degree + 1 ) ** self.Dimensions,
                      # self._solutions_per_face_node * ( self.poly_degree + 1 ) ** (self.Dimensions-1) * 2 * self.Dimensions)) # should be 16x4 for 2D 1st order
    
    matrix_0 = np.zeros((self.unknowns_per_cell_node * ( self.poly_degree + 1 ) ** self.Dimensions,
                      self._solutions_per_face_node * ( self.poly_degree + 1 ) ** (self.Dimensions-1) * 2 * self.Dimensions))
    matrix_1 = np.zeros((self.unknowns_per_cell_node * ( self.poly_degree + 1 ) ** self.Dimensions,
                      self._solutions_per_face_node * ( self.poly_degree + 1 ) ** (self.Dimensions-1) * 2 * self.Dimensions))
    # Keep specific for debugging
    assert( self.Dimensions == 2 and self.poly_degree == 1, "Please fix hardcoded stuff here...")

    matrix_0[ :, [2,3] ] = A_CF_u_left
    matrix_0[ :, [6,7] ] = A_CF_u_bottom
    matrix_0[ :, [10,11] ] = A_CF_u_right
    matrix_0[ :, [14,15] ] = A_CF_u_top

    matrix_1[ :, [0,1] ] = A_CF_q_left
    matrix_1[ :, [4,5] ] = A_CF_q_bottom
    matrix_1[ :, [8,9] ] = A_CF_q_right
    matrix_1[ :, [12,13] ] = A_CF_q_top
    
    
    output_matrices = [ matrix_0, matrix_1 ]
    output_scalings = [ 0, 1 ]

    assert( len(output_matrices) == len(output_scalings) )

    for matrix in output_matrices:
      assert matrix.shape == output_matrices[0].shape

    return output_matrices, output_scalings
  
  # SCALING: Consider our case with 4x16 matrix:
    #
    #    by h        by h        by 1        by 1        by h        by h        by 1        by 1        by h        by h        by 1        by 1        by h        by h        by 1        by 1      
    # [[ 0.33333333  0.16666667  0.33333333  0.16666667 -0.33333333 -0.16666667  0.33333333  0.16666667  0.          0.         -0.33333333 -0.16666667  0.          0.         -0.33333333 -0.16666667]
    #  [ 0.          0.         -0.33333333 -0.16666667 -0.16666667 -0.33333333  0.16666667  0.33333333 -0.33333333 -0.16666667  0.33333333  0.16666667  0.          0.         -0.16666667 -0.33333333]
    #  [ 0.16666667  0.33333333  0.16666667  0.33333333  0.          0.         -0.33333333 -0.16666667  0.          0.         -0.16666667 -0.33333333  0.33333333  0.16666667  0.33333333  0.16666667]
    #  [ 0.          0.         -0.16666667 -0.33333333  0.          0.         -0.16666667 -0.33333333 -0.16666667 -0.33333333  0.16666667  0.33333333  0.16666667  0.33333333  0.16666667  0.33333333]]

  def get_face_from_cell_matrix(self):
    """

    We wanna project the cell solution onto the face projections vector
    We wanna include redundant rows now.

    How many projections are stored on each face?
    Well, it's 2 * self._projections_per_face_node on each node.

    So the output of this matrix should have:                                               extra factor to account for all the faces
    2 * self._projections_per_face_node * ( self.poly_degree + 1 ) ** (self.Dimensions-1) * 2 *self.Dimensions rows (= 32)
    self.unknowns_per_cell_node * ( self.poly_degree + 1 ) ** self.Dimensions                                  cols (= 4)

    """

    ### Compute the normal gradient projections
    A_qp_u_left = self.block_matrix._A_FC[(0,0)]["left"][(0,0)]
    A_qp_u_bottom = self.block_matrix._A_FC[(0,0)]["bottom"][(0,0)]
    A_qm_u_right = self.block_matrix._A_FC[(1,0)]["right"][(0,0)]
    A_qm_u_top = self.block_matrix._A_FC[(1,0)]["top"][(0,0)]

    M_qp_qp_left   = self.block_matrix._A_FF[(0,0)]["left"][(0,1)]   # These are now the same;
    M_qp_qp_bottom = self.block_matrix._A_FF[(0,0)]["bottom"][(1,0)] # keep for generality
    M_qm_qm_right = self.block_matrix._A_FF[(1,1)]["right"][(0,1)]   #
    M_qm_qm_top  = self.block_matrix._A_FF[(1,1)]["top"][(1,0)]      #

    # SCALING: All P_q matrices are scaled with power -1 (i.e. 1/h)
    P_qp_u_left = -np.dot(np.linalg.inv(M_qp_qp_left), A_qp_u_left)
    P_qp_u_bottom = -np.dot(np.linalg.inv(M_qp_qp_bottom), A_qp_u_bottom)
    P_qm_u_right = -np.dot(np.linalg.inv(M_qm_qm_right), A_qm_u_right)
    P_qm_u_top = -np.dot(np.linalg.inv(M_qm_qm_top), A_qm_u_top)

    ### Compute the solution projections
    A_up_u_left = self.block_matrix._A_FC[(3,0)]["left"][(0,1)]
    A_up_u_bottom = self.block_matrix._A_FC[(3,0)]["bottom"][(1,0)]
    A_um_u_right = self.block_matrix._A_FC[(4,0)]["right"][(0,1)]
    A_um_u_top = self.block_matrix._A_FC[(4,0)]["top"][(1,0)]

    M_up_up_left   = self.block_matrix._A_FF[(3,3)]["left"][(0,1)]   # These are now the same;
    M_up_up_bottom = self.block_matrix._A_FF[(3,3)]["bottom"][(1,0)] # keep for generality
    M_um_um_right = self.block_matrix._A_FF[(4,4)]["right"][(0,1)]   #
    M_um_um_top  = self.block_matrix._A_FF[(4,4)]["top"][(1,0)]      #

    # SCALING: All P_u matrices are scaled with power 0
    P_up_u_left = -np.dot(np.linalg.inv(M_up_up_left), A_up_u_left)
    P_up_u_bottom = -np.dot(np.linalg.inv(M_up_up_bottom), A_up_u_bottom)
    P_um_u_right = -np.dot(np.linalg.inv(M_um_um_right), A_um_u_right)
    P_um_u_top = -np.dot(np.linalg.inv(M_um_um_top), A_um_u_top)

    # Components of the resulting matrix  
    # SCALING: Resulting matrix is made by concatenating P_q matrices and P_u matrices in the following order
    # P_left = np.concatenate((P_qp_u_left, P_up_u_left), axis=0)
    # P_bottom = np.concatenate((P_qp_u_bottom, P_up_u_bottom), axis=0)
    # P_right = np.concatenate((P_qm_u_right, P_um_u_right), axis=0)
    # P_top = np.concatenate((P_qm_u_top, P_um_u_top), axis=0)

    # Add Zeros for redundant projections
    # Zeros = np.zeros_like(P_left)
    # return np.concatenate((Zeros, P_left, Zeros, P_bottom, P_right, Zeros, P_top, Zeros), axis=0)
    
    # Keep specific for debugging
    assert( self.Dimensions == 2 and self.poly_degree == 1, "Please fix hardcoded stuff here...")
    # q^\pm - gradient projections, scaled by h^-1
    matrix_0 =  np.zeros((2 * self._projections_per_face_node * ( self.poly_degree + 1 ) ** (self.Dimensions-1) * 2 *self.Dimensions,
                      self.unknowns_per_cell_node * ( self.poly_degree + 1 ) ** self.Dimensions)) # should be 32x4 for 2D 1st order
    matrix_0[ [4,5],   : ] = P_qp_u_left
    matrix_0[ [12,13], : ] = P_qp_u_bottom
    matrix_0[ [16,17], : ] = P_qm_u_right
    matrix_0[ [24,25], : ] = P_qm_u_top
    # u^\pm - solution projections, scaled by h^0
    matrix_1 =  np.zeros((2 * self._projections_per_face_node * ( self.poly_degree + 1 ) ** (self.Dimensions-1) * 2 *self.Dimensions,
                      self.unknowns_per_cell_node * ( self.poly_degree + 1 ) ** self.Dimensions)) # should be 32x4 for 2D 1st order
    matrix_1[ [6,7],   : ] = P_up_u_left
    matrix_1[ [14,15], : ] = P_up_u_bottom
    matrix_1[ [18,19], : ] = P_um_u_right
    matrix_1[ [26,27], : ] = P_um_u_top


    output_matrices = [ matrix_0, matrix_1 ]
    output_scalings = [ -1, 0 ]

    assert( len(output_matrices) == len(output_scalings) )

    for matrix in output_matrices:
      assert matrix.shape == output_matrices[0].shape

    return output_matrices, output_scalings
  
    # SCALING: Consider our case with 32x4 matrix:
    # [[ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 1. -1. -0. -0.]  -- scale by 1/h
    #  [-0. -0.  1. -1.]  -- scale by 1/h
    #  [-1. -0. -0. -0.]  -- scale by 1
    #  [-0. -0. -1. -0.]  -- scale by 1
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 1. -0. -1. -0.]  -- scale by 1/h
    #  [-0.  1. -0. -1.]  -- scale by 1/h
    #  [-1. -0. -0. -0.]  -- scale by 1
    #  [-0. -1. -0. -0.]  -- scale by 1
    #  [-1.  1. -0. -0.]  -- scale by 1/h
    #  [-0. -0. -1.  1.]  -- scale by 1/h
    #  [-0.  1. -0. -0.]  -- scale by 1
    #  [-0. -0. -0.  1.]  -- scale by 1
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [-1. -0.  1. -0.]  -- scale by 1/h
    #  [-0. -1. -0.  1.]  -- scale by 1/h
    #  [-0. -0.  1. -0.]  -- scale by 1
    #  [-0. -0. -0.  1.]  -- scale by 1
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant
    #  [ 0.  0.  0.  0.]  -- redundant

  def get_boundary_matrix(self):
    '''
    @Alex shape of this should be 
    solutions_per_face_node * ( self.poly_degree + 1 ) ** (self.Dimensions-1)   rows (= 4)
    projections_per_face_node * ( self.poly_degree + 1 ) ** (self.Dimensions-1) cols (= 4)

    we are fixing the value of the solution vector on the boundary with the value of 
    the projections here. you can assume everything was set to 0 initially

    @Sean: check the shape
    '''
    return np.eye( 
      # ( 
      self._solutions_per_face_node * ( self.poly_degree + 1 ) ** (self.Dimensions-1)
      # ,
      # self._projections_per_face_node * ( self.poly_degree + 1 ) ** (self.Dimensions-1))
     , dtype=int )
  
  def get_cell_system_matrix_for_laplacian(self):
    """!
    Return the superclass's matrix, alongside a array of length
    1 for scale factor. Hardcoding the number
    """
    return [super(GLMatrixFree,self).get_cell_system_matrix_for_laplacian()], [0]

  def get_cell_mass_matrix(self):
    """!
    Return the superclass's matrix, alongside a array of length
    1 for scale factor. Hardcoding the number
    """
    return [super(GLMatrixFree,self).get_cell_mass_matrix()], [2]
