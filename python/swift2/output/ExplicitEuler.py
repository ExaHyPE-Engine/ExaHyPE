# This file is part of the SWIFT 2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .Project                import Project


import peano4


class ExplicitEuler(Project):
  """
   
  Represents an abstract SWIFT 2 project. An SWIFT 2 project is a Peano 4 
  project with a particular set of actions (algorithmic phases) 
  that you can choose from and with particular solver types. It 
  realises a builder mechanism, i.e. you build up your SWIFT 2
  project and then you finally tell the project "give me the Peano 4
  project". From hereon, you can use this Peano 4 project to actually
  set up the Peano 4 application.
  
  Please do not use this class directly. Use one of its subclasses.
  
  @see generate_Peano4_project()
  
  """
  def __init__(self, namespace, project_name, directory = ".", executable="swift2"):
    super( ExplicitEuler,self ).__init__(namespace, project_name, directory, executable)
    
    self._algorithm_step_force_calculation  = peano4.solversteps.Step( "CalculateForce", False )
    self._algorithm_step_time_step          = peano4.solversteps.Step( "TimeStep",       False )
    
    
  def main_file_template(self):
    return "ExplicitEulerMain.template"
    

  def generate_Peano4_project(self, verbose=False):
    self._project.solversteps.add_step(self._algorithm_step_force_calculation)
    self._project.solversteps.add_step(self._algorithm_step_time_step)
    
    return super( ExplicitEuler, self ).generate_Peano4_project(verbose)
    
    