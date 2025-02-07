# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractLimiterActionSet import AbstractLimiterActionSet

import peano4.solversteps
import jinja2
class SolveRiemannProblem(AbstractLimiterActionSet):
  TemplateCopyFaceData = """

  if ( {{PREDICATE}} ) {
  
    constexpr int Order = {{SOLVER_ORDER}};
    constexpr int NumberOfVariables = repositories::{{REGULAR_SOLVER_INSTANCE}}.NumberOfUnknowns;
    constexpr int NumberOfParameters = repositories::{{REGULAR_SOLVER_INSTANCE}}.NumberOfAuxiliaryVariables;
    
    #if Dimensions==2
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif

    constexpr int basisElementsPerFace  = spaceFaceSize*(NumberOfVariables+NumberOfParameters);
    constexpr int fluxElementsPerFace   = spaceFaceSize*NumberOfVariables;
  
    for(int d=0; d<Dimensions; d++){
    
      if( fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getTroubled_Marker()==facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED ){
      
        //Regular face to Limiter
        std::copy_n(fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsPerFace, basisElementsPerFace, fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsPerFace);
        std::copy_n(fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsPerFace, fluxElementsPerFace, fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsPerFace);

        //Limiter face to Regular
        std::copy_n(fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).value, basisElementsPerFace, fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).value);
        std::copy_n(fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).value, fluxElementsPerFace, fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).value);

      }


      if( fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).getTroubled_Marker()==facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED ){
        //Regular face to Limiter
        std::copy_n(fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value, basisElementsPerFace, fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value);
        std::copy_n(fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value, fluxElementsPerFace, fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value);

        //Limiter face to Regular
        std::copy_n(fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value+basisElementsPerFace, basisElementsPerFace, fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value+basisElementsPerFace);
        std::copy_n(fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value+fluxElementsPerFace, fluxElementsPerFace, fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value+fluxElementsPerFace);
      }


    }


  }"""
  
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
      return ( super( SolveRiemannProblem, self).get_includes() )