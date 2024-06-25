# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2

def create_predictor_allocations():
    Template_kernelgenerator = """
    const int basisElementsPerFace  = kernels::{{SOLVER_NAME}}::getBndFaceSize();
    const int fluxElementsPerFace   = kernels::{{SOLVER_NAME}}::getBndFluxSize();

    constexpr int totalSize = kernels::{{SOLVER_NAME}}::getFusedSTPVISize();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} memory[totalSize] __attribute__((aligned({{ARCHITECTURE_ALIGNMENT}})));

    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lQi           = memory + kernels::{{SOLVER_NAME}}::getlQiShift();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lQhi          = memory + kernels::{{SOLVER_NAME}}::getlQhiShift();

    {% if LINEARITY=="linear" or USE_FLUX!="<none>" %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFi           = memory + kernels::{{SOLVER_NAME}}::getlFiShift();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFhi          = memory + kernels::{{SOLVER_NAME}}::getlFhiShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lFhi          = nullptr;
    {% endif %}

    {% if USE_SOURCE!="<none>" or (LINEARITY=="nonlinear" and USE_NCP!="<none>") %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lSi           = memory + kernels::{{SOLVER_NAME}}::getlSiShift();
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lShi          = memory + kernels::{{SOLVER_NAME}}::getlShiShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lSi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* lShi          = nullptr;
    {% endif %}

    {% if USE_NCP!="<none>" %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* gradQ         = memory + kernels::{{SOLVER_NAME}}::getgradQShift();
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* gradQ         = nullptr;
    {% endif %}

    {% if LINEARITY=="linear" and USE_POINT_SOURCE!="<none>" %}
    {% else %}
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* PSi           = nullptr;
    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* PSderivatives = nullptr;
    {% endif %}

    constexpr int sizeLduh = kernels::{{SOLVER_NAME}}::getUpdateSize();

    {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} lduh[sizeLduh]{({{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}})0.0};

    {{CORRECTOR_COMPUTATION_PRECISION}} lQhbnd[kernels::{{SOLVER_NAME}}::getBndFaceTotalSize()]{({{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}})0.0};
    {{CORRECTOR_COMPUTATION_PRECISION}} lFhbnd[kernels::{{SOLVER_NAME}}::getBndFluxTotalSize()]{({{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}})0.0};
"""
    return Template_kernelgenerator

def create_fstpvi_call(is_linear, usePointSource):

    Template_kernelgenerator_linear_no_ps = """
    //linear, no pointSources
    int numberOfIterations = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
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
    int numberOfIterations = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
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
    std::vector<int>* pointSources = kernels::{{SOLVER_NAME}}::getPointSources(
      repositories::{{SOLVER_INSTANCE}},
      marker.x(),
      marker.h()
    );

    if(pointSources != nullptr) {
        {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} PSi[kernels::{{SOLVER_NAME}}::getPSiSize()] __attribute__((aligned({{ARCHITECTURE_ALIGNMENT}})));
        // perform pointsource
        int numberOfIterations = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
          repositories::{{SOLVER_INSTANCE}},
          lduh, lQhbnd, lFhbnd,
          lQi, lFi, lSi,
          lQhi, lFhi, lShi,
          gradQ, PSi, nullptr,
          luh,
          marker.x(), marker.h(), timeStamp, timeStepSize,
          pointSources // will be deleted in the end if set
        );
    } else {
        int numberOfIterations = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral_WithoutPS<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
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

    if(is_linear):
        if(usePointSource):
            return Template_kernelgenerator_linear_w_ps
        else:
            return Template_kernelgenerator_linear_no_ps
    else:
        return Template_kernelgenerator_nonlinear


def create_solution_update():
    template_kernelgenerator = """
    kernels::{{SOLVER_NAME}}::solutionUpdate(
      luh,
      luh,
      lduh,
      timeStepSize
    );
"""
    return template_kernelgenerator


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


def riemann_solver(use_custom_riemann_solver, is_linear):
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
    template_kernelgenerator = """
    kernels::{{SOLVER_NAME}}::riemannSolver<{{CORRECTOR_COMPUTATION_PRECISION}}>(
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
    else:
        return template_kernelgenerator


def face_integral(is_linear):
    template_kernelgenerator = """
        const double inverseDxDirection = 1/marker.h()[d];
        kernels::{{SOLVER_NAME}}::faceIntegral(
          lduh,
          FIN,
          direction,
          orientation,
          inverseDxDirection
        );
"""
    return template_kernelgenerator
    
def copy_estimates_and_perform_riemann_solution_on_hanging_faces(use_custom_riemann_solver, is_linear):
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
            {{CORRECTOR_COMPUTATION_PRECISION}}* FL = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value;
            {{CORRECTOR_COMPUTATION_PRECISION}}* FR = &lFhbnd[2*d*fluxElementsPerFace];
            {{CORRECTOR_COMPUTATION_PRECISION}}* QL = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value;
            {{CORRECTOR_COMPUTATION_PRECISION}}* QR = &lQhbnd[2*d*basisElementsPerFace];
            int orientation = 0;
            int faceNumber  = d;
            bool isBoundary = false;
            tarch::la::Vector<Dimensions,double> faceCentre = marker.x();
            faceCentre[d] -= 0.5*marker.h()[d];

    """ + riemann_solver(use_custom_riemann_solver, is_linear) + """

            {{CORRECTOR_COMPUTATION_PRECISION}}* FIN = FR;

    """ + face_integral(is_linear) + """

        }
        else{
            //project to faces and update face timestamp
            std::copy_n(&lQhbnd[2*d*basisElementsPerFace], basisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsPerFace);
            std::copy_n(&lFhbnd[2*d*fluxElementsPerFace], fluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsPerFace);
            fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setNewTimeStamp(1, timeStamp);
        }

        //Right values
        if(fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).getIsHanging()){
            {{CORRECTOR_COMPUTATION_PRECISION}}* FL = &lFhbnd[(2*d+1)*fluxElementsPerFace];
            {{CORRECTOR_COMPUTATION_PRECISION}}* FR = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value+fluxElementsPerFace;
            {{CORRECTOR_COMPUTATION_PRECISION}}* QL = &lQhbnd[(2*d+1)*basisElementsPerFace];
            {{CORRECTOR_COMPUTATION_PRECISION}}* QR = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value+basisElementsPerFace;
            int orientation = 1;
            int faceNumber  = d+Dimensions;
            bool isBoundary = false;
            tarch::la::Vector<Dimensions,double> faceCentre = marker.x();
            faceCentre[d] += 0.5*marker.h()[d];
        
    """ + riemann_solver(use_custom_riemann_solver, is_linear) + """

            {{CORRECTOR_COMPUTATION_PRECISION}}* FIN = FL;

    """ + face_integral(is_linear) + """
        
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
    d["RIEMANN_SOLVER"] = riemann_solver(use_custom_riemann_solver, is_linear)
    d["FACE_INTEGRAL"] = face_integral(is_linear)
    return Template

def create_corrector_allocations():
    template_kernelgenerator = """
    {{CORRECTOR_COMPUTATION_PRECISION}} lduh[kernels::{{SOLVER_NAME}}::getUpdateSize()]{0.0};
    const int fluxElementsPerFace  = kernels::{{SOLVER_NAME}}::getBndFluxSize();
"""

    return template_kernelgenerator

def create_includes(solver_name, is_linear):
    return """
#include "../kernels/""" + solver_name + """/Kernels.h"
"""
