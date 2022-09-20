# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
#import peano4.output.TemplatedHeaderImplementationFilePair
import os

from peano4.runner.DefaultSequence import DefaultSequence

class SPHSequence(DefaultSequence):
  """
  In this class we simply want to re-define the original template files used by
  the DefaultSequence class so we can use a main deploying a full SPH
  simulation.
  Notice that templatefile_prefix is equal to dir + this-filename, and hence
  will work untouched if we name the .h and .cpp template files correctly, as in
  the DefaultSequence case.
    
  """
  def _get_header_file_template(self):
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )    
    return templatefile_prefix+".h.template"
    
  def _get_implementation_file_template(self):
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )    
    return templatefile_prefix+".cpp.template"
    
