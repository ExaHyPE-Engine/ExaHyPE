# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org


from abc import abstractmethod


class LagrangeBasis():
  """
  
  Abstract base class for all Lagrange bases available in ExaHyPE 2
  
  This tool relies heavily on SciPy. I could have used sympy as well, but 
  eventually decided to keep it as simple as possible. So I work with 
  functors and no symbolic representation. But I could 
  
  """
  
  
  def __init__(self, polynomial_order, render_digits = 64):  
    self._polynomial_order = polynomial_order
    self._render_digits    = render_digits
    

  @property 
  def order(self):
    return self._polynomial_order


  @property 
  def dofs_per_axis(self):
    return self._polynomial_order+1


  # protected 
  @abstractmethod
  def init_dictionary_with_default_parameters(self,dictionary,use_multidimensional_arrays):
    """
    
    To be implemented by subclass.
    
      
    multidimensional_arrays: Boolean
      If it is to False, all the matrices should be flattened into one array (C 
      enumeration). If it is set, the matrices should be written down as proper
      two-dimensional arrays. Analogously, all inter-grid transfer operators 
      either are represented as array or as d-dimensional construct.
    
    """
    assert False, "To be implemented by subclass."

    
  def _render_tensor_1(self,tensor,entry_seperator=",",vector_seperator=("{","}")):
    """
    Converts nested list or numpy matrix to nested list of strings.
    :param tensor: list or numpy vector storing  mpmath numbers
    """
    result = "{} {}".format( vector_seperator[0], tensor[0] )
    for elem in tensor[1:]:
      result += ", "
      result += str(elem)
    result += vector_seperator[1]
    return result
  
  
  def _render_tensor_2(self,tensor,use_multidimensional_arrays):
    """
    Converts nested list or numpy matrix to nested list of strings.
    :param tensor: nested list or numpy matrix storing  mpmath numbers
    """
    result = []
    for row in tensor:
       resultRow = []
       for elem in row:
         resultRow.append(mp.nstr(elem,n=self._render_digits))
       if use_multidimensional_arrays:
         result.append(resultRow)
       else:
         result += resultRow
    return result
  

  def _render_tensor_3(self,tensor,use_multidimensional_arrays):
    """
    
    Converts nested list or numpy matrix to nested list of strings.
    
    :param tensor: nested list or numpy matrix storing  mpmath numbers
    
    """
    result = []
    for r1 in tensor:
       tmp1 = []
       for r2 in r1:
          tmp2 = []
          for elem in r2:
            tmp2.append(mp.nstr(elem,n=self._render_digits))
          if use_multidimensional_arrays:
            tmp1.append(tmp2)
          else:
            result += tmp2
       if use_multidimensional_arrays:
         result.append(tmp1)
    return result
