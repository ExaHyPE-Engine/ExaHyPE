// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FusedSpaceTimePredictorVolumeIntegral.h"

#include "DGMatrices.h"
#include "Quadrature.h"
#include "CellData.h"
#include "exahype2/dg/DGUtils.h"


{% if USE_LIBXSMM %}
#include "Gemms.h"
{% endif %}

#include <cstring>
#include <algorithm>

{% import 'Gemms.template.h' as m with context %}{# get template macros #}

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

template <typename cStoreType, typename pCompType>
pCompType picardIteration(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  pCompType* NOALIAS                       lQi,
  pCompType* NOALIAS                       rhs,
  pCompType* NOALIAS                       lFi,
  pCompType* NOALIAS                       lSi,
  pCompType* NOALIAS                       gradQ,
  pCompType* NOALIAS                       new_lQi_slice,
  const cStoreType* const NOALIAS          luh,
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize,
  const double                                  timeStamp,
  const double                                  timeStepSize
);

{% for item in NAMESPACE -%}
}
{%- endfor %}

template <typename cStoreType, typename pCompType, typename pStoreType>
int {{FULL_QUALIFIED_NAMESPACE}}::fusedSpaceTimePredictorVolumeIntegral(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  pStoreType* NOALIAS                      lQhbnd[2*Dimensions],
  pStoreType* NOALIAS                      lFhbnd[2*Dimensions],
  cStoreType* const NOALIAS                luh,
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize,
  const double                                  timeStamp,
  const double                                  timeStepSize
) {
  pCompType memory[getFusedSTPVISize()] __attribute__((aligned({{ALIGNMENT}})));

  pCompType* lQi   = memory + getlQiShift();
  pCompType* lQhi  = memory + getlQhiShift();

  pCompType* rhs   = memory + getRhsShift();

  {% if USE_FLUX %}
  pCompType* lFi   = memory + getlFiShift();
  pCompType* lFhi  = memory + getlFhiShift();
  {% else %}
  pCompType* lFi   = nullptr;
  {% endif %}

  {% if USE_SOURCE or USE_NCP %}
  pCompType* lSi   = memory + getlSiShift();
  pCompType* lShi  = memory + getlShiShift();
  {% else %}
  pCompType* lSi   = nullptr;
  {% endif %}

  {% if USE_NCP %}
  pCompType* gradQ = memory + getGradQShift();
  {% else %}
  pCompType* gradQ = nullptr;
  {% endif %}

  const pCompType* Kxi    = DGMatrices<pCompType>::Kxi;
  const pCompType* Kxi_T  = DGMatrices<pCompType>::Kxi_T;
  const pCompType* iK1_T  = DGMatrices<pCompType>::iK1_T;

  //********************
  //****** Picard ******
  //********************

  const pCompType invDx[Dimensions] = {1.0 / cellSize[0], 1.0 / cellSize[1]
    {% if DIMENSIONS == 3 %}
      , 1.0 / cellSize[2]
    {% endif %}
  };

  pCompType new_lQi_slice[{{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned({{ALIGNMENT}})));

  // 1: Trivial initial guess
  for (int t = 0; t < {{NUMBER_OF_DOF}}; t++) {
    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
      std::copy_n(&luh[{{NUMBER_OF_DATA}} * xyz], {{NUMBER_OF_DATA}}, &lQi[{{NUMBER_OF_DATA_PADDED}} * (xyz + {{NUMBER_OF_DOF**DIMENSIONS}} * t)]);
    }
  }

  // 2: Discrete Picard iterations
  int numberOfPicardIterations = 0;
  for (; numberOfPicardIterations < {{2*NUMBER_OF_DOF+1}}; numberOfPicardIterations++) {
    pCompType sqRes = picardIteration<cStoreType, pCompType>(
      solver,
      lQi,
      rhs,
      lFi,
      lSi,
      gradQ,
      new_lQi_slice,
      luh,
      cellCentre,
      cellSize,
      timeStamp,
      timeStepSize
    );

    if (sqRes < (1e-7 * 1e-7)) {
      break;
    }
  }

  //***********************
  //****** Predictor ******
  //***********************

  std::memset(lQhi, 0, {{(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DATA_PADDED}} * sizeof(pCompType));

{% if USE_FLUX %}
  std::memset(lFhi, 0, {{DIMENSIONS*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));
{% endif %}

{% if USE_NCP or USE_SOURCE %}
  std::memset(lShi, 0, {{(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));
{% endif %}

  for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
    for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        // Matrix-Vector Products
        for (int t = 0; t < {{NUMBER_OF_DOF}}; t++) {
          #ifdef SharedOMP
          #pragma omp simd aligned(lQhi,lQi:{{ALIGNMENT}})
          #endif
          for (int n = 0; n < {{NUMBER_OF_DATA_PADDED}}; n++) {
            lQhi[((z * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n] += Quadrature<pCompType>::weights1[t]
              * lQi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n];
          }
{% if USE_FLUX %}
          #ifdef SharedOMP
          #pragma omp simd aligned(lFhi,lFi:{{ALIGNMENT}})
          #endif
          for (int n = 0; n<{{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
            lFhi[((z * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{0*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] += Quadrature<pCompType>::weights1[t]
              * lFi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{0*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];
          }
          #ifdef SharedOMP
          #pragma omp simd aligned(lFhi,lFi:{{ALIGNMENT}})
          #endif
          for (int n = 0; n<{{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
            lFhi[((z * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] += Quadrature<pCompType>::weights1[t]
              * lFi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];
          }
{% if DIMENSIONS==3%}
          #ifdef SharedOMP
          #pragma omp simd aligned(lFhi,lFi:{{ALIGNMENT}})
          #endif
          for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
            lFhi[((y * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DOF}} + z) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] += Quadrature<pCompType>::weights1[t]
              * lFi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];
          }
{% endif %}
{% endif %}{# USE_FLUX #}
{% if USE_NCP or USE_SOURCE %}
          #ifdef SharedOMP
          #pragma omp simd aligned(lShi,lSi:{{ALIGNMENT}})
          #endif
          for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
            lShi[((z * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += Quadrature<pCompType>::weights1[t]
              * lSi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
          }
{% endif %}
        }
      }
    }
  }

  //**************************
  //****** Extrapolator ******
  //**************************

  for(int i=0; i<2*Dimensions; i++){
    std::memset(lQhbnd[i], 0, {{NUMBER_OF_DATA*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}} * sizeof(pStoreType));
    std::memset(lFhbnd[i], 0, {{NUMBER_OF_UNKNOWNS*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}} * sizeof(pStoreType));
  }

  // x-direction: face 0 (left) and face 2(2D) / 3(3D) (right)
  for (int yz = 0; yz < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; yz++) {
    // Matrix-Vector Products
    for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
      #ifdef SharedOMP
      #pragma omp simd aligned(lQhbnd,lQhi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
        lQhbnd[0][n + {{NUMBER_OF_DATA}} * yz]
          += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (x + {{NUMBER_OF_DOF}} * yz)] * DGMatrices<pCompType>::FLCoeff[x];

        lQhbnd[Dimensions][n + {{NUMBER_OF_DATA}} * yz]
          += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (x + {{NUMBER_OF_DOF}} * yz)] * DGMatrices<pCompType>::FRCoeff[x];
{% if USE_FLUX %}
{% if NUMBER_OF_DATA != NUMBER_OF_UNKNOWNS %}
      }
      #ifdef SharedOMP
      #pragma omp simd aligned(lFhbnd,lFhi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
{% endif %}
        lFhbnd[0][n + {{NUMBER_OF_UNKNOWNS}} * yz]
          += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (x + {{NUMBER_OF_DOF}} * yz)] * DGMatrices<pCompType>::FLCoeff[x];

        lFhbnd[Dimensions][n + {{NUMBER_OF_UNKNOWNS}} * yz]
          += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (x + {{NUMBER_OF_DOF}} * yz)] * DGMatrices<pCompType>::FRCoeff[x];
{% endif %}{# USE_FLUX #}
      }
    }
  }

  // y-direction: face 1 (left) and face 3(2D) / 4(3D) (right)
  for (int xz = 0; xz < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xz++) {
    // Matrix-Vector Products
    for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
{% if DIMENSIONS==3 %}
      const int z = xz / {{NUMBER_OF_DOF}};
      const int x = xz % {{NUMBER_OF_DOF}};
{% else %}
      const int z = 0;
      const int x = xz;
{% endif %}
      #ifdef SharedOMP
      #pragma omp simd aligned(lQhbnd,lQhi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
        lQhbnd[1][n + {{NUMBER_OF_DATA}} * xz]
          += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (x + {{NUMBER_OF_DOF}} * (y + {{NUMBER_OF_DOF_3D}} * z))] * DGMatrices<pCompType>::FLCoeff[y];

        lQhbnd[1+Dimensions][n + {{NUMBER_OF_DATA}} * xz]
          += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (x + {{NUMBER_OF_DOF}} * (y + {{NUMBER_OF_DOF_3D}} * z))] * DGMatrices<pCompType>::FRCoeff[y];
{% if USE_FLUX %}
{% if NUMBER_OF_DATA != NUMBER_OF_UNKNOWNS %}
      }
      #ifdef SharedOMP
      #pragma omp simd aligned(lFhbnd,lFhi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
{% endif %}
        lFhbnd[1][n + {{NUMBER_OF_UNKNOWNS}} * xz]
          += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (y + {{NUMBER_OF_DOF}} * xz) + {{1*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] * DGMatrices<pCompType>::FLCoeff[y];

        lFhbnd[1+Dimensions][n + {{NUMBER_OF_UNKNOWNS}} * xz]
          += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (y + {{NUMBER_OF_DOF}} * xz) + {{1*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] * DGMatrices<pCompType>::FRCoeff[y];
{% endif %}{# USE_FLUX #}
      }
    }
  }

{% if DIMENSIONS==3 %}
  // z-direction: face 2 (left) and face 5 (right)
  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF}}; xy++) {
    // Matrix-Vector Products
    for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
      #ifdef SharedOMP
      #pragma omp simd aligned(lQhbnd,lQhi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
        lQhbnd[2][n + {{NUMBER_OF_DATA}} * xy]
          += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (xy + {{NUMBER_OF_DOF*NUMBER_OF_DOF}} * z)] * DGMatrices<pCompType>::FLCoeff[z];

        lQhbnd[2+Dimensions][n + {{NUMBER_OF_DATA}} * xy]
          += lQhi[n + {{NUMBER_OF_DATA_PADDED}} * (xy + {{NUMBER_OF_DOF*NUMBER_OF_DOF}} * z)] * DGMatrices<pCompType>::FRCoeff[z];
{% if USE_FLUX %}
{% if NUMBER_OF_DATA != NUMBER_OF_UNKNOWNS %}
      }
      #ifdef SharedOMP
      #pragma omp simd aligned(lFhbnd,lFhi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
{% endif %}
        lFhbnd[2][n + {{NUMBER_OF_UNKNOWNS}} * xy]
          += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (z + {{NUMBER_OF_DOF}} * xy) + {{2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] * DGMatrices<pCompType>::FLCoeff[z];

        lFhbnd[2+Dimensions][n + {{NUMBER_OF_UNKNOWNS}} * xy]
          += lFhi[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (z + {{NUMBER_OF_DOF}} * xy) + {{2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}] * DGMatrices<pCompType>::FRCoeff[z];
{% endif %}{# USE_FLUX #}
      }
    }
  }
{% endif %}{# DIMENSIONS==3 #}

  //*****************************
  //****** Volume Integral ******
  //*****************************
  //[nk] We start accumulate volume terms of the corrector here as soon as the predictor is computed
  // Here we use that Kxi already consists 1D spatial weights. Two other spatial weights have the same
  // indexing, therefore they cancel out.
  pCompType coeffVolume[{{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}] __attribute__((aligned({{ALIGNMENT}})));
  for(int i=0; i<{{NUMBER_OF_DOF}}; i++) {
    for(int j=0; j<{{NUMBER_OF_DOF}}; j++) {
      coeffVolume[i * {{NUMBER_OF_DOF_PADDED}} + j] = Kxi_T[i * {{NUMBER_OF_DOF_PADDED}} + j] *
        timeStepSize * Quadrature<pCompType>::iweights1[i];
    }
  }

{% if USE_FLUX %}
  // Assume dx[0] == dx[1] == dx[2]
  for (int j = 0; j < {{NUMBER_OF_DOF_3D}}; j++) {
    for (int i = 0; i < {{NUMBER_OF_DOF}}; i++) {
      // x
      {{ m.gemm('lduh_x', 'lFhi', 'invDx[0]*coeffVolume', 'luh',
        '(j*'~NUMBER_OF_DOF~'+i)*'~(NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF)~'+'~(0*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)),
        '0',
        '(j*'~NUMBER_OF_DOF~'+i)*'~(NUMBER_OF_DATA*NUMBER_OF_DOF)) | indent(6) }}{##}

      // y
      {{ m.gemm('lduh_y', 'lFhi', 'invDx[1]*coeffVolume', 'luh',
        '(j*'~NUMBER_OF_DOF~'+i)*'~(NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF)~'+'~(1*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)),
        '0',
        '(j*'~(NUMBER_OF_DOF*NUMBER_OF_DOF)~'+i)*'~NUMBER_OF_DATA) | indent(6) }}{##}

{% if DIMENSIONS==3 %}
      // z
      {{ m.gemm('lduh_z', 'lFhi', 'invDx[2]*coeffVolume', 'luh',
        '(j*'~NUMBER_OF_DOF~'+i)*'~(NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF)~'+'~(2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)),
        '0',
        '(j*'~NUMBER_OF_DOF~'+i)*'~NUMBER_OF_DATA) | indent(6) }}{##}
{% endif %}
    }
  }
{% endif %}{# USE_FLUX #}

{% if USE_NCP or USE_SOURCE %}
  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    #ifdef SharedOMP
    #pragma omp simd aligned(luh,lShi:{{ALIGNMENT}})
    #endif
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
      luh[xyz * {{NUMBER_OF_DATA}} + n] += timeStepSize * lShi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
    }
  }
{% endif %}

  // Return number of Picard iterations, std::min to avoid doing a +1 if the loop has not been exited early.
  return std::min(numberOfPicardIterations + 1, {{2*NUMBER_OF_DOF+1}});
}

{% for PRECISION_NUM in range(0,PREDICTOR_COMPUTATION_PRECISIONS|length) %}
template int {{FULL_QUALIFIED_NAMESPACE}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                                   solver,
  {{CORRECTOR_COMPUTATION_PRECISION}}* NOALIAS                 lQhbnd[2*Dimensions],
  {{CORRECTOR_COMPUTATION_PRECISION}}* NOALIAS                 lFhbnd[2*Dimensions],
  {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS                luh,
  const tarch::la::Vector<Dimensions, double>&                      cellCentre,
  const tarch::la::Vector<Dimensions, double>&                      cellSize,
  const double                                                      timeStamp,
  const double                                                      timeStepSize
);
{% endfor %}

template <typename cStoreType, typename pCompType>
pCompType {{FULL_QUALIFIED_NAMESPACE}}::picardIteration(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  pCompType* NOALIAS                       lQi,
  pCompType* NOALIAS                       rhs,
  pCompType* NOALIAS                       lFi,
  pCompType* NOALIAS                       lSi,
  pCompType* NOALIAS                       gradQ,
  pCompType* NOALIAS                       new_lQi_slice,
  const cStoreType* const NOALIAS          luh,
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize,
  const double                                  timeStamp,
  const double                                  timeStepSize
) {

  const pCompType* Kxi    = DGMatrices<pCompType>::Kxi;
  const pCompType* iK1_T  = DGMatrices<pCompType>::iK1_T;

  const pCompType invDx[Dimensions] = {1. / cellSize[0], 1. / cellSize[1]
    {% if DIMENSIONS == 3 %}
      , 1. / cellSize[2]
    {% endif %}
  };

  const pCompType dtByDx[Dimensions] = {timeStepSize * invDx[0], timeStepSize * invDx[1]
    {% if DIMENSIONS == 3 %}
      , timeStepSize * invDx[2]
    {% endif %}
  };

  for (int t = 0; t < {{NUMBER_OF_DOF}}; t++) {  // Time DOFs
{% if USE_NCP %}
    std::memset(gradQ, 0, {{(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED*DIMENSIONS}} * sizeof(pCompType));

    // Compute the "derivatives" (contributions of the stiffness matrix)
    // x-direction (independent from the y- and z-derivatives)
    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
        {{ m.gemm('gradQ_x', 'lQi', 'invDx[0] * DGMatrices<pCompType>::dudx_T', 'gradQ',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF~'+y)*'~NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED,
          '0',
          '(z*'~NUMBER_OF_DOF~'+y)*'~NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF) | indent(10) }}{##}
      }
    }

    // y-direction (independent from the x- and z-derivatives)
    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        {{ m.gemm('gradQ_y', 'lQi', 'invDx[1] * DGMatrices<pCompType>::dudx_T', 'gradQ',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA_PADDED,
          '0',
          '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED~'+'~NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)) | indent(10) }}{##}
      }
    }

{% if DIMENSIONS==3 %}
    // z-direction (independent from the x- and y-derivatives)
    for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        {{ m.gemm('gradQ_z', 'lQi', 'invDx[2] * DGMatrices<pCompType>::dudx_T', 'gradQ',
          '((t*'~NUMBER_OF_DOF_3D*NUMBER_OF_DOF~'+y)*'~NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA_PADDED,
          '0',
          '(y*'~NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED~'+'~2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)) | indent(10) }}{##}
      }
    }
{% endif %}
{% endif %}{# USE_NCP #}

{% if USE_FLUX %}
    pCompType* F[{{DIMENSIONS}}];

    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
{% if DIMENSIONS==2 %}
      F[0] = &lFi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}}];
      F[1] = &lFi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];

      // [nk] Convert an xyz index into a {indexX, indexY} structure.
      // Note: In general, the flattened index is constructed from outer to inner dimensions in the following order: time -> z -> y -> x -> variables.
      const tarch::la::Vector<Dimensions, int> index           = {xyz % {{NUMBER_OF_DOF}}, xyz / {{NUMBER_OF_DOF}} };
{% endif %}
{% if DIMENSIONS==3 %}
      F[0] = &lFi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}}];
      F[1] = &lFi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];
      F[2] = &lFi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + {{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}];
      // [nk] Convert an xyz index into a {indexX, indexY, indexZ} structure.
      // Note: In general, the flattened index is constructed from outer to inner dimensions in the following order: time -> z -> y -> x -> variables.
      const tarch::la::Vector<Dimensions, int> index           = {xyz % {{NUMBER_OF_DOF}}, (xyz % {{NUMBER_OF_DOF**(DIMENSIONS - 1)}}) / {{NUMBER_OF_DOF}} xyz / {{NUMBER_OF_DOF ** DIMENSIONS}}};
{% endif %}
      // [nk] Calculate the exact time-node position for the flux calculation.
      // -- Time positions are calculated as: t = tk + dt * tau
      //    - t (timePosition)        : Time node in the global system.
      //    - t_{k}                   : Current timestamp.
      //    - dt = t_{k + 1} - t_{k}  : Current time step size.
      //    - tau                     : Time node in the reference cell (normalized to [0, 1]).
      const double timePosition = timeStamp + Quadrature<pCompType>::nodes[t] * timeStepSize;

      // [nk] Calculate the exact space-node position for the flux calculation.
      // -- Space positions are calculated as: x = xc + dx * (r - 0.5)
      //    - x (nodePosition) : Node coordinates in the global coordinate system.
      //    - r                : Node coordinates in the reference cell (normalized to [0, 1]^dimensions).
      //    - dx               : Spatial step size.
      //    - xc               : Cell center coordinates in the global coordinate system.
      //    - rc = 0.5         : Cell center coordinates in the reference cell.
      tarch::la::Vector<Dimensions,double> nodePosition = ::exahype2::dg::getQuadraturePoint(cellCentre, cellSize, index, {{NUMBER_OF_DOF - 1}}, Quadrature<pCompType>::nodes);

      // [nk] Measure the flux contribution to the local cell time evaluation.
      // Note: No connections with neighboring cells are considered during this step (e.g., no Riemann solver is used).
      //       The numerical fluxes from neighbors (penalty) are considered during the correction stage.
      for(int d = 0; d < Dimensions; d++){
        solver.flux(
          lQi + (t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_DATA_PADDED}},
          nodePosition,
          cellSize,
          timePosition,
          timeStepSize,
          d,
          F[d]
        );
      }
    }
{% endif %}{# USE_FLUX #}

    // Compute the contribution of the initial condition (uh) to the right-hand side (rhs).
    // [nk] ADER-DG allows discontinuities in time as well. Time evaluation ranges from 0 to infinity.
    // Following the upwinding principle, the "upstream" value of the corrector (luh) from the previous time step is used.
    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
      const pCompType weight = Quadrature<pCompType>::weights3[xyz] * DGMatrices<pCompType>::FLCoeff[t];
      #ifdef SharedOMP
      #pragma omp simd aligned(rhs,luh:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        rhs[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * (xyz + {{NUMBER_OF_DOF**DIMENSIONS}} * t)] = weight * luh[n + {{NUMBER_OF_DATA}} * xyz];
      }
    }

{% if USE_FLUX %}
    // Compute the "derivatives" (contributions of the stiffness matrix).
    // [nk] Note: Kx_i here is not the pure stiffness matrix K itself but a Kronecker product of 1D weights (weights1) with it.
    // Therefore, weights3 is not a 3D spatial weight but a combination (Kronecker product) of 1D time weights and 2D spatial weights.

    // x-direction (independent from the y- and z-derivatives)
    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
        {{ m.gemm('rhs_x', 'lFi', 'coeffRhsX', 'rhs',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF~'+y)*'~NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF,
          '0',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF~'+y)*'~NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF,
          true_B='Kxi',
          true_alpha='-Quadrature<pCompType>::weights3[t*'~NUMBER_OF_DOF*NUMBER_OF_DOF_3D~'+z*'~NUMBER_OF_DOF~'+y] * dtByDx[0]') | indent(10) }}{##}
      }
    }

    // y-direction (independent from the x- and z-derivatives)
    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        {{ m.gemm('rhs_y', 'lFi', 'coeffRhsY', 'rhs',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED~'+'~1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED,
          '0',
          '((t*'~NUMBER_OF_DOF_3D~'+z)*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED,
          true_B='Kxi',
          true_alpha='-Quadrature<pCompType>::weights3[t*'~NUMBER_OF_DOF*NUMBER_OF_DOF_3D~'+z*'~NUMBER_OF_DOF~'+x] * dtByDx[1]') | indent(10) }}{##}
      }
    }

{% if DIMENSIONS==3 %}
    // z-direction (independent from the x- and y-derivatives)
    for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        {{ m.gemm('rhs_z', 'lFi','coeffRhsZ', 'rhs',
          '((t*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+y)*'~NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED~'+'~2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED,
          '0',
          '((t*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+y)*'~NUMBER_OF_DOF~'+x)*'~NUMBER_OF_UNKNOWNS_PADDED,
          true_B='Kxi',
          true_alpha='-Quadrature<pCompType>::weights3[t*'~NUMBER_OF_DOF*NUMBER_OF_DOF_3D~'+y*'~NUMBER_OF_DOF~'+x] * dtByDx[2]') | indent(10) }}{##}
      }
    }
{% endif %}
{% endif %}{# USE_FLUX #}

{% if USE_NCP or USE_SOURCE %}
{% if USE_NCP %}
    pCompType tmpNCP[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned({{ALIGNMENT}})));
    pCompType gradQ_PDE[{{NUMBER_OF_UNKNOWNS*DIMENSIONS}}] __attribute__((aligned({{ALIGNMENT}})));
{% endif %}

    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {

// [nk] Calculate the exact space and time nodes' positions for the source and ncp calculation.
{% if USE_NCP or USE_SOURCE %}
{% if DIMENSIONS==2 %}
      const tarch::la::Vector<Dimensions, int> index           = {xyz % {{NUMBER_OF_DOF}}, xyz / {{NUMBER_OF_DOF}} };
{% endif %}
{% if DIMENSIONS==3 %}
      const tarch::la::Vector<Dimensions, int> index           = {xyz % {{NUMBER_OF_DOF}}, (xyz % {{NUMBER_OF_DOF**(DIMENSIONS - 1)}}) / {{NUMBER_OF_DOF}} xyz / {{NUMBER_OF_DOF ** DIMENSIONS}}};
{% endif %}
      const double timePosition = timeStamp + Quadrature<pCompType>::nodes[t] * timeStepSize;
      tarch::la::Vector<Dimensions,double> nodePosition = ::exahype2::dg::getQuadraturePoint(cellCentre, cellSize, index, {{NUMBER_OF_DOF - 1}}, Quadrature<pCompType>::nodes);
{% endif %}

{% if USE_NCP %}
      std::copy_n(gradQ + {{NUMBER_OF_UNKNOWNS_PADDED}} * xyz, {{NUMBER_OF_UNKNOWNS}}, gradQ_PDE);
      std::copy_n(gradQ + {{NUMBER_OF_UNKNOWNS_PADDED}} * xyz + {{1*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}, {{NUMBER_OF_UNKNOWNS}}, gradQ_PDE + {{1*NUMBER_OF_UNKNOWNS}});
{% if DIMENSIONS==3 %}
      std::copy_n(gradQ + {{NUMBER_OF_UNKNOWNS_PADDED}} * xyz + {{2*NUMBER_OF_UNKNOWNS_PADDED*(NUMBER_OF_DOF**DIMENSIONS)}}, {{NUMBER_OF_UNKNOWNS}}, gradQ_PDE + {{2*NUMBER_OF_UNKNOWNS}});
{% endif %}
{% endif %}

{% if USE_SOURCE %}
      //[nk] The indexing for the source should be the same as for NCP
      solver.algebraicSource(
        lQi + (t * {{NUMBER_OF_DOF ** DIMENSIONS}} + xyz) * {{NUMBER_OF_DATA_PADDED}},
        nodePosition,
        cellSize,
        timePosition,
        timeStepSize,
        lSi + (t * {{NUMBER_OF_DOF ** DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}}
      );
{% else %}
      std::fill_n(&lSi[0 + (t * {{NUMBER_OF_DOF ** DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}}], {{NUMBER_OF_UNKNOWNS_PADDED}}, 0.0);
{% endif %}

{% if USE_NCP %}
      for (int d = 0; d < Dimensions; d++) {
        // [nk] NCP are computed at nodes positions during the prediction stage, since only local evaluation should be considered
        solver.nonconservativeProduct(
          lQi + (t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_DATA_PADDED}},
          gradQ_PDE + {{NUMBER_OF_UNKNOWNS_PADDED}} * d,
          nodePosition,
          cellSize,
          timePosition,
          timeStepSize,
          d,
          tmpNCP
        );

        #ifdef SharedOMP
        #pragma omp simd aligned(lSi,tmpNCP:{{ALIGNMENT}})
        #endif
        for(int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
          //[nk] NCP are computed as a part of source terms in the predictor
          lSi[n + (t * {{NUMBER_OF_DOF ** DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}}] -= tmpNCP[n];
        }
      }
{% endif %}

      // Update rhs
      const pCompType updateSize = Quadrature<pCompType>::weights1[t] * Quadrature<pCompType>::weights3[xyz] * timeStepSize;
      #ifdef SharedOMP
      #pragma omp simd aligned(rhs,lSi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
        rhs[n + (t * {{NUMBER_OF_DOF ** DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}}]
          += updateSize * lSi[n + (t * {{NUMBER_OF_DOF ** DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}}];
      }
    }
{% endif %}{# USE_NCP or USE_SOURCE #}
  } // t

  // 3: Multiply with (K1)^(-1) to get the discrete time integral of the discrete Picard iteration
  pCompType sqRes = 0.0;
  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    {{ m.gemm('lqi', 'rhs', 's_m_QSlice', 'new_lQi_slice',
      NUMBER_OF_UNKNOWNS_PADDED~'*xyz', '0', '0',
      true_B='iK1_T',
      true_alpha='Quadrature<pCompType>::iweights3[xyz]') | indent(6) }}{##}

    for (int t = 0; t < {{NUMBER_OF_DOF}}; t++) {
      for (int n = 0; n< {{NUMBER_OF_UNKNOWNS}}; n++) {
        sqRes += (new_lQi_slice[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * t] - lQi[n + {{NUMBER_OF_DATA_PADDED}} * (xyz + {{NUMBER_OF_DOF**DIMENSIONS}} * t)])
          * (new_lQi_slice[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * t] - lQi[n + {{NUMBER_OF_DATA_PADDED}} * (xyz + {{NUMBER_OF_DOF**DIMENSIONS}} * t)]);

        lQi[n + {{NUMBER_OF_DATA_PADDED}} * (xyz + {{NUMBER_OF_DOF**DIMENSIONS}} * t)] = new_lQi_slice[n + {{NUMBER_OF_UNKNOWNS_PADDED}} * t];
      }
    }
  }

  return sqRes; // TODO: Is this the wrong return statement? We expect an integer to be returned. Is this return value used at all?
}

{% for PRECISION_NUM in range(0,PREDICTOR_COMPUTATION_PRECISIONS|length) %}
template {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}} {{FULL_QUALIFIED_NAMESPACE}}::picardIteration(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                                         solver,
  {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* NOALIAS       lQi,
  {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* NOALIAS       rhs,
  {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* NOALIAS       lFi,
  {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* NOALIAS       lSi,
  {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* NOALIAS       gradQ,
  {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}* NOALIAS       new_lQi_slice,
  const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS                luh,
  const tarch::la::Vector<Dimensions, double>&                            cellCentre,
  const tarch::la::Vector<Dimensions, double>&                            cellSize,
  const double                                                            timeStamp,
  const double                                                            timeStepSize
);
{% endfor %}
