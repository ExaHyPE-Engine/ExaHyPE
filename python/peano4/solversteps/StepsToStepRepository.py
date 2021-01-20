# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


import peano4.output.TemplatedHeaderImplementationFilePair
import os


class StepsToStepRepository(object):
  def __init__(self,steps):
    self.steps = steps
    self.d     = {}
    self.d["FORWARD_DECLARATIONS"] = ""
    self.d["ENUM_VARIANTS"]        = ""
    self.d["STRING_RETURN_CASE"]   = ""
    self.d["INT_RETURN_CASE"]   = ""
    self.d["STEP_RETURN_CASE"]   = ""

  def __get_full_namespace(self):
    return self.steps._namespace + [ "repositories" ]

  def __get_class_name(self):
    return "StepRepository"

  def construct_output(self,output):
    for i in self.steps._steps:
      self.d["FORWARD_DECLARATIONS"] += "  class " + i.name + ";\n"
      self.d["ENUM_VARIANTS"]        += ", " + i.name 
      self.d["STRING_RETURN_CASE"]   += "    case Steps::" + i.name + ": return \"" + i.name + "\";\n"
      self.d["INT_RETURN_CASE"]      += "    case Steps::" + i.name + ": return " + str(self.steps._steps.index(i)+1) + ";\n"
      self.d["STEP_RETURN_CASE"]     += "    case " + str(self.steps._steps.index(i)+1) + ": return Steps::" + i.name + ";\n"
        
    #     + " = peano4::parallel::Node::UndefProgramStep + " + 
    
    namespace  = self.__get_full_namespace()
    class_name = self.__get_class_name()
    output.makefile.add_cpp_file( namespace[-1] + "/" + class_name + ".cpp" )
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    generated_files = peano4.output.TemplatedHeaderImplementationFilePair(
      templatefile_prefix+".h.template",
      templatefile_prefix+".cpp.template",
      class_name, 
      namespace,
      namespace[-1], 
      self.d,
      True)
    output.add(generated_files)
    pass
