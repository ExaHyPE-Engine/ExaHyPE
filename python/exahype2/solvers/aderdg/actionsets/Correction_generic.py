# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet

#import peano4
import peano4.solversteps
import jinja2

class Correction_generic(AbstractAderDGActionSet):
  """
  
  The linear combination of the Runge Kutta trials has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection 
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  
  
  """
  
  TemplateProject = """
  if ({{PREDICATE}}) {
  
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
  
    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    
    const int Order = {{ORDER}};
    const int NumberOfVariables = {{NUMBER_OF_UNKNOWNS}};
    const int NumberOfParameters = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    
    #if Dimensions==2
    constexpr int spaceBasisSize     = (Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceBasisSize     = (Order+1)*(Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif
    
    double* luh = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
    {{CORRECTOR_COMPUTATION_PRECISION}} lduh[spaceBasisSize*NumberOfVariables]{0.0};

    const int basisElementsByFace = spaceFaceSize*(NumberOfVariables+NumberOfParameters);
    const int fluxElementsByFace = spaceFaceSize*NumberOfVariables;
    
    {{CORRECTOR_COMPUTATION_PRECISION}} FL[fluxElementsByFace];
    {{CORRECTOR_COMPUTATION_PRECISION}} FR[fluxElementsByFace];
    
    /*
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */
    
    for(int d=0; d<Dimensions; d++){
    
      //negative face
      {{PREDICTOR_STORAGE_PRECISION}}* QL = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value;
      {{PREDICTOR_STORAGE_PRECISION}}* QR = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsByFace;

      /*
      **  Data from the face is copied into a buffer because operations are directly performed on this
      **  by the kernels and we want the faces to remain unmodified for the neighbour.
      **  The solution elements are only accessed but not used as buffers and therefore no copying is needed.
      */
      std::copy_n(fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value, fluxElementsByFace, FL);
      std::copy_n(fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsByFace, fluxElementsByFace, FR);

      tarch::la::Vector<Dimensions,double> faceCentre = marker.x();
      faceCentre[d] -= 0.5*marker.h()[d];
      
      {% if IS_LINEAR is sameas True %}
      //template <bool useFlux, bool useNCP, bool useMM, typename SolverType>
      kernels::aderdg::generic::c::riemannSolverLinear<
        {{ "true" if USE_FLUX!="<none>" else "false" }}, //useFlux
        {{ "true" if USE_NCP!="<none>" else "false" }}, //useNCP
        false, //useMM
        {{SOLVER_NAME}}>(
          repositories::{{SOLVER_INSTANCE}},
          FL,
          FR,
          QL,
          QR,
          timeStamp,
          timeStepSize,
          faceCentre,
          marker.h(),
          d //direction
          );
      {% else %}
      //template <bool useNCP, bool useViscousFlux, typename SolverType>
      kernels::aderdg::generic::c::riemannSolverNonlinear<
        {{ "true" if USE_NCP!="<none>" else "false" }}, //useNCP
        {{SOLVER_NAME}}>(
          repositories::{{SOLVER_INSTANCE}},
          FL,
          FR,
          QL,
          QR,
          timeStamp,
          timeStepSize,
          faceCentre,
          marker.h(),
          d //direction
          );
      {% endif %}    

      {% if IS_LINEAR is sameas True %}
      kernels::aderdg::generic::c::faceIntegralLinear<
        {{SOLVER_NAME}},
        {{NUMBER_OF_UNKNOWNS}},
        {{ORDER}}+1>(
          lduh,
          FR,
          d, //direction
          0, //orientation
          marker.h()
          );
      {% else %}
      kernels::aderdg::generic::c::faceIntegralNonlinear<
        {{SOLVER_NAME}},
        {{NUMBER_OF_UNKNOWNS}},
        {{ORDER}}+1>(
          lduh,
          FR,
          d, //direction
          0, //orientation
          marker.h()
          );
      {% endif %}


      //positive face
      QL = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value;
      QR = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value+basisElementsByFace;
      std::copy_n(fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value, fluxElementsByFace, FL);
      std::copy_n(fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value+fluxElementsByFace, fluxElementsByFace, FR);

      faceCentre[d] += marker.h()[d];

      {% if IS_LINEAR is sameas True %}
      //template <bool useFlux, bool useNCP, bool useMM, typename SolverType>
      kernels::aderdg::generic::c::riemannSolverLinear<
        {{ "true" if USE_FLUX!="<none>" else "false" }}, //useFlux
        {{ "true" if USE_NCP!="<none>" else "false" }}, //useNCP
        false, //useMM
        {{SOLVER_NAME}}>(
          repositories::{{SOLVER_INSTANCE}},
          FL,
          FR,
          QL,
          QR,
          timeStamp,
          timeStepSize,
          faceCentre,
          marker.h(),
          d //direction
          );
      {% else %}
      //template <bool useNCP, bool useViscousFlux, typename SolverType>
      kernels::aderdg::generic::c::riemannSolverNonlinear<
        {{ "true" if USE_NCP!="<none>" else "false" }}, //useNCP
        {{SOLVER_NAME}}>(
          repositories::{{SOLVER_INSTANCE}},
          FL,
          FR,
          QL,
          QR,
          timeStamp,
          timeStepSize,
          faceCentre,
          marker.h(),
          d //direction
          );
      {% endif %}    

      {% if IS_LINEAR is sameas True %}
      kernels::aderdg::generic::c::faceIntegralLinear<
        {{SOLVER_NAME}},
        {{NUMBER_OF_UNKNOWNS}},
        {{ORDER}}+1>(
          lduh,
          FL,
          d, //direction
          1, //orientation
          marker.h()
          );
      {% else %}
      kernels::aderdg::generic::c::faceIntegralNonlinear<
        {{SOLVER_NAME}},
        {{NUMBER_OF_UNKNOWNS}},
        {{ORDER}}+1>(
          lduh,
          FL,
          d, //direction
          1, //orientation
          marker.h()
          );
      {% endif %}
      
    }// for d    
    

    kernels::aderdg::generic::c::solutionUpdate<
      {{SOLVER_NAME}},
      {{CORRECTOR_COMPUTATION_PRECISION}}
        >(
        repositories::{{SOLVER_INSTANCE}},
        luh,
        luh,
        lduh,
        timeStepSize
      );


    {{COMPUTE_NEW_TIME_STEP_SIZE}}
  
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp+timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
      
    repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0) );
   
  }
"""
  
  def __init__(self,solver,guard):
    """
    
    guard_project: String (C++ code)
      Predicate which controls if the solution is actually projected
      
    guard_safe_old_time_step: String (C++ code)
      Predicate which controls if the projection should be copied into
      the old solution and the time step should also be moved over
    
    """
    super(Correction_generic,self).__init__(solver)
    self.guard            = guard
    

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]                 = self.guard
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
    return super(Correction_generic,self).get_includes() + """
#include "exahype2/aderdg/kernels/Kernels.h"
      """