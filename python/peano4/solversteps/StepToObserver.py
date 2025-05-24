# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Observer import Observer


class StepToObserver(object):
  def __init__(self, step):
    self.step = step

  def construct_output(self, output, included_action_sets):
    observer = Observer(
      self.step.name,
      self.step.project.namespace + ["observers"],
      self.step.project.subdirectory + "observers",
      included_action_sets,
      self.step.vertex_data,
      self.step.face_data,
      self.step.cell_data
    )
    output.artefacts.append(observer)
    output.makefile.add_h_file(self.step.project.subdirectory + "observers" + "/" + self.step.name + ".h", generated=True)
    output.makefile.add_cpp_file(self.step.project.subdirectory + "observers" + "/" + self.step.name + ".cpp", generated=True)
