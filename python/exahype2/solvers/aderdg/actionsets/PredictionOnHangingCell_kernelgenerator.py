# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet
from ..kernels import create_array_and_convert_from_type

#import peano4
import peano4.solversteps
import jinja2

class PredictionOnHangingCell_kernelgenerator(AbstractAderDGActionSet):
  """
  
  The extrapolated solution from the space-time predictor has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection 
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  
  """
  
  TemplateProject = """

  bool isHangingCell = false;
  for(int d=0; d<2*Dimensions; d++){
    if(fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()){
      isHangingCell = true;
      break;
    }
  }

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

    {% if NUMBER_OF_PREDICTOR_PRECISIONS!=1 %}
    ::exahype2::PrecisionCommand myPrecision = repositories::{{SOLVER_INSTANCE}}.precisionCriterion(
      luh,
      marker.x(),
      marker.h(),
      timeStepSize,
      repositories::{{SOLVER_INSTANCE}}.getSolverState()
    );

    switch(myPrecision){
    {% endif %}

    {% for PRECISION_NUM in range(0,PREDICTOR_COMPUTATION_PRECISIONS|length) %}
    {% if NUMBER_OF_PREDICTOR_PRECISIONS!=1 %}
    case ::exahype2::PrecisionCommand::{{PRECISIONS_CAPITALIZED[PRECISION_NUM]}}:
    {
    {% endif %}
    
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} memory[totalSize] __attribute__((aligned({{ARCHITECTURE_ALIGNMENT}})));
        
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lQi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getlQiShift();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lQhi          = memory + generated::kernels::AderDG::{{LINEARITY}}::getlQhiShift();

    {% if USE_LFI is sameas True %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getlFiShift();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFhi          = memory + generated::kernels::AderDG::{{LINEARITY}}::getlFhiShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFhi          = nullptr;
    {% endif %}    

    {% if USE_LSI is sameas True %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lSi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getlSiShift();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lShi          = memory + generated::kernels::AderDG::{{LINEARITY}}::getlShiShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lSi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lShi          = nullptr;
    {% endif %}
    
    {% if USE_GRADQ is sameas True %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* gradQ         = memory + generated::kernels::AderDG::{{LINEARITY}}::getgradQShift(); 
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* gradQ         = nullptr;
    {% endif %}
    
    {% if USE_PSI is sameas True %}    
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* PSi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getPSiShift();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* PSderivatives = memory + generated::kernels::AderDG::{{LINEARITY}}::getPSderivativesShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* PSi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* PSderivatives = nullptr;
    {% endif %}
        
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} lduh[generated::kernels::AderDG::{{LINEARITY}}::getUpdateSize()]{0.0};
        
    {{CORRECTOR_COMPUTATION_PRECISION}} lQhbnd[generated::kernels::AderDG::{{LINEARITY}}::getBndFaceTotalSize()]{0.0};
    {{CORRECTOR_COMPUTATION_PRECISION}} lFhbnd[generated::kernels::AderDG::{{LINEARITY}}::getBndFluxTotalSize()]{0.0};
    //{{CORRECTOR_COMPUTATION_PRECISION}} lGradQhbnd[generated::kernels::AderDG::{{LINEARITY}}::getBndGradQTotalSize()]{0.0};

    {% if IS_LINEAR is sameas True %}

    {% if USE_POINT_SOURCE!="<none>" %}
    
    std::vector<int>* pointSources = generated::kernels::AderDG::linear::getPointSources(
      repositories::{{SOLVER_INSTANCE}},
      marker.x(),
      marker.h()
    );
    
    int numberOfIterations = 0;
    
    if(pointSources != nullptr) {
        // perform pointsource
        numberOfIterations = generated::kernels::AderDG::linear::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
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
          marker.x(),
          marker.h(),
          timeStamp, // tStep, doesn't seem to ever get used
          timeStepSize,
          pointSources // will be deleted in the end if set
        );    
    } else {
        numberOfIterations = generated::kernels::AderDG::linear::fusedSpaceTimePredictorVolumeIntegral_WithoutPS<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
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
          marker.x(),
          marker.h(),
          timeStamp,
          timeStepSize,
          pointSources // will be deleted in the end if set
        );
    }
    
    
    {% else %}{# point_sources #}
    
    //linear, no pointSources
    int numberOfIterations = generated::kernels::AderDG::linear::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
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
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize,
      nullptr // will be deleted in the end if set
    );    

    {% endif %} {# linear #}

    {% else %} {# nonlinear #}
    
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} rhs[(NumberOfVariables+NumberOfParameters)*spaceTimeBasisSize];

    //nonlinear
    int numberOfIterations = generated::kernels::AderDG::nonlinear::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
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
        marker.x(),
        marker.h(),
        timeStamp,
        timeStepSize
      );

    {% endif %} {# nonlinear #}
    
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
  
    const int basisElementsPerFace  = generated::kernels::AderDG::{{LINEARITY}}::getBndFaceSize();
    const int fluxElementsPerFace   = generated::kernels::AderDG::{{LINEARITY}}::getBndFluxSize();

    for(int d=0; d<Dimensions; d++){
    
      //Left values
      if(fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()){
        //compute riemann solution and face integral
        const double inverseDxDirection = 1/marker.h()[d];
        {% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.riemannSolver(
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value, //FL
          &lFhbnd[2*d*fluxElementsPerFace],            //FR
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value,     //QL
          &lQhbnd[2*d*basisElementsPerFace],           //QR
          timeStamp,
          timeStepSize,
          marker.x(),
          marker.h(),
          d, //direction
          false,
          d
        );
        {% else %}
        generated::kernels::AderDG::{{LINEARITY}}::riemannSolver<double>(
          repositories::{{SOLVER_INSTANCE}},
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value, //FL
          &lFhbnd[2*d*fluxElementsPerFace],            //FR
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value,     //QL
          &lQhbnd[2*d*basisElementsPerFace],           //QR
          timeStamp,
          timeStepSize,
          marker.x(), //TODO: fix exact position
          marker.h(),
          d //direction
        );
        {% endif %}

        generated::kernels::AderDG::{{LINEARITY}}::faceIntegral<{{CORRECTOR_COMPUTATION_PRECISION}}>(
          lduh,
          &lFhbnd[2*d*fluxElementsPerFace],
          d, //direction
          0, //orientation
          inverseDxDirection
        );
      }
      else{
        //project to faces and update face timestamp
        std::copy_n(&lQhbnd[2*d*basisElementsPerFace], basisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsPerFace);
        std::copy_n(&lFhbnd[2*d*fluxElementsPerFace], fluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsPerFace);
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setNewTimeStamp(1, timeStamp);

      }

      //Right values
      if(fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).getIsHanging()){
        //compute riemann solution and face integral
        const double inverseDxDirection = 1/marker.h()[d];

        {% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.riemannSolver(
          &lFhbnd[(2*d+1)*fluxElementsPerFace],//FL
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value+fluxElementsPerFace,//FR
          &lQhbnd[(2*d+1)*basisElementsPerFace],//QL
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value+basisElementsPerFace,//QR
          timeStamp,
          timeStepSize,
          marker.x(),
          marker.h(),
          d, //direction
          false,
          d+Dimensions
        );
        {% else %}
        generated::kernels::AderDG::{{LINEARITY}}::riemannSolver<double>(
          repositories::{{SOLVER_INSTANCE}},
          &lFhbnd[(2*d+1)*fluxElementsPerFace],//FL
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value+fluxElementsPerFace,//FR
          &lQhbnd[(2*d+1)*basisElementsPerFace],//QL
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value+basisElementsPerFace,//QR
          timeStamp,
          timeStepSize,
          marker.x(), //TODO: fix exact position
          marker.h(),
          d //direction
        );
        {% endif %}

        generated::kernels::AderDG::{{LINEARITY}}::faceIntegral<{{CORRECTOR_COMPUTATION_PRECISION}}>(
          lduh,
          &lFhbnd[(2*d+1)*fluxElementsPerFace],
          d, //direction
          1, //orientation
          inverseDxDirection
        );

      }
      else{

        //project to faces and update face timestamp
        std::copy_n(&lFhbnd[(2*d+1)*fluxElementsPerFace], fluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value);
        std::copy_n(&lQhbnd[(2*d+1)*basisElementsPerFace], basisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value);
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).setNewTimeStamp(0, timeStamp);

      }
            
    }

    {% if NUMBER_OF_PREDICTOR_PRECISIONS!=1 %}
      break;
      }
    {% endif %}

    {% endfor %}

    {% if NUMBER_OF_PREDICTOR_PRECISIONS!=1 %}
      default:
        assertion1WithExplanation(false, myPrecision, " chosen precision was not in the previously specified options, as such there are no compiled kernels for this option");
        assert(false);
      }
    {% endif %}

    generated::kernels::AderDG::{{LINEARITY}}::solutionUpdate( 
      luh,
      luh, 
      lduh, 
      timeStepSize
    );
  
    repositories::{{SOLVER_INSTANCE}}.update(timeStepSize, timeStamp, marker.h()(0) );
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
    super(PredictionOnHangingCell_kernelgenerator,self).__init__(solver)
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
      d[ "PRECISIONS_CAPITALIZED" ] = [x.replace(" ", "_").capitalize() for x in d["PREDICTOR_COMPUTATION_PRECISIONS"]]
      
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
    
    linearity = ( "linear" if self._solver._is_linear else "nonlinear" )
    return super(PredictionOnHangingCell_kernelgenerator,self).get_includes() + """
#include "../generated/kernels/aderdg/""" + linearity + """/Kernels.h"
      """