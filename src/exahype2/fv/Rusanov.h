// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_H_
#define _EXAHYPE2_FV_RUSANOV_H_



#include "../PatchUtils.h"
#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/multicore.h"


#include <functional>


namespace exahype2 {
  namespace fv {


    //*
     //The routine has exactly the same semantics as applySplit1DRiemannToPatch_Overlap1AoS2d()
     //but runs through the faces slightly differently.

     //@image html applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop.png

     //As in applySplit1DRiemannToPatch_Overlap1AoS2d(), we have two big
     //blocks. The first one runs through all vertical faces, the second one
     //through all horizontal ones. Each block now runs through the whole data
     //twice though with a spacing of two and an offset of 0 or 1. That is,
     //we run through every second face (faces 0, 2, 4 along the x-axis) and
     //then through the other ones (1,3,5,...).

     //Each face writes to its left and right adjacent volume in Qout. As we
     //skip every second face, we know that all writes are parallel. No race
     //conditions can arise. That is: Though we now run through the data structure
     //twice, we can process the for loops embarassingly parallel.
     //
     //
     //
     // NOTE: we decided to have a dedicated split loop implementation for the
     // Rusanov solver as all attempts to offload the original bit using functors failed.
     // One benefit here is that the helper variables are on the stack.
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    template<
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER>
    void applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop_Rusanov(
      const tarch::la::Vector<Dimensions,double>&  thepatchCentre,
      const tarch::la::Vector<Dimensions,double>&  thepatchSize,
      double                                       t,
      double                                       dt,
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    )
    {


      // Sadly we cannot directly give the target region a vector due to 'non-trivially copyable'
      double x0 = thepatchCentre[0];
      double x1 = thepatchCentre[1];
      double h0 = thepatchSize[0];
      double h1 = thepatchSize[1];
#if Dimensions==3
      double x2 = thepatchCentre[2];
      double h2 = thepatchSize[2];
#endif


      for (int shift = 0; shift < 2; shift++)
      {
        #ifdef SharedOMP
           #if defined(OpenMPGPUOffloading)
           #pragma omp target
           #endif
        #pragma omp parallel for collapse(2)
        #endif

        for (int x = shift; x <= numberOfVolumesPerAxisInPatch; x += 2)
        {
          for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
          {
        #if Dimensions==2
        tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1};
        tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1};
        #elif Dimensions==3
        tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1,x2};
        tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1,h2};
        #endif
            const int leftVoxelInPreimage  = x +      (y + 1) * (2 + numberOfVolumesPerAxisInPatch);
            const int rightVoxelInPreimage = x + 1  + (y + 1) * (2 + numberOfVolumesPerAxisInPatch);

            const int leftVoxelInImage     = x - 1 + y * numberOfVolumesPerAxisInPatch;
            const int rightVoxelInImage    = x     + y * numberOfVolumesPerAxisInPatch;

     

            // getVolumeSize
            tarch::la::Vector<Dimensions,double> volumeH;
            for (int d=0; d<Dimensions; d++) {
              volumeH(d) = patchSize(d)/numberOfVolumesPerAxisInPatch;
            }

            // Assignment vectorA = vectorB - 0.5*vectorC
            tarch::la::Vector<Dimensions, double> volumeX;
            for (int d=0; d<Dimensions; d++) {
             volumeX(d) = patchCentre(d) - 0.5 * patchSize(d);
            }

            volumeX (0) += x * volumeH (0);
            volumeX (1) += (y + 0.5) * volumeH (1);

            auto QL = Qin + leftVoxelInPreimage  * (unknowns + auxiliaryVariables);
            auto QR = Qin + rightVoxelInPreimage * (unknowns + auxiliaryVariables);
            
            tarch::la::Vector<Dimensions, double> xx = {volumeX(0), volumeX(1)};
            
            auto dx = volumeH(0);
            int normal = 0;

            double fluxFL[unknowns];
            double fluxFR[unknowns];

            SOLVER::flux( QL, x, dx, t, normal, fluxFL );
            SOLVER::flux( QR, x, dx, t, normal, fluxFR );

            double lambdaMaxL = SOLVER::maxEigenvalue(QL,x,dx,t,normal);
            double lambdaMaxR = SOLVER::maxEigenvalue(QR,x,dx,t,normal);

            double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );


            for (int unknown = 0; unknown < unknowns; unknown++)
            {
              if (x > 0)
              {
                double fl = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                Qout[leftVoxelInImage * (unknowns + auxiliaryVariables) + unknown]  -= dt / volumeH (0) * fl;
              }
              if (x < numberOfVolumesPerAxisInPatch)
              {
                double fr = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                Qout[rightVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
              }
            }
          }
        }
      }

      // Iterate over other normal
      for (int shift = 0; shift < 2; shift++)
      {
        #ifdef SharedOMP
           #if defined(OpenMPGPUOffloading)
           #pragma omp target
           #endif
        #pragma omp parallel for collapse(2)
        #endif
        for (int y = shift; y <= numberOfVolumesPerAxisInPatch; y += 2)
        {
          for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
          {
        #if Dimensions==2
        tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1};
        tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1};
        #elif Dimensions==3
        tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1,x2};
        tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1,h2};
        #endif
            const int lowerVoxelInPreimage = x + 1  +       y * (2 + numberOfVolumesPerAxisInPatch);
            const int upperVoxelInPreimage = x + 1  + (y + 1) * (2 + numberOfVolumesPerAxisInPatch);
            const int lowerVoxelInImage    = x      + (y - 1) *      numberOfVolumesPerAxisInPatch ;
            const int upperVoxelInImage    = x      +       y *      numberOfVolumesPerAxisInPatch ;


            tarch::la::Vector<Dimensions,double> volumeH;
            for (int d=0; d<Dimensions; d++) {
              volumeH(d) = patchSize(d)/numberOfVolumesPerAxisInPatch;
            }

            tarch::la::Vector<Dimensions, double> volumeX;
            for (int d=0; d<Dimensions; d++) {
             volumeX(d) = patchCentre(d) - 0.5 * patchSize(d);
            }

            volumeX (0) += (x + 0.5) * volumeH (0);
            volumeX (1) +=         y * volumeH (1);

            auto QL = Qin + lowerVoxelInPreimage  * (unknowns + auxiliaryVariables);
            auto QR = Qin + upperVoxelInPreimage * (unknowns + auxiliaryVariables);
            
            tarch::la::Vector<Dimensions, double> xx = {volumeX(0), volumeX(1)};
            auto dx = volumeH(0);
            int normal = 1;

            double fluxFL[unknowns];
            double fluxFR[unknowns];

            SOLVER::flux( QL, x, dx, t, normal, fluxFL );
            SOLVER::flux( QR, x, dx, t, normal, fluxFR );

            double lambdaMaxL = SOLVER::maxEigenvalue(QL,x,dx,t,normal);
            double lambdaMaxR = SOLVER::maxEigenvalue(QR,x,dx,t,normal);
            double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

            for (int unknown = 0; unknown < unknowns; unknown++)
            {
              if (y > 0)
              {
                double fl = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                Qout[lowerVoxelInImage * (unknowns + auxiliaryVariables) + unknown] -= dt / volumeH (0) * fl;
              }
              if (y < numberOfVolumesPerAxisInPatch)
              {
                double fr = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                Qout[upperVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
              }
            }
          }
        }
      }
    };
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif

    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    template<
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename T>
    void applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop_Rusanov(
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif





    /**
     * 1d Riemann accepting a flux and eigenvalue function.
     */
#if defined(GPUOffloading)
#pragma omp declare target
#endif
    void splitRusanov1d(
      std::function< void(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ F
      ) >   flux,
      std::function< double(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   eigenvalues,
      const double * __restrict__ QL,
      const double * __restrict__ QR,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       dx,
      double                                       t,
      double                                       dt,
      int                                          normal,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ FL,
      double * __restrict__ FR
    );
#if defined(GPUOffloading)
#pragma omp end declare target
#endif


    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    /**
     * Extension of standard Rusanov1d. This one also supports non-conservative fluxes.
     */
    void splitRusanov1d(
      std::function< void(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ F
      ) >   flux,
      std::function< void(
        const double * __restrict__                  Q,
        const double * __restrict__     dQdn,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ BgradQ
      ) >   nonconservativeProduct,
      std::function< double(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   eigenvalues,
      const double * __restrict__ QL,
      const double * __restrict__ QR,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       dx,
      double                                       t,
      double                                       dt,
      int                                          normal,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ FL,
      double * __restrict__ FR
    );
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
  }
}


#endif
