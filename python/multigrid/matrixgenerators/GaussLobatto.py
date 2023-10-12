# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import numpy as np
from   numpy.polynomial.legendre  import leggauss
from   scipy.interpolate import lagrange

import peano4

# from   .DGMatrixGenerator import DGMatrixGenerator
from   .MatrixGenerator import MatrixGenerator



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
               dimensions,
               poly_degree,
               unknowns_per_cell_node=1,
               unknowns_per_face_node=1):

    super( GaussLobatto, self ).__init__(dimensions,
                                         poly_degree
                                         )
    self.unknowns_per_cell_node = unknowns_per_cell_node
    self.unknowns_per_face_node = unknowns_per_face_node

    self.celldofs    = ( self.poly_degree + 1 ) ** (self.Dimensions)
    self.facedofs    = ( self.poly_degree + 1 ) ** (self.Dimensions - 1)

    # self.polynomials = self.set_polynomials()
    # self.points,\
    # self.weights     = leggauss( self.polynomial_degree + 1 )
    # self.weights2d   = [ w1*w2  for w1 in self.weights for w2 in self.weights]
    # self.points2d    = [(p1,p2) for p1 in self.points  for p2 in self.points ]
    
  def getPoints1d(self):
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

  def getWeights1d(self):
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

    output = []
    for p in self.getPoints1d():
      weight = firstWeight / poly(p)**2
      output.append(weight)
    return output

  def getCellIdentityMatrix(self):
    dim    = self.celldofs * self.unknowns_per_cell_node
    output = np.eye(dim,dim) * 2**(-self.dimensions)
    return output
      
  def getCellCellMassMatrix(self):
    '''!
    
    Create a cell-cell mass matrix
    
    This matrix does not couple the individual unknowns per degree of freedom. 
    The resulting matrix has to be scaled by @f$ h^d @f$.

    '''
    dim    = self.celldofs * self.unknowns_per_cell_node
    output = np.zeros((dim,dim))
    for i in range( self.celldofs ):
      for j in range( self.celldofs ):
        for k in range( self.unknowns_per_cell_node ):
          output[self.unknowns_per_cell_node*i+k, self.unknowns_per_cell_node*j+k] += self.evaluateIntegral2d(functions2d=[ self.getPoly2d(i), self.getPoly2d(j) ]) # phi_x-phi_x coupling 
    return output

  def getCellCellSystemMatrixForLaplacian(self):
    '''!

    Create a cell-cell mass matrix
    
    
    needs to be scaled 0.5*h, that is why we use factor=2.0 as evaluateIntegral2d() has coeff. 0.25 as default,
    so normalise by h
    '''
    dim    = self.celldofs * self.unknowns_per_cell_node
    output = np.zeros((dim,dim))
    for i in range( self.celldofs ):
      for j in range( self.celldofs ):
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
    faceDim = self.facedofs * self.unknowns_per_face_node * 2 * self.dimensions * 2 # * 2 at the end since we are glueing together 2 faces
    cellDim = self.celldofs * self.unknowns_per_cell_node
    return np.zeros((faceDim,cellDim))
  
  def getFaceFaceMatrix(self):
    dim = self.facedofs * self.unknowns_per_face_node * 2 # * 2 at the end since we are glueing together 2 faces
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
  