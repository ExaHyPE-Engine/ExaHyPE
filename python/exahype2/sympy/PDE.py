# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import sympy


class PDE(object):
  def __init__(self, unknowns, auxiliary_variables, dimensions ):
    self.unknowns              = unknowns
    self.auxiliary_variables   = auxiliary_variables
    self.dimensions            = dimensions
    
    self.Q              = sympy.symarray("Q",               (unknowns+auxiliary_variables))
  
    self.initial_values = sympy.symarray("Q0",              (unknowns+auxiliary_variables))
    self.x              = sympy.symarray("x",               (dimensions))
    pass
  
  
  def _implementation_of_mapping_onto_named_quantities(self, is_cell_mapping = True):
    """
    
      Return the C code that maps the quantities from Q onto
      properly labelled quantities
      
    """
    result = ""
    for i in range(0,self.unknowns+self.auxiliary_variables):
      result += "const "
      result += "double "
      result += sympy.printing.cxxcode( self.Q[i] )
      result += " = Q[" + str(i) + "];\n"
      
    for i in range(0,self.dimensions):
      result += "const double x_"
      result += str(i)
      result += " = "
      if is_cell_mapping:
        result += "volumeCentre"
      else:
        result += "faceCentre"
      result += "(" + str(i) + ");\n"
      result += ";\n"
    return result




  def name_Q_entry(self,offset_in_Q,name):
    """
     Q covers both unknowns plus auxiliary variables. They simply are concatenated.
     So if you have 10 unknowns and 2 auxiliary variables, the name you assign the
     10s Q entry is the one for the first auxiliary variable. 
     
     There's also a dedicated routine for auxiliary variables if you prefer this one.
    """
    self.Q[offset_in_Q] = sympy.symbols( name )
    return self.Q[offset_in_Q]


  def name_auxiliary_variable(self,number,name):
    """
     Q covers both unknowns plus auxiliary variables. They simply are concatenated.
     So if you have 10 unknowns and 2 auxiliary variables, the name you assign the
     10s Q entry is the one for the first auxiliary variable. 
     
     There's also a dedicated routine for auxiliary variables if you prefer this one.
    """
    self.Q[number+self.auxiliary_variables] = sympy.symbols( name )
    return self.Q[number+self.auxiliary_variables]


  def name_Q_entries(self,offset_in_Q,cardinality,name):
    new_entry = sympy.symarray(name, cardinality)
    for i in range(0,cardinality):
      self.Q[offset_in_Q+i] = new_entry[i]
    return new_entry
  
  
  
  def implementation_of_homogeneous_Neumann_BC(self):
    result = ""
    for i in range(0,self.unknowns + self.auxiliary_variables):
      result += "Qoutside[" + str(i) + "] = Qinside[" + str(i) + "];\n"
    return result
  
  
  def implementation_of_initial_conditions(self, invoke_evalf_before_output = True):
    """
      invoke_evalf_before_output: boolean
        If your expression is a symbolic expression (default) then we use evalf 
        before we pipe it into the output. If your expression is something numeric,
        then evalf will fail (as it is not defined for scalar quantities). 
    """
    result  = self.__implementation_of_mapping_onto_named_quantities(is_cell_mapping = True)
    for i in range(0,self.unknowns + self.auxiliary_variables):
      if invoke_evalf_before_output:
        result += sympy.printing.cxxcode( self.initial_values[i].evalf(), assign_to="Q[" + str(i) + "]" )
      else:
        result += "Q[" + str(i) + "] = " + sympy.printing.cxxcode( self.initial_values[i] ) + ";"
      result += "\n"
    return result  
