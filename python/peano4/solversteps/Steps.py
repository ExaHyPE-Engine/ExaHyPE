# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .StepsToStepRepository import StepsToStepRepository


class Steps(object):
  """!
  
  All operations within Peano 4 model
  
  Represents the total output generated from the Peano4 data model plus all
  the operations on it. This is basically a big collection. Each step holds
  all the action sets aka operations that are executed within one grid
  traversal.
  
  """
  
  def __init__(self,project):
    self._project = project
    self._steps = []
    self._namespace   = project.namespace
    self.generator = StepsToStepRepository(self)
    
    
  def __str__(self):
    return "(#steps=" + str(len(self._steps)) + ")"
    
    
  def add_step(self,step):
    step.set_project(self._project)
    self._steps.append(step)
    
    
  def construct_output(self,output):
    for step in self._steps:
      step.construct_output(output)
    self.generator.construct_output(output)
    

  def clear(self):
    self._steps = []
    self.generator = StepsToStepRepository(self)
      
