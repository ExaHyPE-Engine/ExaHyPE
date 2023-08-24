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
'''

import numpy as np
from numpy.polynomial.legendre  import leggauss
from scipy.interpolate import lagrange


#for now, no inheritance
class MatrixGenerator:
  '''
  use this class to pre-generate matrices to be used 
  in Peano as far as possible. 
  '''
  def __init__(self, dimensions, poly_degree):
    self.dimensions  = dimensions
    self.poly_degree = poly_degree

    self.celldofs    = ( self.poly_degree + 1 ) ** (self.dimensions)
    self.facedofs    = ( self.poly_degree + 1 ) ** (self.dimensions - 1)

    self.polynomials = self.set_polynomials()
    self.points,\
    self.weights     = leggauss( self.poly_degree + 1 )
    self.weights2d   = [ w1*w2  for w1 in self.weights for w2 in self.weights]
    self.points2d    = [(p1,p2) for p1 in self.points  for p2 in self.points ]
    #2d face normal for   0       1       2      3
    self.faceNormals = [ [-1,0], [0,-1], [1,0], [0,1] ]

  @staticmethod
  def convert1dPointTo2d(faceNo, point1d):
    '''
    dummy function to convert 1d quadpoint on a face to a
    2d quadpoint, for use in 2d basis functions
    
     _ 3 
    |   |2
   0|_ _| 
       1

    '''
    directions = [-1,-1,1,1]
    if faceNo % 2 == 0: #evens. ie 0 and 2
      return [directions[faceNo], point1d]
    else:
      return [point1d, directions[faceNo]]

  def set_polynomials(self):
    polys = []
    x = np.linspace(-1,1, self.poly_degree + 1)
    for j in range(self.poly_degree + 1):
      y = np.zeros_like(x)
      y[j] = 1.0
      polys.append( lagrange(x,y) )
    return polys

  def convertIndexTo2d(self, index):
    x = index %  ( self.poly_degree + 1 )
    y = index // ( self.poly_degree + 1 )
    return [x,y]

  def convertMatrixToInitList(self, matrix):
    output = "{ " + str(matrix[0,0])
    for el in matrix.flatten():
      output += f", {str(el)}"
    output += "}"
    return output

  def getPoly1d(self,index):
    '''
    returns the 1d poly at the given index.
    you should treat the returned object like a function!
    '''
    assert 0 <= index <= self.poly_degree
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

  def getMatrixPart1(self):
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
  
  def getMatrixPart2(self, dim):
    '''
    pass in a dimension argument - note that we are 
    producing 2.1 to denote the x deriv 
    and       2.2 to denote the y deriv 
    SEE NOTES!
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
  
  def getMatrixPart5(self, dim):
    '''
    dim to take \psi derivative in
    '''
    output = np.zeros((self.celldofs, self.celldofs))
    for i in range(output.shape[0]):
      for j in range(output.shape[1]):
        output[i,j] += self.evaluateIntegral2d(functions2d=[ self.getDeriv2d(i, dim), self.getPoly2d(j) ])
    return output

  def getMatrixPart6(self, faceNo, dim):
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

  def getAllMatrices(self):
    '''
    returns a dict with all the matrices involved
    here we do some ugly hardcoding, for now
    '''
    output = {}

    temp = self.getMatrixPart1()
    output["partOne"] = [temp.shape, self.convertMatrixToInitList(temp)]

    return output

if __name__ == "__main__":
  generator = MatrixGenerator(2, 1)