// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FusedSpaceTimePredictorVolumeIntegral{{NAME_SUFFIX}}.h"

#include "DGMatrices.h"
#include "Quadrature.h"
#include "BufferSizes.h"

#include "tarch/CompilerSpecificSettings.h"

{% if USE_POINT_SOURCE %}
{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
{{BASIS_FUNCTIONS}}
{% for item in NAMESPACE -%}
}
{%- endfor %}
{% endif %}

/**
 * Copies material parameters through each predictor representation because
 * linear kernels do not evolve them.
 */
template <typename cStoreType, typename pCompType, typename pStoreType>
int {{FULL_QUALIFIED_NAMESPACE}}::fusedSpaceTimePredictorVolumeIntegral{{NAME_SUFFIX}}(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&  solver,
  pStoreType* const NOALIAS                         lQhbnd[TWO_TIMES_D],
  pStoreType* const NOALIAS                         lFhbnd[TWO_TIMES_D],
  cStoreType* const NOALIAS                         luh,
  const tarch::la::Vector<DIMENSIONS, double>&      cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&      cellSize,
  const double                                      timeStamp,
  const double                                      timeStepSize
{% if USE_POINT_SOURCE %}
  , const std::vector<int>&                         pointSources
{% endif %}
) {
  static thread_local {{FULL_QUALIFIED_NAMESPACE}}::PredictorScratchBuffer{{NAME_SUFFIX}}<pCompType> scratch;

  pCompType* memory = scratch.get(getFusedSTPVISize());

  pCompType* lQi  = memory + getlQiShift();
  pCompType* lQhi = memory + getlQhiShift();

  pCompType* lFi  = memory + getlFiShift();
  pCompType* lFhi = memory + getlFhiShift();

{% if USE_SOURCE %}
  pCompType* lSi  = memory + getlSiShift();
  pCompType* lShi = memory + getlShiShift();
{% endif %}

{% if USE_NCP %}
  pCompType* gradQ = memory + getGradQShift();
{% endif %}

  const tarch::la::Vector<DIMENSIONS, pCompType> invDx = convertScalar<pCompType>(invertEntries(cellSize));

{% if USE_FLUX %}
  pCompType tmpArray[{{TMP_ARRAY_SIZE}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
{% endif %}

  // The initial condition supplies the zeroth time derivative.
  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    auto* const localPredictorState = lQi + xyz * {{NUMBER_OF_DATA_PADDED}};
    std::fill_n(localPredictorState, {{NUMBER_OF_DATA_PADDED}}, pCompType(0.0));
    std::copy_n(luh + xyz * {{NUMBER_OF_DATA}}, {{NUMBER_OF_DATA}}, localPredictorState);
  }

  std::memset(lFhi, 0.0, {{(DIMENSIONS*NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));

{% if USE_SOURCE %}
  std::memset(lShi, 0.0, {{(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));
{% endif %}

  // The ith Taylor derivative enters a time average with dt^i / (i + 1)!.
  pCompType dtavFac = 1.0;

{% if USE_POINT_SOURCE %}
  pCompType PSi[getPSiSize()] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));

  {
    std::memset(PSi, 0, {{(NUMBER_OF_DOF+1)*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));

    pCompType tn = 0;
    double x1[{{DIMENSIONS}}];
    const pCompType ivolume = 1.0 / (cellSize[0] * cellSize[1]{{' * cellSize[2]' if DIMENSIONS==3}});

    pCompType forceVectorSource[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
    std::memset(forceVectorSource, 0, {{NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));

    for (int t = 0; t < {{NUMBER_OF_DOF+1}}; t++) {
      // The first point is the current time; later points use Gauss-Legendre nodes.
      t == 0 ? tn = timeStamp : tn = timeStepSize * Quadrature<pCompType>::nodes[t - 1] + timeStamp;
      for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
{% if DIMENSIONS==3 %}
        x1[2] = cellCentre[2] + cellSize[2] * (Quadrature<pCompType>::nodes[z] - 0.5);
{% endif %}
        for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
          x1[1] = cellCentre[1] + cellSize[1] * (Quadrature<pCompType>::nodes[y] - 0.5);
          for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
            x1[0] = cellCentre[0] + cellSize[0] * (Quadrature<pCompType>::nodes[x] - 0.5);
            for (int p : pointSources) {
{% if STATELESS_POINT_SOURCE %}
              {{FULL_QUALIFIED_SOLVER_NAME}}::pointSource(
{% else %}
              solver.pointSource(
{% endif %}
                luh + ((z * {{NUMBER_OF_DOF_3D}} + y)*{{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA}},
                x1,
                tn,
                timeStepSize,
                forceVectorSource,
                p
              );

{% if STATELESS_INIT_POINT_SOURCES %}
              const double* const pointSourceLocation = {{FULL_QUALIFIED_SOLVER_NAME}}::pointSourceLocation[p];
{% else %}
              const double* const pointSourceLocation = solver.pointSourceLocation[p];
{% endif %}

              // Basis functions use the source's reference coordinates;
              // reciprocal quadrature weights and inverse cell volume normalise its delta.
              const pCompType update = basisFunction[x]((pointSourceLocation[0] - cellCentre[0]) * invDx[0] + 0.5)
                                     * basisFunction[y]((pointSourceLocation[1] - cellCentre[1]) * invDx[1] + 0.5)
{% if DIMENSIONS==3 %}
                                     * basisFunction[z]((pointSourceLocation[2] - cellCentre[2]) * invDx[2] + 0.5)
{% endif %}
                                     * Quadrature<pCompType>::iweights3[(z * {{NUMBER_OF_DOF_3D}} + y) * {{NUMBER_OF_DOF}} + x] * ivolume;

              SIMD(aligned(PSi,forceVectorSource:PEANO_WITH_ALIGNMENT))
              for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
                PSi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += update * forceVectorSource[n];
              }
            }
          }
        }
      }
    }
  }

  // Time quadrature adds the point source to the zeroth derivative.
  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    for (int t = 0; t < {{NUMBER_OF_DOF}}; t++) {
      SIMD(aligned(lQi,PSi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        lQi[xyz * {{NUMBER_OF_DATA_PADDED}} + n]
          += timeStepSize * Quadrature<pCompType>::weights1[t] * PSi[((t + 1) * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
      }
    }
  }

  // FLCoeff initialises the temporal point-source derivative operator.
  pCompType derPS[{{NUMBER_OF_DOF_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
  std::copy_n(DGMatrices<pCompType>::FLCoeff, {{NUMBER_OF_DOF}}, derPS);
{% endif %}

  std::copy_n(lQi, {{NUMBER_OF_DATA_PADDED*NUMBER_OF_DOF**DIMENSIONS}}, lQhi);

  for (int i = 0; i < {{NUMBER_OF_DOF}}; i++) {
    std::fill_n(lFi, {{DIMENSIONS*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}, pCompType(0.0));
{% if USE_SOURCE %}
    std::fill_n(lSi, {{(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}, pCompType(0.0));
{% endif %}
{% if USE_FLUX %}
    pCompType* F[{{DIMENSIONS}}];
    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
      F[0] = lFi + xyz * {{NUMBER_OF_UNKNOWNS_PADDED}};
      F[1] = lFi + xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}};
{% if DIMENSIONS==3 %}
      F[2] = lFi + xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + {{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}};
{% endif %}

      for (int d = 0; d < DIMENSIONS; d++) {
{% if STATELESS_FLUX %}
        {{FULL_QUALIFIED_SOLVER_NAME}}::flux(
{% else %}
        solver.flux(
{% endif %}
          lQi + xyz * {{NUMBER_OF_DATA_PADDED}},
          cellCentre,
          cellSize,
          timeStamp,
          timeStepSize,
          d,
          F[d]
        );
      }
    }

    for (int yz = 0; yz < {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF}}; yz++) {
      std::memset(tmpArray, 0, {{TMP_ARRAY_SIZE}} * sizeof(pCompType));

      UNROLL({{NUMBER_OF_DOF}})
      for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
        UNROLL({{NUMBER_OF_DOF}})
        for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
          SIMD(aligned(tmpArray,lFi:PEANO_WITH_ALIGNMENT))
          for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
            tmpArray[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3] += lFi[yz * {{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} + it_2 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3]
              * invDx[0] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF_PADDED}} + it_2];
          }
        }
      }
      std::copy_n(tmpArray, {{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}, lFi + yz * {{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}});
    }

    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        std::memset(tmpArray, 0, {{TMP_ARRAY_SIZE}} * sizeof(pCompType));

        UNROLL({{NUMBER_OF_DOF}})
        for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
          UNROLL({{NUMBER_OF_DOF}})
          for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
            SIMD(aligned(tmpArray,lFi:PEANO_WITH_ALIGNMENT))
            for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
              tmpArray[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3]
                += lFi[{{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} + z * {{NUMBER_OF_DOF*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} + x * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_2 * {{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} + it_3]
                * invDx[1] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF_PADDED}} + it_2];
            }
          }
        }

        UNROLL({{NUMBER_OF_DOF}})
        for (int l = 0; l < {{NUMBER_OF_DOF}}; l++) {
          SIMD(aligned(lFi,tmpArray:PEANO_WITH_ALIGNMENT))
          for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
            lFi[((z * {{NUMBER_OF_DOF}} + l) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}]
              = tmpArray[l * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
          }
        }
      }
    }
{% if DIMENSIONS==3 %}
    for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF}}; xy++) {
      std::memset(tmpArray, 0, {{TMP_ARRAY_SIZE}} * sizeof(pCompType));

      UNROLL({{NUMBER_OF_DOF}})
      for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
        UNROLL({{NUMBER_OF_DOF}})
        for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
          SIMD(aligned(tmpArray,lFi:PEANO_WITH_ALIGNMENT))
          for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
            tmpArray[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3]
              += lFi[{{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} + xy * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_2 * {{NUMBER_OF_DOF*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} + it_3]
              * invDx[2] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF_PADDED}} + it_2];
          }
        }
      }

      UNROLL({{NUMBER_OF_DOF}})
      for (int l = 0; l < {{NUMBER_OF_DOF}}; l++) {
        SIMD(aligned(lFi,tmpArray:PEANO_WITH_ALIGNMENT))
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
          lFi[{{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} + (l * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n]
            = tmpArray[l * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
        }
      }
    }
{% endif %}
{% endif %}{# USE_FLUX #}

{% if USE_NCP %}
    for (int yz = 0; yz < {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF}}; yz++) {
      std::memset(gradQ, 0, {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));

      UNROLL({{NUMBER_OF_DOF}})
      for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
        UNROLL({{NUMBER_OF_DOF}})
        for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
          SIMD(aligned(lQi,gradQ:PEANO_WITH_ALIGNMENT))
          for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
            gradQ[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3] += lQi[yz*{{NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}} + it_2 * {{NUMBER_OF_DATA_PADDED}} + it_3]
              * invDx[0] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF_PADDED}} + it_2];
          }
        }
      }

      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        pCompType tmpNcp[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT))) = {0.0};
{% if STATELESS_NCP %}
        {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(
{% else %}
        solver.nonconservativeProduct(
{% endif %}
          lQi + yz*{{NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}} + x * {{NUMBER_OF_DATA_PADDED}},
          gradQ + x*{{NUMBER_OF_UNKNOWNS_PADDED}},
          cellCentre,
          cellSize,
          timeStamp,
          timeStepSize,
          0,
          tmpNcp
        );

        SIMD(aligned(lFi,tmpNcp:PEANO_WITH_ALIGNMENT))
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
          lFi[yz*{{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} + x * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += tmpNcp[n];
        }
      }
    }

    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        std::memset(gradQ, 0, {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));

        UNROLL({{NUMBER_OF_DOF}})
        for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
          UNROLL({{NUMBER_OF_DOF}})
          for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
            SIMD(aligned(lQi,gradQ:PEANO_WITH_ALIGNMENT))
            for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
              gradQ[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3] += lQi[z * {{NUMBER_OF_DOF*NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}} + x * {{NUMBER_OF_DATA_PADDED}} + it_2 * {{NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}} + it_3]
                * invDx[1] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF_PADDED}} + it_2];
            }
          }
        }

        for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
          pCompType tmpNcp[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT))) = {0.0};
{% if STATELESS_NCP %}
          {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(
{% else %}
          solver.nonconservativeProduct(
{% endif %}
            lQi + z * {{NUMBER_OF_DOF*NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}} + x * {{NUMBER_OF_DATA_PADDED}} + y * {{NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}},
            gradQ + y*{{NUMBER_OF_UNKNOWNS_PADDED}},
            cellCentre,
            cellSize,
            timeStamp,
            timeStepSize,
            1,
            tmpNcp
          );

          SIMD(aligned(lFi,tmpNcp:PEANO_WITH_ALIGNMENT))
          for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
            lFi[((z * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}]
              += tmpNcp[n];
          }
        }
      }
    }

{% if DIMENSIONS==3 %}
    for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF}}; xy++) {
      std::memset(gradQ, 0, {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));

      UNROLL({{NUMBER_OF_DOF}})
      for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
        UNROLL({{NUMBER_OF_DOF}})
        for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
          SIMD(aligned(lQi,gradQ:PEANO_WITH_ALIGNMENT))
          for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
            gradQ[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3] += lQi[xy * {{NUMBER_OF_DATA_PADDED}} + it_2 * {{NUMBER_OF_DOF*NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}} + it_3]
              * invDx[2] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF_PADDED}} + it_2];
          }
        }
      }

      for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
        pCompType tmpNcp[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT))) = {0.0};
{% if STATELESS_NCP %}
        {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(
{% else %}
        solver.nonconservativeProduct(
{% endif %}
          lQi + xy * {{NUMBER_OF_DATA_PADDED}} + z * {{NUMBER_OF_DOF*NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}},
          gradQ + z*{{NUMBER_OF_UNKNOWNS_PADDED}},
          cellCentre,
          cellSize,
          timeStamp,
          timeStepSize,
          2,
          tmpNcp
        );

        SIMD(aligned(lFi,tmpNcp:PEANO_WITH_ALIGNMENT))
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
          lFi[{{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} + (z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n]
            += tmpNcp[n];
        }
      }
    }
{% endif %}{# DIMENSIONS 3 #}
{% endif %}{# USE_NCP #}

{% if USE_MATERIAL_PARAMETERS %}
    {
      pCompType* F[{{DIMENSIONS}}];
      for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
        F[0] = lFi + xyz * {{NUMBER_OF_UNKNOWNS_PADDED}};
        F[1] = lFi + xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}};
{% if DIMENSIONS==3 %}
        F[2] = lFi + xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + {{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}};
{% endif %}

        for (int d = 0; d < DIMENSIONS; d++) {
{% if STATELESS_MATERIAL_PARAMETERS %}
          {{FULL_QUALIFIED_SOLVER_NAME}}::multiplyMaterialParameterMatrix(
{% else %}
          solver.multiplyMaterialParameterMatrix(
{% endif %}
            lQi + xyz * {{NUMBER_OF_DATA_PADDED}},
            cellCentre,
            cellSize,
            timeStamp,
            timeStepSize,
            d,
            F[d]
          );
        }
      }
    }
{% endif %}{# USE_MATERIAL_PARAMETERS #}

{% if USE_SOURCE %}
    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
{% if STATELESS_SOURCE_TERM %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::sourceTerm(
{% else %}
      solver.sourceTerm(
{% endif %}
        lQi + xyz * {{NUMBER_OF_DATA_PADDED}},
        cellCentre,
        cellSize,
        timeStamp,
        timeStepSize,
        lSi + xyz * {{NUMBER_OF_UNKNOWNS_PADDED}}
      );
    }
{% endif %}{# USE_SOURCE #}

    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
      SIMD(aligned(lFhi,lFi{{',lShi,lSi' if USE_SOURCE else ''}}:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
        lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += dtavFac * lFi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
        lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}] += dtavFac * lFi[{{1 * (NUMBER_OF_DOF**DIMENSIONS) * NUMBER_OF_UNKNOWNS_PADDED}} + xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
{% if DIMENSIONS==3 %}
        lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}] += dtavFac * lFi[{{2 * (NUMBER_OF_DOF**DIMENSIONS) * NUMBER_OF_UNKNOWNS_PADDED}} + xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
{% endif %}
{% if USE_SOURCE %}
        lShi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += dtavFac * lSi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
{% endif %}
      }
    }

    // The final derivative contributes no later predictor state.
    if (i == {{NUMBER_OF_DOF-1}}) {
      break;
    }

    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
      SIMD(aligned(lQi,lFi{{',lSi' if USE_SOURCE else ''}}:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        lQi[xyz*{{NUMBER_OF_DATA_PADDED}} + n] = -(
            lFi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n]
          + lFi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1 * (NUMBER_OF_DOF**DIMENSIONS) * NUMBER_OF_UNKNOWNS_PADDED}}]
{% if DIMENSIONS==3 %}
          + lFi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2 * (NUMBER_OF_DOF**DIMENSIONS) * NUMBER_OF_UNKNOWNS_PADDED}}]
{% endif %}
{% if USE_SOURCE %}
          + lSi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n]
{% endif %}
        );
      }
    }

{% if USE_POINT_SOURCE %}
    for (int t = 0; t < {{NUMBER_OF_DOF}}; t++) {
      for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
        SIMD(aligned(lQi,PSi,derPS:PEANO_WITH_ALIGNMENT))
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
          lQi[xyz * {{NUMBER_OF_DATA_PADDED}} + n] += derPS[t] * PSi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
        }
      }
    }

    pCompType derPSBuffer[{{NUMBER_OF_DOF_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
    std::memset(derPSBuffer, 0.0, {{NUMBER_OF_DOF_PADDED}} * sizeof(pCompType));
    for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
      SIMD(aligned(derPSBuffer,derPS:PEANO_WITH_ALIGNMENT))
      for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
        derPSBuffer[it_2] += derPS[it_1] * DGMatrices<pCompType>::dudx[it_1 * {{NUMBER_OF_DOF}} + it_2];
      }
    }
    // std::copy_n(derPSBuffer, {{NUMBER_OF_DOF_PADDED}}, derPS);
{% endif %}

    dtavFac *= static_cast<pCompType>(timeStepSize / (i + 2));

    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
      SIMD(aligned(lQhi,lQi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        lQhi[xyz * {{NUMBER_OF_DATA_PADDED}} + n] += dtavFac * lQi[xyz * {{NUMBER_OF_DATA_PADDED}} + n];
      }
    }
  } // i

{% if USE_POINT_SOURCE %}
  pCompType sumArray[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    std::memset(sumArray, 0, {{NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));
    for (int n0 = 1; n0 < {{NUMBER_OF_DOF+1}}; n0++) {
      const pCompType coeff = Quadrature<pCompType>::weights1[n0 - 1] / {{DIMENSIONS}};
      SIMD(aligned(sumArray,PSi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
        sumArray[n] += coeff * PSi[(n0 * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
      }
    }
    SIMD(aligned(lFhi,sumArray:PEANO_WITH_ALIGNMENT))
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
      lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += sumArray[n];
      lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS) * NUMBER_OF_UNKNOWNS_PADDED}}] += sumArray[n];
{% if DIMENSIONS==3 %}
      lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*(NUMBER_OF_DOF**DIMENSIONS) * NUMBER_OF_UNKNOWNS_PADDED}}] += sumArray[n];
{% endif %}
    }
  }
{% endif %}

  for (int i = 0; i < 2 * DIMENSIONS; i++) {
    std::memset(lQhbnd[i], 0, {{NUMBER_OF_DATA*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}} * sizeof(pStoreType));
    std::memset(lFhbnd[i], 0, {{NUMBER_OF_UNKNOWNS*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}} * sizeof(pStoreType));
  }

  for (int yz = 0; yz < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; yz++) {
    for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
      SIMD(aligned(lQhi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
        lQhbnd[0][yz * {{NUMBER_OF_DATA}} + n] += lQhi[(yz * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FLCoeff[x];
        lQhbnd[DIMENSIONS][yz * {{NUMBER_OF_DATA}} + n] += lQhi[(yz * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FRCoeff[x];
      }
      SIMD(aligned(lFhi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        lFhbnd[0][yz * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[(yz * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] * DGMatrices<pCompType>::FLCoeff[x];
        lFhbnd[DIMENSIONS][yz * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[(yz * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] * DGMatrices<pCompType>::FRCoeff[x];
      }
    }
  }

  for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
    for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
      for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
        SIMD(aligned(lQhi:PEANO_WITH_ALIGNMENT))
        for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
          lQhbnd[1][(z * {{NUMBER_OF_DOF_3D}} + x) * {{NUMBER_OF_DATA}} + n] += lQhi[((z * {{NUMBER_OF_DOF_3D}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FLCoeff[y];
          lQhbnd[1+DIMENSIONS][(z * {{NUMBER_OF_DOF_3D}} + x) * {{NUMBER_OF_DATA}} + n] += lQhi[((z * {{NUMBER_OF_DOF_3D}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FRCoeff[y];
        }
        SIMD(aligned(lFhi:PEANO_WITH_ALIGNMENT))
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
          lFhbnd[1][(z * {{NUMBER_OF_DOF_3D}} + x) * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[((z * {{NUMBER_OF_DOF_3D}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}] * DGMatrices<pCompType>::FLCoeff[y];
          lFhbnd[1+DIMENSIONS][(z * {{NUMBER_OF_DOF_3D}} + x) * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[((z * {{NUMBER_OF_DOF_3D}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}] * DGMatrices<pCompType>::FRCoeff[y];
        }
      }
    }
  }

{% if DIMENSIONS==3 %}
  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
      SIMD(aligned(lQhi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
        lQhbnd[2][xy * {{NUMBER_OF_DATA}} + n] += lQhi[(z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FLCoeff[z];
        lQhbnd[2+DIMENSIONS][xy * {{NUMBER_OF_DATA}} + n] += lQhi[(z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FRCoeff[z];
      }
      SIMD(aligned(lFhi:PEANO_WITH_ALIGNMENT))
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        lFhbnd[2][xy * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[(z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}] * DGMatrices<pCompType>::FLCoeff[z];
        lFhbnd[2+DIMENSIONS][xy * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[(z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}] * DGMatrices<pCompType>::FRCoeff[z];
      }
    }
  }
{% endif %}

  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    SIMD(aligned(lFhi{{',lShi' if USE_SOURCE else ''}}:PEANO_WITH_ALIGNMENT))
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        luh[xyz*{{NUMBER_OF_DATA}} + n] -= static_cast<cStoreType>(timeStepSize *
            (
{% if USE_FLUX or USE_NCP %}
                lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{0*NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF**DIMENSIONS}}] // x
              + lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF**DIMENSIONS}}] // y
{% if DIMENSIONS==3 %}
              + lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF**DIMENSIONS}}] // z
{% endif %}
{% endif %}
{% if USE_SOURCE %}
              {{'+' if USE_FLUX or USE_NCP }} lShi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n]
{% endif %}
            ));
    }
  }

  // Linear predictors do not run Picard iterations.
  return -1;
}

{% for PRECISION_NUM in range(0,PREDICTOR_COMPUTATION_PRECISIONS|length) %}
template int {{FULL_QUALIFIED_NAMESPACE}}::fusedSpaceTimePredictorVolumeIntegral{{NAME_SUFFIX}}<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                     solver,
  {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS  lQhbnd[TWO_TIMES_D],
  {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS  lFhbnd[TWO_TIMES_D],
  {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS       luh,
  const tarch::la::Vector<DIMENSIONS, double>&        cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&        cellSize,
  const double                                        timeStamp,
  const double                                        timeStepSize
{% if USE_POINT_SOURCE %}
  , const std::vector<int>&                           pointSources
{% endif %}
);
{% endfor %}
