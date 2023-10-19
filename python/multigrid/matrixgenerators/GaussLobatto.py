# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import numpy as np
from   numpy.polynomial.legendre  import leggauss
from   scipy.interpolate import lagrange

import peano4

from .MatrixGenerator import MatrixGenerator
from .BlockMatrix.block_matrix_dgpoisson2d import DGPoisson2dBlockMatrix

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
    self.block_matrix = DGPoisson2dBlockMatrix(r_press, 1) # need to remove the second arg; only one cell space now
    pass

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
    output = np.eye(dim,dim) * 2**(-self.Dimensions)
    return output
      
  def get_cell_mass_matrix(self):
    '''!
    
    Create a cell-cell mass matrix
    
    This matrix does not couple the individual unknowns per degree of freedom. 
    The resulting matrix has to be scaled by @f$ h^d @f$.

    '''
    dim    = self.cell_nodes * self.unknowns_per_cell_node
    output = np.zeros((dim,dim))
    for i in range( self.cell_nodes ):
      for j in range( self.cell_nodes ):
        for k in range( self.unknowns_per_cell_node ):
          output[self.unknowns_per_cell_node*i+k, self.unknowns_per_cell_node*j+k] += self.eval_integral(functions=[ self.get_polynomial(i), self.get_polynomial(j) ]) 
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

  def get_cell_to_face_matrix(self):
    faceDim = self.face_nodes * self.unknowns_per_face_node * 2 * self.Dimensions * 2 # * 2 at the end since we are glueing together 2 faces
    cellDim = self.cell_nodes * self.unknowns_per_cell_node
    return np.zeros((faceDim,cellDim))
  
  def get_face_to_face_matrix(self):
    dim = self.face_nodes * self.unknowns_per_face_node * 2 # * 2 at the end since we are glueing together 2 faces
    return np.eye(dim,dim)

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
    return np.array([0.5, 0.0, 0.5, 0.0, 0.0, 0.5, 0.0, 0.5]).reshape((4,2))
