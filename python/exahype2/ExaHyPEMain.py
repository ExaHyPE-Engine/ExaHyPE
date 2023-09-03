# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import peano4.runner.DefaultSequence

class ExaHyPEMain(peano4.runner.DefaultSequence):
  def __init__(self,
               peano_project,
               exahype2_project
               ):
    """
     project: peano4.Project
    """
    super(ExaHyPEMain,self).__init__(peano_project)
    self.overwrite     = False
    self.d[ "LOG_FILTER_FILE" ] =          exahype2_project._log_filter_file
    self.d[ "NUMBER_OF_THREADS" ] =        exahype2_project._number_of_threads
    self.d[ "MULTICORE_ORCHESTRATION" ] =  exahype2_project._multicore_orchestration
    self.d[ "GPUS" ] =                     exahype2_project._gpus
    self.d[ "ADDITIONAL_INCLUDES" ] =      exahype2_project._additional_includes


  def _get_header_file_template(self):
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    return templatefile_prefix+".template.h"

  def _get_implementation_file_template(self):
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    return templatefile_prefix+".template.cpp"
