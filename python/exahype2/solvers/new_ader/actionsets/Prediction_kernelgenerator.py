# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet
from ..kernels import create_array_and_convert_from_type

#import peano4
import peano4.solversteps
import jinja2

class Prediction_kernelgenerator(AbstractAderDGActionSet):
  """
  
  The extrapolated solution from the space-time predictor has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection 
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  
  """
  
  TemplateProject = """
  if ({{PREDICATE}}) {
  
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}  
  
    {{SOLUTION_STORAGE_PRECISION}}* luh = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
  
    const int Order = {{ORDER}};
    const int NumberOfVariables  = {{NUMBER_OF_UNKNOWNS}};
    const int NumberOfParameters = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    
    #if Dimensions==2
    constexpr int spaceBasisSize     = (Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceBasisSize     = (Order+1)*(Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif
    
    constexpr int spaceTimeBasisSize = spaceBasisSize*(Order+1);
    

    constexpr int totalSize = generated::kernels::AderDG::{{LINEARITY}}::getFusedSTPVISize();  
    
    {{PREDICTOR_COMPUTATION_PRECISION}} memory[totalSize] __attribute__((aligned({{ARCHITECTURE_ALIGNMENT}})));
        
    {{PREDICTOR_COMPUTATION_PRECISION}}* lQi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getlQiShift();
    {{PREDICTOR_COMPUTATION_PRECISION}}* lQhi          = memory + generated::kernels::AderDG::{{LINEARITY}}::getlQhiShift();

    {% if USE_LFI is sameas True %}
    {{PREDICTOR_COMPUTATION_PRECISION}}* lFi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getlFiShift();
    {{PREDICTOR_COMPUTATION_PRECISION}}* lFhi          = memory + generated::kernels::AderDG::{{LINEARITY}}::getlFhiShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISION}}* lFi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISION}}* lFhi          = nullptr;
    {% endif %}    

    {% if USE_LSI is sameas True %}
    {{PREDICTOR_COMPUTATION_PRECISION}}* lSi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getlSiShift();
    {{PREDICTOR_COMPUTATION_PRECISION}}* lShi          = memory + generated::kernels::AderDG::{{LINEARITY}}::getlShiShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISION}}* lSi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISION}}* lShi          = nullptr;
    {% endif %}
    
    {% if USE_GRADQ is sameas True %}
    {{PREDICTOR_COMPUTATION_PRECISION}}* gradQ         = memory + generated::kernels::AderDG::{{LINEARITY}}::getgradQShift(); 
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISION}}* gradQ         = nullptr;
    {% endif %}
    
    {% if USE_PSI is sameas True %}    
    {{PREDICTOR_COMPUTATION_PRECISION}}* PSi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getPSiShift();
    {{PREDICTOR_COMPUTATION_PRECISION}}* PSderivatives = memory + generated::kernels::AderDG::{{LINEARITY}}::getPSderivativesShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISION}}* PSi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISION}}* PSderivatives = nullptr;
    {% endif %}
        
    {{PREDICTOR_COMPUTATION_PRECISION}} lduh[generated::kernels::AderDG::{{LINEARITY}}::getUpdateSize()];
        
    {{PREDICTOR_STORAGE_PRECISION}} lQhbnd[generated::kernels::AderDG::{{LINEARITY}}::getBndFaceTotalSize()]{0.0};
    {{PREDICTOR_STORAGE_PRECISION}} lFhbnd[generated::kernels::AderDG::{{LINEARITY}}::getBndFluxTotalSize()]{0.0};
    //{{PREDICTOR_STORAGE_PRECISION}} lGradQhbnd[generated::kernels::AderDG::{{LINEARITY}}::getBndGradQTotalSize()]{0.0};
    

    {% if IS_LINEAR is sameas True %}
#if Dimensions==2
    double centre[Dimensions] = {marker.x()[0], marker.x()[1]};
#else
    double centre[Dimensions] = {marker.x()[0], marker.x()[1], marker.x()[2]};  
#endif

    {% if USE_POINT_SOURCE!="<none>" %}
    
    std::vector<int>* pointSources = generated::kernels::AderDG::linear::getPointSources(
      repositories::{{SOLVER_INSTANCE}},
      centre,
      marker.h()[0]
    );
    
    int numberOfIterations = 0;
    
    if(pointSources != nullptr) {
        // perform pointsource
        numberOfIterations = generated::kernels::AderDG::linear::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISION}}, {{PREDICTOR_STORAGE_PRECISION}}>(
          repositories::{{SOLVER_INSTANCE}},
          lduh,
          lQhbnd,
          lFhbnd,
          lQi,
          lFi,
          lSi,            // for NCP or Source
          lQhi,
          lFhi,
          lShi,           // for NCP or Source
          gradQ,          // for NCP or Source
          PSi,            // for pointSource
          PSderivatives,  // for pointSource
          luh,
          centre,
          marker.h()[0], // Assume dx[0] == dx[1] == dx[2]
          timeStamp, // tStep, doesn't seem to ever get used
          timeStepSize,
          pointSources // will be deleted in the end if set
        );    
    } else {
        numberOfIterations = generated::kernels::AderDG::linear::fusedSpaceTimePredictorVolumeIntegral_WithoutPS<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISION}}, {{PREDICTOR_STORAGE_PRECISION}}>(
          repositories::{{SOLVER_INSTANCE}},
          lduh,
          lQhbnd,
          lFhbnd,
          lQi,
          lFi,
          lSi,   // for NCP or Source
          lQhi,
          lFhi,
          lShi,  // for NCP or Source
          gradQ, // for NCP or Source
          nullptr, // PSi, for pointSource
          nullptr, // PSderivatives, for pointSource
          luh,
          centre,
          marker.h()[0], // Assume dx[0] == dx[1] == dx[2]
          timeStamp, // tStep, doesn't seem to ever get used
          timeStepSize,
          pointSources // will be deleted in the end if set
        );
    }
    
    
    {% else %}{# point_sources #}
    
    //linear, no pointSources
    int numberOfIterations = generated::kernels::AderDG::linear::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISION}}, {{PREDICTOR_STORAGE_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      lduh,
      lQhbnd,
      lFhbnd,
      lQi,
      lFi,
      lSi,   // for NCP or Source
      lQhi,
      lFhi,
      lShi,  // for NCP or Source
      gradQ, // for NCP or Source
      nullptr, // PSi, for pointSource
      nullptr, // PSderivatives, for pointSource
      luh,
      centre,
      marker.h()[0], // Assume dx[0] == dx[1] == dx[2]
      timeStamp, // tStep, doesn't seem to ever get used
      timeStepSize,
      nullptr // will be deleted in the end if set
    );    

    {% endif %} {# linear #}

    {% else %} {# nonlinear #}
    
    {{PREDICTOR_COMPUTATION_PRECISION}} rhs[(NumberOfVariables+NumberOfParameters)*spaceTimeBasisSize];

    //nonlinear
    int numberOfIterations = generated::kernels::AderDG::nonlinear::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISION}}, {{PREDICTOR_STORAGE_PRECISION}}>(
        repositories::{{SOLVER_INSTANCE}},
        lduh,
        lQhbnd,
        nullptr, //lGradQhbnd,
        lFhbnd,
        lQi,
        rhs,
        lFi,
        lSi,   // for NCP or Source
        lQhi,
        lFhi,
        lShi,  // for NCP or Source
        gradQ, // for NCP or Source or viscousFlux
        nullptr, //gradQAvg, // for viscousFlux
        luh,
        1/(marker.h()[0]), //inverseDx, assume dx[0] == dx[1] == dx[2]
        timeStepSize
      );

    {% endif %} {# nonlinear #}

    generated::kernels::AderDG::{{LINEARITY}}::solutionUpdate( 
      luh,
      luh, 
      lduh, 
      timeStepSize
    );
      
    /*
    Copy the estimates into the face data containers. Note that peano uses a different ordering of
    the faces that the kernels, so these are reordered somewhat.
    The kernels order by dimension, then side of the face whereas peano orders by
    side, then dimension.
    E.g. in 2d the kernels have the order left, right, lower face, upper face
    whereas peano orders: left, lower, right, upper 
    
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */
  
    const int basisElementsByFace  = generated::kernels::AderDG::{{LINEARITY}}::getBndFaceSize();
    const int fluxElementsByFace   = generated::kernels::AderDG::{{LINEARITY}}::getBndFluxSize();

    for(int d=0; d<Dimensions; d++){
      //Projected solution estimations
      std::copy_n(&lQhbnd[2*d*basisElementsByFace], basisElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsByFace);
      std::copy_n(&lQhbnd[(2*d+1)*basisElementsByFace], basisElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value);
      
      //Projected flux estimations
      std::copy_n(&lFhbnd[2*d*fluxElementsByFace], fluxElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsByFace);
      std::copy_n(&lFhbnd[(2*d+1)*fluxElementsByFace], fluxElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value);            
    }
  

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
    super(Prediction_kernelgenerator,self).__init__(solver)
    self.guard      = guard
    self._use_lfi   = solver._is_linear or solver._flux_implementation!="<none>"
    self._use_lsi   = solver._source_term_implementation!="<none>" or (not solver._is_linear and solver._ncp_implementation!="<none>")
    self._use_gradQ = solver._ncp_implementation!="<none>"
    self._use_psi   = solver._is_linear and solver._point_sources_implementation!="<none>"

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]  = self.guard
      d[ "USE_LFI" ]    = self._use_lfi
      d[ "USE_LSI" ]    = self._use_lsi
      d[ "USE_GRADQ" ]  = self._use_gradQ
      d[ "USE_PSI" ]    = self._use_psi
      
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
    
    linearity = ( "linear" if self._solver._is_linear else "nonlinear" )
    return super(Prediction_kernelgenerator,self).get_includes() + """
#include "../generated/kernels/aderdg/""" + linearity + """/Kernels.h"
      """