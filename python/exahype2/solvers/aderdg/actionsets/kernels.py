# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

import jinja2

def create_predictor_allocations(use_kernelgenerator=True):
    Template_generic = """
    const int basisElementsPerFace  = spaceFaceSize*(NumberOfVariables+NumberOfParameters);
    const int fluxElementsPerFace   = spaceFaceSize*NumberOfVariables;

    constexpr int sizeLQi   = (NumberOfVariables+NumberOfParameters)*spaceTimeBasisSize;
    constexpr int sizeRhs   = sizeLQi;
    constexpr int sizeLFi   = (Dimensions + 1)*NumberOfVariables*spaceTimeBasisSize; //Dimensions+1 because this is also used for lShi, which contains source terms. Don't ask me why that isn't it's own variable
    constexpr int sizeGradQ = Dimensions*NumberOfVariables*spaceBasisSize;
    
    constexpr int sizeLQhi = (NumberOfVariables+NumberOfParameters)*spaceBasisSize;
    constexpr int sizeLFhi = (Dimensions + 1)*NumberOfVariables*spaceBasisSize;

    constexpr int sizeLduh = sizeLQhi;
    
    constexpr int totalSize = sizeLQi + sizeRhs + sizeLFi + sizeGradQ +
                          sizeLQhi + sizeLFhi + sizeLduh;
  
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} block[totalSize] __attribute__((aligned({{ARCHITECTURE_ALIGNMENT}})));
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* memory = block;
        
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lQi   = memory; memory+=sizeLQi;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* rhs   = memory; memory+=sizeRhs;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFi   = memory; memory+=sizeLFi;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* gradQ = memory; memory+=sizeGradQ;
    
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lQhi = memory; memory+=sizeLQhi;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFhi = memory; memory+=sizeLFhi;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lduh = memory; memory+=sizeLduh;
    
    {{PREDICTOR_STORAGE_PRECISION}} lQhbnd[2*Dimensions*spaceFaceSize*(NumberOfVariables+NumberOfParameters)]{0.0};
    {{PREDICTOR_STORAGE_PRECISION}} lFhbnd[2*Dimensions*spaceFaceSize*NumberOfVariables]{0.0};
"""

    Template_kernelgenerator = """
    const int basisElementsPerFace  = generated::kernels::AderDG::{{LINEARITY}}::getBndFaceSize();
    const int fluxElementsPerFace   = generated::kernels::AderDG::{{LINEARITY}}::getBndFluxSize();

    constexpr int totalSize = generated::kernels::AderDG::{{LINEARITY}}::getFusedSTPVISize();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} memory[totalSize] __attribute__((aligned({{ARCHITECTURE_ALIGNMENT}})));
        
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lQi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getlQiShift();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lQhi          = memory + generated::kernels::AderDG::{{LINEARITY}}::getlQhiShift();

    {% if LINEARITY=="linear" or USE_FLUX!="<none>" %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getlFiShift();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFhi          = memory + generated::kernels::AderDG::{{LINEARITY}}::getlFhiShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFhi          = nullptr;
    {% endif %}    

    {% if USE_SOURCE!="<none>" or (LINEARITY=="nonlinear" and USE_NCP!="<none>") %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lSi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getlSiShift();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lShi          = memory + generated::kernels::AderDG::{{LINEARITY}}::getlShiShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lSi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lShi          = nullptr;
    {% endif %}
    
    {% if USE_NCP!="<none>" %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* gradQ         = memory + generated::kernels::AderDG::{{LINEARITY}}::getgradQShift(); 
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* gradQ         = nullptr;
    {% endif %}
    
    {% if LINEARITY=="linear" and USE_POINT_SOURCE!="<none>" %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* PSi           = memory + generated::kernels::AderDG::{{LINEARITY}}::getPSiShift();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* PSderivatives = memory + generated::kernels::AderDG::{{LINEARITY}}::getPSderivativesShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* PSi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* PSderivatives = nullptr;
    {% endif %}
        
    constexpr int sizeLduh = generated::kernels::AderDG::{{LINEARITY}}::getUpdateSize();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} lduh[sizeLduh]{({{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}})0.0};

    {{CORRECTOR_COMPUTATION_PRECISION}} lQhbnd[generated::kernels::AderDG::{{LINEARITY}}::getBndFaceTotalSize()]{({{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}})0.0};
    {{CORRECTOR_COMPUTATION_PRECISION}} lFhbnd[generated::kernels::AderDG::{{LINEARITY}}::getBndFluxTotalSize()]{({{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}})0.0};
"""
    return (Template_kernelgenerator if use_kernelgenerator else Template_generic)

def create_fstpvi_call(is_linear, usePointSource, useKernelGenerator=True):
    Template_generic_linear = """
    //template <bool usePointSource, bool useSource, bool useFlux, bool useNCP, bool useMM,typename SolverType>
    kernels::aderdg::generic::c::spaceTimePredictorLinear<
      false, //usePointSource
      {{ "true" if USE_SOURCE!="<none>" else "false" }}, //source
      {{ "true" if USE_FLUX!="<none>" else "false" }}, //useFlux
      {{ "true" if USE_NCP!="<none>" else "false" }}, //useNCP
      false, //useMM
      {{SOLVER_NAME}},
      {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, //predictor_computation_precision
      {{PREDICTOR_STORAGE_PRECISION}} //predictor_storage_precision
      >(
        repositories::{{SOLVER_INSTANCE}},
        lQhbnd, lFhbnd,
        lQi, lFi, gradQ,
        nullptr, nullptr, nullptr, //PSi, PSderivatives, tmp_PSderivatives
        lQhi, lFhi,
        luh,
        marker.x(), marker.h(), timeStamp, timeStepSize
    );

    //template <typename SolverType,bool useSource, bool useFlux, int numberOfVariables, int basisSize>
    kernels::aderdg::generic::c::volumeIntegralLinear<
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
    """

    Template_generic_nonlinear = """
    //template <bool useSource, bool useFlux, bool useViscousFlux, bool useNCP, typename SolverType>
    int numberOfIterations = kernels::aderdg::generic::c::spaceTimePredictorNonlinear<
      {{ "true" if USE_SOURCE!="<none>" else "false" }}, //source
      {{ "true" if USE_FLUX!="<none>" else "false" }}, //useFlux
      false, //viscousFlux
      {{ "true" if USE_NCP!="<none>" else "false" }}, //useNCP
      false, //noTimeAveraging
      {{SOLVER_NAME}},
      {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, //predictor_computation_precision
      {{PREDICTOR_STORAGE_PRECISION}} //predictor_storage_precision
      >(
        repositories::{{SOLVER_INSTANCE}},
        lQhbnd, nullptr, lFhbnd,
        lQi, rhs, lFi,
        gradQ, lQhi, lFhi,
        luh,
        marker.x(), marker.h(), timeStamp, timeStepSize
    );

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
"""
    Template_kernelgenerator_linear_no_ps = """
    //linear, no pointSources
    int numberOfIterations = generated::kernels::AderDG::linear::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      lduh, lQhbnd, lFhbnd,
      lQi, lFi, lSi,
      lQhi, lFhi, lShi,
      gradQ, nullptr, nullptr,
      luh,
      marker.x(), marker.h(), timeStamp, timeStepSize,
      nullptr
    );
"""

    Template_kernelgenerator_nonlinear = """
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} rhs[(NumberOfVariables+NumberOfParameters)*spaceTimeBasisSize];

    //nonlinear
    int numberOfIterations = generated::kernels::AderDG::nonlinear::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      lduh, lQhbnd, nullptr, lFhbnd,
      lQi, rhs, lFi, lSi,
      lQhi, lFhi, lShi,
      gradQ, nullptr,
      luh,
      marker.x(), marker.h(), timeStamp, timeStepSize
    );
"""

    Template_kernelgenerator_linear_w_ps = """
    std::vector<int>* pointSources = generated::kernels::AderDG::linear::getPointSources(
      repositories::{{SOLVER_INSTANCE}},
      marker.x(),
      marker.h()
    );
    
    if(pointSources != nullptr) {
        // perform pointsource
        int numberOfIterations = generated::kernels::AderDG::linear::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
          repositories::{{SOLVER_INSTANCE}},
          lduh, lQhbnd, lFhbnd,
          lQi, lFi, lSi,
          lQhi, lFhi, lShi,
          gradQ, PSi, PSderivatives,
          luh,
          marker.x(), marker.h(), timeStamp, timeStepSize,
          pointSources // will be deleted in the end if set
        );    
    } else {
        int numberOfIterations = generated::kernels::AderDG::linear::fusedSpaceTimePredictorVolumeIntegral_WithoutPS<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
          repositories::{{SOLVER_INSTANCE}},
          lduh, lQhbnd, lFhbnd,
          lQi, lFi, lSi,
          lQhi, lFhi, lShi,
          gradQ, nullptr, nullptr,
          luh,
          marker.x(), marker.h(), timeStamp, timeStepSize,
          pointSources // will be deleted in the end if set
        );
    }
"""
    #decides which of the five options to provide
    if(useKernelGenerator):
        if(is_linear):
            if(usePointSource):
                return Template_kernelgenerator_linear_w_ps
            else:
                return Template_kernelgenerator_linear_no_ps
        else:
            return Template_kernelgenerator_nonlinear
    else:
        if(is_linear):
            return Template_generic_linear
        else:
            return Template_generic_nonlinear


def create_solution_update(use_kernelgenerator):
    template_generic = """
    kernels::aderdg::generic::c::solutionUpdate<
    {{SOLVER_NAME}},
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} //predictor_computation_precision
      >(
      repositories::{{SOLVER_INSTANCE}},
      luh,
      luh,
      lduh,
      timeStepSize
    );
"""
    template_kernelgenerator = """
    generated::kernels::AderDG::{{LINEARITY}}::solutionUpdate( 
      luh,
      luh, 
      lduh, 
      timeStepSize
    );
"""
    return (template_kernelgenerator if use_kernelgenerator else template_generic)

def reset_troubled_markers():
    return """
    // reset cell and face labels for limiting
    fineGridCell{{SOLVER_NAME}}CellLabel.setTroubled_Marker(celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::ADER);

    for(int d=0; d<Dimensions; d++){
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::ADER);
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::ADER);
    }
"""

def compute_local_min_and_max_and_project():
    return """
    constexpr int numberOfObservables = repositories::{{SOLVER_INSTANCE}}.NumberOfDMPObservables;
    double localMinPerVariables[numberOfObservables], localMaxPerVariables[numberOfObservables];
    generated::kernels::limiter::findCellLocalMinAndMax(
      fineGridCell{{SOLVER_NAME}}Q_old.value,
      repositories::{{SOLVER_INSTANCE}},
      localMinPerVariables, localMaxPerVariables
    );

    for(int d=0; d<Dimensions; d++){
        //Projection of cell local min and max
        std::copy_n(localMinPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value+numberOfObservables);
        std::copy_n(localMinPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value);
        std::copy_n(localMaxPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value+3*numberOfObservables);
        std::copy_n(localMaxPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value+2*numberOfObservables);
    }
"""

def copy_estimates_into_faces():
    return """
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

    for(int d=0; d<Dimensions; d++){
        //Projected solution estimations
        std::copy_n(&lQhbnd[2*d*basisElementsPerFace], basisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsPerFace);
        std::copy_n(&lQhbnd[(2*d+1)*basisElementsPerFace], basisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value);
        
        //Projected flux estimations
        std::copy_n(&lFhbnd[2*d*fluxElementsPerFace], fluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsPerFace);
        std::copy_n(&lFhbnd[(2*d+1)*fluxElementsPerFace], fluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value);            

        //Timestamp information for boundary handling
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setNewTimeStamp(1, timeStamp);
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).setNewTimeStamp(0, timeStamp);
    }
"""


def number_of_precisions_iterator(iterator_content):
    Template = jinja2.Template(
        """
    ::exahype2::PrecisionCommand myPrecision = repositories::{{SOLVER_INSTANCE}}.precisionCriterion(
      luh,
      marker.x(),
      marker.h(),
      timeStepSize,
      repositories::{{SOLVER_INSTANCE}}.getSolverState()
    );

    switch(myPrecision){
    {% for PRECISION_NUM in range(0,PREDICTOR_COMPUTATION_PRECISIONS|length) %}
        case ::exahype2::PrecisionCommand::{{PRECISIONS_CAPITALIZED[PRECISION_NUM]}}:
        {

{{ITERATION_CONTENT}}

        break;
        }
    {% endfor %}
        default:
            assertion1WithExplanation(false, myPrecision, " chosen precision was not in the previously specified options, as such there are no compiled kernels for this option");
            assert(false);
    }
""",
        undefined=jinja2.DebugUndefined,
    )
    d = {}
    d["ITERATION_CONTENT"]    = iterator_content
    return Template.render(**d)


def riemann_solver(use_custom_riemann_solver, use_kernelgenerator, is_linear):
    template_custom = """
    repositories::{{SOLVER_INSTANCE}}.riemannSolver(
      FL,
      FR,
      QL,
      QR,
      timeStamp,
      timeStepSize,
      marker.x(),
      faceCentre,
      direction,
      isBoundary,
      faceNumber
    );
"""
    template_generic_linear = """
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
        direction
    );
"""
    template_generic_nonlinear = """
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
        direction
    );
"""
    template_kernelgenerator = """
    generated::kernels::AderDG::{{LINEARITY}}::riemannSolver<{{CORRECTOR_COMPUTATION_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      FL,
      FR,
      QL,
      QR,
      timeStamp,
      timeStepSize,
      faceCentre,
      marker.h(),
      direction
    );
"""
    if use_custom_riemann_solver:
        return template_custom
    elif use_kernelgenerator:
        return template_kernelgenerator
    elif is_linear:
        return template_generic_linear
    else:
        return template_generic_nonlinear

def face_integral(use_kernelgenerator, is_linear):
    template_generic_linear = """
        kernels::aderdg::generic::c::faceIntegralLinear<
          {{SOLVER_NAME}},
          {{NUMBER_OF_UNKNOWNS}},
          {{ORDER}}+1>(
            lduh,
            FIN,
            direction,
            orientation,
            marker.h()
        );
"""
    template_generic_nonlinear = """
        kernels::aderdg::generic::c::faceIntegralNonlinear<
          {{SOLVER_NAME}},
          {{NUMBER_OF_UNKNOWNS}},
          {{ORDER}}+1>(
            lduh,
            FIN,
            direction,
            orientation,
            marker.h()
        );
"""
    template_kernelgenerator = """
        const double inverseDxDirection = 1/marker.h()[d];
        generated::kernels::AderDG::{{LINEARITY}}::faceIntegral<{{CORRECTOR_COMPUTATION_PRECISION}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
          lduh,
          FIN,
          direction,
          orientation,
          inverseDxDirection
        );
"""
    if use_kernelgenerator:
        return template_kernelgenerator
    elif is_linear:
        return template_generic_linear
    else:
        return template_generic_nonlinear
    
def copy_estimates_and_perform_riemann_solution_on_hanging_faces(use_custom_riemann_solver, use_kernelgenerator, is_linear):
    Template = """
    /*
    Copy the estimates into the face data containers. Note that peano uses a different ordering of
    the faces that the kernels, so these are reordered somewhat.
    The kernels order by dimension, then side of the face whereas peano orders by
    side, then dimension.
    E.g. in 2d the kernels have the order left, right, lower face, upper face
    whereas peano orders: left, lower, right, upper 
    
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5

    On hanging faces, this insteads computes the riemann solution and face integral, then
    stores the solution of this into the face. The reason for this is that performing the
    riemann solution on the hanging cell and restricting these is much more accurate than
    performing them on the coarse faces and projecting those to the fine faces, to the
    degree that the later breaks many simulations.
    */

    for(int d=0; d<Dimensions; d++){

        int direction   = d;

        //Left values
        if(fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()){
            {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* FL = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value;
            {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* FR = &lFhbnd[2*d*fluxElementsPerFace];
            {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* QL = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value;
            {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* QR = &lQhbnd[2*d*basisElementsPerFace];
            int orientation = 0;
            int faceNumber  = d;
            bool isBoundary = false;
            tarch::la::Vector<Dimensions,double> faceCentre = marker.x();
            faceCentre[d] -= 0.5*marker.h()[d];

    """ + riemann_solver(use_custom_riemann_solver, use_kernelgenerator, is_linear) + """

            {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* FIN = FR;

    """ + face_integral(use_kernelgenerator, is_linear) + """

        }
        else{
            //project to faces and update face timestamp
            std::copy_n(&lQhbnd[2*d*basisElementsPerFace], basisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsPerFace);
            std::copy_n(&lFhbnd[2*d*fluxElementsPerFace], fluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsPerFace);
            fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setNewTimeStamp(1, timeStamp);
        }

        //Right values
        if(fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).getIsHanging()){
            {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* FL = &lFhbnd[(2*d+1)*fluxElementsPerFace];
            {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* FR = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value+fluxElementsPerFace;
            {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* QL = &lQhbnd[(2*d+1)*basisElementsPerFace];
            {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* QR = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value+basisElementsPerFace;
            int orientation = 0;
            int faceNumber  = d+Dimensions;
            bool isBoundary = false;
            tarch::la::Vector<Dimensions,double> faceCentre = marker.x();
            faceCentre[d] += 0.5*marker.h()[d];
        
    """ + riemann_solver(use_custom_riemann_solver, use_kernelgenerator, is_linear) + """

            {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* FIN = FL;

    """ + face_integral(use_kernelgenerator, is_linear) + """
        
        }
        else{
            //project to faces and update face timestamp
            std::copy_n(&lFhbnd[(2*d+1)*fluxElementsPerFace], fluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value);
            std::copy_n(&lQhbnd[(2*d+1)*basisElementsPerFace], basisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value);
            fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).setNewTimeStamp(0, timeStamp);
        }

    }

"""
    d = {}
    d["RIEMANN_SOLVER"] = riemann_solver(use_custom_riemann_solver, use_kernelgenerator, is_linear)
    d["FACE_INTEGRAL"] = face_integral(use_kernelgenerator, is_linear)
    return Template#Template.render(**d)

def check_troubledness(usePAC, useDMP, useTroubledMarkers):
    templatePAC = """
    isUnTroubled &= generated::kernels::limiter::isPhysicallyAdmissible(
      luh,
      repositories::{{SOLVER_INSTANCE}},
      marker.x(),
      marker.h(),
      timeStamp+timeStepSize
    );
    """
    template_DMP = """
    constexpr int numberOfObservables = repositories::{{SOLVER_INSTANCE}}.NumberOfDMPObservables;
    double boundaryMinPerVariables[2*Dimensions*numberOfObservables];
    double boundaryMaxPerVariables[2*Dimensions*numberOfObservables];

    for(int d=0; d<Dimensions; d++){
        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value,                                 numberOfObservables, &boundaryMinPerVariables[d*numberOfObservables]);
        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value+numberOfObservables,  numberOfObservables, &boundaryMinPerVariables[(d+Dimensions)*numberOfObservables]);

        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value+2*numberOfObservables,            numberOfObservables, &boundaryMaxPerVariables[d*numberOfObservables]);
        std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value+3*numberOfObservables, numberOfObservables, &boundaryMaxPerVariables[(d+Dimensions)*numberOfObservables]);
    }

    isUnTroubled &= generated::kernels::limiter::discreteMaximumPrincipleAndMinAndMaxSearch(
      luh,
      repositories::{{SOLVER_INSTANCE}},
      repositories::{{SOLVER_INSTANCE}}.RelaxationParameter, //const double relaxationParameter
      repositories::{{SOLVER_INSTANCE}}.DifferencesScaling, //const double differenceScaling
      boundaryMinPerVariables, 
      boundaryMaxPerVariables
    );
"""
    template_set_troubled_markers = """
    if(!isUnTroubled){

      repositories::{{SOLVER_INSTANCE}}.addTroubledCell();
      fineGridCell{{SOLVER_NAME}}CellLabel.setTroubled_Marker(celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::TROUBLED);
      for(int d=0; d<2*Dimensions; d++){
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED);
      }

      return;
    }
"""
    return ("""
    bool isUnTroubled = true;
""" + (templatePAC if usePAC else "")
    + (template_DMP if useDMP else "")
    + (template_set_troubled_markers if useTroubledMarkers else "")
    )
    

def create_corrector_allocations(use_kernelgenerator):
    template_generic = """
    {{CORRECTOR_COMPUTATION_PRECISION}} lduh[spaceBasisSize*NumberOfVariables]{0.0};
    const int fluxElementsPerFace = spaceFaceSize*NumberOfVariables;
"""
    template_kernelgenerator = """
    {{CORRECTOR_COMPUTATION_PRECISION}} lduh[generated::kernels::AderDG::{{LINEARITY}}::getUpdateSize()]{0.0};
    const int fluxElementsPerFace  = generated::kernels::AderDG::{{LINEARITY}}::getBndFluxSize();
"""
    if use_kernelgenerator:
        return template_kernelgenerator
    else:
        return template_generic
    
def create_includes(use_kernelgenerator, use_limiter, is_linear):
    template_generic = """
#include "exahype2/aderdg/kernels/Kernels.h"
"""
    template_kernelgenerator = """
#include "../generated/kernels/aderdg/""" + ("linear" if is_linear else "nonlinear") + """/Kernels.h"
"""
    template_limiter = """
#include "../generated/kernels/limiter/fv/Kernels.h"
"""
    if use_kernelgenerator:
        if use_limiter:
            return template_kernelgenerator + template_limiter
        return template_kernelgenerator
    return template_generic
