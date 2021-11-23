// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_H_
#define _EXAHYPE2_FV_RUSANOV_H_



#include "../PatchUtils.h"
#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/multicore.h"


#include <functional>
#include <algorithm>

namespace exahype2 {
  namespace fv {

    struct patchData2d
    {
      double*  reco; int* id; double* x0; double* h0; double* x1; double*
        h1; double* t; double* dt; int npatches; double * result;
    } ;

    struct patchData3d
    {
      double*  reco; int* id; double* x0; double* h0; double* x1; double*
        h1; double* x2; double* h2; double* t; double* dt; int npatches;
      double * result;
    } ;



    template<
      int                                          numVPAIP, // numberofVolumesPerAxisInPatch
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER,
      bool                                         skipFluxEvaluation,
      bool                                         skipNCPEvaluation,
      bool                                         skipSourceTerm
      >
    void Fusanov_2D(
      const int       haloSize,
      patchData2d     pV,
      const int       sourcePatchSize,
      const int       destPatchSize
    )
    {
      const size_t NPT  = pV.npatches;
      const size_t LTOT = NPT*destPatchSize;
      const size_t LR   = NPT*sourcePatchSize;
      printf("2D target to %lu and %lu and  %lu\n", NPT, LR, LTOT);

#if defined(CCZ4EINSTEIN)
#pragma omp target enter data map(to:SOLVER::CCZ4GLMc0) map(to:SOLVER::CCZ4GLMc) map(to:SOLVER::CCZ4GLMd) map(to:SOLVER::CCZ4GLMepsA) map(to:SOLVER::CCZ4GLMepsP) map(to:SOLVER::CCZ4GLMepsD) map(to:SOLVER::CCZ4itau) map(to:SOLVER::CCZ4k1) map(to:SOLVER::CCZ4k2) map(to:SOLVER::CCZ4k3) map(to:SOLVER::CCZ4eta)  map(to:SOLVER::CCZ4f) map(to:SOLVER::CCZ4g) map(to:SOLVER::CCZ4xi) map(to:SOLVER::CCZ4e) map(to:SOLVER::CCZ4c) map(to:SOLVER::CCZ4mu) map(to:SOLVER::CCZ4ds) map(to:SOLVER::CCZ4sk) map(to:SOLVER::CCZ4bs) map(to:SOLVER::CCZ4LapseType) map(to:SOLVER::CCZ4tp_grid_setup) map(to:SOLVER::CCZ4swi) map(to:SOLVER::CCZ4ReSwi) map(to:SOLVER::CCZ4source) map(to:SOLVER::Scenario)
#endif

#pragma omp target map(to:pV.reco[0:LR]) map(to:pV.id[0:NPT]) map(to:pV.x0[0:NPT]) map(to:pV.h0[0:NPT]) map(to:pV.x1[0:NPT]) map(to:pV.h1[0:NPT]) map(to:pV.t[0:NPT]) map(to:pV.dt[0:NPT]) map(from:pV.result[0:LTOT]) map(to:SOLVER::NumberOfUnknowns) map(to:SOLVER::NumberOfAuxiliaryVariables)
      {
#pragma omp teams distribute// num_teams(1)
        for (int pidx=0;pidx<NPT;pidx++)
        {
          const double                    t =   pV.t[pidx];
          const double                   dt =  pV.dt[pidx];
          const int                  taskId = pV.id[pidx];
          const double                   x0 =  pV.x0[pidx];
          const double                   h0 =  pV.h0[pidx];
          const double                   x1 =  pV.x1[pidx];
          const double                   h1 =  pV.h1[pidx];
          double *reconstructedPatch = pV.reco + sourcePatchSize*pidx;
          const tarch::la::Vector<2 ,double> volumeH = {h0/numVPAIP,h1/numVPAIP};

            // This is copyPatch
           #pragma omp parallel for collapse(3)
          for (int x = 0; x < numVPAIP; x++)
          for (int y = 0; y < numVPAIP; y++)
          for (int i=0; i<unknowns+auxiliaryVariables; i++)
          {
            int sourceIndex      = (y+1)*(numVPAIP+ 3*haloSize) + x - y;
            int destinationIndex = y*numVPAIP + x;
            pV.result[pidx*destPatchSize + destinationIndex*(unknowns+auxiliaryVariables)+i] =  reconstructedPatch[sourceIndex*(unknowns+auxiliaryVariables)+i];
          }

          if (not skipSourceTerm)
          {
           #pragma omp parallel for collapse(2)
            for (int x = 0; x < numVPAIP; x++)
            for (int y = 0; y < numVPAIP; y++)
            {
              tarch::la::Vector<2, double> volumeX = {x0-0.5*h0, x1-0.5*h1};
              volumeX (0) += (x + 0.5) * volumeH (0);
              volumeX (1) += (y + 0.5) * volumeH (1);
              const int voxelInPreImage  = x+1      + (y+1) * (numVPAIP+2);
              const int voxelInImage     = x            + y * numVPAIP;
              double sourceTermContributions[unknowns];
              SOLVER::sourceTerm( reconstructedPatch + voxelInPreImage * (unknowns + auxiliaryVariables),  volumeX, volumeH(0), t, dt, sourceTermContributions, SOLVER::Offloadable::Yes);

              for (int unknown = 0; unknown < unknowns; unknown++)
              {
                pV.result[pidx*destPatchSize + voxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt * sourceTermContributions[unknown];
              }
            }
          }


          for (int shift = 0; shift < 2; shift++)
          {
            #pragma omp parallel for collapse(2)
            for (int x = shift; x <= numVPAIP; x += 2)
            for (int y = 0; y < numVPAIP; y++)
            {
                tarch::la::Vector<2,double> volumeX = {x0-0.5*h0, x1-0.5*h1};

                volumeX(0) += x * volumeH(0);
                volumeX(1) += (y + 0.5) * volumeH(1);
                
                int leftVoxelInPreimage  = x +      (y + 1) * (2 + numVPAIP);
                int rightVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP);
                double * QL = reconstructedPatch + leftVoxelInPreimage  * (unknowns + auxiliaryVariables);
                double * QR = reconstructedPatch + rightVoxelInPreimage * (unknowns + auxiliaryVariables);

                double dx = volumeH(0);
                const int normal = 0;
                
                double fluxFL[unknowns], fluxFR[unknowns], fluxNCP[unknowns];
                
                if (not skipFluxEvaluation)
                {
                  SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL, SOLVER::Offloadable::Yes );
                  SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR, SOLVER::Offloadable::Yes );
                }

                if (not skipNCPEvaluation)
                {
                  double Qaverage[unknowns+auxiliaryVariables];
                  double deltaQ[unknowns+auxiliaryVariables];

                  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++)
                  {
                    Qaverage[unknown] = 0.5 * QL[unknown] + 0.5 * QR[unknown];
                    deltaQ[unknown]   = QR[unknown] - QL[unknown];
                  }
                  SOLVER::nonconservativeProduct(Qaverage,deltaQ,x,dx,t,normal,fluxNCP, SOLVER::Offloadable::Yes );
                }


                double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal, SOLVER::Offloadable::Yes );
                double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal, SOLVER::Offloadable::Yes );
                double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );
                
                int leftVoxelInImage     = x - 1 + y * numVPAIP;
                int rightVoxelInImage    = x     + y * numVPAIP;

                for (int unknown = 0; unknown < unknowns; unknown++)
                {
                  if (x > 0)
                  {
                    double fl = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                    if (not skipFluxEvaluation) fl +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown];
                    if (not skipNCPEvaluation)  fl += + 0.5 * fluxNCP[unknown];
                    pV.result[pidx*destPatchSize + leftVoxelInImage * (unknowns + auxiliaryVariables) + unknown]  -= dt / volumeH (0) * fl;
                  }
                  if (x < numVPAIP)
                  {
                    double fr = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                    if (not skipFluxEvaluation) fr +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown]; 
                    if (not skipNCPEvaluation)  fr += - 0.5 * fluxNCP[unknown];
                    pV.result[pidx*destPatchSize + rightVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
                  }
                }
              }

            //Iterate over other normal
            #pragma omp parallel for collapse(2)
            for (int y = shift; y <= numVPAIP; y += 2)
            for (int x = 0;     x <  numVPAIP; x++   )
            {
                tarch::la::Vector<2,double> patchCentre = {x0,x1};
                tarch::la::Vector<2,double> patchSize   = {h0,h1};
                 int lowerVoxelInPreimage = x + 1  +       y * (2 + numVPAIP);
                 int upperVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP);
                 int lowerVoxelInImage    = x      + (y - 1) *      numVPAIP ;
                 int upperVoxelInImage    = x      +       y *      numVPAIP ;

                // getVolumeSize
                tarch::la::Vector<2,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP};
                // Assignment vectorA = vectorB - 0.5*vectorC
                tarch::la::Vector<2, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1)};

                volumeX (0) += (x + 0.5) * volumeH (0);
                volumeX (1) +=         y * volumeH (1);

                double* QL = reconstructedPatch + lowerVoxelInPreimage  * (unknowns + auxiliaryVariables);
                double* QR = reconstructedPatch + upperVoxelInPreimage * (unknowns + auxiliaryVariables);
                
                auto dx = volumeH(0);
                int normal = 1;

                double fluxFL[unknowns], fluxFR[unknowns], fluxNCP[unknowns];

                if (not skipFluxEvaluation)
                {
                  SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL , SOLVER::Offloadable::Yes );
                  SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR , SOLVER::Offloadable::Yes );
                }
                
                if (not skipNCPEvaluation)
                {
                  double Qaverage[unknowns+auxiliaryVariables];
                  double deltaQ[unknowns+auxiliaryVariables];

                  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++)
                  {
                    Qaverage[unknown] = 0.5 * QL[unknown] + 0.5 * QR[unknown];
                    deltaQ[unknown]   = QR[unknown] - QL[unknown];
                  }
                  SOLVER::nonconservativeProduct(Qaverage,deltaQ,x,dx,t,normal,fluxNCP, SOLVER::Offloadable::Yes );
                }

                double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal, SOLVER::Offloadable::Yes );
                double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal, SOLVER::Offloadable::Yes );
                double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

                for (int unknown = 0; unknown < unknowns; unknown++)
                {
                  if (y > 0)
                  {
                    double fl = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                    if (not skipFluxEvaluation) fl +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown];
                    if (not skipNCPEvaluation)  fl += + 0.5 * fluxNCP[unknown];
                    pV.result[pidx*destPatchSize + lowerVoxelInImage * (unknowns + auxiliaryVariables) + unknown] -= dt / volumeH (0) * fl;
                  }
                  if (y < numVPAIP)
                  {
                    double fr = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                    if (not skipFluxEvaluation) fr +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown]; 
                    if (not skipNCPEvaluation)  fr += - 0.5 * fluxNCP[unknown];
                    pV.result[pidx*destPatchSize + upperVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
                  }
                }
            }
          }
        }
      }
#pragma omp taskwait
    };


    /**
     * Fused kernel that supports execution on GPUs as well as CPUs. This is a
     * rewrite of the original OpenMP kernel.
     */
    template<
      int      numVPAIP, // numberofVolumesPerAxisInPatch
      int      unknowns,
      int      auxiliaryVariables,
      typename SOLVER,
      bool     skipFluxEvaluation,
      bool     skipNCPEvaluation,
      bool     skipSourceTerm
      >
    void Fusanov_2D_host(
      const int       haloSize,
      patchData2d     pV,
      const int       sourcePatchSize,
      const int       destPatchSize
    ) {
      const size_t NPT  = pV.npatches;
      const size_t LTOT = NPT*destPatchSize;
      const size_t LR   = NPT*sourcePatchSize;

      {
        #pragma omp for collapse(4)
        for (int pidx=0;pidx<NPT;pidx++)
        for (int x = 0; x < numVPAIP; x++)
        for (int y = 0; y < numVPAIP; y++)
        for (int i=0; i<unknowns+auxiliaryVariables; i++) {
          const double                    t =   pV.t[pidx];
          const double                   dt =  pV.dt[pidx];
          const int                  taskId = pV.id[pidx];
          const double                   x0 =  pV.x0[pidx];
          const double                   h0 =  pV.h0[pidx];
          const double                   x1 =  pV.x1[pidx];
          const double                   h1 =  pV.h1[pidx];
          double *reconstructedPatch = pV.reco + sourcePatchSize*pidx;
          const tarch::la::Vector<2 ,double> volumeH = {h0/numVPAIP,h1/numVPAIP};

          int sourceIndex      = (y+1)*(numVPAIP+ 3*haloSize) + x - y;
          int destinationIndex = y*numVPAIP + x;
          pV.result[pidx*destPatchSize + destinationIndex*(unknowns+auxiliaryVariables)+i] =  reconstructedPatch[sourceIndex*(unknowns+auxiliaryVariables)+i];
        }
      }

      if (not skipSourceTerm) {
        #pragma omp for collapse(3)
        for (int pidx=0;pidx<NPT;pidx++)
        for (int x = 0; x < numVPAIP; x++)
        for (int y = 0; y < numVPAIP; y++) {
          const double                    t =   pV.t[pidx];
          const double                   dt =  pV.dt[pidx];
          const int                  taskId = pV.id[pidx];
          const double                   x0 =  pV.x0[pidx];
          const double                   h0 =  pV.h0[pidx];
          const double                   x1 =  pV.x1[pidx];
          const double                   h1 =  pV.h1[pidx];
          double *reconstructedPatch = pV.reco + sourcePatchSize*pidx;
          const tarch::la::Vector<2 ,double> volumeH = {h0/numVPAIP,h1/numVPAIP};

          tarch::la::Vector<2, double> volumeX = {x0-0.5*h0, x1-0.5*h1};
          volumeX (0) += (x + 0.5) * volumeH (0);
          volumeX (1) += (y + 0.5) * volumeH (1);
          const int voxelInPreImage  = x+1      + (y+1) * (numVPAIP+2);
          const int voxelInImage     = x            + y * numVPAIP;
          double sourceTermContributions[unknowns];
          SOLVER::sourceTerm( reconstructedPatch + voxelInPreImage * (unknowns + auxiliaryVariables),  volumeX, volumeH(0), t, dt, sourceTermContributions, SOLVER::Offloadable::Yes);

          for (int unknown = 0; unknown < unknowns; unknown++) {
            pV.result[pidx*destPatchSize + voxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt * sourceTermContributions[unknown];
          }
        }
      }


      for (int shift = 0; shift < 2; shift++) {
        #pragma omp parallel for collapse(3)
        for (int pidx=0;pidx<NPT;pidx++)
        for (int x = shift; x <= numVPAIP; x += 2)
        for (int y = 0; y < numVPAIP; y++) {
          const double                    t =   pV.t[pidx];
          const double                   dt =  pV.dt[pidx];
          const int                  taskId = pV.id[pidx];
          const double                   x0 =  pV.x0[pidx];
          const double                   h0 =  pV.h0[pidx];
          const double                   x1 =  pV.x1[pidx];
          const double                   h1 =  pV.h1[pidx];
          double *reconstructedPatch = pV.reco + sourcePatchSize*pidx;
          const tarch::la::Vector<2 ,double> volumeH = {h0/numVPAIP,h1/numVPAIP};

          tarch::la::Vector<2,double> volumeX = {x0-0.5*h0, x1-0.5*h1};

          volumeX(0) += x * volumeH(0);
          volumeX(1) += (y + 0.5) * volumeH(1);

          int leftVoxelInPreimage  = x +      (y + 1) * (2 + numVPAIP);
          int rightVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP);
          double * QL = reconstructedPatch + leftVoxelInPreimage  * (unknowns + auxiliaryVariables);
          double * QR = reconstructedPatch + rightVoxelInPreimage * (unknowns + auxiliaryVariables);

          double dx = volumeH(0);
          const int normal = 0;

          double fluxFL[unknowns], fluxFR[unknowns], fluxNCP[unknowns];

          if (not skipFluxEvaluation) {
            SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL, SOLVER::Offloadable::Yes );
            SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR, SOLVER::Offloadable::Yes );
          }

          if (not skipNCPEvaluation) {
            double Qaverage[unknowns+auxiliaryVariables];
            double deltaQ[unknowns+auxiliaryVariables];

            for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
              Qaverage[unknown] = 0.5 * QL[unknown] + 0.5 * QR[unknown];
              deltaQ[unknown]   = QR[unknown] - QL[unknown];
            }
            SOLVER::nonconservativeProduct(Qaverage,deltaQ,x,dx,t,normal,fluxNCP, SOLVER::Offloadable::Yes );
          }

          double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal, SOLVER::Offloadable::Yes );
          double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal, SOLVER::Offloadable::Yes );
          double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

          int leftVoxelInImage     = x - 1 + y * numVPAIP;
          int rightVoxelInImage    = x     + y * numVPAIP;

          for (int unknown = 0; unknown < unknowns; unknown++) {
            if (x > 0) {
              double fl = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
              if (not skipFluxEvaluation) fl +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown];
              if (not skipNCPEvaluation)  fl += + 0.5 * fluxNCP[unknown];
              pV.result[pidx*destPatchSize + leftVoxelInImage * (unknowns + auxiliaryVariables) + unknown]  -= dt / volumeH (0) * fl;
            }
            if (x < numVPAIP) {
              double fr = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
              if (not skipFluxEvaluation) fr +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown];
              if (not skipNCPEvaluation)  fr += - 0.5 * fluxNCP[unknown];
              pV.result[pidx*destPatchSize + rightVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
            }
          }
        }
      }

      for (int shift = 0; shift < 2; shift++) {
        #pragma omp parallel for collapse(3)
        for (int pidx=0;pidx<NPT;pidx++)
        for (int y = shift; y <= numVPAIP; y += 2)
        for (int x = 0;     x <  numVPAIP; x++   ) {
          const double                    t =   pV.t[pidx];
          const double                   dt =  pV.dt[pidx];
          const int                  taskId = pV.id[pidx];
          const double                   x0 =  pV.x0[pidx];
          const double                   h0 =  pV.h0[pidx];
          const double                   x1 =  pV.x1[pidx];
          const double                   h1 =  pV.h1[pidx];
          double *reconstructedPatch = pV.reco + sourcePatchSize*pidx;
          const tarch::la::Vector<2 ,double> volumeH = {h0/numVPAIP,h1/numVPAIP};

          tarch::la::Vector<2,double> patchCentre = {x0,x1};
          tarch::la::Vector<2,double> patchSize   = {h0,h1};
          int lowerVoxelInPreimage = x + 1  +       y * (2 + numVPAIP);
          int upperVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP);
          int lowerVoxelInImage    = x      + (y - 1) *      numVPAIP ;
          int upperVoxelInImage    = x      +       y *      numVPAIP ;

          //tarch::la::Vector<2,double>  volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP};
          tarch::la::Vector<2, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1)};

          volumeX (0) += (x + 0.5) * volumeH (0);
          volumeX (1) +=         y * volumeH (1);

          double* QL = reconstructedPatch + lowerVoxelInPreimage  * (unknowns + auxiliaryVariables);
          double* QR = reconstructedPatch + upperVoxelInPreimage * (unknowns + auxiliaryVariables);

          auto dx = volumeH(0);
          int normal = 1;

          double fluxFL[unknowns], fluxFR[unknowns], fluxNCP[unknowns];

          if (not skipFluxEvaluation) {
            SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL , SOLVER::Offloadable::Yes );
            SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR , SOLVER::Offloadable::Yes );
          }

          if (not skipNCPEvaluation) {
            double Qaverage[unknowns+auxiliaryVariables];
            double deltaQ[unknowns+auxiliaryVariables];

            for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
              Qaverage[unknown] = 0.5 * QL[unknown] + 0.5 * QR[unknown];
              deltaQ[unknown]   = QR[unknown] - QL[unknown];
            }
            SOLVER::nonconservativeProduct(Qaverage,deltaQ,x,dx,t,normal,fluxNCP, SOLVER::Offloadable::Yes );
          }

          double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal, SOLVER::Offloadable::Yes );
          double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal, SOLVER::Offloadable::Yes );
          double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

          for (int unknown = 0; unknown < unknowns; unknown++) {
            if (y > 0) {
              double fl = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
              if (not skipFluxEvaluation) fl +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown];
              if (not skipNCPEvaluation)  fl += + 0.5 * fluxNCP[unknown];
              pV.result[pidx*destPatchSize + lowerVoxelInImage * (unknowns + auxiliaryVariables) + unknown] -= dt / volumeH (0) * fl;
            }
            if (y < numVPAIP) {
              double fr = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
              if (not skipFluxEvaluation) fr +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown];
              if (not skipNCPEvaluation)  fr += - 0.5 * fluxNCP[unknown];
              pV.result[pidx*destPatchSize + upperVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
            }
          }
        }
      }
    };



//
//   3D fused copyPatch and Rusanov1D
//
//
    template<
      int                                          numVPAIP, //numberofVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER,
      bool                                         skipFluxEvaluation,
      bool                                         skipNCPEvaluation,
      bool                                         skipSourceTerm
    >
    void Fusanov_3D(
      int                                          haloSize,
      patchData3d                            pV,
      int                                          sourcePatchSize,
      int                                          destPatchSize
    )
    {
      const size_t NPT  = pV.npatches;
      const size_t LTOT = NPT*destPatchSize;
      const size_t LR   = NPT*sourcePatchSize;
      printf("3D target npatches: %lu  total source: %lu total dest:  %lu --- numVPAIP: %d  source size %i  dest size %i\n", NPT, LR, LTOT, numVPAIP, sourcePatchSize, destPatchSize);


      
#if defined(CCZ4EINSTEIN)
#pragma omp target enter data map(to:SOLVER::CCZ4GLMc0) map(to:SOLVER::CCZ4GLMc) map(to:SOLVER::CCZ4GLMd) map(to:SOLVER::CCZ4GLMepsA) map(to:SOLVER::CCZ4GLMepsP) map(to:SOLVER::CCZ4GLMepsD) map(to:SOLVER::CCZ4itau) map(to:SOLVER::CCZ4k1) map(to:SOLVER::CCZ4k2) map(to:SOLVER::CCZ4k3) map(to:SOLVER::CCZ4eta)  map(to:SOLVER::CCZ4f) map(to:SOLVER::CCZ4g) map(to:SOLVER::CCZ4xi) map(to:SOLVER::CCZ4e) map(to:SOLVER::CCZ4c) map(to:SOLVER::CCZ4mu) map(to:SOLVER::CCZ4ds) map(to:SOLVER::CCZ4sk) map(to:SOLVER::CCZ4bs) map(to:SOLVER::CCZ4LapseType) map(to:SOLVER::CCZ4tp_grid_setup) map(to:SOLVER::CCZ4swi) map(to:SOLVER::CCZ4ReSwi) map(to:SOLVER::CCZ4source) map(to:SOLVER::Scenario)
#endif

#pragma omp target map(to:pV.reco[0:LR]) map(to:pV.t[0:NPT]) map(to:pV.dt[0:NPT]) map(to:pV.id[0:NPT]) map(to:pV.x0[0:NPT]) map(to:pV.x1[0:NPT])  map(to:pV.x2[0:NPT])  map(to:pV.h0[0:NPT]) map(to:pV.h1[0:NPT])  map(to:pV.h2[0:NPT]) map(from:pV.result[0:LTOT]) map(to:SOLVER::NumberOfUnknowns) map(to:SOLVER::NumberOfAuxiliaryVariables)
      {
#pragma omp teams distribute 
        for (int pidx=0;pidx<NPT;pidx++)
        {
          const double                    t =   pV.t[pidx];
          const double                   dt =  pV.dt[pidx];
          const int                  taskId = pV.id[pidx];
          const double                   x0 =  pV.x0[pidx];
          const double                   h0 =  pV.h0[pidx];
          const double                   x1 =  pV.x1[pidx];
          const double                   h1 =  pV.h1[pidx];
          const double                   x2 =  pV.x2[pidx];
          const double                   h2 =  pV.h2[pidx];
          double *reconstructedPatch = pV.reco + sourcePatchSize*pidx;


          const int sourceSerialised = (numVPAIP + haloSize * 2) * (numVPAIP + haloSize * 2) + numVPAIP + haloSize * 2 + haloSize;
          const int helper = numVPAIP+haloSize*2;

          #pragma omp parallel for collapse(4)
          for (int z = 0; z < numVPAIP; z++)
          for (int y = 0; y < numVPAIP; y++)
          for (int x = 0; x < numVPAIP; x++)
          for (int i = 0; i < unknowns + auxiliaryVariables; i++)
          {
             const int mydest = z*numVPAIP*numVPAIP + y*numVPAIP + x;
             const int mysrc  = z*helper*helper + y*helper + x + sourceSerialised;
             pV.result[pidx*destPatchSize + mydest * (unknowns + auxiliaryVariables) + i] =reconstructedPatch[mysrc * (unknowns + auxiliaryVariables) + i];
          }

         if (not skipSourceTerm)
         {
          #pragma omp parallel for collapse(3)
            for (int z = 0; z < numVPAIP; z++)
            for (int y = 0; y < numVPAIP; y++)
            for (int x = 0; x < numVPAIP; x++)
            {
               tarch::la::Vector<Dimensions, double> volumeX = {x0-0.5*h0, x1-0.5*h1, x2-0.5*h2};

               volumeX (0) += (x + 0.5) * h0/numVPAIP;
               volumeX (1) += (y + 0.5) * h1/numVPAIP;
               volumeX (2) += (z + 0.5) * h2/numVPAIP;
               
               const int voxelInPreImage  = x+1    + (y+1) * (numVPAIP+2)  + (z+1) * (numVPAIP+2) * (numVPAIP+2);
               const int voxelInImage     = x          + y * numVPAIP          + z * numVPAIP * numVPAIP;

               double sourceTermContributions[unknowns];
               SOLVER::sourceTerm( reconstructedPatch + voxelInPreImage * (unknowns + auxiliaryVariables), volumeX, h0/numVPAIP, t, dt, sourceTermContributions, SOLVER::Offloadable::Yes);

               for (int unknown = 0; unknown < unknowns; unknown++)
               {
                 pV.result[pidx*destPatchSize + voxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt * sourceTermContributions[unknown];
               }
             }
           }

        for (int shift = 0; shift < 2; shift++)
        {

          #pragma omp parallel for collapse(3)
            for (int x = shift; x <= numVPAIP; x += 2)
            for (int z = 0; z < numVPAIP; z++)
            for (int y = 0; y < numVPAIP; y++)
            {
                tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1,x2};
                tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1,h2};


                // getVolumeSize
                const double sf = 1./numVPAIP;
                tarch::la::Vector<Dimensions,double> volumeH = {sf*patchSize(0),sf*patchSize(1),sf*patchSize(2)};
                // Assignment vectorA = vectorB - 0.5*vectorC
                tarch::la::Vector<Dimensions, double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1), patchCentre(2)-0.5*patchSize(2)};

                volumeX (0) += x * volumeH (0);
                volumeX (1) += (y + 0.5) * volumeH (1);
                volumeX (2) += (z + 0.5) * volumeH (2);

                int leftVoxelInPreimage  = x      + (y + 1) * (2 + numVPAIP) + (z + 1) * (2 + numVPAIP) * (2 + numVPAIP);
                int rightVoxelInPreimage = x + 1  + (y + 1) * (2 + numVPAIP) + (z + 1) * (2 + numVPAIP) * (2 + numVPAIP);
                double* QL = reconstructedPatch + leftVoxelInPreimage  * (unknowns + auxiliaryVariables);
                double* QR = reconstructedPatch + rightVoxelInPreimage * (unknowns + auxiliaryVariables);
                
                auto dx = volumeH(0);
                int normal = 0;

                double fluxFL[unknowns], fluxFR[unknowns], fluxNCP[unknowns];

                if (not skipFluxEvaluation)
                {
                  SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL , SOLVER::Offloadable::Yes);
                  SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR , SOLVER::Offloadable::Yes);
                }
                
                if (not skipNCPEvaluation)
                {
                  double Qaverage[unknowns+auxiliaryVariables];
                  double deltaQ[unknowns+auxiliaryVariables];

                  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++)
                  {
                    Qaverage[unknown] = *(QL+unknown)*0.5 + *(QR +unknown)*0.5;
                    deltaQ[unknown]   = *(QR+unknown) - *(QL+unknown);
                  }
                  SOLVER::nonconservativeProduct(Qaverage,deltaQ,x,dx,t,normal,fluxNCP, SOLVER::Offloadable::Yes);
                }

                double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal, SOLVER::Offloadable::Yes);
                double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal, SOLVER::Offloadable::Yes);
                double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

                const int leftVoxelInImage     = x - 1  +       y * numVPAIP       +       z * numVPAIP * numVPAIP;
                const int rightVoxelInImage(x      +       y * numVPAIP       +       z * numVPAIP * numVPAIP);
                for (int unknown = 0; unknown < unknowns; unknown++)
                {
                  if (x > 0)
                  {
                    double fl = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                    if (not skipFluxEvaluation) fl +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown];
                    if (not skipNCPEvaluation)  fl += + 0.5 * fluxNCP[unknown];
                    pV.result[pidx*destPatchSize + leftVoxelInImage * (unknowns + auxiliaryVariables) + unknown]  -= dt / volumeH (0) * fl;
                  }
                  if (x < numVPAIP)
                  {
                    double fr = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                    if (not skipFluxEvaluation) fr +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown]; 
                    if (not skipNCPEvaluation)  fr += - 0.5 * fluxNCP[unknown];
                    pV.result[pidx*destPatchSize + rightVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
                  }
                }
            }

          #pragma omp parallel for collapse(3)
            for (int y = shift; y <= numVPAIP; y += 2)
            for (int z = 0; z < numVPAIP; z++)
            for (int x = 0; x < numVPAIP; x++)
            {
              tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1,x2};
              tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1,h2};

              tarch::la::Vector<Dimensions,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP,patchSize(2)/numVPAIP};
              tarch::la::Vector<Dimensions,double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1), patchCentre(2)-0.5*patchSize(2)};

              volumeX(0) += (x + 0.5) * volumeH(0);
              volumeX(1) +=         y * volumeH(1);
              volumeX(2) += (z + 0.5) * volumeH(2);
              
              const int lowerVoxelInPreimage = x + 1 +       y * (2 + numVPAIP) + (z + 1) * (2 + numVPAIP) * (2 + numVPAIP);
              const int upperVoxelInPreimage = x + 1 + (y + 1) * (2 + numVPAIP) + (z + 1) * (2 + numVPAIP) * (2 + numVPAIP);
              const int lowerVoxelInImage    = x     + (y - 1) * numVPAIP       + z * numVPAIP * numVPAIP;
              const int upperVoxelInImage = x + y * numVPAIP                    + z * numVPAIP * numVPAIP;

              auto QL = reconstructedPatch + lowerVoxelInPreimage * (unknowns + auxiliaryVariables);
              auto QR = reconstructedPatch + upperVoxelInPreimage * (unknowns + auxiliaryVariables);

              auto dx = volumeH(0);
              int normal = 1;

              double fluxFL[unknowns], fluxFR[unknowns], fluxNCP[unknowns];

              if (not skipFluxEvaluation)
              {
                SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL, SOLVER::Offloadable::Yes );
                SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR, SOLVER::Offloadable::Yes );
              }
              
              if (not skipNCPEvaluation)
              {
                double Qaverage[unknowns+auxiliaryVariables];
                double deltaQ[unknowns+auxiliaryVariables];

                for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++)
                {
                  Qaverage[unknown] = *(QL+unknown)*0.5 + *(QR +unknown)*0.5;
                  deltaQ[unknown]   = *(QR+unknown) - *(QL+unknown);
                }
                SOLVER::nonconservativeProduct(Qaverage,deltaQ,x,dx,t,normal,fluxNCP, SOLVER::Offloadable::Yes);
              }

              double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal, SOLVER::Offloadable::Yes);
              double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal, SOLVER::Offloadable::Yes);
              double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );


              for (int unknown = 0; unknown < unknowns; unknown++)
              {
                if (y > 0)
                {
                  double fl = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  if (not skipFluxEvaluation) fl +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown];
                  if (not skipNCPEvaluation)  fl += + 0.5 * fluxNCP[unknown];
                  pV.result[pidx*destPatchSize + lowerVoxelInImage * (unknowns + auxiliaryVariables) + unknown] -= dt / volumeH (0) * fl;
                }
                if (y < numVPAIP)
                {
                  double fr = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                  if (not skipFluxEvaluation) fr +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown]; 
                  if (not skipNCPEvaluation)  fr += - 0.5 * fluxNCP[unknown];
                  pV.result[pidx*destPatchSize + upperVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
                }
              }
            }


          #pragma omp parallel for collapse(3)
            for (int z = shift; z <= numVPAIP; z += 2)
            {
              for (int y = 0; y < numVPAIP; y++)
              {
                for (int x = 0; x < numVPAIP; x++)
                {
                  tarch::la::Vector<Dimensions,double> patchCentre = {x0,x1,x2};
                  tarch::la::Vector<Dimensions,double> patchSize   = {h0,h1,h2};

                  tarch::la::Vector<Dimensions,double> volumeH = {patchSize(0)/numVPAIP,patchSize(1)/numVPAIP,patchSize(2)/numVPAIP};
                  tarch::la::Vector<Dimensions,double> volumeX = {patchCentre(0)-0.5*patchSize(0), patchCentre(1)-0.5*patchSize(1), patchCentre(2)-0.5*patchSize(2)};

                  volumeX(0) += (x + 0.5) * volumeH(0);
                  volumeX(1) += (y + 0.5) * volumeH(1);
                  volumeX(2) +=         z * volumeH(2);

                  const int lowerVoxelInPreimage = x + 1 + (y + 1) * (2 + numVPAIP) +       z * (2 + numVPAIP) * (2 + numVPAIP);
                  const int upperVoxelInPreimage = x + 1 + (y + 1) * (2 + numVPAIP) + (z + 1) * (2 + numVPAIP) * (2 + numVPAIP);
                  const int lowerVoxelInImage = x + y * numVPAIP                    + (z - 1) * numVPAIP * numVPAIP;
                  const int upperVoxelInImage = x + y * numVPAIP                    +       z * numVPAIP * numVPAIP;

                  auto QL = reconstructedPatch + lowerVoxelInPreimage * (unknowns + auxiliaryVariables);
                  auto QR = reconstructedPatch + upperVoxelInPreimage * (unknowns + auxiliaryVariables);

                  auto dx = volumeH(0);
                  int normal = 2;

                  double fluxFL[unknowns], fluxFR[unknowns], fluxNCP[unknowns];

                  if (not skipFluxEvaluation)
                  {
                    SOLVER::flux( QL, volumeX, dx, t, normal, fluxFL , SOLVER::Offloadable::Yes);
                    SOLVER::flux( QR, volumeX, dx, t, normal, fluxFR , SOLVER::Offloadable::Yes);
                  }
                
                  if (not skipNCPEvaluation)
                  {
                    double Qaverage[unknowns+auxiliaryVariables];
                    double deltaQ[unknowns+auxiliaryVariables];

                    for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++)
                    {
                      Qaverage[unknown] = *(QL+unknown)*0.5 + *(QR +unknown)*0.5;
                      deltaQ[unknown]   = *(QR+unknown) - *(QL+unknown);
                    }
                    SOLVER::nonconservativeProduct(Qaverage,deltaQ,x,dx,t,normal,fluxNCP, SOLVER::Offloadable::Yes);
                  }

                  double lambdaMaxL = SOLVER::maxEigenvalue(QL,volumeX,dx,t,normal, SOLVER::Offloadable::Yes);
                  double lambdaMaxR = SOLVER::maxEigenvalue(QR,volumeX,dx,t,normal, SOLVER::Offloadable::Yes);
                  double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );


                  for (int unknown = 0; unknown < unknowns; unknown++)
                  {
                    if (z > 0)
                    {
                      double fl = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                      if (not skipFluxEvaluation) fl +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown];
                      if (not skipNCPEvaluation)  fl += + 0.5 * fluxNCP[unknown];
                      pV.result[pidx*destPatchSize + lowerVoxelInImage * (unknowns + auxiliaryVariables) + unknown] -= dt / volumeH (0) * fl;
                    }
                    if (z < numVPAIP)
                    {
                      double fr = - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
                      if (not skipFluxEvaluation) fr +=   0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown]; 
                      if (not skipNCPEvaluation)  fr += - 0.5 * fluxNCP[unknown];
                      pV.result[pidx*destPatchSize + upperVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * fr;
                    }
                  }
                }
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
      #pragma omp parallel for // collapse(2)
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
        #pragma omp parallel for // collapse(2)
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
        #pragma omp parallel for // collapse(2)
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
      #pragma omp parallel for // collapse(3)
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
        #pragma omp parallel for // collapse(3)
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
        #pragma omp parallel for // collapse(3)
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
        #pragma omp parallel for // collapse(3)
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
      double * __restrict__     FL,
      double * __restrict__     FR,
      bool skipFluxEvaluation   = false,
      bool skipNCPEvaluation    = false
    );
  }
}


#endif
