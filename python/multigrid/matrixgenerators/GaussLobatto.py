# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import numpy as np
from   numpy.polynomial.legendre  import leggauss
from   scipy.interpolate import lagrange

import peano4

from   .DGMatrixGenerator import DGMatrixGenerator



class GaussLobatto(DGMatrixGenerator):
  '''!
  
  Simple matrix generator assuming that we use a Gauss Lobatto unknown layout
  
  If you use this class, you assuem that a cell hosts @f$ (p+1)^d @f$
  nodes carrying Legendre polynomials. As you use a Gauss Lobatto layout,
  the "outer" nodes will end up on the cell faces. Each node can hold a
  fixed number of unknowns. If you multiply the number of nodes per cell
  with the corresponding number of unknowns, you get the degrees of freedom
  within a global assembly matrix which correspond to volumetric data. 
  
  '''
  def __init__(self,
               dimensions,
               polynomial_degree,
               unknowns_per_cell_node,
               unknowns_per_face_node):

    super( GaussLobatto, self ).__init__(dimensions, 
                                         )
    self.polynomial_degree = polynomial_degree
    self.unknowns_per_cell_node = unknowns_per_cell_node
    self.unknowns_per_face_node = unknowns_per_face_node

    self.cell_nodes    = ( self.polynomial_degree + 1 ) ** (self.dimensions)
    self.face_nodes    = ( self.polynomial_degree + 1 ) ** (self.dimensions - 1)

    self.polynomials = self.set_polynomials()
    self.points,\
    self.weights     = leggauss( self.polynomial_degree + 1 )
    self.weights2d   = [ w1*w2  for w1 in self.weights for w2 in self.weights]
    self.points2d    = [(p1,p2) for p1 in self.points  for p2 in self.points ]
    if dimensions==2:
      self.faceNormals = [ [-1,0], [0,-1], [1,0], [0,1] ]
    else:
      assert False, "not yet implemented"
    
  def getCellIdentityMatrix(self):
    dim    = self.cell_nodes * self.unknowns_per_cell_node
    output = np.zeros((dim,dim))
    for i in range(dim):
      output[i,i] = 2**(-self.dimensions)
    return output

      
  def getCellCellMassMatrix(self):
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
          output[self.unknowns_per_cell_node*i+k, self.unknowns_per_cell_node*j+k] += self.evaluateIntegral2d(functions2d=[ self.getPoly2d(i), self.getPoly2d(j) ]) # phi_x-phi_x coupling 
    return output

  def getCellCellSystemMatrixForLaplacian(self):
    '''!

    Create a cell-cell mass matrix
    
    
    needs to be scaled 0.5*h, that is why we use factor=2.0 as evaluateIntegral2d() has coeff. 0.25 as default,
    so normalise by h
    '''
    dim    = self.cell_nodes * self.unknowns_per_cell_node
    output = np.zeros((dim,dim))
    for i in range( self.cell_nodes ):
      for j in range( self.cell_nodes ):
        for k in range( self.unknowns_per_cell_node ):
          pass
          # They are all the same, but they don't have to be!
          # We should have different getDeriv2d, getPoly2d for different fields: u, phi_x, phi_y
          #output[3*i+1, 3*j] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,0), self.getPoly2d(j) ]) # phi_x-u coupling
          #output[3*i+2, 3*j] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,1), self.getPoly2d(j) ]) # phi_y-u coupling
          #output[3*i, 3*j+1] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,0), self.getPoly2d(j) ]) # u-phi_x coupling
          #output[3*i, 3*j+2] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,1), self.getPoly2d(j) ]) # u-phi_y coupling
    return output

  def getCellToFaceMatrix(self):
    faceDim = self.face_nodes * self.unknowns_per_face_node * 2 * self.dimensions * 2 # * 2 at the end since we are glueing together 2 faces
    cellDim = self.cell_nodes * self.unknowns_per_cell_node
    return np.zeros((faceDim,cellDim))
  
  def getAveragingRiemannSolver(self):
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

  def set_polynomials(self):
    polys = []
    x = np.linspace(-1,1, self.polynomial_degree + 1)
    for j in range(self.polynomial_degree + 1):
      y = np.zeros_like(x)
      y[j] = 1.0
      polys.append( lagrange(x,y) )
    return polys

  def convertIndexTo2d(self, index):
    x = index %  ( self.polynomial_degree + 1 )
    y = index // ( self.polynomial_degree + 1 )
    return [x,y]

  def getPoly1d(self,index):
    '''
    returns the 1d poly at the given index.
    you should treat the returned object like a function!
    '''
    assert 0 <= index <= self.polynomial_degree
    return self.polynomials[index]
 
  def getPoly2d(self,index):
    '''
    returns the 2d poly a the given index
    '''
    assert 0 <= index <= self.cell_nodes

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
  