import numpy as np

def StencilToElement(stencil, dimensions):
  if   dimensions == 2:
    return convertStencilToElementMatrix2d(stencil)
  elif dimensions == 3:
    return convertStencilToElementMatrix3d(stencil)
  else:
    raise NotImplementedError

def convertStencilToElementMatrix3d(stencil):
  """!
  We return a square matrix
  with 2**Dimensions rows. 

  Much of this code is identical to 2d version. We could 
  even reuse this for the 2d version with clever indexing,
  but it the code is much more readable this way.
  """
  # assert stencil.shape == (3,3,3)

  Dimensions = 3
  TwoPowerD  = 2 ** Dimensions
  result     = np.zeros((TwoPowerD, TwoPowerD))

  AAi = 0
  BBi = 0
  AAj = 0
  BBj = 0
  CCi = 0
  CCj = 0

  #replicating for dfor2(i) in 3d
  for iScalar in range(TwoPowerD):
    i    = [0 for _ in range(Dimensions)]
    i[0] = AAi
    i[1] = BBi
    i[2] = CCi
    AAi  = int(not AAi)
    BBi  = int( not( AAi ^ BBi ) )
    CCi  = int( CCi or ( not(AAi) and not(BBi) and not(CCi) ) )    
    
    for jScalar in range(TwoPowerD):
      j    = [0 for _ in range(Dimensions)]
      j[0] = AAj
      j[1] = BBj
      j[2] = CCj
      AAj  = int(not AAj)
      BBj  = int( not( AAj ^ BBj ) )
      CCj  = int( CCi or ( not(AAj) and not(BBj) and not(CCj) ) )

      stencilEntry = [0 for _ in range(Dimensions)]
      commonFacesPower2 = 1.0

      for d in range(Dimensions):
        stencilEntry[d] = i[d] - j[d] + 1
        if i[d] == j[d]:
          commonFacesPower2 *= 2.0
      result[iScalar, jScalar] = stencil.flatten()[ dLinearised(stencilEntry,3, Dimensions) ] / commonFacesPower2
  return result

def convertStencilToElementMatrix2d(stencil):
  """!
  This is borrowed from src/toolbox/ElementMatrix.cpp.
  In particular toolbox::finiteelements::getElementWiseAssemblyMatrix()
  """
  #should be an np matrix. check size before proceeding
  assert stencil.shape == (3,3), "Wrong shape for this dimensions!"

  Dimensions = 2
  TwoPowerD  = 2 ** Dimensions
  result     = np.zeros((TwoPowerD, TwoPowerD))

  AAi = 0
  BBi = 0
  AAj = 0
  BBj = 0

  #replicating dfor2(i)
  for iScalar in range(TwoPowerD):
    i    = [0 for _ in range(Dimensions)]
    i[0] = AAi
    i[1] = BBi
    AAi  = int(not AAi)
    BBi  = int( not( AAi ^ BBi ) )

    #replicating dfor2(j)
    for jScalar in range(TwoPowerD):
      j    = [0 for _ in range(Dimensions)]
      j[0] = AAj
      j[1] = BBj
      AAj  = int(not AAj)
      BBj  = int( not( AAj ^ BBj ) )

      stencilEntry = [0 for _ in range(Dimensions)]
      commonFacesPower2 = 1.0

      for d in range(Dimensions):
        stencilEntry[d] = i[d] - j[d] + 1
        if i[d] == j[d]:
          commonFacesPower2 *= 2.0
      result[iScalar, jScalar] = stencil.flatten()[ dLinearised(stencilEntry,3, Dimensions) ] / commonFacesPower2
  return result

def dLinearised(vector, max, dimensions):
  result = vector[dimensions - 1]
  for d in range(dimensions-2, -1, -1): #Dimensions-2 -> 0 inclusive
    result = result * max + vector[d]
  return result
