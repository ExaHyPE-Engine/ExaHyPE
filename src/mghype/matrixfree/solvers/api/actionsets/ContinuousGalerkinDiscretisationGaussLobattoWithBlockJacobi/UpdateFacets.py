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
      // in the first step, diagonal is zero
      if ( tarch::la::equals(fineGridFace{{SOLVER_NAME}}.getDiagonal(i),0.0) ) {
        fineGridFace{{SOLVER_NAME}}.setDeltaU( i, 0.0 );
      }
      else {
        double du = repositories::{{SOLVER_INSTANCE}}.OmegaFace / fineGridFace{{SOLVER_NAME}}.getDiagonal(i) * fineGridFace{{SOLVER_NAME}}.getResidual(i);
        assertion( du==du );
        assertion( not std::isnan(du) );
        fineGridFace{{SOLVER_NAME}}.setDeltaU( i, du );
      }
      fineGridFace{{SOLVER_NAME}}.setResidual( i, 0.0 );
      fineGridFace{{SOLVER_NAME}}.setDiagonal( i, 0.0 );
    }
    
    logTraceOutWith1Argument( "touchFaceFirstTime", fineGridFace{{SOLVER_NAME}}.getDeltaU() );
  }
  else {
    for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknowns; i++) {
      fineGridFace{{SOLVER_NAME}}.setDeltaU( i, 0.0 );
      fineGridFace{{SOLVER_NAME}}.setResidual( i, 0.0 );
      fineGridFace{{SOLVER_NAME}}.setDiagonal( i, 0.0 );
    }
  }
"""

  templateTouchCellFirstTime = """ 
  if ( 
    fineGridCell{{SOLVER_NAME}}.getType() == celldata::{{SOLVER_NAME}}::Type::Interior 
    and
    repositories::{{SOLVER_INSTANCE}}.updateCell()
  ) {
    logTraceInWith2Arguments("touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());

    // ==============================
    // project updates into the cells
    // ==============================
    // @todo Very similar to Alex's matrix calibration discussion on Slack, 
    //       there is some scaling missing here.
    for (int d=0; d<Dimensions; d++) {
      int unknownInPreimage = 0;
      dfore(node,repositories::{{SOLVER_INSTANCE}}.PolyDegree+1,d,0) {
        tarch::la::Vector< Dimensions, int > nodeOnLeftSideOfImage  = node;
        tarch::la::Vector< Dimensions, int > nodeOnRightSideOfImage = node;
        nodeOnLeftSideOfImage(d)  = 0;
        nodeOnRightSideOfImage(d) = repositories::{{SOLVER_INSTANCE}}.PolyDegree;

        for (int unknown=0; unknown<repositories::{{SOLVER_INSTANCE}}.UnknownsPerCellNode; unknown++) {
          int unknownInLeftImage  = peano4::utils::dLinearised( nodeOnLeftSideOfImage,  repositories::{{SOLVER_INSTANCE}}.PolyDegree+1) * repositories::{{SOLVER_INSTANCE}}.UnknownsPerCellNode + unknown;
          int unknownInRightImage = peano4::utils::dLinearised( nodeOnRightSideOfImage, repositories::{{SOLVER_INSTANCE}}.PolyDegree+1) * repositories::{{SOLVER_INSTANCE}}.UnknownsPerCellNode + unknown;

          double update;
          update = fineGridFaces{{SOLVER_NAME}}( d ).getDeltaU(unknownInPreimage);
          assertion( update==update );
          assertion( not std::isnan(update) );
          fineGridCell{{SOLVER_NAME}}.setSolution( 
            unknownInLeftImage,      
            fineGridCell{{SOLVER_NAME}}.getSolution(unknownInLeftImage) + 
            update
          );
          update = fineGridFaces{{SOLVER_NAME}}( d+Dimensions ).getDeltaU(unknownInPreimage);
          assertion( not std::isnan(update) );
          fineGridCell{{SOLVER_NAME}}.setSolution( 
            unknownInRightImage,      
            fineGridCell{{SOLVER_NAME}}.getSolution(unknownInRightImage) + 
            update
          );
          
          unknownInPreimage++;
        }
      }
    }
    
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > r =
      repositories::{{SOLVER_INSTANCE}}.getRhsMatrix( marker.x(), marker.h() ) * fineGridCell{{SOLVER_NAME}}.getRhs();
    r = r - repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * fineGridCell{{SOLVER_NAME}}.getSolution();

    for (int d=0; d<Dimensions; d++) {
      // names are inverted here, i.e. code works, but variable names are wrong
      int unknownInPreimage = 0;
      dfore(node,repositories::{{SOLVER_INSTANCE}}.PolyDegree+1,d,0) {
        tarch::la::Vector< Dimensions, int > nodeOnLeftSideOfImage  = node;
        tarch::la::Vector< Dimensions, int > nodeOnRightSideOfImage = node;
        nodeOnLeftSideOfImage(d)  = 0;
        nodeOnRightSideOfImage(d) = repositories::{{SOLVER_INSTANCE}}.PolyDegree;

        for (int unknown=0; unknown<repositories::{{SOLVER_INSTANCE}}.UnknownsPerCellNode; unknown++) {
          int unknownInLeftImage  = peano4::utils::dLinearised( nodeOnLeftSideOfImage,  repositories::{{SOLVER_INSTANCE}}.PolyDegree+1) * repositories::{{SOLVER_INSTANCE}}.UnknownsPerCellNode + unknown;
          int unknownInRightImage = peano4::utils::dLinearised( nodeOnRightSideOfImage, repositories::{{SOLVER_INSTANCE}}.PolyDegree+1) * repositories::{{SOLVER_INSTANCE}}.UnknownsPerCellNode + unknown;
  
          double diagonalUpdate;
          diagonalUpdate = repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h())(unknownInLeftImage,unknownInLeftImage);
          assertion( diagonalUpdate==diagonalUpdate );
          assertion2(diagonalUpdate>0.0,repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()),unknownInLeftImage);
          fineGridFaces{{SOLVER_NAME}}( d ).setDiagonal( 
            unknownInPreimage,
            fineGridFaces{{SOLVER_NAME}}( d ).getDiagonal( unknownInPreimage ) 
            + 
            diagonalUpdate
          );
          diagonalUpdate = repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h())(unknownInRightImage,unknownInRightImage);
          assertion( diagonalUpdate==diagonalUpdate );
          assertion2(diagonalUpdate>0.0,repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()),unknownInLeftImage);
          fineGridFaces{{SOLVER_NAME}}( d+Dimensions ).setDiagonal( 
            unknownInPreimage,
            fineGridFaces{{SOLVER_NAME}}( d+Dimensions ).getDiagonal( unknownInPreimage ) 
            + 
            diagonalUpdate
          );
          
          assertion(r(unknownInLeftImage)==r(unknownInLeftImage));
          assertion( not std::isnan(r(unknownInLeftImage)) );
          fineGridFaces{{SOLVER_NAME}}( d ).setResidual( 
            unknownInPreimage,
            fineGridFaces{{SOLVER_NAME}}( d ).getResidual( unknownInPreimage ) 
            + 
            r(unknownInLeftImage)
          );
          assertion(r(unknownInRightImage)==r(unknownInRightImage));
          assertion( not std::isnan(r(unknownInRightImage)) );
          assertion2( r(unknownInRightImage)>-1.0, r(unknownInRightImage), unknownInRightImage );
          fineGridFaces{{SOLVER_NAME}}( d+Dimensions ).setResidual( 
            unknownInPreimage,
            fineGridFaces{{SOLVER_NAME}}( d+Dimensions ).getResidual( unknownInPreimage ) 
            + 
            r(unknownInRightImage)
          );
          
          unknownInPreimage++;
        }
      }
    }

    logTraceOutWith2Arguments("touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
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
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass
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
