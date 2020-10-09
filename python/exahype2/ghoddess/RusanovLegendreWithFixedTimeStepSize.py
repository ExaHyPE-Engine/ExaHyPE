# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair


import exahype2.grid.AMROnPatch


from .Solver import Solver


class RusanovLegendreWithFixedTimeStepSize(Solver):
  """ 
  
  
   
  """
  def __init__(self,name, order, unknowns, dimensions, time_step_size):
    super( RusanovLegendreWithFixedTimeStepSize, self ).__init__(name, order, unknowns, dimensions)
    self._time_step_size = time_step_size

  #
  # We have a couple of predefined macros here. See Solver superclass.
  #
  _AdjustCellTemplate = """
  {SOLVER_INSTANCE}.adjustSolution(
    cellData, marker.x(), marker.h(), {SOLVER_INSTANCE}.getMinTimeStamp()
  );  
"""


  #_AMRTemplate = """
  #  {SOLVER_INSTANCE}.refinementCriterion(
  #    cellData,
  #    marker.x(), 
  #    marker.h(), 
  #    {SOLVER_INSTANCE}.getMinTimeStamp()
  #  )
  #);
  #"""

  BoundaryTemplate = """
  {SOLVER_INSTANCE}.boundaryConditions(
    faceData, marker.x(), marker.h(), {SOLVER_INSTANCE}.getMinTimeStamp(), marker.outerNormal()
  );
"""

  ProjectOntoFaceTemplate = """
  {SOLVER_INSTANCE}.projectOntoFaces(
    cellData, 
    faceData[0], faceData[1], faceData[2], faceData[3],
    marker.x(), marker.h(), {SOLVER_INSTANCE}.getMinTimeStamp()
  );  
"""


  SolveCellTemplate = """
  {SOLVER_INSTANCE}.solveCell(
    cellData, 
    faceData[0], faceData[1], faceData[2], faceData[3],
    marker.x(), marker.h(), {SOLVER_INSTANCE}.getMinTimeStamp()
  );  
"""
     

  def add_entries_to_text_replacement_dictionary(self,d):
    d[ "TIME_STEP_SIZE" ] = self._time_step_size
    pass  
