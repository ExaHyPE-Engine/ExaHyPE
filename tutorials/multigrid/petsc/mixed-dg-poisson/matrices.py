import numpy as np
from   numpy.polynomial.legendre  import leggauss
from   scipy.interpolate import lagrange
import peano4, petsc
from   petsc.matrixgenerators import MatrixGeneratorBase


class MixedGenerator(MatrixGeneratorBase):
  '''
  use this class to pre-generate matrices to be used 
  in Peano as far as possible. 
  '''
  def __init__(self,
               dimensions,
               poly_degree,
               unknowns_per_cell_dof,
               unknowns_per_face_dof):

    super( MixedGenerator, self ).__init__(dimensions, 
                                           poly_degree,
                                           unknowns_per_cell_dof,
                                           unknowns_per_face_dof)
    
  def getCellCellMassMatrix(self):
    '''
    needs to be scaled by h**2

    we make 3 different 4*4 matrices and spread them out amongst the 12*12 matrix
    '''
    dim    = self.celldofs * self.unknowns_per_cell_dof
    output = np.zeros((dim,dim))
    for i in range( self.celldofs ):
      for j in range( self.celldofs ):
        output[3*i,3*j] += self.evaluateIntegral2d(functions2d=[ self.getPoly2d(i), self.getPoly2d(j) ])
    return output

  def getCellCellSystemMatrix(self):
    '''
    needs to be scaled 0.5*h, but we include 0.5 factor
    already in evaluateIntegral.
    so normalise by h
    '''
    dim    = self.celldofs * self.unknowns_per_cell_dof
    output = np.zeros((dim,dim))

    return output

  def getCellToFaceMatrix(self):
    faceDim = self.facedofs * self.unknowns_per_face_dof * 2 * self.dimensions * 2 # * 2 at the end since we are glueing together 2 faces
    cellDim = self.celldofs * self.unknowns_per_cell_dof
    return np.zeros((faceDim,cellDim))
  
  def getFaceFaceMatrix(self):
    dim = self.facedofs * self.unknowns_per_face_dof * 2 # * 2 at the end since we are glueing together 2 faces
    return np.zeros((dim,dim))
  