import numpy as np

def StencilToElement(stencil, dimensions):
  if   dimensions == 2:
    return convertStencilToElementMatrix2d(stencil)
  elif dimensions == 3:
    return convertStencilToElementMatrix3d(stencil)
  else:
    raise NotImplementedError

def convertStencilToElementMatrix3d(stencil):
  raise NotImplementedError

def convertStencilToElementMatrix2d(stencil):
  #should be an np matrix. check size before proceeding
  assert stencil.shape == (3,3), "Wrong shape for this dimensions!"

  Dimensions = 2
  TwoPowerD  = 2 ** 2
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
