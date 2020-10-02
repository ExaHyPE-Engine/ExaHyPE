# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import sympy

from .PDE import PDE


class FirstOrderConservativePDEFormulation(PDE):
  """
    
    Helper class to model a hyperbolic PDE in first-order conservative 
    formulation.
    
    To model your PDE, you typically run through a couple of single steps. 
    First, include this package plus sympy. Next, create a new instance
    of this class to which you pass the number of equations in your PDE 
    system, i.e. the number of unknowns you want to develop, plus the 
    dimension.
    
    As a consequence, the solver holds an equation pde which describes the
    pde's right-hand side in the representation
    
    \partial Q(t) + div_x F(Q)
    
    Most people prefer not to work with a vector Q but with some symbolic
    names. Feel free to derive them from Q via constructs similar to
      
    rho = euler.Q[0]
    j = sympy.Array( euler.Q[1:4] )
    E = euler.Q[4]

        
  """
  def __init__(self, unknowns, auxiliary_variables, dimensions ):
    PDE.__init__(self, unknowns, auxiliary_variables, dimensions)
    
    self.eigenvalue     = sympy.symarray("alpha",           (unknowns,dimensions))
    self.F              = sympy.symarray("F",               (unknowns,dimensions))
    
    for i in range(0,unknowns):
      self.F[i]          = [0 for i in range(0,dimensions)]
      self.eigenvalue[i] = [0 for i in range(0,dimensions)]
    pass


  def __str__(self):
    result = ""
    for i in range(0,self.unknowns):
      result += "d_t " + str( self.Q[i] ) + "(x) + div_x( "
      result += str( self.F[i] )
      result += " ) = 0\n"
    for i in range(0,self.unknowns):
      result += "\lambda _{max," + str( i ) + "}(x) = "
      result += str( self.eigenvalue[i] )
      result += "\n"
    return result
    

  def LaTeX(self):
    result = "\\begin{eqnarray*}"
    for i in range(0,self.unknowns):
      result += "\partial _t " + str( self.Q[i] ) + "(x) + div( "
      for j in range(0,len(self.F[i])):
        if j!=0:
          result += ","
        result += str( self.F[i][j] )
      result += " ) & = & 0 \\\\"
    for i in range(0,self.unknowns):
      result += "\\lambda _{max," + str( i ) + "}(x) & = & ["
      for j in range(0,len(self.eigenvalue[i])):
        if j!=0:
          result += ","
        result += str( self.eigenvalue[i][j] )
      result += "] \\\\"
    result += "\\end{eqnarray*}"
    
    #
    # Some text replacement magic
    #
    result = result.replace( "**", "^" )
    result = result.replace( "sqrt", "\\sqrt" )
    return result  


  def substitute_expression(self,expression,new_expression):
    """
    
      Usually used to set a symbolic variable (expression) to a value 
      (new_expression). We run over all internal expressions and set 
      them accordin
      
    """
    for i in range(0,self.unknowns):
      for d in range(0,self.dimensions):
        self.F[i,d]          = self.F[i,d].subs(expression,new_expression)
        self.eigenvalue[i,d] = self.eigenvalue[i,d].subs(expression,new_expression)


  def implementation_of_flux(self, invoke_evalf_before_output = False):
    """
      Return implementation for flux along one coordinate axis (d) as C code.
        
      invoke_evalf_before_output: boolean
        If your expression is a symbolic expression (default) then we use evalf 
        before we pipe it into the output. If your expression is something numeric,
        then evalf will fail (as it is not defined for scalar quantities). 
    """
    result  = self._implementation_of_mapping_onto_named_quantities(is_cell_mapping = False)
    result += "switch( normal ) {\n"
    for d in range(0,self.dimensions):
      result += "  case " + str(d) + ":\n"
      for i in range(0,self.unknowns):
        if invoke_evalf_before_output:
          result += sympy.printing.cxxcode( self.F[i,d].evalf(), assign_to="F[" + str(i) + "]" )
        else:
          result += "F[" + str(i) + "] = " + sympy.printing.cxxcode( self.F[i,d] ) + ";"
        result += "\n"
      result += "  break;\n"
    result += "}\n"
    return result


  def implementation_of_eigenvalues(self, invoke_evalf_before_output = False):
    """
      Return eigenvalues
      
      This yields a set of eigenvalues, i.e. one per unknown. For many solvers such as 
      Rusanov, you need only one.
      
      d: int
        The axis along which we wanna have the eigenvalues
        
      invoke_evalf_before_output: boolean
        If your expression is a symbolic expression (default) then we use evalf 
        before we pipe it into the output. If your expression is something numeric,
        then evalf will fail (as it is not defined for scalar quantities). 
    """
    result  = self._implementation_of_mapping_onto_named_quantities(is_cell_mapping = False)
    result += "switch (normal) {\n"
    for d in range(0,self.dimensions):
      result += "  case " + str(d) + ":\n"
      for i in range(0,self.unknowns):
        if invoke_evalf_before_output:
          result += sympy.printing.cxxcode( self.eigenvalue[i,d].evalf(), assign_to="lambda[" + str(i) + "]" )
        else:
          result += "lambda[" + str(i) + "] = " + sympy.printing.cxxcode( self.eigenvalue[i,d] ) + ";"
        result += "\n"
      result += "  break;\n"
    result += "}\n"
    return result
  
  

  def implementation_of_max_eigenvalue(self, invoke_evalf_before_output = False):
    """
      Return maximum eigenvalue
    """
    result  = self._implementation_of_mapping_onto_named_quantities(is_cell_mapping = False)
    result += "double lambda[" + str(self.unknowns) + "];\n"
    result += "switch (normal) {\n"
    for d in range(0,self.dimensions):
      result += "  case " + str(d) + ":\n"
      for i in range(0,self.unknowns):
        if invoke_evalf_before_output:
          result += sympy.printing.cxxcode( self.eigenvalue[i,d].evalf(), assign_to="lambda[" + str(i) + "]" )
        else:
          result += "lambda[" + str(i) + "] = " + sympy.printing.cxxcode( self.eigenvalue[i,d] ) + ";"
        result += "\n"
      result += "  break;\n"
    result += "}\n"
    result += "double result = 0.0;\n"
    for i in range(0,self.unknowns):
      result += "result = std::max( result, lambda[" + str(i)+ "] );\n"
    result += "return result;\n"
    return result  

    
