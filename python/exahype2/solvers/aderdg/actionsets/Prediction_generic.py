# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet
from ..kernels import create_array_and_convert_from_type

#import peano4
import peano4.solversteps
import jinja2


class Prediction_generic(AbstractAderDGActionSet):
  """
  The extrapolated solution from the space-time predictor has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  """

  TemplateProject = """
  bool isHangingCell = false;
  for (int d = 0; d < 2 * Dimensions; d++) {
    if (fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()) {
      isHangingCell = true;
      break;
    }
  }

  if ({{PREDICATE}}) {
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    // Needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}

    {{SOLUTION_STORAGE_PRECISION}}* luh = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;

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

    constexpr int spaceTimeBasisSize = spaceBasisSize*(Order+1);
    constexpr int sizeLQi   = (NumberOfVariables+NumberOfParameters)*spaceTimeBasisSize;
    constexpr int sizeRhs   = sizeLQi;
    constexpr int sizeLFi   = (Dimensions + 1)*NumberOfVariables*spaceTimeBasisSize; //Dimensions+1 because this is also used for lShi, which contains source terms. Don't ask me why that isn't it's own variable
    constexpr int sizeGradQ = Dimensions*NumberOfVariables*spaceBasisSize;

    constexpr int sizeLQhi = (NumberOfVariables+NumberOfParameters)*spaceBasisSize;
    constexpr int sizeLFhi = (Dimensions + 1)*NumberOfVariables*spaceBasisSize;

    constexpr int totalSize = sizeLQi + sizeRhs + sizeLFi + sizeGradQ + sizeLQhi + sizeLFhi + sizeLQhi;

    {% if NUMBER_OF_PREDICTOR_PRECISIONS!=1 %}
    ::exahype2::PrecisionCommand myPrecision = repositories::{{SOLVER_INSTANCE}}.precisionCriterion(
      luh,
      marker.x(),
      marker.h(),
      timeStepSize,
      repositories::{{SOLVER_INSTANCE}}.getSolverState()
    );

    switch(myPrecision) {
    {% endif %}

    {% for PRECISION_NUM in range(0,PREDICTOR_COMPUTATION_PRECISIONS|length) %}
    {% if NUMBER_OF_PREDICTOR_PRECISIONS!=1 %}
    case ::exahype2::PrecisionCommand::{{PRECISIONS_CAPITALIZED[PRECISION_NUM]}}:
    {
    {% endif %}

    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} block[totalSize] __attribute__((aligned({{ARCHITECTURE_ALIGNMENT}})));
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* memory = block;

    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lQi   = memory; memory += sizeLQi;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* rhs   = memory; memory += sizeRhs;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFi   = memory; memory += sizeLFi;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* gradQ = memory; memory += sizeGradQ;

    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lQhi = memory; memory += sizeLQhi;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFhi = memory; memory += sizeLFhi;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lduh = memory; memory += sizeLQhi;

    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} lQhbnd[2*Dimensions*spaceFaceSize*(NumberOfVariables+NumberOfParameters)]{0.0};
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} lFhbnd[2*Dimensions*spaceFaceSize*NumberOfVariables]{0.0};
    //{{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} lGradQhbnd[2*Dimensions*Dimensions*spaceFaceSize*(NumberOfVariables+NumberOfParameters)]{0.0};

    {% if IS_LINEAR is sameas True %}
    // template <bool EvaluatePointSource, bool EvaluateAlgebraicSource, bool EvaluateFlux,
    //   bool EvaluateNonconservativeProduct, bool EvaluateMaterialParameters, class SolverType>

    ::exahype2::aderdg::generic::spaceTimePredictorLinear<
      false, // EvaluatePointSource
      {{ "true" if USE_SOURCE!="<none>" else "false" }}, // EvaluateAlgebraicSource
      {{ "true" if USE_FLUX!="<none>" else "false" }}, // EvaluateFlux
      {{ "true" if USE_NCP!="<none>" else "false" }}, // EvaluateNonconservativeProduct
      {{ "true" if USE_MATERIAL_PARAMETERS!="<none>" else "false" }}, // EvaluateMaterialParameters
      {{SOLVER_NAME}},
      {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, //predictor_computation_precision
      {{SOLUTION_STORAGE_PRECISION}} //predictor_storage_precision
      >(
        repositories::{{SOLVER_INSTANCE}},
        lQhbnd,
        lFhbnd,
        lQi,
        lFi,
        gradQ,
        nullptr, // PSi
        nullptr, // PSderivatives
        nullptr, // tmp_PSderivatives
        lQhi,
        lFhi,
        luh,
        marker.x(), // cellCentre
        marker.h(), // cellSize
        timeStamp,
        timeStepSize
      );
      int numberOfIterations = 1;
    {% else %}
    //template <bool useSource, bool useFlux, bool useViscousFlux, bool useNCP, typename SolverType>
    int numberOfIterations = kernels::aderdg::generic::c::spaceTimePredictorNonlinear<
      {{ "true" if USE_SOURCE!="<none>" else "false" }}, //source
      {{ "true" if USE_FLUX!="<none>" else "false" }}, //useFlux
      false, //viscousFlux
      {{ "true" if USE_NCP!="<none>" else "false" }}, //useNCP
      false, //noTimeAveraging
      {{SOLVER_NAME}},
      {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, //predictor_computation_precision
      {{SOLUTION_STORAGE_PRECISION}} //predictor_storage_precision
      >(
        repositories::{{SOLVER_INSTANCE}},
        lQhbnd,
        nullptr,
        lFhbnd,
        lQi,
        rhs,
        lFi,
        gradQ,
        lQhi,
        lFhi,
        luh,
        marker.x(), //cellCentre
        marker.h(), //cellSize
        timeStamp,
        timeStepSize
      );
    {% endif %}

    {% if IS_LINEAR is sameas True %}
    //template <typename SolverType,bool useSource, bool useFlux, int numberOfVariables, int basisSize>
    ::exahype2::aderdg::generic::volumeIntegralLinear<
      {{SOLVER_NAME}},
      {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, //predictor_computation_precision
      {{ "true" if USE_SOURCE!="<none>" else "false" }}, //useSource
      {{ "true" if USE_FLUX!="<none>" else "false" }}, //useFlux
      {{NUMBER_OF_UNKNOWNS}},
      {{ORDER}}+1>(
        lduh,
        lFhi,
        marker.h() //cellSize
        );
    {% else %}
    //template <typename SolverType, bool useSourceOrNCP, bool useFlux, bool noTimeAveraging, const int numberOfVariables, int basisSize>
    kernels::aderdg::generic::c::volumeIntegralNonlinear<
      {{SOLVER_NAME}},
      {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, //predictor_computation_precision
      {{ "true" if USE_NCP!="<none>" or USE_SOURCE!="<none>" else "false" }}, //useSourceOrNCP
      {{ "true" if USE_FLUX!="<none>" else "false" }}, //useFlux
      false, //noTimeAveraging
      {{NUMBER_OF_UNKNOWNS}},
      {{ORDER}}+1>(
        lduh,
        lFhi,
        marker.h() //cellSize
    );
    {% endif %}

    ::exahype2::aderdg::generic::solutionUpdate<
    {{SOLVER_NAME}},
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} //predictor_computation_precision
      >(
        repositories::{{SOLVER_INSTANCE}},
        luh,
        luh,
        lduh,
        timeStepSize
      );

    /**
     * Copy the estimates into the face data containers. Note that Peano uses a different ordering of
     * the faces that the kernels, so these are reordered somewhat.
     * The kernels order by dimension, then side of the face whereas Peano orders by
     * side, then dimension.
     * E.g., in 2d the kernels have the order left, right, lower face, upper face
     * whereas Peano orders: left, lower, right, upper.
     *
     * 2d: 0,1,2,3 -> 0,2,1,3
     * 3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
     */

    const int basisElementsByFace  = spaceFaceSize*(NumberOfVariables+NumberOfParameters);
    const int fluxElementsByFace   = spaceFaceSize*NumberOfVariables;

    for (int d=0; d<Dimensions; d++) {
      // Projected solution estimations
      std::copy_n(&lQhbnd[2*d*basisElementsByFace], basisElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsByFace);
      std::copy_n(&lQhbnd[(2*d+1)*basisElementsByFace], basisElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value);

      // Projected flux estimations
      std::copy_n(&lFhbnd[2*d*fluxElementsByFace], fluxElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsByFace);
      std::copy_n(&lFhbnd[(2*d+1)*fluxElementsByFace], fluxElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value);            

      // Timestamp information for boundary handling
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setNewTimeStamp(1, timeStamp);
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).setNewTimeStamp(0, timeStamp);
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

    repositories::{{SOLVER_INSTANCE}}.update(timeStepSize, timeStamp, marker.h()(0));
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
    super(Prediction_generic,self).__init__(solver)
    self.guard            = guard


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]                  = self.guard
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
    return super(Prediction_generic,self).get_includes() + """
      """
