import numpy as np
from   numpy.polynomial.legendre  import leggauss
from   scipy.interpolate import lagrange
import peano4, petsc
from   petsc.matrixgenerators import MatrixGeneratorBase


class DgGenerator(MatrixGeneratorBase):
  '''
  use this class to pre-generate matrices to be used 
  in Peano as far as possible. 
  '''
  def __init__(self,
               dimensions,
               poly_degree,
               unknowns_per_cell_dof):
    
    #set unknowns per face dof to be the same for now

    super( DgGenerator, self ).__init__(dimensions, 
                                        poly_degree,
                                        unknowns_per_cell_dof,
                                        unknowns_per_cell_dof)
    
  def getCellCellMassMatrix(self):
    '''
    needs to be scaled by h**2
    '''
    dim    = self.celldofs * self.unknowns_per_cell_dof
    output = np.eye(dim,dim)
    return output
    # for i in range( self.celldofs ):
    #   for j in range( self.celldofs ):
    #     output[3*i+1, 3*j+1] += self.evaluateIntegral2d(functions2d=[ self.getPoly2d(i), self.getPoly2d(j) ]) # phi_x-phi_x coupling 
    #     output[3*i+2, 3*j+2] += self.evaluateIntegral2d(functions2d=[ self.getPoly2d(i), self.getPoly2d(j) ]) # phi_y-phi_y coupling
    # return output

  def getCellCellSystemMatrix(self):
    '''
    needs to be scaled 0.5*h, that is why we use factor=2.0 as evaluateIntegral2d() has coeff. 0.25 as default,
    so normalise by h
    '''
    dim    = self.celldofs * self.unknowns_per_cell_dof
    output = np.eye(dim,dim)
    return output
    # for i in range( self.celldofs ):
    #   for j in range( self.celldofs ):
    #     # They are all the same, but they don't have to be!
    #     # We should have different getDeriv2d, getPoly2d for different fields: u, phi_x, phi_y
    #     output[3*i+1, 3*j] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,0), self.getPoly2d(j) ]) # phi_x-u coupling
    #     output[3*i+2, 3*j] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,1), self.getPoly2d(j) ]) # phi_y-u coupling
    #     output[3*i, 3*j+1] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,0), self.getPoly2d(j) ]) # u-phi_x coupling
    #     output[3*i, 3*j+2] += self.evaluateIntegral2d(factor=2.0, functions2d=[ self.getDeriv2d(i,1), self.getPoly2d(j) ]) # u-phi_y coupling
    # return output

  def getCellToFaceMatrix(self):
    faceDim = self.facedofs * self.unknowns_per_face_dof * 2 * self.dimensions * 2 # * 2 at the end since we are glueing together 2 faces
    cellDim = self.celldofs * self.unknowns_per_cell_dof
    return np.eye(faceDim,cellDim)
  
  def getFaceFaceMatrix(self):
    dim = self.facedofs * self.unknowns_per_face_dof * 2 # * 2 at the end since we are glueing together 2 faces
    return np.eye(dim,dim)
  