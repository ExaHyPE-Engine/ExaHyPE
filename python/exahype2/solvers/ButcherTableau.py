# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

import peano4
import jinja2


def RungeKutta_steps(order):
  if order<1:
    raise Exception( "RK order has to be 1 at least" )
  if order>4:
    raise Exception( "max RK order supported is 4" )
  return order


class ButcherTableau(object):
  def __init__(self,order):
    self._order = order
    
    
  def time_step_sizes(self):
    """
    
     These are the relative time step sizes in the scheme, i.e. the very left
     column in the classic Butcher tableau.
     
    """
    if self._order==1:
      return [0]
    elif self._order==2:
      return [0,1/2]
    elif self._order==3:
      return [0,1/2,1]
    elif self._order==4:
      return [0,1/3,2/3,1]
    else:
      raise Exception( "order {} is not yet supported".format(self._order) ) 
    
    
  def final_estimate_weights(self):
    """
    
     These are the relative time step sizes in the scheme, i.e. the very left
     column in the classic Butcher tableau.
     
    """
    if self._order==1:
      return [1]
    elif self._order==2:
      return [0,1]
    elif self._order==3:
      return [1/6,2/3,1/6]
    elif self._order==4:
      return [1/8, 3/8, 3/8, 1/8 ]
    else:
      raise Exception( "order {} is not yet supported".format(self._order) ) 
    
    
  def weight_matrix(self):
    if self._order==1:
      return [
        [0]
      ]
    if self._order==2:
      return [
        [0,0],
        [0.5,0]
      ]
    if self._order==3:
      return [
        [  0, 0, 0],
        [0.5, 0, 0],
        [ -1, 2, 0],
      ]
    if self._order==4:
#0      
#1/3   1/3
#2/3   -1/3   1
#1   1   −1   1   
#  1/8   3/8   3/8   1/8 
      return [
        [ 0,   0, 0, 0],
        [ 1/3, 0, 0, 0],
        [-1/3, 1, 0, 0],
        [ 1,  -1, 1, 0],
      ]
    else:
      raise Exception( "order {} is not yet supported".format(self._order) ) 
    
    
    
    