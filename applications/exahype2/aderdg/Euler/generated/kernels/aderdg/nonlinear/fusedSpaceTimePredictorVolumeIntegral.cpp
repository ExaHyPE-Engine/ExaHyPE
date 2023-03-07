
#include <cstring>
#include <algorithm>

#include "./Kernels.h"
#include "./DGMatrices.h"
#include "./Quadrature.h"
#include "./gemmsCPP.h"

#include "../../../../euler2D.h"

template <typename cStoreType, typename pCompType, typename pStoreType>
int generated::kernels::AderDG::nonlinear::fusedSpaceTimePredictorVolumeIntegral(
  examples::exahype2::euler2D::euler2D& solver, 
  pCompType* __restrict__ lduh,
  pStoreType* __restrict__ lQhbnd, 
  pStoreType* __restrict__ lGradQhbnd,
  pStoreType* __restrict__ lFhbnd,
  pCompType* __restrict__ lQi,
  pCompType* __restrict__ rhs,
  pCompType* __restrict__ lFi,
  pCompType* __restrict__ lSi,   // for NCP or Source
  pCompType* __restrict__ lQhi,
  pCompType* __restrict__ lFhi,
  pCompType* __restrict__ lShi,  // for NCP or Source
  pCompType* __restrict__ gradQ, // for NCP or Source or viscousFlux
  pCompType* __restrict__ gradQAvg, // for viscousFlux
  const cStoreType* const __restrict__ luh,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  dx,
  const double timeStamp,
  const double dt
) {

  const double inverseDx = 1/dx[0]; //assume dx[0] == dx[1] == dx[2]

  //********************
  //****** Picard ******
  //********************

#ifdef __INTEL_COMPILER
  __assume_aligned(lQi, 16);
  __assume_aligned(rhs, 16);
  __assume_aligned(lFi, 16);
  __assume_aligned(FLCoeff, 16); // == F0
  __assume_aligned(Kxi, 16);
  __assume_aligned(iK1_T, 16);
  __assume_aligned(nodes, 16);
  __assume_aligned(weights1, 16);
  __assume_aligned(weights3, 16);
  __assume_aligned(iweights3, 16);
  __assume_aligned(luh, 16); //luh should be aligned, see Solver.h

#endif

  // 0. Allocate local variable

  pCompType new_lQi_slice[16] __attribute__((aligned(16))); //for step 4 (computing new lQi value), doesn't update parameters
  const pCompType dtBydx = inverseDx * dt; //Assume dx[0] == dx[1] == dx[2]

  // 1. Trivial initial guess
  for (int t = 0; t < 4; t++) {
    for (int xyz = 0; xyz < 16; xyz++) {
      std::copy_n(&luh[4*xyz], 4, &lQi[4*(xyz+16*t)]);
    }
  }
  
  // 2. Discrete Picard iterations
  constexpr int MaxIterations = 9;
  int iter = 0;
  for (; iter < MaxIterations; iter++) {
    for (int t = 0; t < 4; t++) {  // time DOF


      
      // Compute the "derivatives" (contributions of the stiffness matrix)      
      // x direction (independent from the y and z derivatives)
       



      { // Compute the fluxes
        pCompType* F[2];
        for (int xyz = 0; xyz < 16; xyz++) {
          // Call PDE fluxes
          F[0] = &lFi[(t*16+xyz)*4];
          F[1] = &lFi[(t*16+xyz)*4+256];

          #ifdef USE_IPO
              #pragma forceinline recursive
          #endif
          for(int d=0; d<Dimensions; d++){
            solver.examples::exahype2::euler2D::euler2D::flux(
              lQi+(t*16+xyz)*4,
              volumeCentre, //TODO, fix to node position
              dx,
              timeStamp, //+nodes[t]*dt exact timestamp for node, 
              dt,
              d,
              F[d]
            );
          }
        }
      }

      // Compute the contribution of the initial condition uh to the right-hand side (rhs)
      for (int xyz = 0; xyz < 16; xyz++) {
        const pCompType weight = weights3[xyz] * FLCoeff[t];
        #pragma omp simd aligned(rhs,luh:16)
        for (int n = 0; n < 4; n++) {
          rhs[n+4*(xyz+16*t)] = weight * luh[n+4*xyz];
        }
      }
      
      // Compute the "derivatives" (contributions of the stiffness matrix)      
      // x direction (independent from the y and z derivatives)
      for (int z = 0; z < 1; z++) {
        for (int y = 0; y < 4; y++) {
          pCompType coeffRhsX[16] __attribute__((aligned(16)));
          #pragma omp simd aligned(coeffRhsX,Kxi:16)
          for (int it = 0; it < 16; it++) {
            coeffRhsX[it] = - weights3[t*4+z*4+y] * dtBydx * Kxi[it];
          }
          #if defined(USE_IPO) && !defined(UNSAFE_IPO)
          volatile double doNotOptimizeAway_coeffRhsX = coeffRhsX[0]; //used to prevent the compiler from optimizing temp array away. Needs to be volatile
          #endif
          #ifdef USE_IPO
          #pragma forceinline
          #endif
          gemm_4_4_4_rhs_x(lFi+((t*1+z)*4+y)*16, coeffRhsX, rhs+((t*1+z)*4+y)*16);
        }
      }
      
      // y direction (independent from the x and z derivatives)
      for (int z = 0; z < 1; z++) {
        for (int x = 0; x < 4; x++) {
          pCompType coeffRhsY[16] __attribute__((aligned(16)));
          #pragma omp simd aligned(coeffRhsY,Kxi:16)
          for (int it = 0; it < 16; it++) {
            coeffRhsY[it] = - weights3[t*4+z*4+x] * dtBydx * Kxi[it];
          }
          #if defined(USE_IPO) && !defined(UNSAFE_IPO)
          volatile double doNotOptimizeAway_coeffRhsY = coeffRhsY[0]; //used to prevent the compiler from optimizing temp array away. Needs to be volatile
          #endif
          #ifdef USE_IPO
          #pragma forceinline
          #endif
          gemm_4_4_4_rhs_y(lFi+((t*1+z)*16+x)*4+256, coeffRhsY, rhs+((t*1+z)*16+x)*4);
        }
      }
       


    }  // end time dof

    // 3. Multiply with (K1)^(-1) to get the discrete time integral of the
    // discrete Picard iteration
    pCompType sq_res = 0.0;
    for (int xyz = 0; xyz < 16; xyz++) {
      pCompType s_m_QSlice[16] __attribute__((aligned(16)));
      #pragma omp simd aligned(s_m_QSlice,iK1_T:16)
      for (int it = 0; it < 16; it++) {
        s_m_QSlice[it] = iweights3[xyz] * iK1_T[it];
      }
      #if defined(USE_IPO) && !defined(UNSAFE_IPO)
      volatile double doNotOptimizeAway_s_m_QSlice = s_m_QSlice[0]; //used to prevent the compiler from optimizing temp array away. Needs to be volatile
      #endif
      #ifdef USE_IPO
      #pragma forceinline
      #endif
      gemm_4_4_4_lqi(rhs+4*xyz, s_m_QSlice, new_lQi_slice);
      for(int t = 0; t < 4; t++) {
        for(int n=0; n<4; n++) { //only copy and change the variables, skip parameters
          sq_res += (new_lQi_slice[n+4*t] - lQi[n+4*(xyz+16*t)]) * (new_lQi_slice[n+4*t] - lQi[n+4*(xyz+16*t)]);
          lQi[n+4*(xyz+16*t)] = new_lQi_slice[n+4*t];
        }
      }
    }

    // 4. Exit condition
    constexpr pCompType tol2 = 1e-7 * 1e-7;
    if (sq_res < tol2) {
      break;
    }
  }  // end iter

  //***********************
  //****** Predictor ******
  //***********************
  
#ifdef __INTEL_COMPILER
  __assume_aligned(lQi, 16);
  __assume_aligned(lQhi, 16);
  __assume_aligned(lFi, 16);
  __assume_aligned(lFhi, 16);
  __assume_aligned(weights1, 16);
#endif  

  std::memset(lQhi, 0, 64 * sizeof(pCompType));
  std::memset(lFhi, 0, 128 * sizeof(pCompType));

  for (int z=0; z<1; z++) {
    for (int y=0; y<4; y++) {
      for (int x=0; x<4; x++) {
        
        // Matrix-Vector Products
        for (int t=0; t<4; t++) {
          #pragma omp simd aligned(lQhi,lQi:16)
          for (int n=0; n<4; n++) {
            // Fortran: lQhi(:,x,y,z) = lQi(:,:,x,y,z) * wGPN(:)
            lQhi[((z*4+y)*4+x)*4+n] += weights1[t] *
                lQi[(((t*1+z)*4+y)*4+x)*4+n];
          }
          #pragma omp simd aligned(lFhi,lFi:16)
          for (int n=0; n<4; n++) {
            // Fortran: lFhi_x(:,x,y,z) = lFh(:,1,x,y,z,:) * wGPN(:)
            lFhi[((z*4+y)*4+x)*4+n+0] += weights1[t] *
                lFi[(((t*1+z)*4+y)*4+x)*4+n+0];
          }  
          #pragma omp simd aligned(lFhi,lFi:16)
          for (int n=0; n<4; n++) {
            // Fortran: lFhi_y(:,y,x,z) = lFh(:,2,:x,y,z,:) * wGPN(:)
            lFhi[((z*4+x)*4+y)*4+n+64] += weights1[t] *
                lFi[(((t*1+z)*4+y)*4+x)*4+n+256];
          }  
            
        }
      
      }
    }
  }
  
  //**************************
  //****** Extrapolator ******
  //**************************
  
#ifdef __INTEL_COMPILER
  __assume_aligned(lQhi, 16);
  __assume_aligned(lFhi, 16);
  __assume_aligned(lFhbnd, 16);
  __assume_aligned(FRCoeff, 16);
  __assume_aligned(FLCoeff, 16);
#endif
  
  std::memset(lQhbnd, 0, 64 * sizeof(pStoreType));
  std::memset(lFhbnd, 0, 64 * sizeof(pStoreType));


  // x-direction: face 1 (left) and face 2 (right)
  for (int yz = 0; yz < 4; yz++) {
    // Matrix-Vector Products
    for (int x = 0; x < 4; x++) {
      #pragma omp simd aligned(lQhbnd,lQhi:16)
      for (int n = 0; n < 4; n++) {    
        // Fortran: lQhbnd(:,j,i,1) = lQhi(:,:,j,i) * FLCoeff(:)
        lQhbnd[n+4*yz+0] +=
            lQhi[n+4*(x+4*yz)] * FLCoeff[x];

        // Fortran: lQhbnd(:,j,i,2) = lQhi(:,:,j,i) * FRCoeff(:)
        lQhbnd[n+4*yz+16] +=
            lQhi[n+4*(x+4*yz)] * FRCoeff[x];
        // Fortran: lFhbnd(:,j,i,1) = lFhi_x(:,:,j,i) * FLCoeff(:)
        lFhbnd[n+4*yz+0] +=
            lFhi[n+4*(x+4*yz)] * FLCoeff[x];

        // Fortran: lFhbnd(:,j,i,2) = lFhi_x(:,:,j,i) * FRCoeff(:)
        lFhbnd[n+4*yz+16] +=
            lFhi[n+4*(x+4*yz)] * FRCoeff[x];

      }
    }
  }

  // y-direction: face 3 (left) and face 4 (right)
  for (int xz = 0; xz < 4; xz++) {  
    // Matrix-Vector Products
    for (int y = 0; y < 4; y++) {
      const int z = 0;
      const int x = xz;
      #pragma omp simd aligned(lQhbnd,lQhi:16)
      for (int n = 0; n < 4; n++) {
        // Fortran: lQhbnd(:,j,i,3) = lQhi(:,j,:,i) * FLCoeff(:)
        lQhbnd[n+4*xz+32] +=
            lQhi[n+4*(x+4*(y+1*z))] * FLCoeff[y];

        // Fortran: lQhbnd(:,j,i,4) = lQhi(:,j,:,i) * FRCoeff(:)
        lQhbnd[n+4*xz+48] +=
            lQhi[n+4*(x+4*(y+1*z))] * FRCoeff[y];
        // Fortran: lFhbnd(:,j,i,3) = lFhi_y(:,:,j,i) * FLCoeff(:)
        lFhbnd[n+4*xz+32] +=
            lFhi[n+4*(y+4*xz)+64] * FLCoeff[y];

        // Fortran: lFhbnd(:,j,i,4) = lFhi_y(:,:,j,i) * FRCoeff(:)
        lFhbnd[n+4*xz+48] +=
            lFhi[n+4*(y+4*xz)+64] * FRCoeff[y];
      }
    }
  }

  
  
  //*****************************
  //****** Volume Integral ******
  //*****************************
  

  memset(lduh, 0, 64*sizeof(pCompType));

#ifdef __INTEL_COMPILER
  __assume_aligned(lFhi,     16);
  __assume_aligned(Kxi_T,    16);
  __assume_aligned(weights2, 16);
  __assume_aligned(lduh,     16); //lduh should be aligned, see Solver.h
#endif
  
  // Assume equispaced mesh, dx[0] == dx[1] == dx[2]
  for (int j=0; j<1; j++) {
    for (int i=0; i<4; i++) {
      
      //x, also define coefficient matrix coeffVolume
      pCompType coeffVolume[16] __attribute__((aligned(16)));
      #pragma omp simd aligned(coeffVolume,Kxi_T:16)
      for (int it = 0; it < 16; it++) {
        coeffVolume[it] = weights2[i+j*4] * inverseDx * Kxi_T[it];
      }
      #if defined(USE_IPO) && !defined(UNSAFE_IPO)
      volatile double doNotOptimizeAway_coeffVolume = coeffVolume[0]; //used to prevent the compiler from optimizing temp array away. Needs to be volatile
      #endif
      #ifdef USE_IPO
      #pragma forceinline
      #endif
      gemm_4_4_4_lduh_x(lFhi+(j*4+i)*16+0, coeffVolume, lduh+(j*4+i)*16);

      //y, reuse coeffVolume
      #ifdef USE_IPO
      #pragma forceinline
      #endif
      gemm_4_4_4_lduh_y(lFhi+(j*4+i)*16+64, coeffVolume, lduh+(j*16+i)*4);

    }
  }

  return std::min(iter+1, MaxIterations); //return number of Picard iterations, min to avoid doing a +1 if the loop wasn't exited early
}

template int generated::kernels::AderDG::nonlinear::fusedSpaceTimePredictorVolumeIntegral(
  examples::exahype2::euler2D::euler2D& solver, 
  double* __restrict__ lduh,
  double* __restrict__ lQhbnd, 
  double* __restrict__ lGradQhbnd,
  double* __restrict__ lFhbnd,
  double* __restrict__ lQi,
  double* __restrict__ rhs,
  double* __restrict__ lFi,
  double* __restrict__ lSi,   // for NCP or Source
  double* __restrict__ lQhi,
  double* __restrict__ lFhi,
  double* __restrict__ lShi,  // for NCP or Source
  double* __restrict__ gradQ, // for NCP or Source or viscousFlux
  double* __restrict__ gradQAvg, // for viscousFlux
  const double* const __restrict__ luh,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  dx,
  const double timeStamp,
  const double dt);