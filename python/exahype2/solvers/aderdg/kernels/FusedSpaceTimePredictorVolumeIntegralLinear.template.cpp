// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FusedSpaceTimePredictorVolumeIntegral{{NAME_SUFFIX}}.h"

#include "DGMatrices.h"
#include "Quadrature.h"
#include "BufferSizes.h"
{% if USE_LIBXSMM %}
#include "Gemms.h"
{% endif %}

#include "tarch/accelerator/MemoryManager.h"

#include <cstring>
#include <algorithm>

{% if USE_POINT_SOURCE %}
{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
{{BASIS_FUNCTIONS}}
{% for item in NAMESPACE -%}
}
{%- endfor %}
{% endif %}

{% import 'Gemms.template.h' as m with context %}{# get template macros #}

/*
 * For the linear kernels, we need the material parameters in the
 * space-time predictor lQi, time-averaged predictor lQhi,
 * and extrapolated predictor lQhbnd.
 * Currently we simply copy them over from the solution array.
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
  pCompType memory[getFusedSTPVISize()] __attribute__((aligned({{ALIGNMENT}})));
  // TODO: Use the memory manager for very high ADER-DG orders where the stack is not enough or when offloading to the GPU
  //pCompType* memory = tarch::accelerator::MemoryManager::getInstance().allocate<pCompType>(getFusedSTPVISize(), tarch::MemoryLocation::Heap, tarch::accelerator::Device::DefaultDevice);

  pCompType* lQi   = memory + getlQiShift();
  pCompType* lQhi  = memory + getlQhiShift();

  pCompType* lFi   = memory + getlFiShift();
  pCompType* lFhi  = memory + getlFhiShift();

{% if USE_SOURCE %}
  pCompType* lSi   = memory + getlSiShift();
  pCompType* lShi  = memory + getlShiShift();
{% endif %}

{% if USE_NCP %}
  pCompType* gradQ = memory + getGradQShift();
{% endif %}

  const tarch::la::Vector<DIMENSIONS, pCompType> invDx = convertScalar<pCompType>(invertEntries(cellSize));

  pCompType tmpArray[{{TMP_ARRAY_SIZE}}] __attribute__((aligned({{ALIGNMENT}})));

  //***************************
  //**** Cauchy-Kovalewski ****
  //***************************

  // 1: Initialise time derivative
  // The zeroth time derivative (time dof number 1) is the initial condition
  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    std::copy_n(luh + xyz * {{NUMBER_OF_DATA}}, {{NUMBER_OF_DATA}}, lQi + xyz * {{NUMBER_OF_DATA_PADDED}});
  }

  // Zero out the arrays which will contain the time-averaged space-time polynomials
  std::memset(lFhi, 0.0, {{(DIMENSIONS*NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));

{% if USE_SOURCE %}
  std::memset(lShi, 0.0, {{(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));
{% endif %}

  // Will be used to store the operator for the contribution to the average sp-t polynomials
  pCompType dtavFac = 1.0;

{% if USE_POINT_SOURCE %}
  pCompType PSi[getPSiSize()] __attribute__((aligned({{ALIGNMENT}})));

  { // 1.5: Compute point source contribution
    std::memset(PSi, 0, {{(NUMBER_OF_DOF+1)*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));

    pCompType tn = 0;
    double x1[{{DIMENSIONS}}]; // Position
    const pCompType ivolume = 1.0 / (cellSize[0] * cellSize[1]{{' * cellSize[2]' if DIMENSIONS==3}});

    pCompType forceVectorSource[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned({{ALIGNMENT}})));
    std::memset(forceVectorSource, 0, {{NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));

    // Loop over all quadrature points
    for (int t = 0; t < {{NUMBER_OF_DOF+1}}; t++) { // Time loop
      t == 0 ? tn = timeStamp : tn = timeStepSize * Quadrature<pCompType>::nodes[t - 1] + timeStamp; // First point in time is current time, then use GL nodes in time.
      for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
{% if DIMENSIONS==3 %}
        x1[2] = cellCentre[2] + cellSize[2] * (Quadrature<pCompType>::nodes[z] - 0.5); // Transform onto element
{% endif %}
        for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
          x1[1] = cellCentre[1] + cellSize[1] * (Quadrature<pCompType>::nodes[y] - 0.5);
          for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
            x1[0] = cellCentre[0] + cellSize[0] * (Quadrature<pCompType>::nodes[x] - 0.5);
            // Loop over all point sources
            for (int p : pointSources) {
              // Get the source vector located at quadrature node x at time tn
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

              // Map point source location onto reference element
{% if STATELESS_INIT_POINT_SOURCES %}
              const double* const pointSourceLocation = {{FULL_QUALIFIED_SOLVER_NAME}}::pointSourceLocation[p];
{% else %}
              const double* const pointSourceLocation = solver.pointSourceLocation[p];
{% endif %}

              const pCompType update = basisFunction[x]((pointSourceLocation[0] - cellCentre[0]) * invDx[0] + 0.5)
                                     * basisFunction[y]((pointSourceLocation[1] - cellCentre[1]) * invDx[1] + 0.5)
{% if DIMENSIONS==3 %}
                                     * basisFunction[z]((pointSourceLocation[2] - cellCentre[2]) * invDx[2] + 0.5)
{% endif %}
                                     * Quadrature<pCompType>::iweights3[(z * {{NUMBER_OF_DOF_3D}} + y) * {{NUMBER_OF_DOF}} + x] * ivolume; // Division by weights product and volume
              #ifdef WITH_OPENMP
              #pragma omp simd aligned(PSi,forceVectorSource:{{ALIGNMENT}})
              #endif
              for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) { // Evaluate
                PSi[(((t * {{NUMBER_OF_DOF_3D}} + z) * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += update * forceVectorSource[n];
              }
            }
          }
        }
      }
    }
  }

  // Add point source to the zeroth time derivative
  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    for (int t = 0; t < {{NUMBER_OF_DOF}}; t++) {
      #ifdef WITH_OPENMP
      #pragma omp simd aligned(lQi,PSi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        lQi[xyz * {{NUMBER_OF_DATA_PADDED}} + n]
          += timeStepSize * Quadrature<pCompType>::weights1[t] * PSi[((t + 1) * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
      }
    }
  }

  // Initialise point source (PS) derivation operator
  pCompType derPS[{{NUMBER_OF_DOF_PADDED}}] __attribute__((aligned({{ALIGNMENT}})));
  std::copy_n(DGMatrices<pCompType>::FLCoeff, {{NUMBER_OF_DOF}}, derPS);
{% endif %}

  // Add first contribution to lQhi
  std::copy_n(lQi, {{NUMBER_OF_DATA_PADDED*NUMBER_OF_DOF**DIMENSIONS}}, lQhi);

  // 3: Cauchy-Kovalewski procedure
  for (int i = 0; i < {{NUMBER_OF_DOF}}; i++) { // Time DOFs
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
      for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
        for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
          #ifdef WITH_OPENMP
          #pragma omp simd aligned(tmpArray,lFi:{{ALIGNMENT}})
          #endif
          for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
            tmpArray[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3] += lFi[yz * {{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} + it_2 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3]
              * invDx[0] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF}} + it_2];
          }
        }
      }
      std::copy_n(tmpArray, {{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}}, lFi + yz * {{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}});
    }

    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        std::memset(tmpArray, 0, {{TMP_ARRAY_SIZE}} * sizeof(pCompType));
        for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
          for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
            #ifdef WITH_OPENMP
            #pragma omp simd aligned(tmpArray,lFi:{{ALIGNMENT}})
            #endif
            for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
              tmpArray[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3]
                += lFi[{{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} + z * {{NUMBER_OF_DOF*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} + x * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_2 * {{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} + it_3]
                * invDx[1] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF}} + it_2];
            }
          }
        }
        for (int l = 0; l < {{NUMBER_OF_DOF}}; l++) {
          #ifdef WITH_OPENMP
          #pragma omp simd aligned(lFi,tmpArray:{{ALIGNMENT}})
          #endif
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
      for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
        for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
          #ifdef WITH_OPENMP
          #pragma omp simd aligned(tmpArray,lFi:{{ALIGNMENT}})
          #endif
          for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
            tmpArray[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3]
              += lFi[{{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} + xy * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_2 * {{NUMBER_OF_DOF*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} + it_3]
              * invDx[2] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF}} + it_2];
          }
        }
      }
      for (int l = 0; l < {{NUMBER_OF_DOF}}; l++) {
        #ifdef WITH_OPENMP
        #pragma omp simd aligned(lFi,tmpArray:{{ALIGNMENT}})
        #endif
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
          lFi[{{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} + (l * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n]
            = tmpArray[l * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
        }
      }
    }
{% endif %}
{% endif %}{# USE_FLUX #}

{% if USE_NCP %}
    // x-loop
    for (int yz = 0; yz < {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF}}; yz++) {
      std::memset(gradQ, 0, {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));
      for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
        for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
          #ifdef WITH_OPENMP
          #pragma omp simd aligned(lQi,gradQ:{{ALIGNMENT}})
          #endif
          for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
            gradQ[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3] += lQi[yz*{{NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}} + it_2 * {{NUMBER_OF_DATA_PADDED}} + it_3]
              * invDx[0] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF}} + it_2];
          }
        }
      } // it_2

      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        pCompType tmpNcp[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned({{ALIGNMENT}}))) = {0.0};
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
        #ifdef WITH_OPENMP
        #pragma omp simd aligned(lFi,tmpNcp:{{ALIGNMENT}})
        #endif
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
          lFi[yz*{{NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} + x * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += tmpNcp[n];
        }
      } // x
    } // yz

    // y-loop
    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        std::memset(gradQ, 0, {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));

        for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
          for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
            #ifdef WITH_OPENMP
            #pragma omp simd aligned(lQi,gradQ:{{ALIGNMENT}})
            #endif
            for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
              gradQ[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3] += lQi[z * {{NUMBER_OF_DOF*NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}} + x * {{NUMBER_OF_DATA_PADDED}} + it_2 * {{NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}} + it_3]
                * invDx[1] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF}} + it_2];
            }
          }
        } // it_2

        for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {

          pCompType tmpNcp[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned({{ALIGNMENT}}))) = {0.0};
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

          for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
            lFi[((z * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}]
              += tmpNcp[n];
          }
        }
      }
    } // z

{% if DIMENSIONS==3 %}
    // z-loop
    for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF}}; xy++) {
      std::memset(gradQ, 0, {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));
      for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
        for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
          #ifdef WITH_OPENMP
          #pragma omp simd aligned(lQi,gradQ:{{ALIGNMENT}})
          #endif
          for (int it_3 = 0; it_3 < {{NUMBER_OF_UNKNOWNS_PADDED}}; it_3++) {
            gradQ[it_1 * {{NUMBER_OF_UNKNOWNS_PADDED}} + it_3] += lQi[xy * {{NUMBER_OF_DATA_PADDED}} + it_2 * {{NUMBER_OF_DOF*NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED}} + it_3]
              * invDx[2] * DGMatrices<pCompType>::dudx_T[it_1 * {{NUMBER_OF_DOF}} + it_2];
          }
        }
      } // it_2

      for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
        pCompType tmpNcp[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned({{ALIGNMENT}}))) = {0.0};
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

        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
          lFi[{{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}} + (z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n]
            += tmpNcp[n];
        }
      }
    } // xy
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
      {{FULL_QUALIFIED_SOLVER_NAME}}::algebraicSource(
{% else %}
      solver.algebraicSource(
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

    // Averaging lFhi (and if necessary lShi over time directly in the CK-loop)
    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
      #ifdef WITH_OPENMP
      #pragma omp simd aligned(lFhi,lFi{{',lShi,lSi' if USE_SOURCE else ''}}:{{ALIGNMENT}})
      #endif
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

    // Break for loop to avoid having an extra contribution to lQhi
    // and some unnecessary computation for lQi.
    if (i == {{NUMBER_OF_DOF-1}}) {
      break;
    }

    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
      #ifdef WITH_OPENMP
      #pragma omp simd aligned(lQi,lFi{{',lSi' if USE_SOURCE else ''}}:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
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
    // Add point source contribution to the time derivatives
    for (int t = 0; t < {{NUMBER_OF_DOF}}; t++) {
      for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
        #ifdef WITH_OPENMP
        #pragma omp simd aligned(lQi,PSi,derPS:{{ALIGNMENT}})
        #endif
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
          lQi[xyz * {{NUMBER_OF_DATA_PADDED}} + n] += derPS[t] * PSi[(t * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
        }
      }
    }

    pCompType derPS_buffer[{{NUMBER_OF_DOF_PADDED}}] __attribute__((aligned({{ALIGNMENT}})));
    std::memset(derPS_buffer, 0.0, {{NUMBER_OF_DOF_PADDED}} * sizeof(pCompType));
    for (int it_1 = 0; it_1 < {{NUMBER_OF_DOF}}; it_1++) {
      #ifdef WITH_OPENMP
      #pragma omp simd aligned(derPS_buffer,derPS:{{ALIGNMENT}})
      #endif
      for (int it_2 = 0; it_2 < {{NUMBER_OF_DOF}}; it_2++) {
        derPS_buffer[it_2] += derPS[it_1] * DGMatrices<pCompType>::dudx[it_1 * {{NUMBER_OF_DOF}} + it_2];
      }
    }
{% endif %}

    dtavFac *= timeStepSize / (i + 2);

    for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
      #ifdef WITH_OPENMP
      #pragma omp simd aligned(lQhi,lQi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        lQhi[xyz * {{NUMBER_OF_DATA_PADDED}} + n] += dtavFac * lQi[xyz * {{NUMBER_OF_DATA_PADDED}} + n];
      }
    }
  } // i

{% if USE_POINT_SOURCE %}
  // Add point source to the right hand side
  pCompType sumArray[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned({{ALIGNMENT}})));
  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    std::memset(sumArray, 0, {{NUMBER_OF_UNKNOWNS_PADDED}} * sizeof(pCompType));
    for (int n0 = 1; n0 < {{NUMBER_OF_DOF+1}}; n0++) {
      const pCompType coeff = Quadrature<pCompType>::weights1[n0 - 1] / {{DIMENSIONS}};
      #ifdef WITH_OPENMP
      #pragma omp simd aligned(sumArray,PSi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
        sumArray[n] += coeff * PSi[(n0 * {{NUMBER_OF_DOF**DIMENSIONS}} + xyz) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
      }
    }
    #ifdef WITH_OPENMP
    #pragma omp simd aligned(lFhi,sumArray:{{ALIGNMENT}})
    #endif
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
      lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += sumArray[n];
      lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS) * NUMBER_OF_UNKNOWNS_PADDED}}] += sumArray[n];
{% if DIMENSIONS==3 %}
      lFhi[xyz * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*(NUMBER_OF_DOF**DIMENSIONS) * NUMBER_OF_UNKNOWNS_PADDED}}] += sumArray[n];
{% endif %}
    }
  }
{% endif %}

  //***********************
  //**** Extrapolation ****
  //***********************
  for(int i = 0; i < 2 * DIMENSIONS; i++) {
    std::memset(lQhbnd[i], 0, {{NUMBER_OF_DATA*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}} * sizeof(pStoreType));
    std::memset(lFhbnd[i], 0, {{NUMBER_OF_UNKNOWNS*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}} * sizeof(pStoreType));
  }

  // x-direction: face 0 (left) and face 2(2D) / 3(3D) (right)
  for (int yz = 0; yz < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; yz++) {
    for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
      #ifdef WITH_OPENMP
      #pragma omp simd aligned(lQhbnd,lQhi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
        // Left
        lQhbnd[0][yz * {{NUMBER_OF_DATA}} + n] += lQhi[(yz * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FLCoeff[x];
        // Right
        lQhbnd[DIMENSIONS][yz * {{NUMBER_OF_DATA}} + n] += lQhi[(yz * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FRCoeff[x];
      }
      #ifdef WITH_OPENMP
      #pragma omp simd aligned(lFhbnd,lFhi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        // Left
        lFhbnd[0][yz * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[(yz * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] * DGMatrices<pCompType>::FLCoeff[x];
        // Right
        lFhbnd[DIMENSIONS][yz * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[(yz * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] * DGMatrices<pCompType>::FRCoeff[x];
      }
    }
  }

  // y-direction: face 1 (left) and face 3(2D) / 4(3D) (right)
  for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
    for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
      for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
        #ifdef WITH_OPENMP
        #pragma omp simd aligned(lQhbnd,lQhi:{{ALIGNMENT}})
        #endif
        for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
          // Left
          lQhbnd[1][(z * {{NUMBER_OF_DOF_3D}} + x) * {{NUMBER_OF_DATA}} + n] += lQhi[((z * {{NUMBER_OF_DOF_3D}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FLCoeff[y];
          // Right
          lQhbnd[1+DIMENSIONS][(z * {{NUMBER_OF_DOF_3D}} + x) * {{NUMBER_OF_DATA}} + n] += lQhi[((z * {{NUMBER_OF_DOF_3D}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FRCoeff[y];
        }
        #ifdef WITH_OPENMP
        #pragma omp simd aligned(lFhbnd,lFhi:{{ALIGNMENT}})
        #endif
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
          // Left
          lFhbnd[1][(z * {{NUMBER_OF_DOF_3D}} + x) * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[((z * {{NUMBER_OF_DOF_3D}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}] * DGMatrices<pCompType>::FLCoeff[y];
          // Right
          lFhbnd[1+DIMENSIONS][(z * {{NUMBER_OF_DOF_3D}} + x) * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[((z * {{NUMBER_OF_DOF_3D}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{1*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}] * DGMatrices<pCompType>::FRCoeff[y];
        }
      }
    }
  }

{% if DIMENSIONS==3 %}
  // z-direction: face 2 (left) and face 5 (right)
  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
      #ifdef WITH_OPENMP
      #pragma omp simd aligned(lQhbnd,lQhi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
        // Left
        lQhbnd[2][xy * {{NUMBER_OF_DATA}} + n] += lQhi[(z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FLCoeff[z];
        // Right
        lQhbnd[2+DIMENSIONS][xy * {{NUMBER_OF_DATA}} + n] += lQhi[(z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_DATA_PADDED}} + n] * DGMatrices<pCompType>::FRCoeff[z];
      }
      #ifdef WITH_OPENMP
      #pragma omp simd aligned(lFhbnd,lFhi:{{ALIGNMENT}})
      #endif
      for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
        // Left
        lFhbnd[2][xy * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[(z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}] * DGMatrices<pCompType>::FLCoeff[z];
        // Right
        lFhbnd[2+DIMENSIONS][xy * {{NUMBER_OF_UNKNOWNS}} + n] += lFhi[(z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + xy) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n + {{2*(NUMBER_OF_DOF**DIMENSIONS)*NUMBER_OF_UNKNOWNS_PADDED}}] * DGMatrices<pCompType>::FRCoeff[z];
      }
    }
  }
{% endif %}

  //*****************************
  //****** Volume Integral ******
  //*****************************

  // For linear non-conservative PDE, the volume integral is trivial, since it
  // only involves the element mass matrix, which cancels out.

  for (int xyz = 0; xyz < {{NUMBER_OF_DOF**DIMENSIONS}}; xyz++) {
    #ifdef WITH_OPENMP
    #pragma omp simd aligned(luh,lFhi{{',lShi' if USE_SOURCE else ''}}:{{ALIGNMENT}})
    #endif
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
        luh[xyz*{{NUMBER_OF_DATA}} + n] -= timeStepSize * //Quadrature<pCompType>::weights3[xyz] *
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
            );
    }
  }

  //tarch::accelerator::MemoryManager::getInstance().free(memory, tarch::accelerator::Device::DefaultDevice);

  return -1; // No picard iteration in linear case
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
