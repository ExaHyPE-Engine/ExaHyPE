# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import jinja2


import peano4.toolbox.blockstructured.ProjectPatchOntoFaces


class ProjectPatchOntoFaces( peano4.toolbox.blockstructured.ProjectPatchOntoFaces ):
  """
  
   This class is very similar to peano4.toolbox.blockstructured.ProjectPatchOntoFaces, 
   but there are some delicate differences.
   
  """
  def __init__(self,solver, predicate):
    peano4.toolbox.blockstructured.ProjectPatchOntoFaces.__init__(self,solver._patch,solver._patch_overlap_update,predicate,solver._get_default_includes() + solver.get_user_includes(), True)


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


#  def get_includes(self):
#    return """
##include "peano4/utils/Loop.h"
#""" + self.additional_includes
