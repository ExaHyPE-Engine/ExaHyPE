# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractLimiterActionSet import AbstractLimiterActionSet

import peano4.solversteps
import jinja2
class SolveRiemannProblem(AbstractLimiterActionSet):
  TemplateCopyFaceData = """
  if ( {{PREDICATE}} ) {

    constexpr int FluxElementsPerFace   = facedata::{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates::Cardinality / 2;
    constexpr int BasisElementsPerFace  = facedata::{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates::Cardinality / 2;

    constexpr int FluxElementsPerFace2  = facedata::{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates::Cardinality / 2;
    constexpr int BasisElementsPerFace2 = facedata::{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates::Cardinality / 2;

    if(fineGridCell{{SOLVER_NAME}}CellLabel.getTroubled_Marker()==celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::REGULAR){
      for(int d=0; d<DIMENSIONS; d++){
        if( fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getTroubled_Marker()==facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED ){
          //Regular face to Limiter
          // Projected solution estimations
          kernels::{{SOLVER_NAME}}::regularToLimiterFace(
            fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data()+BasisElementsPerFace,
            fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data()+BasisElementsPerFace2,
            NumberOfVariables+NumberOfParameters
          );
          // Projected flux estimations
          kernels::{{SOLVER_NAME}}::regularToLimiterFace(
            fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data()+FluxElementsPerFace,
            fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data()+FluxElementsPerFace2,
            NumberOfVariables
          );
        }

        if( fineGridFaces{{SOLVER_NAME}}FaceLabel(d+DIMENSIONS).getTroubled_Marker()==facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED ){
          //Regular face to Limiter
          // Projected solution estimations
          kernels::{{SOLVER_NAME}}::regularToLimiterFace(
            fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+DIMENSIONS).data(),
            fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+DIMENSIONS).data(),
            NumberOfVariables+NumberOfParameters
          );
          // Projected flux estimations
          kernels::{{SOLVER_NAME}}::regularToLimiterFace(
            fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+DIMENSIONS).data(),
            fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+DIMENSIONS).data(),
            NumberOfVariables
          );
        }

      }
    }
    if(fineGridCell{{SOLVER_NAME}}CellLabel.getTroubled_Marker()==celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::TROUBLED){
      for(int d=0; d<DIMENSIONS; d++){

        // Projected solution estimations
        kernels::{{SOLVER_NAME}}::limiterToRegularFace(
          fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data()+BasisElementsPerFace2,
          fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data()+BasisElementsPerFace,
          NumberOfVariables+NumberOfParameters
        );
        // Projected flux estimations
        kernels::{{SOLVER_NAME}}::limiterToRegularFace(
          fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data()+FluxElementsPerFace2,
          fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data()+FluxElementsPerFace,
          NumberOfVariables
        );

        // Projected solution estimations
        kernels::{{SOLVER_NAME}}::limiterToRegularFace(
          fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+DIMENSIONS).data(),
          fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+DIMENSIONS).data(),
          NumberOfVariables+NumberOfParameters
        );
        // Projected flux estimations
        kernels::{{SOLVER_NAME}}::limiterToRegularFace(
          fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+DIMENSIONS).data(),
          fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+DIMENSIONS).data(),
          NumberOfVariables
        );
      }
    }

  }
  """

  # TemplateCopyFaceData = """

  # if ( {{PREDICATE}} ) {
  
  #   constexpr int Order = repositories::{{REGULAR_SOLVER_INSTANCE}}.Order;
  #   constexpr int NumberOfVariables = repositories::{{REGULAR_SOLVER_INSTANCE}}.NumberOfUnknowns;
  #   constexpr int NumberOfParameters = repositories::{{REGULAR_SOLVER_INSTANCE}}.NumberOfAuxiliaryVariables;
    
  #   #if DIMENSIONS==2
  #   constexpr int spaceFaceSize      = (Order+1);
  #   #else
  #   constexpr int spaceFaceSize      = (Order+1)*(Order+1);
  #   #endif

  #   constexpr int basisElementsPerFace  = spaceFaceSize*(NumberOfVariables+NumberOfParameters);
  #   constexpr int fluxElementsPerFace   = spaceFaceSize*NumberOfVariables;

  #   if(fineGridCell{{SOLVER_NAME}}CellLabel.getTroubled_Marker()==celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::REGULAR){
  #     for(int d=0; d<DIMENSIONS; d++){
  #       if( fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getTroubled_Marker()==facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED ){
  #         //Regular face to Limiter
  #         std::copy_n(fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data()+basisElementsPerFace, basisElementsPerFace, fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data()+basisElementsPerFace);
  #         std::copy_n(fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data()+fluxElementsPerFace, fluxElementsPerFace, fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data()+fluxElementsPerFace);
  #       }

  #       if( fineGridFaces{{SOLVER_NAME}}FaceLabel(d+DIMENSIONS).getTroubled_Marker()==facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED ){
  #         //Regular face to Limiter
  #         std::copy_n(fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+DIMENSIONS).data(), basisElementsPerFace, fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+DIMENSIONS).data());
  #         std::copy_n(fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+DIMENSIONS).data(), fluxElementsPerFace, fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+DIMENSIONS).data());
  #       }

  #     }
  #   }
  #   if(fineGridCell{{SOLVER_NAME}}CellLabel.getTroubled_Marker()==celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::TROUBLED){
  #     for(int d=0; d<DIMENSIONS; d++){
  #         std::copy_n(fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data()+basisElementsPerFace, basisElementsPerFace, fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data()+basisElementsPerFace);
  #         std::copy_n(fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data()+fluxElementsPerFace, fluxElementsPerFace, fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data()+fluxElementsPerFace);

  #         std::copy_n(fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+DIMENSIONS).data(), basisElementsPerFace, fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+DIMENSIONS).data());
  #         std::copy_n(fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+DIMENSIONS).data(), fluxElementsPerFace, fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+DIMENSIONS).data());
  #     }
  #   }

  # }"""

  def __init__(self, solver, guard):
    super(SolveRiemannProblem,self).__init__(solver)
    self.guard = guard

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE"] = self.guard
      result = jinja2.Template(self.TemplateCopyFaceData).render(**d)
      pass
    return result

  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
      return ( super( SolveRiemannProblem, self).get_includes() 
      + """
#include "kernels/"""  + self._solver._name + """/Limiter.h"
    """)