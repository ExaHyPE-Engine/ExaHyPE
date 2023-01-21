# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org


from abc import abstractmethod



    
def render_tensor_1(tensor,entry_seperator=",",vector_seperator=("{","}")):
  """
  Converts nested list or numpy matrix to nested list of strings.
  :param tensor: list or numpy vector storing  mpmath numbers
  """
  result = "{} {}".format( vector_seperator[0], tensor[0] )
  for elem in tensor[1:]:
    result += entry_seperator
    result += str(elem)
  result += vector_seperator[1]
  return result


def render_tensor_2(tensor,use_multidimensional_arrays,entry_seperator=",",vector_seperator=("{","}")):
  """
  Converts nested list or numpy matrix to nested list of strings.
  :param tensor: nested list or numpy matrix storing  mpmath numbers
  """
  def render_row(row):
    row_result = ""
    if use_multidimensional_arrays:
      row_result += vector_seperator[0]
    row_result += str(row[0])
    for elem in row[1:]:
      row_result += entry_seperator
      row_result += str(elem)
    if use_multidimensional_arrays:
      row_result += vector_seperator[1]
    return row_result
      
  
  result = vector_seperator[0]
  result += render_row(tensor[0])
  for row in tensor[1:]:
    result += entry_seperator
    result += render_row(row)
  result += vector_seperator[1]
  return result


def render_tensor_3(tensor,use_multidimensional_arrays,entry_seperator=",",vector_seperator=("{","}")):
  """
  Converts nested list or numpy matrix to nested list of strings.
  :param tensor: nested list or numpy matrix storing  mpmath numbers
  """
  def render_row(row):
    row_result = ""
    if use_multidimensional_arrays:
      row_result += vector_seperator[0]
    row_result += str(row[0])
    for elem in row[1:]:
      row_result += entry_seperator
      row_result += str(elem)
    if use_multidimensional_arrays:
      row_result += vector_seperator[1]
    return row_result
      
  
  def render_block(block):
    row_result = ""
    if use_multidimensional_arrays:
      row_result += vector_seperator[0]
    row_result += render_row(block[0])
    for elem in block[1:]:
      row_result += entry_seperator
      row_result += render_row(elem)
    if use_multidimensional_arrays:
      row_result += vector_seperator[1]
    return row_result

  result = vector_seperator[0]
  result += render_block(tensor[0])
  for row in tensor[1:]:
    result += entry_seperator
    result += render_block(row)
  result += vector_seperator[1]
  return result



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
