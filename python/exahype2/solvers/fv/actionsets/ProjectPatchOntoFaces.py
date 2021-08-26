# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

import peano4.toolbox.blockstructured

from .AbstractFVActionSet import AbstractFVActionSet

import peano4
import jinja2

class ProjectPatchOntoFaces( peano4.toolbox.blockstructured.ProjectPatchOntoFaces ):
  def __init__(self,solver, predicate, project_onto_new = True):
    if project_onto_new:
      peano4.toolbox.blockstructured.ProjectPatchOntoFaces.__init__(
        self,
        solver._patch,
        solver._patch_overlap_new,
        predicate, 
        solver._get_default_includes() + solver.get_user_includes()
      )
    else:
      peano4.toolbox.blockstructured.ProjectPatchOntoFaces.__init__(
        self,
        solver._patch,
        solver._patch_overlap_old,
        predicate, 
        solver._get_default_includes() + solver.get_user_includes()
      )
