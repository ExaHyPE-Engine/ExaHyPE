# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

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
    return templatefile_prefix+".template.h"

  def _get_implementation_file_template(self):
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    return templatefile_prefix+".template.cpp"

  def construct_output(self,output,main_name):
    """
      Pass in a version of output

      main_name Is the name of you main file. By default, this might be
         Main.cpp, but you might want to have different main files for
         different experiments. Please do not give the file an extension.
    """
    output.makefile.add_h_file( self.project.subdirectory + main_name + ".h", generated=True )
    output.makefile.add_cpp_file( self.project.subdirectory + main_name + ".cpp", generated=True )

    header_file_template = self._get_header_file_template()
    cpp_file_template = self._get_implementation_file_template()
    self.d[ "MAIN_NAME" ] = main_name
    self.d[ "HEADER_FILE_TEMPLATE" ] = os.path.basename(header_file_template)
    self.d[ "CPP_FILE_TEMPLATE" ] = os.path.basename(cpp_file_template)
    self.d[ "PROJECT_NAME" ] = self.project.project_name

    generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      header_file_template,
      cpp_file_template,
      main_name,
      self.project.namespace,
      "./" + self.project.subdirectory,
      self.d,
      self.overwrite)
    output.add(generated_files)
