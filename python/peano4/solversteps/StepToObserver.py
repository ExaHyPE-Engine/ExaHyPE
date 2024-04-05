# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.output.ActionSet


class StepToObserver(object):
  def __init__(self,step):
    self.step = step
    
    
  def construct_output(self,output,included_action_sets):
    """
      Construct one mapping
    """
    observer = peano4.output.Observer(
      self.step.name, self.step.project.namespace + [ "observers" ], self.step.project.subdirectory + "observers",
      included_action_sets,
      self.step.vertex_data,
      self.step.face_data,
      self.step.cell_data
    )
    output.artefacts.append( observer )
    output.makefile.add_cpp_file( observer.get_cpp_file_name(), generated=True )
