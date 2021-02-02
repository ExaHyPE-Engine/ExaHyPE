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

    template<
      int                                          numVPAIP, // numberofVolumesPerAxisInPatch
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER>
    void Fusanov_2D(
      int                                          haloSize,
      double                                       dt,
      std::vector<std::tuple<double*, const double, int, double, double, double, double> > patchVec,
      double * __restrict__                        Qout,
      int                                          sourcePatchSize,
      int                                          destPatchSize
    )
    {
      const size_t NPT  = patchVec.size();
      const size_t LTOT = NPT*destPatchSize;
      const size_t LR   = NPT*sourcePatchSize;
      double destinationPatch[LTOT];
      //double * reconstructedPatch;//[sourcePatchSize];

      double* RP = ::tarch::allocateMemory(sourcePatchSize*NPT, ::tarch::MemoryLocation::Heap);

      // Move all data to the device, prepare arrays first
      double T[NPT], X0[NPT], X1[NPT], H0[NPT], H1[NPT];
      int TID[NPT];
      for (size_t i=0; i<patchVec.size(); i++)
      {
        auto patch = patchVec[i];
        T[i]    = std::get<1>(patch);
        TID[i]  = std::get<2>(patch);
        X0[i]   = std::get<3>(patch);
        H0[i]   = std::get<4>(patch);
        X1[i]   = std::get<5>(patch);
        H1[i]   = std::get<6>(patch);
        std::copy(std::get<0>(patch), std::get<0>(patch)+sourcePatchSize, RP + i*sourcePatchSize);
      }

 //map(alloc:destinationPatch[0:LTOT]) 
#pragma omp target map(to:T[0:NPT]) map(to:TID[0:NPT]) map(to:X0[0:NPT]) map(to:X1[0:NPT])  map(to:H0[0:NPT]) map(to:H1[0:NPT]) map(to:RP[0:LR]) map(tofrom:destinationPatch[0:LTOT])//map(alloc:reconstructedPatch[0:sourcePatchSize])
#pragma omp teams
#pragma omp distribute 
      for (size_t pidx=0;pidx<NPT;pidx++)
      {
        //auto patch = patchVec[pidx];
        //double *reconstructedPatch = std::get<0>(patch);
        double                    t =   T[pidx];
        int                  taskId = TID[pidx];
        double                   x0 =  X0[pidx];
        double                   h0 =  H0[pidx];
        double                   x1 =  X1[pidx];
        double                   h1 =  H1[pidx];
        double *reconstructedPatch = RP + LR*pidx;
        //printf("x1*: %f\n", x1);
        //printf("there %lu\n",patchVec.size());

        //#pragma omp target update to( reconstructedPatch[0:sourcePatchSize])


      #ifdef SharedOMP
        #if defined(OpenMPGPUOffloading)
        //#pragma omp target map(to:reconstructedPatch[0:sourcePatchSize])
        #endif
      #pragma omp parallel for collapse(2)
      #endif
        for (int x = 0; x < numVPAIP; x++)
        {
          for (int y = 0; y < numVPAIP; y++)
          {

            // This is copyPatch
            for (int i=0; i<unknowns+auxiliaryVariables; i++)
            {
              int sourceIndex      = (y+1)*(numVPAIP+ 3*haloSize) + x - y;
              int destinationIndex = y*numVPAIP + x;
              destinationPatch[pidx*destPatchSize + destinationIndex*(unknowns+auxiliaryVariables)+i] = reconstructedPatch[sourceIndex*(unknowns+auxiliaryVariables)+i];
            }
            
            tarch::la::Vector<2,double> patchCentre = {x0,x1};
            tarch::la::Vector<2,double> patchSize   = {h0,h1};

            tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP};
            tarch::la::Vector<2, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1)};
            volumeX (0) += (x + 0.5) * volumeH (0);
            volumeX (1) += (y + 0.5) * volumeH (1);
            const int voxelInPreImage  = x+1      + (y+1) * (numVPAIP+2);
            const int voxelInImage     = x            + y * numVPAIP;
            double sourceTermContributions[unknowns];
            SOLVER::sourceTerm( reconstructedPatch + voxelInPreImage * (unknowns + auxiliaryVariables),  volumeX, volumeH(0), t, dt, sourceTermContributions);

            for (int unknown = 0; unknown < unknowns; unknown++)
            {
              destinationPatch[pidx*destPatchSize + voxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt * sourceTermContributions[unknown];
            }
          }
        }
        
        
        for (int shift = 0; shift < 2; shift++)
        {
          #ifdef SharedOMP
             //#if defined(OpenMPGPUOffloading)
             //#pragma omp target
             //#endif
          #pragma omp parallel for collapse(2)
          #endif

          for (int x = shift; x <= numVPAIP; x += 2)
          {
            for (int y = 0; y < numVPAIP; y++)
            {
              tarch::la::Vector<2,double> patchCentre = {x0,x1};
              tarch::la::Vector<2,double> patchSize   = {h0,h1};

              const int leftVoxelInPreimage  = x +      (y + 1) * (2 + numVPAIP);
              const int rightVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP);

              const int leftVoxelInImage     = x - 1 + y * numVPAIP;
              const int rightVoxelInImage    = x     + y * numVPAIP;


              // getVolumeSize
              tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP};
              // Assignment vectorA = vectorB - 0.5*vectorC
              tarch::la::Vector<2, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1)};

              volumeX(0) += x * volumeH (0);
              volumeX(1) += (y + 0.5) * volumeH (1);

              auto QL = reconstructedPatch + leftVoxelInPreimage  * (unknowns + auxiliaryVariables);
              auto QR = reconstructedPatch + rightVoxelInPreimage * (unknowns + auxiliaryVariables);
              
              auto dx = volumeH(0);
              int normal = 0;

              double fluxFL[unknowns];
              double fluxFR[unknowns];

              // The arguments don't make any sense
              SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL );
              SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR );

              double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal);
              double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal);

              double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

              for (int unknown = 0; unknown < unknowns; unknown++)
              {
                if (x > 0)
                {
                  double fl = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  destinationPatch[pidx*destPatchSize + leftVoxelInImage * (unknowns + auxiliaryVariables) + unknown]  -= dt / volumeH (0) * fl;
                }
                if (x < numVPAIP)
                {
                  double fr = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  destinationPatch[pidx*destPatchSize + rightVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
                }
              }
            }
          }
        }

         //Iterate over other normal
        for (int shift = 0; shift < 2; shift++)
        {
          #ifdef SharedOMP
             //#if defined(OpenMPGPUOffloading)
             //#pragma omp target
             //#endif
          #pragma omp parallel for collapse(2)
          #endif
          for (int y = shift; y <= numVPAIP; y += 2)
          {
            for (int x = 0; x < numVPAIP; x++)
            {
              tarch::la::Vector<2,double> patchCentre = {x0,x1};
              tarch::la::Vector<2,double> patchSize   = {h0,h1};
              const int lowerVoxelInPreimage = x + 1  +       y * (2 + numVPAIP);
              const int upperVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP);
              const int lowerVoxelInImage    = x      + (y - 1) *      numVPAIP ;
              const int upperVoxelInImage    = x      +       y *      numVPAIP ;

              // getVolumeSize
              tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP};
              // Assignment vectorA = vectorB - 0.5*vectorC
              tarch::la::Vector<2, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1)};

              volumeX (0) += (x + 0.5) * volumeH (0);
              volumeX (1) +=         y * volumeH (1);

              auto QL = reconstructedPatch + lowerVoxelInPreimage  * (unknowns + auxiliaryVariables);
              auto QR = reconstructedPatch + upperVoxelInPreimage * (unknowns + auxiliaryVariables);
              
              auto dx = volumeH(0);
              int normal = 1;

              double fluxFL[unknowns];
              double fluxFR[unknowns];

              SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL );
              SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR );

              double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal);
              double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal);
              double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

              for (int unknown = 0; unknown < unknowns; unknown++)
              {
                if (y > 0)
                {
                  double fl = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  destinationPatch[pidx*destPatchSize + lowerVoxelInImage * (unknowns + auxiliaryVariables) + unknown] -= dt / volumeH (0) * fl;
                }
                if (y < numVPAIP)
                {
                  double fr = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  destinationPatch[pidx*destPatchSize + upperVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
                }
              }
            }
          }
        }
      }

      ::tarch::freeMemory(RP, tarch::MemoryLocation::Heap);
    };








    //#if defined(OpenMPGPUOffloading)
    //#pragma omp declare target
    //#endif
    template<
      int                                          numVPAIP, // numberofVolumesPerAxisInPatch
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER>
    void Fusanov_2D(
      const tarch::la::Vector<2,double>&  thepatchCentre,
      const tarch::la::Vector<2,double>&  thepatchSize,
      int                                          haloSize,
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

      #ifdef SharedOMP
        #if defined(OpenMPGPUOffloading)
        #pragma omp target
        #endif
      #pragma omp parallel for collapse(2)
      #endif
      for (int x = 0; x < numVPAIP; x++)
      {
        for (int y = 0; y < numVPAIP; y++)
        {

          // This is copyPatch
          for (int i=0; i<unknowns+auxiliaryVariables; i++)
          {
            int sourceIndex      = (y+1)*(numVPAIP+ 3*haloSize) + x - y;
            int destinationIndex = y*numVPAIP + x;
            Qout[destinationIndex*(unknowns+auxiliaryVariables)+i] = Qin[sourceIndex*(unknowns+auxiliaryVariables)+i];
          }
          
          tarch::la::Vector<2,double> patchCentre = {x0,x1};
          tarch::la::Vector<2,double> patchSize   = {h0,h1};

          tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP};
          tarch::la::Vector<2, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1)};
          volumeX (0) += (x + 0.5) * volumeH (0);
          volumeX (1) += (y + 0.5) * volumeH (1);
          const int voxelInPreImage  = x+1      + (y+1) * (numVPAIP+2);
          const int voxelInImage     = x            + y * numVPAIP;
          double sourceTermContributions[unknowns];
          SOLVER::sourceTerm( Qin + voxelInPreImage * (unknowns + auxiliaryVariables),  volumeX, volumeH(0), t, dt, sourceTermContributions);

          for (int unknown = 0; unknown < unknowns; unknown++)
          {
            Qout[voxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt * sourceTermContributions[unknown];
          }
        }
      }
      
      
      for (int shift = 0; shift < 2; shift++)
      {
        #ifdef SharedOMP
           #if defined(OpenMPGPUOffloading)
           #pragma omp target
           #endif
        #pragma omp parallel for collapse(2)
        #endif

        for (int x = shift; x <= numVPAIP; x += 2)
        {
          for (int y = 0; y < numVPAIP; y++)
          {
            tarch::la::Vector<2,double> patchCentre = {x0,x1};
            tarch::la::Vector<2,double> patchSize   = {h0,h1};

            const int leftVoxelInPreimage  = x +      (y + 1) * (2 + numVPAIP);
            const int rightVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP);

            const int leftVoxelInImage     = x - 1 + y * numVPAIP;
            const int rightVoxelInImage    = x     + y * numVPAIP;


            // getVolumeSize
            tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP};
            // Assignment vectorA = vectorB - 0.5*vectorC
            tarch::la::Vector<2, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1)};

            volumeX(0) += x * volumeH (0);
            volumeX(1) += (y + 0.5) * volumeH (1);

            auto QL = Qin + leftVoxelInPreimage  * (unknowns + auxiliaryVariables);
            auto QR = Qin + rightVoxelInPreimage * (unknowns + auxiliaryVariables);
            
            auto dx = volumeH(0);
            int normal = 0;

            double fluxFL[unknowns];
            double fluxFR[unknowns];

            // The arguments don't make any sense
            SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL );
            SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR );

            double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal);
            double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal);

            double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

            for (int unknown = 0; unknown < unknowns; unknown++)
            {
              if (x > 0)
              {
                double fl = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                Qout[leftVoxelInImage * (unknowns + auxiliaryVariables) + unknown]  -= dt / volumeH (0) * fl;
              }
              if (x < numVPAIP)
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
        for (int y = shift; y <= numVPAIP; y += 2)
        {
          for (int x = 0; x < numVPAIP; x++)
          {
            tarch::la::Vector<2,double> patchCentre = {x0,x1};
            tarch::la::Vector<2,double> patchSize   = {h0,h1};
            const int lowerVoxelInPreimage = x + 1  +       y * (2 + numVPAIP);
            const int upperVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP);
            const int lowerVoxelInImage    = x      + (y - 1) *      numVPAIP ;
            const int upperVoxelInImage    = x      +       y *      numVPAIP ;

            // getVolumeSize
            tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP};
            // Assignment vectorA = vectorB - 0.5*vectorC
            tarch::la::Vector<2, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1)};

            volumeX (0) += (x + 0.5) * volumeH (0);
            volumeX (1) +=         y * volumeH (1);

            auto QL = Qin + lowerVoxelInPreimage  * (unknowns + auxiliaryVariables);
            auto QR = Qin + upperVoxelInPreimage * (unknowns + auxiliaryVariables);
            
            auto dx = volumeH(0);
            int normal = 1;

            double fluxFL[unknowns];
            double fluxFR[unknowns];

            SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL );
            SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR );

            double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal);
            double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal);
            double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

            for (int unknown = 0; unknown < unknowns; unknown++)
            {
              if (y > 0)
              {
                double fl = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                Qout[lowerVoxelInImage * (unknowns + auxiliaryVariables) + unknown] -= dt / volumeH (0) * fl;
              }
              if (y < numVPAIP)
              {
                double fr = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                Qout[upperVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
              }
            }
          }
        }
      }
    };







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
      int                                          numVPAIP, // numberofVolumesPerAxisInPatch
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

      #ifdef SharedOMP
        #if defined(OpenMPGPUOffloading)
        #pragma omp target
        #endif
      #pragma omp parallel for collapse(2)
      #endif
      for (int x = 0; x < numVPAIP; x++)
      for (int y = 0; y < numVPAIP; y++) {
        tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1};
        tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1};

        // getVolumeSize
        tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP};
        // Assignment vectorA = vectorB - 0.5*vectorC
        tarch::la::Vector<Dimensions, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1)};

        volumeX (0) += (x + 0.5) * volumeH (0);
        volumeX (1) += (y + 0.5) * volumeH (1);

        const int voxelInPreImage  = x+1
                                   + (y+1) * (numVPAIP+2);
        const int voxelInImage     = x
                                   + y * numVPAIP;

        double sourceTermContributions[unknowns];
        
        SOLVER::sourceTerm(
          Qin + voxelInPreImage * (unknowns + auxiliaryVariables),
          volumeX, volumeH(0), t, dt,
          sourceTermContributions
        );

        for (int unknown = 0; unknown < unknowns; unknown++) {
          Qout[voxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt * sourceTermContributions[unknown];
        }
      }
      
      
      for (int shift = 0; shift < 2; shift++)
      {
        #ifdef SharedOMP
           #if defined(OpenMPGPUOffloading)
           #pragma omp target
           #endif
        #pragma omp parallel for collapse(2)
        #endif

        for (int x = shift; x <= numVPAIP; x += 2)
        {
          for (int y = 0; y < numVPAIP; y++)
          {
            tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1};
            tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1};

            const int leftVoxelInPreimage  = x +      (y + 1) * (2 + numVPAIP);
            const int rightVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP);

            const int leftVoxelInImage     = x - 1 + y * numVPAIP;
            const int rightVoxelInImage    = x     + y * numVPAIP;


            // getVolumeSize
            tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP};
            // Assignment vectorA = vectorB - 0.5*vectorC
            tarch::la::Vector<Dimensions, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1)};

            volumeX(0) += x * volumeH (0);
            volumeX(1) += (y + 0.5) * volumeH (1);

            auto QL = Qin + leftVoxelInPreimage  * (unknowns + auxiliaryVariables);
            auto QR = Qin + rightVoxelInPreimage * (unknowns + auxiliaryVariables);
            
            auto dx = volumeH(0);
            int normal = 0;

            double fluxFL[unknowns];
            double fluxFR[unknowns];

            // The arguments don't make any sense
            SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL );
            SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR );

            double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal);
            double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal);

            double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

            for (int unknown = 0; unknown < unknowns; unknown++)
            {
              if (x > 0)
              {
                double fl = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                Qout[leftVoxelInImage * (unknowns + auxiliaryVariables) + unknown]  -= dt / volumeH (0) * fl;
              }
              if (x < numVPAIP)
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
        for (int y = shift; y <= numVPAIP; y += 2)
        {
          for (int x = 0; x < numVPAIP; x++)
          {
            tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1};
            tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1};
            const int lowerVoxelInPreimage = x + 1  +       y * (2 + numVPAIP);
            const int upperVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP);
            const int lowerVoxelInImage    = x      + (y - 1) *      numVPAIP ;
            const int upperVoxelInImage    = x      +       y *      numVPAIP ;

            // getVolumeSize
            tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP};
            // Assignment vectorA = vectorB - 0.5*vectorC
            tarch::la::Vector<Dimensions, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1)};

            volumeX (0) += (x + 0.5) * volumeH (0);
            volumeX (1) +=         y * volumeH (1);

            auto QL = Qin + lowerVoxelInPreimage  * (unknowns + auxiliaryVariables);
            auto QR = Qin + upperVoxelInPreimage * (unknowns + auxiliaryVariables);
            
            auto dx = volumeH(0);
            int normal = 1;

            double fluxFL[unknowns];
            double fluxFR[unknowns];

            SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL );
            SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR );

            double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal);
            double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal);
            double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

            for (int unknown = 0; unknown < unknowns; unknown++)
            {
              if (y > 0)
              {
                double fl = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                Qout[lowerVoxelInImage * (unknowns + auxiliaryVariables) + unknown] -= dt / volumeH (0) * fl;
              }
              if (y < numVPAIP)
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
      int                                          numVPAIP,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER>
    void applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop_Rusanov(
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
      double x2 = thepatchCentre[2];
      double h0 = thepatchSize[0];
      double h1 = thepatchSize[1];
      double h2 = thepatchSize[2];

      #ifdef SharedOMP
        #if defined(OpenMPGPUOffloading)
        #pragma omp target
        #endif
      #pragma omp parallel for collapse(3)
      #endif
      for (int x = 0; x < numVPAIP; x++)
      for (int y = 0; y < numVPAIP; y++)
      for (int z = 0; z < numVPAIP; z++) {
        tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1,x2};
        tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1,h2};

        // getVolumeSize
        tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP,patchSize(2)/numVPAIP};
        // Assignment vectorA = vectorB - 0.5*vectorC
        tarch::la::Vector<Dimensions, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1), patchCentre(2)-0.5*patchSize(2)};

        volumeX (0) += (x + 0.5) * volumeH (0);
        volumeX (1) += (y + 0.5) * volumeH (1);
        volumeX (2) += (z + 0.5) * volumeH (2);

        const int voxelInPreImage  = x+1
                                   + (y+1) * (numVPAIP+2)
                                   + (z+1) * (numVPAIP+2) * (numVPAIP+2);
        const int voxelInImage     = x
                                   + y * numVPAIP
                                   + z * numVPAIP * numVPAIP;

        double sourceTermContributions[unknowns];
  
        SOLVER::sourceTerm(
          Qin + voxelInPreImage * (unknowns + auxiliaryVariables),
          volumeX, volumeH(0), t, dt,
          sourceTermContributions
        );

        for (int unknown = 0; unknown < unknowns; unknown++) {
          Qout[voxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt * sourceTermContributions[unknown];
        }
      }


      for (int shift = 0; shift < 2; shift++)
      {
        #ifdef SharedOMP
           #if defined(OpenMPGPUOffloading)
           #pragma omp target
           #endif
        #pragma omp parallel for collapse(3)
        #endif
        for (int x = shift; x <= numVPAIP; x += 2)
        {
          for (int z = 0; z < numVPAIP; z++)
          {
            for (int y = 0; y < numVPAIP; y++)
            {
              tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1,x2};
              tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1,h2};

              const int leftVoxelInPreimage  = x      + (y + 1) * (2 + numVPAIP) + (z + 1) * (2 + numVPAIP) * (2 + numVPAIP);
              const int rightVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP) + (z + 1) * (2 + numVPAIP) * (2 + numVPAIP);
              const int leftVoxelInImage     = x - 1  +       y * numVPAIP       +       z * numVPAIP * numVPAIP;
              const int rightVoxelInImage    = x      +       y * numVPAIP       +       z * numVPAIP * numVPAIP;

              // getVolumeSize
              tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP,patchSize(2)/numVPAIP};
              // Assignment vectorA = vectorB - 0.5*vectorC
              tarch::la::Vector<Dimensions, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1), patchCentre(2)-0.5*patchSize(2)};

              volumeX (0) += x * volumeH (0);
              volumeX (1) += (y + 0.5) * volumeH (1);
              volumeX (2) += (z + 0.5) * volumeH (2);

              auto QL = Qin + leftVoxelInPreimage  * (unknowns + auxiliaryVariables);
              auto QR = Qin + rightVoxelInPreimage * (unknowns + auxiliaryVariables);

              auto dx = volumeH(0);
              int normal = 0;

              double fluxFL[unknowns];
              double fluxFR[unknowns];

              SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL );
              SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR );

              double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal);
              double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal);
              double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

              for (int unknown = 0; unknown < unknowns; unknown++)
              {
                if (x > 0)
                {
                  double fl = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  Qout[leftVoxelInImage * (unknowns + auxiliaryVariables) + unknown]  -= dt / volumeH (0) * fl;
                }
                if (x < numVPAIP)
                {
                  double fr = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  Qout[rightVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
                }
              }
            }
          }
        }
      }

      for (int shift = 0; shift < 2; shift++)
      {
        #ifdef SharedOMP
           #if defined(OpenMPGPUOffloading)
           #pragma omp target
           #endif
        #pragma omp parallel for collapse(3)
        #endif
        for (int y = shift; y <= numVPAIP; y += 2)
        {
          for (int z = 0; z < numVPAIP; z++)
          {
            for (int x = 0; x < numVPAIP; x++)
            {
              tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1,x2};
              tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1,h2};

              const int lowerVoxelInPreimage = x + 1 +       y * (2 + numVPAIP) + (z + 1) * (2 + numVPAIP) * (2 + numVPAIP);
              const int upperVoxelInPreimage = x + 1 + (y + 1) * (2 + numVPAIP) + (z + 1) * (2 + numVPAIP) * (2 + numVPAIP);
              const int lowerVoxelInImage    = x     + (y - 1) * numVPAIP       + z * numVPAIP * numVPAIP;
              const int upperVoxelInImage = x + y * numVPAIP                    + z * numVPAIP * numVPAIP;

              tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP,patchSize(2)/numVPAIP};
              tarch::la::Vector<Dimensions, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1), patchCentre(2)-0.5*patchSize(2)};

              volumeX(0) += (x + 0.5) * volumeH(0);
              volumeX(1) +=         y * volumeH(1);
              volumeX(2) += (z + 0.5) * volumeH(2);

              auto QL = Qin + lowerVoxelInPreimage * (unknowns + auxiliaryVariables);
              auto QR = Qin + upperVoxelInPreimage * (unknowns + auxiliaryVariables);

              auto dx = volumeH(0);
              int normal = 1;

              double fluxFL[unknowns];
              double fluxFR[unknowns];

              SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL );
              SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR );

              double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal);
              double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal);
              double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );


              for (int unknown = 0; unknown < unknowns; unknown++)
              {
                if (y > 0)
                {
                  double fl = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  Qout[lowerVoxelInImage * (unknowns + auxiliaryVariables) + unknown] -= dt / volumeH (0) * fl;
                }
                if (y < numVPAIP)
                {
                  double fr = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  Qout[upperVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
                }
              }
            }
          }
        }
      }

      for (int shift = 0; shift < 2; shift++)
      {
        #ifdef SharedOMP
           #if defined(OpenMPGPUOffloading)
           #pragma omp target
           #endif
        #pragma omp parallel for collapse(3)
        #endif
        for (int z = shift; z <= numVPAIP; z += 2)
        {
          for (int y = 0; y < numVPAIP; y++)
          {
            for (int x = 0; x < numVPAIP; x++)
            {
              tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1,x2};
              tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1,h2};

              const int lowerVoxelInPreimage = x + 1 + (y + 1) * (2 + numVPAIP) +       z * (2 + numVPAIP) * (2 + numVPAIP);
              const int upperVoxelInPreimage = x + 1 + (y + 1) * (2 + numVPAIP) + (z + 1) * (2 + numVPAIP) * (2 + numVPAIP);
              const int lowerVoxelInImage = x + y * numVPAIP                    + (z - 1) * numVPAIP * numVPAIP;
              const int upperVoxelInImage = x + y * numVPAIP                    +       z * numVPAIP * numVPAIP;

              tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP,patchSize(2)/numVPAIP};
              tarch::la::Vector<Dimensions, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1), patchCentre(2)-0.5*patchSize(2)};

              volumeX(0) += (x + 0.5) * volumeH(0);
              volumeX(1) += (y + 0.5) * volumeH(1);
              volumeX(2) +=         z * volumeH(2);

              auto QL = Qin + lowerVoxelInPreimage * (unknowns + auxiliaryVariables);
              auto QR = Qin + upperVoxelInPreimage * (unknowns + auxiliaryVariables);

              auto dx = volumeH(0);
              int normal = 2;

              double fluxFL[unknowns];
              double fluxFR[unknowns];

              SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL );
              SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR );

              double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal);
              double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal);
              double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );


              for (int unknown = 0; unknown < unknowns; unknown++)
              {
                if (z > 0)
                {
                  double fl = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  Qout[lowerVoxelInImage * (unknowns + auxiliaryVariables) + unknown] -= dt / volumeH (0) * fl;
                }
                if (z < numVPAIP)
                {
                  double fr = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  Qout[upperVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
                }
              }
            }
          }
        }
      }
    };
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif





    /**
     * 1d Riemann accepting a flux and eigenvalue function.
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
        const double * __restrict__     deltaQ,
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
  }
}


#endif
