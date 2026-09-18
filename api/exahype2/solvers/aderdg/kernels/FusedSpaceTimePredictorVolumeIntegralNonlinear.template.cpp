// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FusedSpaceTimePredictorVolumeIntegral.h"

#include "DGMatrices.h"
#include "Quadrature.h"
#include "BufferSizes.h"

#include "exahype2/dg/DGUtils.h"

#include "tarch/CompilerSpecificSettings.h"

{%- import 'Gemms.template.h' as m with context %}

template <typename SolutionStorageType, typename PredictorComputationType, typename PicardComputationType, typename PredictorStorageType>
int {{FULL_QUALIFIED_NAMESPACE}}::fusedSpaceTimePredictorVolumeIntegral(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&  solver,
  PredictorStorageType* const NOALIAS               lQhbnd[peano4::FacesPerCell],
  PredictorStorageType* const NOALIAS               lFhbnd[peano4::FacesPerCell],
  SolutionStorageType* const NOALIAS                luh,
  const tarch::la::Vector<DIMENSIONS, double>&      cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&      cellSize,
  const double                                      timeStamp,
  const double                                      timeStepSize
) {
  const PredictorComputationType predictorTimeStepSize = static_cast<PredictorComputationType>(timeStepSize);
  static thread_local {{FULL_QUALIFIED_NAMESPACE}}::PredictorScratchBuffer{{NAME_SUFFIX}}<PredictorComputationType> predictorScratch;
  static thread_local {{FULL_QUALIFIED_NAMESPACE}}::PredictorScratchBuffer{{NAME_SUFFIX}}<PicardComputationType> picardScratch;
  static thread_local {{FULL_QUALIFIED_NAMESPACE}}::PredictorScratchBuffer{{NAME_SUFFIX}}<PicardComputationType> newLQiSliceScratch;
  static thread_local {{FULL_QUALIFIED_NAMESPACE}}::PredictorScratchBuffer{{NAME_SUFFIX}}<PredictorComputationType> coeffVolumeScratch;

  PredictorComputationType* const memory = predictorScratch.get(getFusedSTPVISize() - getPicardSize());

  PredictorComputationType* const lQhi = memory;
{% if USE_FLUX %}
  PredictorComputationType* const lFhi = memory + getlQhiSize();
{% endif %}
{% if USE_SOURCE or USE_DIFFUSIVE_SOURCE or USE_NCP %}
  PredictorComputationType* const lShi = memory + getlQhiSize() + getlFhiSize();
{% endif %}

  PicardComputationType* const picardMemory = picardScratch.get(getPicardSize());

  PicardComputationType* const lQi = picardMemory;
  PicardComputationType* const rhs = picardMemory + getlQiSize();

{% if USE_FLUX %}
  PicardComputationType* const lFi = picardMemory + getlQiSize() + getRhsSize();
{% else %}
  PicardComputationType* const lFi = nullptr;
{% endif %}

{% if USE_SOURCE or USE_DIFFUSIVE_SOURCE or USE_NCP %}
  PicardComputationType* const lSi = picardMemory + getlQiSize() + getRhsSize() + getlFiSize();
{% else %}
  PicardComputationType* const lSi = nullptr;
{% endif %}

{% if USE_NCP or USE_DIFFUSIVE_FLUX or USE_DIFFUSIVE_SOURCE %}
  PicardComputationType* const gradQ = picardMemory + getlQiSize() + getRhsSize() + getlFiSize() + getlSiSize();
{% else %}
  PicardComputationType* const gradQ = nullptr;
{% endif %}

{% if USE_FLUX %}
  const tarch::la::Vector<DIMENSIONS, PredictorComputationType> invDx = convertScalar<PredictorComputationType>(invertEntries(cellSize));
{% endif %}

  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    PicardComputationType* const localPredictorState = &lQi[{{NUMBER_OF_DATA_PADDED}} * xyz];
    std::fill_n(localPredictorState, {{NUMBER_OF_DATA_PADDED}}, PicardComputationType(0.0));
    std::copy_n(&luh[{{NUMBER_OF_DATA}} * xyz], {{NUMBER_OF_DATA}}, localPredictorState);
  }
  for (int t = 1; t < {{NUMBER_OF_DOF}}; t++) {
    std::copy_n(
      lQi,
      {{NUMBER_OF_DATA_PADDED * NUMBER_OF_DOF**DIMENSIONS}},
      lQi + t * {{NUMBER_OF_DATA_PADDED * NUMBER_OF_DOF**DIMENSIONS}}
    );
  }

  PicardComputationType* const newLQiSlice = newLQiSliceScratch.get({{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}});
  int numberOfPicardIterations = 0;
  bool picardIterationDiverged = false;
  constexpr double PicardResidualTolerance = 1e-7;
  for (; numberOfPicardIterations < {{2*NUMBER_OF_DOF+1}}; numberOfPicardIterations++) {
    const double squaredResidual = picardIteration<SolutionStorageType, PicardComputationType>(
      solver,
      lQi,
      rhs,
      lFi,
      lSi,
      gradQ,
      newLQiSlice,
      luh,
      cellCentre,
      cellSize,
      timeStamp,
      timeStepSize
    );

    // An infinite residual signals a divergent Picard iteration.
    if (not std::isfinite(squaredResidual)) {
      picardIterationDiverged = true;
      break;
    }
    if (squaredResidual < PicardResidualTolerance * PicardResidualTolerance) {
      break;
    }
  }

  std::memset(lQhi, 0, {{(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DATA_PADDED}} * sizeof(PredictorComputationType));

{% if USE_FLUX %}
  std::memset(lFhi, 0, {{DIMENSIONS*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(PredictorComputationType));
{% endif %}

{% if USE_NCP or USE_SOURCE or USE_DIFFUSIVE_SOURCE %}
  std::memset(lShi, 0, {{(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(PredictorComputationType));
{% endif %}

  for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
    for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        for (int t = 0; t < {{NUMBER_OF_DOF}}; t++) {
          SIMD(aligned(lQhi,lQi:PEANO_WITH_ALIGNMENT))
          for (int n = 0; n < {{NUMBER_OF_DATA_PADDED}}; n++) {
            lQhi[((z * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n] += Quadrature<PredictorComputationType>::weights1[t]
              * lQi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n];
          }
{% if USE_FLUX %}
          SIMD(aligned(lFhi,lFi:PEANO_WITH_ALIGNMENT))
          for (int n = 0; n<{{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
            lFhi[((z * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{0*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] += Quadrature<PredictorComputationType>::weights1[t]
              * lFi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{0*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];
          }
          SIMD(aligned(lFhi,lFi:PEANO_WITH_ALIGNMENT))
          for (int n = 0; n<{{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
            lFhi[((z * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] += Quadrature<PredictorComputationType>::weights1[t]
              * lFi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];
          }
{% if DIMENSIONS==3%}
          SIMD(aligned(lFhi,lFi:PEANO_WITH_ALIGNMENT))
          for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
            lFhi[((y * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DOF}} + z) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] += Quadrature<PredictorComputationType>::weights1[t]
              * lFi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];
          }
{% endif %}
{% endif %}
{% if USE_NCP or USE_SOURCE or USE_DIFFUSIVE_SOURCE %}
          SIMD(aligned(lShi,lSi:PEANO_WITH_ALIGNMENT))
          for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
            lShi[((z * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += Quadrature<PredictorComputationType>::weights1[t]
              * lSi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
          }
{% endif %}
        }
      }
    }
  }

  for (int i = 0; i < 2 *DIMENSIONS; i++) {
    std::memset(lQhbnd[i], 0, {{NUMBER_OF_DATA*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}} * sizeof(PredictorStorageType));
    std::memset(lFhbnd[i], 0, {{NUMBER_OF_UNKNOWNS*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}} * sizeof(PredictorStorageType));
  }

  for (int yz = 0; yz < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; yz++) {
    for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
      SIMD(aligned(lQhi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
        lQhbnd[0][n + {{NUMBER_OF_DATA}} * yz] += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (x + {{NUMBER_OF_DOF}} * yz)] * DGMatrices<PredictorComputationType>::FLCoeff[x];
        lQhbnd[DIMENSIONS][n + {{NUMBER_OF_DATA}} * yz] += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (x + {{NUMBER_OF_DOF}} * yz)] * DGMatrices<PredictorComputationType>::FRCoeff[x];
{% if USE_FLUX %}
{% if NUMBER_OF_DATA != NUMBER_OF_UNKNOWNS %}
      }
      SIMD(aligned(lFhi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
{% endif %}
        lFhbnd[0][n + {{NUMBER_OF_UNKNOWNS}} * yz] += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (x + {{NUMBER_OF_DOF}} * yz)] * DGMatrices<PredictorComputationType>::FLCoeff[x];
        lFhbnd[DIMENSIONS][n + {{NUMBER_OF_UNKNOWNS}} * yz] += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (x + {{NUMBER_OF_DOF}} * yz)] * DGMatrices<PredictorComputationType>::FRCoeff[x];
{% endif %}
      }
    }
  }

  for (int xz = 0; xz < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xz++) {
    for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
{% if DIMENSIONS==3 %}
      const int z = xz / {{NUMBER_OF_DOF}};
      const int x = xz % {{NUMBER_OF_DOF}};
{% else %}
      const int z = 0;
      const int x = xz;
{% endif %}
      SIMD(aligned(lQhi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
        lQhbnd[1][n + {{NUMBER_OF_DATA}} * xz] += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (x + {{NUMBER_OF_DOF}} * (y + {{NUMBER_OF_DOF_3D}} * z))] * DGMatrices<PredictorComputationType>::FLCoeff[y];
        lQhbnd[1 + DIMENSIONS][n + {{NUMBER_OF_DATA}} * xz] += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (x + {{NUMBER_OF_DOF}} * (y + {{NUMBER_OF_DOF_3D}} * z))] * DGMatrices<PredictorComputationType>::FRCoeff[y];
{% if USE_FLUX %}
{% if NUMBER_OF_DATA != NUMBER_OF_UNKNOWNS %}
      }
      SIMD(aligned(lFhi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
{% endif %}
        lFhbnd[1][n + {{NUMBER_OF_UNKNOWNS}} * xz] += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (y + {{NUMBER_OF_DOF}} * xz) + {{1*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] * DGMatrices<PredictorComputationType>::FLCoeff[y];
        lFhbnd[1 + DIMENSIONS][n + {{NUMBER_OF_UNKNOWNS}} * xz] += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (y + {{NUMBER_OF_DOF}} * xz) + {{1*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] * DGMatrices<PredictorComputationType>::FRCoeff[y];
{% endif %}
      }
    }
  }

{% if DIMENSIONS==3 %}
  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF}}; xy++) {
    for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
      SIMD(aligned(lQhi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
        lQhbnd[2][n + {{NUMBER_OF_DATA}} * xy] += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (xy + {{NUMBER_OF_DOF*NUMBER_OF_DOF}} * z)] * DGMatrices<PredictorComputationType>::FLCoeff[z];
        lQhbnd[2 + DIMENSIONS][n + {{NUMBER_OF_DATA}} * xy] += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (xy + {{NUMBER_OF_DOF*NUMBER_OF_DOF}} * z)] * DGMatrices<PredictorComputationType>::FRCoeff[z];
{% if USE_FLUX %}
{% if NUMBER_OF_DATA != NUMBER_OF_UNKNOWNS %}
      }
      SIMD(aligned(lFhi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
{% endif %}
        lFhbnd[2][n + {{NUMBER_OF_UNKNOWNS}} * xy] += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (z + {{NUMBER_OF_DOF}} * xy) + {{2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] * DGMatrices<PredictorComputationType>::FLCoeff[z];
        lFhbnd[2 + DIMENSIONS][n + {{NUMBER_OF_UNKNOWNS}} * xy] += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (z + {{NUMBER_OF_DOF}} * xy) + {{2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] * DGMatrices<PredictorComputationType>::FRCoeff[z];
{% endif %}
      }
    }
  }
{% endif %}

  // Kxi already contains one spatial quadrature factor; the matching
  // tensor-product factors in the remaining directions cancel.
  PredictorComputationType* const coeffVolume = coeffVolumeScratch.get({{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}});
  for (int i = 0; i < {{NUMBER_OF_DOF}}; i++) {
    for (int j = 0; j < {{NUMBER_OF_DOF}}; j++) {
      coeffVolume[i * {{NUMBER_OF_DOF_PADDED}} + j] = DGMatrices<PredictorComputationType>::KxiTransposed[i * {{NUMBER_OF_DOF_PADDED}} + j] * predictorTimeStepSize * Quadrature<PredictorComputationType>::iweights1[i];
    }
  }

{% if USE_FLUX %}
  for (int j = 0; j < {{NUMBER_OF_DOF_3D}}; j++) {
    for (int i = 0; i < {{NUMBER_OF_DOF}}; i++) {
      {{ m.gemm('lduhX', 'lFhi', 'coeffVolume', 'luh',
        '(j*'~NUMBER_OF_DOF~'+i)*'~(NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF)~'+'~(0*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)),
        '0',
        '(j*'~NUMBER_OF_DOF~'+i)*'~(NUMBER_OF_DATA*NUMBER_OF_DOF),
        trueB='coeffVolume',
        trueAlpha='invDx[0]') | indent(6) }}{##}

      {{ m.gemm('lduhY', 'lFhi', 'coeffVolume', 'luh',
        '(j*'~NUMBER_OF_DOF~'+i)*'~(NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF)~'+'~(1*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)),
        '0',
        '(j*'~(NUMBER_OF_DOF*NUMBER_OF_DOF)~'+i)*'~NUMBER_OF_DATA,
        trueB='coeffVolume',
        trueAlpha='invDx[1]') | indent(6) }}{##}

{% if DIMENSIONS==3 %}
      {{ m.gemm('lduhZ', 'lFhi', 'coeffVolume', 'luh',
        '(j*'~NUMBER_OF_DOF~'+i)*'~(NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF)~'+'~(2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)),
        '0',
        '(j*'~NUMBER_OF_DOF~'+i)*'~NUMBER_OF_DATA,
        trueB='coeffVolume',
        trueAlpha='invDx[2]') | indent(6) }}{##}
{% endif %}
    }
  }
{% endif %}

{% if USE_NCP or USE_SOURCE or USE_DIFFUSIVE_SOURCE %}
  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    SIMD(aligned(lShi:PEANO_WITH_ALIGNMENT))
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
      luh[xyz * {{NUMBER_OF_DATA}} + n] += predictorTimeStepSize * lShi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
    }
  }
{% endif %}

  // A negative iteration count tells the caller the predictor holds no valid
  // candidate, so the a-posteriori limiter recomputes the cell.
  if (picardIterationDiverged) {
    return -1;
  }
  const int result = std::min(numberOfPicardIterations + 1, {{2*NUMBER_OF_DOF+1}});
  return result;
}

{% for PRECISION_NUM in range(0,PREDICTOR_COMPUTATION_PRECISIONS|length) %}
template int {{FULL_QUALIFIED_NAMESPACE}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{PICARD_COMPUTATION_PRECISION}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                     solver,
  {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS  lQhbnd[peano4::FacesPerCell],
  {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS  lFhbnd[peano4::FacesPerCell],
  {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS       luh,
  const tarch::la::Vector<DIMENSIONS, double>&        cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&        cellSize,
  const double                                        timeStamp,
  const double                                        timeStepSize
);
{% endfor %}

template <typename SolutionStorageType, typename PredictorComputationType>
double {{FULL_QUALIFIED_NAMESPACE}}::picardIteration(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&  solver,
  PredictorComputationType* NOALIAS                 lQi,
  PredictorComputationType* NOALIAS                 rhs,
  PredictorComputationType* NOALIAS                 lFi,
  PredictorComputationType* NOALIAS                 lSi,
  PredictorComputationType* NOALIAS                 gradQ,
  PredictorComputationType* NOALIAS                 newLQiSlice,
  const SolutionStorageType* const NOALIAS          luh,
  const tarch::la::Vector<DIMENSIONS, double>&      cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&      cellSize,
  const double                                      timeStamp,
  const double                                      timeStepSize
) {
{% if USE_FLUX %}
  const PredictorComputationType* const Kxi = DGMatrices<PredictorComputationType>::Kxi;
{% endif %}

{% if USE_FLUX or USE_NCP or USE_DIFFUSIVE_FLUX or USE_DIFFUSIVE_SOURCE %}
  const tarch::la::Vector<DIMENSIONS, PredictorComputationType> invDx = convertScalar<PredictorComputationType>(invertEntries(cellSize));
{% endif %}

{% if USE_FLUX %}
  const PredictorComputationType dtByDx[DIMENSIONS] = { static_cast<PredictorComputationType>(timeStepSize) * invDx[0], static_cast<PredictorComputationType>(timeStepSize) * invDx[1]
{% if DIMENSIONS == 3 %}
      , static_cast<PredictorComputationType>(timeStepSize) * invDx[2]
{% endif %}
  };
{% endif %}

  for (int t = 0; t < {{NUMBER_OF_DOF}}; t++) {
{% if USE_NCP or USE_DIFFUSIVE_FLUX or USE_DIFFUSIVE_SOURCE %}
    std::memset(gradQ, 0, {{(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED*DIMENSIONS}} * sizeof(PredictorComputationType));

    const PredictorComputationType* const dudxTransposed = DGMatrices<PredictorComputationType>::dudxTransposed;

    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
        {{ m.gemm('gradQX', 'lQi', 'dudxTransposed', 'gradQ',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF~'+y)*'~NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED,
          '0',
          '(z*'~NUMBER_OF_DOF~'+y)*'~NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF,
          trueB='dudxTransposed',
          trueAlpha='invDx[0]') | indent(10) }}{##}
      }
    }

    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        {{ m.gemm('gradQY', 'lQi', 'dudxTransposed', 'gradQ',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA_PADDED,
          '0',
          '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED~'+'~NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS),
          trueB='dudxTransposed',
          trueAlpha='invDx[1]') | indent(10) }}{##}
      }
    }

{% if DIMENSIONS==3 %}
    for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        {{ m.gemm('gradQZ', 'lQi', 'dudxTransposed', 'gradQ',
          '((t*'~NUMBER_OF_DOF_3D*NUMBER_OF_DOF~'+y)*'~NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA_PADDED,
          '0',
          '(y*'~NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED~'+'~2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS),
          trueB='dudxTransposed',
          trueAlpha='invDx[2]') | indent(10) }}{##}
      }
    }
{% endif %}
{% endif %}

{% if USE_FLUX %}
    PredictorComputationType* F[{{DIMENSIONS}}];

    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
{% if DIMENSIONS==2 %}
      F[0] = &lFi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}}];
      F[1] = &lFi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];

      // The tensor-product data layout orders the spatial indices as y, x.
      const tarch::la::Vector<DIMENSIONS, int> index = {xyz % {{NUMBER_OF_DOF}}, xyz / {{NUMBER_OF_DOF}} };
{% endif %}
{% if DIMENSIONS==3 %}
      F[0] = &lFi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}}];
      F[1] = &lFi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];
      F[2] = &lFi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + {{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];
      // The tensor-product data layout orders the spatial indices as z, y, x.
      const tarch::la::Vector<DIMENSIONS, int> index = {xyz % {{NUMBER_OF_DOF}}, (xyz % {{NUMBER_OF_DOF**2}}) / {{NUMBER_OF_DOF}}, xyz / {{NUMBER_OF_DOF**2}}};
{% endif %}

      const double timePosition = timeStamp + Quadrature<PredictorComputationType>::nodes[t] * timeStepSize;

      const tarch::la::Vector<DIMENSIONS, double> nodePosition = ::exahype2::dg::getQuadraturePoint(cellCentre, cellSize, index, Quadrature<PredictorComputationType>::nodes);

{% if USE_DIFFUSIVE_FLUX %}
      const PredictorComputationType* gradQFlux[DIMENSIONS] = {
        gradQ + {{NUMBER_OF_UNKNOWNS_PADDED}} * xyz,
        gradQ + {{NUMBER_OF_UNKNOWNS_PADDED}} * xyz + {{1*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}
{% if DIMENSIONS==3 %}
      , gradQ + {{NUMBER_OF_UNKNOWNS_PADDED}} * xyz + {{2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}
{% endif %}
      };
{% endif %}

      // The predictor uses cell-local fluxes; neighbour penalties enter during correction.
      UNROLL(DIMENSIONS)
      for (int d = 0; d < DIMENSIONS; d++) {
        std::fill_n(F[d], {{NUMBER_OF_UNKNOWNS_PADDED}}, PredictorComputationType(0.0));
{% if STATELESS_FLUX %}
        {{FULL_QUALIFIED_SOLVER_NAME}}::flux(
{% else %}
        solver.flux(
{% endif %}
          lQi + (t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_DATA_PADDED}},
{% if USE_DIFFUSIVE_FLUX %}
          gradQFlux,
{% endif %}
          nodePosition,
          cellSize,
          timePosition,
          timeStepSize,
          d,
          F[d]
        );
      }
    }
{% endif %}

    // Temporal upwinding uses the corrected state from the preceding step as the initial state.
    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
      const PredictorComputationType weight = Quadrature<PredictorComputationType>::weights3[xyz] * DGMatrices<PredictorComputationType>::FLCoeff[t];
      PredictorComputationType* const localRightHandSide = rhs + {{NUMBER_OF_UNKNOWNS_PADDED}} * (xyz + {{NUMBER_OF_DOF**DIMENSIONS}} * t);
      std::fill_n(localRightHandSide, {{NUMBER_OF_UNKNOWNS_PADDED}}, PredictorComputationType(0.0));
      SIMD()
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        localRightHandSide[n] = weight * luh[n + {{NUMBER_OF_DATA}} * xyz];
      }
    }

{% if USE_FLUX %}
    // Kxi and weights3 carry complementary tensor-product factors for each derivative.
    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
        {{ m.gemm('rhsX', 'lFi', 'coeffRhsX', 'rhs',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF~'+y)*'~NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF,
          '0',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF~'+y)*'~NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF,
          trueB='Kxi',
          trueAlpha='-Quadrature<PredictorComputationType>::weights3[t*'~NUMBER_OF_DOF*NUMBER_OF_DOF_3D~'+z*'~NUMBER_OF_DOF~'+y] * dtByDx[0]') | indent(10) }}{##}
      }
    }

    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        {{ m.gemm('rhsY', 'lFi', 'coeffRhsY', 'rhs',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED~'+'~1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED,
          '0',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED,
          trueB='Kxi',
          trueAlpha='-Quadrature<PredictorComputationType>::weights3[t*'~NUMBER_OF_DOF*NUMBER_OF_DOF_3D~'+z*'~NUMBER_OF_DOF~'+x] * dtByDx[1]') | indent(10) }}{##}
      }
    }

{% if DIMENSIONS==3 %}
    for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        {{ m.gemm('rhsZ', 'lFi','coeffRhsZ', 'rhs',
          '((t*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+y)*'~NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED~'+'~2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED,
          '0',
          '((t*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+y)*'~NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED,
          trueB='Kxi',
          trueAlpha='-Quadrature<PredictorComputationType>::weights3[t*'~NUMBER_OF_DOF*NUMBER_OF_DOF_3D~'+y*'~NUMBER_OF_DOF~'+x] * dtByDx[2]') | indent(10) }}{##}
      }
    }
{% endif %}
{% endif %}

{% if USE_NCP or USE_SOURCE or USE_DIFFUSIVE_SOURCE %}
{% if USE_NCP %}
    PredictorComputationType nonconservativeProductTerm[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
{% endif %}
{% if USE_SOURCE and USE_DIFFUSIVE_SOURCE %}
    PredictorComputationType diffusiveSource[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
{% endif %}
{% if USE_NCP or USE_DIFFUSIVE_SOURCE %}
    PredictorComputationType gradientForPDE[{{NUMBER_OF_UNKNOWNS*DIMENSIONS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
{% endif %}

    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
      PredictorComputationType* const localSource = lSi + (t * {{NUMBER_OF_DOF ** DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}};
      std::fill_n(localSource, {{NUMBER_OF_UNKNOWNS_PADDED}}, PredictorComputationType(0.0));

{% if USE_NCP or USE_SOURCE or USE_DIFFUSIVE_SOURCE %}
{% if DIMENSIONS==2 %}
      const tarch::la::Vector<DIMENSIONS, int> index = {xyz % {{NUMBER_OF_DOF}}, xyz / {{NUMBER_OF_DOF}} };
{% endif %}
{% if DIMENSIONS==3 %}
      const tarch::la::Vector<DIMENSIONS, int> index = {xyz % {{NUMBER_OF_DOF}}, (xyz % {{NUMBER_OF_DOF**2}}) / {{NUMBER_OF_DOF}}, xyz / {{NUMBER_OF_DOF**2}}};
{% endif %}
      const double timePosition = timeStamp + Quadrature<PredictorComputationType>::nodes[t] * timeStepSize;
      const tarch::la::Vector<DIMENSIONS, double> nodePosition = ::exahype2::dg::getQuadraturePoint(cellCentre, cellSize, index, Quadrature<PredictorComputationType>::nodes);
{% endif %}

{% if USE_NCP or USE_DIFFUSIVE_SOURCE %}
      std::copy_n(gradQ + {{NUMBER_OF_UNKNOWNS_PADDED}} * xyz, {{NUMBER_OF_UNKNOWNS}}, gradientForPDE);
      std::copy_n(gradQ + {{NUMBER_OF_UNKNOWNS_PADDED}} * xyz + {{1*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}, {{NUMBER_OF_UNKNOWNS}}, gradientForPDE + {{1*NUMBER_OF_UNKNOWNS}});
{% if DIMENSIONS==3 %}
      std::copy_n(gradQ + {{NUMBER_OF_UNKNOWNS_PADDED}} * xyz + {{2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}, {{NUMBER_OF_UNKNOWNS}}, gradientForPDE + {{2*NUMBER_OF_UNKNOWNS}});
{% endif %}
{% endif %}

{% if USE_SOURCE %}
{% if STATELESS_SOURCE_TERM %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::sourceTerm(
{% else %}
      solver.sourceTerm(
{% endif %}
        lQi + (t * {{NUMBER_OF_DOF ** DIMENSIONS}} + xyz) * {{NUMBER_OF_DATA_PADDED}},
        nodePosition,
        cellSize,
        timePosition,
        timeStepSize,
        localSource
      );
{% endif %}
{% if USE_DIFFUSIVE_SOURCE %}
{% if USE_SOURCE %}
      std::fill_n(diffusiveSource, {{NUMBER_OF_UNKNOWNS}}, PredictorComputationType(0.0));
{% endif %}
{% if STATELESS_DIFFUSIVE_SOURCE_TERM %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::diffusiveSourceTerm(
{% else %}
      solver.diffusiveSourceTerm(
{% endif %}
        lQi + (t * {{NUMBER_OF_DOF ** DIMENSIONS}} + xyz) * {{NUMBER_OF_DATA_PADDED}},
        gradientForPDE,
        nodePosition,
        cellSize,
        timePosition,
        timeStepSize,
{% if USE_SOURCE %}
        diffusiveSource
{% else %}
        localSource
{% endif %}
      );
{% if USE_SOURCE %}
      SIMD(aligned(diffusiveSource:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        localSource[n] += diffusiveSource[n];
      }
{% endif %}
{% endif %}

{% if USE_NCP %}
      UNROLL(DIMENSIONS)
      for (int d = 0; d < DIMENSIONS; d++) {
        // The predictor evaluates only the cell-local nonconservative product.
        std::fill_n(nonconservativeProductTerm, {{NUMBER_OF_UNKNOWNS}}, PredictorComputationType(0.0));
{% if STATELESS_NCP %}
        {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(
{% else %}
        solver.nonconservativeProduct(
{% endif %}
          lQi + (t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_DATA_PADDED}},
          gradientForPDE + {{NUMBER_OF_UNKNOWNS}} * d,
          nodePosition,
          cellSize,
          timePosition,
          timeStepSize,
          d,
          nonconservativeProductTerm
        );

        SIMD(aligned(nonconservativeProductTerm:PEANO_WITH_ALIGNMENT))
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
          localSource[n] -= nonconservativeProductTerm[n];
        }
      }
{% endif %}

      const PredictorComputationType updateSize = Quadrature<PredictorComputationType>::weights1[t] * Quadrature<PredictorComputationType>::weights3[xyz] * timeStepSize;
      SIMD(aligned(rhs:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        rhs[n + (t * {{NUMBER_OF_DOF ** DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}}] += updateSize * localSource[n];
      }
    }
{% endif %}
  }

  // Multiplication by iK1Transposed integrates the right-hand side in time and forms the current Picard iterate.
  const PredictorComputationType* const iK1Transposed = DGMatrices<PredictorComputationType>::iK1Transposed;
  // Strong discontinuities can make the local predictor diverge before limiter
  // recomputation. The bound below admits well-scaled states and keeps every
  // squared double residual finite.
  constexpr double PicardDivergenceBound = 1e20;
  constexpr auto NativePicardDivergenceBound = static_cast<PredictorComputationType>(PicardDivergenceBound);
  double squaredResidual = 0.0;
  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    {{ m.gemm('lqi', 'rhs', 'qSliceBuffer', 'newLQiSlice',
      NUMBER_OF_UNKNOWNS_PADDED~'*xyz', '0', '0',
      trueB='iK1Transposed',
      trueAlpha='Quadrature<PredictorComputationType>::iweights3[xyz]') | indent(6) }}{##}

    for (int t = 0; t < {{NUMBER_OF_DOF}}; t++) {
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        const auto newValue      = newLQiSlice[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * t];
        const auto previousValue = lQi[n + {{NUMBER_OF_DATA_PADDED}} * (xyz + {{NUMBER_OF_DOF**DIMENSIONS}} * t)];
        if (
          not std::isfinite(newValue) or newValue < -NativePicardDivergenceBound
          or newValue > NativePicardDivergenceBound or not std::isfinite(previousValue)
          or previousValue < -NativePicardDivergenceBound or previousValue > NativePicardDivergenceBound
        ) {
          return std::numeric_limits<double>::infinity();
        }
        const double difference = static_cast<double>(newValue) - static_cast<double>(previousValue);
        squaredResidual += difference * difference;
        lQi[n + {{NUMBER_OF_DATA_PADDED}} * (xyz + {{NUMBER_OF_DOF**DIMENSIONS}} * t)] = newValue;
      }
    }
  }

  return squaredResidual;
}

template double {{FULL_QUALIFIED_NAMESPACE}}::picardIteration(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                               solver,
  {{PICARD_COMPUTATION_PRECISION}}* NOALIAS                     lQi,
  {{PICARD_COMPUTATION_PRECISION}}* NOALIAS                     rhs,
  {{PICARD_COMPUTATION_PRECISION}}* NOALIAS                     lFi,
  {{PICARD_COMPUTATION_PRECISION}}* NOALIAS                     lSi,
  {{PICARD_COMPUTATION_PRECISION}}* NOALIAS                     gradQ,
  {{PICARD_COMPUTATION_PRECISION}}* NOALIAS                     newLQiSlice,
  const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS           luh,
  const tarch::la::Vector<DIMENSIONS, double>&                  cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&                  cellSize,
  const double                                                  timeStamp,
  const double                                                  timeStepSize
);
