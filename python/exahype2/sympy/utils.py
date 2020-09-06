# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org


import sympy



def dot(a,b):
  result = 0
  for i in range(0,len(a)):
    result += a[i]*b[i]
  return result
  


def outer(a,b):
  result = sympy.symarray("result", (len(b),len(a)))
  for row in range(0,len(b)):
    for col in range(0,len(a)):
      result[row,col] = b[row] * a[col]
  return result
    
  