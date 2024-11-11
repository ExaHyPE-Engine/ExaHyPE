# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class UpdateFacets(ActionSet):
  """!

  Action set implementing the facet update of the continuous Galerkin solver

  There are two key elements/events here:
  
  - When we hit the face, we compute the update due to the Jacobi solver and
    subsequently clear all the data that is accumulated. We don't need it 
    anymore as we have already determined the face update.
  - When we hit the cell, we first map the facet update onto the cell. Now we
    have an updated cell guess. After that, we immediately compute the residual
    and throw it back onto the facet.
    
  The latter step might seem to be stupid: After all, we could first update the
  cell interior and then use the updated solution to determine a residual for 
  the facet. This would be a proper block-Jacobi. However, I prefer a pure
  Jacobi here, as it allows us to deploy the block update onto a separate task.
 
  The key properties of the solver that we use here are the following ones:
  
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
self._degrees_of_freedom_per_cell            = unknowns_per_node * ( polynomial_degree + 1) ** dimensions
self._degrees_of_freedom_per_face            = unknowns_per_node * ( polynomial_degree + 1) ** (dimensions-1)
self._inner_degrees_of_freedom_per_cell      = unknowns_per_node * ( polynomial_degree - 1) ** dimensions

self._cell_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "solution", str(self._degrees_of_freedom_per_cell) ))
self._cell_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "residual", str(self._degrees_of_freedom_per_cell) ))
self._cell_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "rhs",      str(self._degrees_of_freedom_per_cell) ))

self._face_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "residual",      str(self._degrees_of_freedom_per_face) ))
self._face_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "diagonal",      str(self._degrees_of_freedom_per_face) ))
self._face_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "deltaU",        str(self._degrees_of_freedom_per_face) ))
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
  Which map onto the 
  
  """
  templateTouchFaceFirstTime="""
  if ( 
    fineGridFace{{SOLVER_NAME}}.getType() == facedata::{{SOLVER_NAME}}::Type::Interior 
    and
    repositories::{{SOLVER_INSTANCE}}.updateFace()
  ) {
    logTraceInWith3Arguments( "touchFaceFirstTime", fineGridFace{{SOLVER_NAME}}.getResidual(), fineGridFace{{SOLVER_NAME}}.getDiagonal(), marker.toString() );
    
    for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknowns; i++) {
      // first step diagonal is zero
      if ( tarch::la::equals(fineGridFace{{SOLVER_NAME}}.getDiagonal(i),0.0) ) {
        double du = repositories::{{SOLVER_INSTANCE}}.OmegaFace / fineGridFace{{SOLVER_NAME}}.getDiagonal(i) * fineGridFace{{SOLVER_NAME}}.getResidual(i);
        fineGridFace{{SOLVER_NAME}}.setDeltaU( du );
      }
      else {  
        fineGridFace{{SOLVER_NAME}}.setDeltaU( 0.0 );
      }
      fineGridFace{{SOLVER_NAME}}.setResidual( 0.0 );
      fineGridFace{{SOLVER_NAME}}.setDiagonal( 0.0 );
    }
    
    logTraceOutWith1Argument( "touchFaceFirstTime", fineGridFace{{SOLVER_NAME}}.getDeltaU() );
  }
  else {
    for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknowns; i++) {
      fineGridFace{{SOLVER_NAME}}.setDeltaU( 0.0 );
      fineGridFace{{SOLVER_NAME}}.setResidual( 0.0 );
      fineGridFace{{SOLVER_NAME}}.setDiagonal( 0.0 );
    }
  }


"""

  templateTouchCellFirstTime = """ 
  if ( 
    fineGridCell{{SOLVER_NAME}}.getType() == celldata::{{SOLVER_NAME}}::Type::Interior 
    and
    repositories::{{SOLVER_INSTANCE}}.update()
  ) {
    logTraceInWith2Arguments("Solve::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());

    repositories::{{SOLVER_INSTANCE}}.updateMinMaxMeshSize(
      marker.h()
    );
    
    for (int unknown=0; unknown<{{VERTEX_CARDINALITY}}; unknown++)
    {
      // Mass and assembly matrix should be block diagonal, 
      // depending on which unknown we are currently dealing with.
      // We will collect the rhs & solution values from the 
      // vertices, and apply the correct matrix as we go.
      // Define the offset:
      int startIndex = TwoPowerD * unknown;
      
      auto A       = repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h());
      auto M       = repositories::{{SOLVER_INSTANCE}}.getMassMatrix(marker.x(), marker.h());

      // collect all of the vertex values into one vector - apply the appropriate
      // part of the matrix as we go.
      tarch::la::Vector<TwoPowerD, double> vertexValues;
      tarch::la::Vector<TwoPowerD, double> rhsValues;
      tarch::la::Vector<TwoPowerD, double> residualValues;
      for (int i=0; i<TwoPowerD; i++){
        double solColSum = 0;
        double rhsColSum = 0;
        for (int j=0; j<TwoPowerD; j++){
          solColSum += A(startIndex+i, startIndex+j) * fineGridVertices{{SOLVER_NAME}}(j).getU(unknown);
          rhsColSum += M(startIndex+i, startIndex+j) * fineGridVertices{{SOLVER_NAME}}(j).getRhs(unknown);
        }
      
        vertexValues(i)   = solColSum;        
        rhsValues(i)      = rhsColSum;
        residualValues(i) = fineGridVertices{{SOLVER_NAME}}(i).getResidual(unknown);
      }

      // residual = rhs - Ax
      residualValues += rhsValues;
      residualValues -= vertexValues;

      for (int i=0; i<TwoPowerD; i++)
      {
        logTraceInWith1Argument("puttingValuesBack", fineGridVertices{{SOLVER_NAME}}(i).toString());

        // increment fieldDiag
        // get current val
        double diagVal = fineGridVertices{{SOLVER_NAME}}(i).getDiag(unknown);
        // increment it
        diagVal += A(i,i);
        fineGridVertices{{SOLVER_NAME}}(i).setDiag(unknown, diagVal);

        // put new residual values back
        fineGridVertices{{SOLVER_NAME}}(i).setResidual(unknown, residualValues(i));
        
        logTraceOutWith1Argument("puttingValuesBack", fineGridVertices{{SOLVER_NAME}}(i).toString());
      }

    }
    logTraceOutWith2Arguments("Solve::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
  }
  """
  
  def __init__(self,
               solver):
    super( UpdateFacets, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()

  def get_body_of_operation(self,operation_name):
    result = ""
#if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
#      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
#      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.templateTouchFaceFirstTime).render(**self.d)
      pass
    return result

  def get_action_set_name(self):
    """!
    
    Configure name of generated C++ action set
    
    This action set will end up in the directory observers with a name that
    reflects how the observer (initialisation) is mapped onto this action 
    set. The name pattern is ObserverName2ActionSetIdentifier where this
    routine co-determines the ActionSetIdentifier. We make is reflect the
    Python class name.
     
    """
    return __name__.replace(".py", "").replace(".", "_")
    
  def user_should_modify_template(self):
    """!
    
    The action set that Peano will generate that corresponds to this class
    should not be modified by users and can safely be overwritten every time
    we run the Python toolkit.
    
    """
    return False

  def get_includes(self):
    """!
   
    We need the solver repository in this action set, as we directly access
    the solver object. We also need access to Peano's d-dimensional loops.
         
    """    
    return """
#include "../repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""
