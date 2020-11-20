# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.output.TemplatedHeaderImplementationFilePair
import os


class DefaultSequence(object):
  """
  
    The default sequence sketches what Peano does if there's no main. You can 
    alter it. The project holds an instance of this object through its main 
    attribute. So it holds an instance of this class. As a consequence, you can
    either change this object's attributes or you can replace this object if
    the object of you choice.
    
    Most people who wanna redefine the main create a subclass of DefaultSequence
    and overwrite _get_header_file_template() and _get_implementation_file_template().
    Some also redefine the default overwrite policy by changing the attribute
    overwrite.
    
  """
  def __init__(self,project):  
    self.project   = project
    self.overwrite = False
    self.d         = {}
    
  def _get_header_file_template(self):
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )    
    return templatefile_prefix+".h.template"
    
  def _get_implementation_file_template(self):
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )    
    return templatefile_prefix+".cpp.template"
    
  def construct_output(self,output,main_name):
    """
      Pass in a version of output
      
      main_name  Is the name of you main file. By default, this might be 
         main.cpp, but you might want to have different main files for 
         different experiments. Please do not give the file an extension.
    """
    output.makefile.add_cpp_file( main_name + ".cpp" )
    print( "generated " + main_name + ".cpp")
    
    self.d[ "MAIN_NAME" ] = main_name

    generated_files = peano4.output.TemplatedHeaderImplementationFilePair(
      self._get_header_file_template(),
      self._get_implementation_file_template(),
      main_name, 
      self.project.namespace,
      ".", 
      self.d,
      self.overwrite)
    output.add(generated_files)
