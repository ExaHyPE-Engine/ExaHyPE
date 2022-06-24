# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from abc import ABC
from abc import abstractmethod


class SolverCodeSnippets(ABC):
  """
  
  Interface for all solvers' code snippets
  
  Every solver has to inject some code snippets into the generated code: Which 
  fields have to be available in the abstract base class, how do you compute
  the time step size within the kernels, and so forth. These routines are 
  collected here. They are often the same for different numerical schemes, so 
  it makes sense to bundle them up in snippet classes.
  
  """

  @abstractmethod 
  def create_abstract_solver_user_declarations(self):
    raise Exception( "abstract method, not implemented")


  @abstractmethod 
  def create_abstract_solver_user_definitions(self):
    raise Exception( "abstract method, not implemented")


  @abstractmethod 
  def create_abstract_solver_constructor_statements(self):
    raise Exception( "abstract method, not implemented")
  
  
  @abstractmethod 
  def create_compute_time_step_size():
    """
     
    Within the actual compute kernels, the kernels ask the solver variant how to 
    determine a new field
    
    
          const double timeStepSize = ...;
          
    You can remove the const if you want. Anyway, this routine has to build up the
    right time step size choice.

    """
    raise Exception( "abstract method, not implemented")

  
  @abstractmethod 
  def create_compute_new_time_step_size():
    """
    
    Very similar to create_compute_time_step_size(), this routine should return
    a code snippet which creates a field newTimeStepSize. It is used for subsequent
    time steps and/or some global analysis. 
    
    """
    raise Exception( "abstract method, not implemented")


  @abstractmethod 
  def create_start_time_step_implementation(self):
    raise Exception( "abstract method, not implemented")


  @abstractmethod 
  def create_finish_time_step_implementation(self):
    raise Exception( "abstract method, not implemented")

