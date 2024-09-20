# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4.runner.DefaultSequence
import os


class PETScMain(peano4.runner.DefaultSequence):

  def __init__(self,
               project,
               domain_offset,
               domain_size
               ):
    """
    
     project: peano4.Project
          
    """
    super(PETScMain,self).__init__(project)
    self.overwrite     = True

    self.d[ "Project_Name"]  = project.project_name
    self.d[ "DomainOffset" ] = "{" + str(domain_offset[0])
    self.d[ "DomainSize" ]   = "{" + str(domain_size[0])
    for i in domain_offset[1:]:
      self.d[ "DomainOffset" ] += ", "
      self.d[ "DomainOffset" ] += str(i)
    for i in domain_size[1:]:
      self.d[ "DomainSize" ] += ", "
      self.d[ "DomainSize" ] += str(i)
    self.d[ "DomainOffset" ] += "}"
    self.d[ "DomainSize" ]   += "}"

  def _get_header_file_template(self):
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )    
    return templatefile_prefix+".template.h"
    
    
  def _get_implementation_file_template(self):
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )    
    return templatefile_prefix+".template.cpp"

