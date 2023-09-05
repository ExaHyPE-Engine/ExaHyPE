'''

here we try and generate as much of the matrices as 
we can for mixed poisson.

most of the motivation is in my notebook

note that in general we will want different bases
for u, v, p , \lambda. but for now we use the same
one. in particular, we use 1d basis which we promote 
to 2d by taking product at evaluation time

TODO:
  - give separate bases for each of the 4 quantities we 
    track here

QUESTIONS:
  - should we just hardcode the number of dofs per face to be 1 less than dofs per cell?
    seeing as this is just one component of the derivative projected onto the face?
'''

import numpy as np
from numpy.polynomial.legendre  import leggauss
from scipy.interpolate import lagrange
import peano4, petsc
from petsc.matrixgenerators import MatrixGeneratorBase


class MatrixGenerator(MatrixGeneratorBase):
  '''
  use this class to pre-generate matrices to be used 
  in Peano as far as possible. 
  '''
  def __init__(self,
               dimensions,
               poly_degree,
               unknowns_per_cell_dof,
               unknowns_per_face_dof):

    super( MatrixGenerator, self ).__init__(dimensions, 
                                            poly_degree,
                                            unknowns_per_cell_dof,
                                            unknowns_per_face_dof)

  def get_U_U_coupling(self):
    '''
    this needs scaling by h**2. 
    this is for u - v coupling.
    in general, we will have different bases
    for these parts. for now, they are the 
    same.
    '''
    output = np.zeros((self.celldofs, self.celldofs))
    for i in range(output.shape[0]):
      for j in range(output.shape[1]):
        output[i,j] += self.evaluateIntegral2d(functions2d=[ self.getPoly2d(i), self.getPoly2d(j) ])
    return output
  
  def get_UV_P_coupling(self, dim):
    '''
    pass in a dimension argument - note that we are 
    producing 2.1 to denote the x deriv <- this is for u
    and       2.2 to denote the y deriv <- this is for v
    SEE NOTES!

    needs scaling by 0.5 * h
    '''
    output = np.zeros((self.celldofs, self.celldofs))
    for i in range(output.shape[0]):
      for j in range(output.shape[1]):
        output[i,j] += self.evaluateIntegral2d(functions2d=[ self.getDeriv2d(i, dim), self.getPoly2d(j) ])
    return output

  def getMatrixPart3(self, faceNo, dim):
    '''
    first face integral.
    this is for interior faces only
    '''
    assert dim in [0,1]
    output = np.zeros(( self.celldofs, self.facedofs ))
    for i in range(output.shape[0]):
      for j in range(output.shape[1]):
        output[i,j] = self.evaluateIntegral1d(faceNo=faceNo, 
                                              functions2d=[self.getPoly2d(i)],
                                              functions1d=[self.getPoly1d(j)])
    #get face normal
    normal = self.faceNormals[faceNo]
    #get the component corresponding to the dim we fed in
    comp   = normal[dim]
    return output * comp

  def getMatrixPart4(self, faceNo, dim):
    #unsure how this should differ.
    #for now, reuse the other part
    return self.getMatrixPart3(faceNo, dim)
  
  def get_P_UV_coupling_part5(self, dim):
    '''
    dim to take \psi derivative in
    needs scaling by 0.5 * h
    '''
    output = np.zeros((self.celldofs, self.celldofs))
    for i in range(output.shape[0]):
      for j in range(output.shape[1]):
        output[i,j] += self.evaluateIntegral2d(functions2d=[ self.getDeriv2d(i, dim), self.getPoly2d(j) ])
    return output

  def get_P_UV_coupling_part6(self, faceNo, dim):
    '''
    boundary faces
    '''
    output = np.zeros((self.celldofs, self.celldofs))
    for i in range(output.shape[0]):
      for j in range(output.shape[1]):
        output[i,j] += self.evaluateIntegral1d(faceNo=faceNo, functions2d=[ self.getPoly2d(i), self.getPoly2d(j) ])
    #get face normal
    normal = self.faceNormals[faceNo]
    #get the component corresponding to the dim we fed in
    comp   = normal[dim]
    return output * comp

  def getMatrixPart7(self, faceNo):
    '''
    boundary faces
    '''
    output = np.zeros((self.celldofs, self.celldofs))
    for i in range(output.shape[0]):
      for j in range(output.shape[1]):
        output[i,j] += self.evaluateIntegral1d(faceNo=faceNo, functions2d=[ self.getPoly2d(i), self.getPoly2d(j) ])
    return output

  def getCellCellSystemMatrix(self):
    '''
    here we amalgamate the coupling for
    u-p, v-p etc
    ie all of parts 2, 5,

    this needs scaling by 0.5 h
    '''
    #alias for self.unknowns_per_cell_dof
    upcd = self.unknowns_per_cell_dof

    output = np.zeros((upcd*self.celldofs, upcd*self.celldofs)) # 3 * celldofs since we have u,v,p at each point

    # u-p coupling
    #take the x deriv for this one
    temp = self.get_UV_P_coupling(0)
    for i in range(temp.shape[0]):
      for j in range(temp.shape[1]):
        #u on the rows, and p on the columns
        output[upcd*i, upcd*j + 2] = temp[i,j]
    
    # v-p coupling
    temp = self.get_UV_P_coupling(1)
    for i in range(temp.shape[0]):
      for j in range(temp.shape[1]):
        #v on the rows, and p on the columns
        output[upcd*i + 1, upcd*j + 2] = temp[i,j]

    #p-u coupling
    temp = self.get_P_UV_coupling_part5(0) #ignore part 6 for now as this sits only on boundary faces
    for i in range(temp.shape[0]):
      for j in range(temp.shape[1]):
        #p on the rows, u on the cols
        output[upcd*i+2, upcd*j] = temp[i,j]
    
    #p-v coupling
    temp = self.get_P_UV_coupling_part5(1)
    for i in range(temp.shape[0]):
      for j in range(temp.shape[1]):
        #p on the rows, v on the cols
        output[upcd*i+2, upcd*j + 1] = temp[i,j]
    
    return output
  
  def getCellCellMassMatrix(self):
    '''
    this contains part 1
    needs scaling by h ** 2
    '''
    #alias for self.unknowns_per_cell_dof
    upcd = self.unknowns_per_cell_dof

    output = np.zeros((upcd*self.celldofs, upcd*self.celldofs)) # 3 * celldofs since we have u,v,p at each point

    #first, the u-u coupling
    temp = self.get_U_U_coupling()
    for i in range(temp.shape[0]):
      for j in range(temp.shape[1]):
        output[upcd*i, upcd*i] = temp[i,j]
    
    #repeat for v-v
    #haven't changed this yet - for now these are identical
    temp = self.get_U_U_coupling()
    for i in range(temp.shape[0]):
      for j in range(temp.shape[1]):
        output[upcd*i + 1, upcd*i + 1] = temp[i,j]
    
    return output

  def getCellToFaceMatrix(self):
    #alias for self.unknowns_per_face_dof
    upfd = self.unknowns_per_face_dof
    #                             2d for the number of faces
    rowDim = self.facedofs * upfd * ( 2 * self.dimensions )
    colDim = self.celldofs * self.unknowns_per_cell_dof
    return np.zeros((rowDim,colDim))

  def getFaceFaceMatrix(self):
    '''
    for now just return empty matrix of correct size

    probably have to expand this to include all the faces, since different
    enumeration in each dimension
    '''
    #alias for self.unknowns_per_face_dof
    upfd = self.unknowns_per_face_dof

    #                          * 2 since we bind two faces here
    dim = self.facedofs * upfd * 2
    return np.zeros((dim,dim))


def prod(iterable):
  prod = 1
  for it in iterable:
    prod *= it
  return prod


if __name__ == "__main__":
  m = MatrixGenerator(2, 1, 3, 2)