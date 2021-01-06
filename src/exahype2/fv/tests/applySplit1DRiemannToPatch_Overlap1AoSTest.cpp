
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"
#include "tarch/la/Vector.h"
#include "applySplit1DRiemannToPatch_Overlap1AoSTest.h"
#include "exahype2/fv/Rusanov.h"

exahype2::fv::tests::applySplit1DRiemannToPatch_Overlap1AoSTest::applySplit1DRiemannToPatch_Overlap1AoSTest():
  TestCase ("exahype2::fv::tests::applySplit1DRiemannToPatch_Overlap1AoSTest") {
}

void exahype2::fv::tests::applySplit1DRiemannToPatch_Overlap1AoSTest::run() {
  testMethod (testMe);
}

void exahype2::fv::tests::applySplit1DRiemannToPatch_Overlap1AoSTest::testMe() {

   // This is really just testing the different ways to iterate throught he x,y grid
   // The input values are ad-hoc

  double * numericalFluxL = ::tarch::allocateMemory(5, tarch::MemoryLocation::Heap);
  double * numericalFluxR = ::tarch::allocateMemory(5, tarch::MemoryLocation::Heap);
  numericalFluxL[0]=1.0;
  numericalFluxL[1]=2.0;
  numericalFluxL[2]=3.0;
  numericalFluxL[3]=4.0;
  numericalFluxL[4]=5.0;
  numericalFluxR[0]=6.0;
  numericalFluxR[1]=7.0;
  numericalFluxR[2]=8.0;
  numericalFluxR[3]=9.0;
  numericalFluxR[4]=10.0;

  int numberOfVolumesPerAxisInPatch = 10;
  
  tarch::la::Vector<Dimensions,double> patchCentre = {0.018519,0.018519};
  tarch::la::Vector<Dimensions,double> patchSize   = {0.037037,0.037037};
  tarch::la::Vector<Dimensions,double> volumeH     = {0.01,0.01};


  int unknowns = 5;
  int auxiliaryVariables = 0;
  double dt = 0.1;

  double * Qout= new double[500];
  for (unsigned int i=0;i<500;++i) Qout[i]=0.0;

  for (int y=0; y<numberOfVolumesPerAxisInPatch; y++)
  {
    for (int x=0; x<=numberOfVolumesPerAxisInPatch; x++)
    {
      const int leftVoxelInImage     =  x - 1 + y * numberOfVolumesPerAxisInPatch;
      const int rightVoxelInImage    =  x     + y * numberOfVolumesPerAxisInPatch;

      tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
      volumeX(0) +=       x * volumeH(0);
      volumeX(1) += (y+0.5) * volumeH(1);

      for (int unknown=0; unknown<unknowns; unknown++)
      {
        if (x>0)
        { 
           Qout[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
        }
        if (x<numberOfVolumesPerAxisInPatch)
        {
           Qout[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
        }
      }
    }
  }

  for (int y=0; y<=numberOfVolumesPerAxisInPatch; y++)
  {
    for (int x=0; x<numberOfVolumesPerAxisInPatch; x++)
    {
      const int lowerVoxelInImage    = x + (y-1) * numberOfVolumesPerAxisInPatch;
      const int upperVoxelInImage    = x   + y   * numberOfVolumesPerAxisInPatch;

      tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
      volumeX(0) += (x+0.5) * volumeH(0);
      volumeX(1) +=       y * volumeH(1);

      for (int unknown=0; unknown<unknowns; unknown++) {
        if (y>0)
        {
          Qout[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
        }
        if (y<numberOfVolumesPerAxisInPatch)
        {
          Qout[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
        }
      }
    }
  }

  // Done with routine 1
   
  double * Qouttest= new double[500];
  for (unsigned int i=0;i<500;++i) Qouttest[i]=0.0;


  for (int shift=0; shift<2; shift++)
  {
    for (int x=shift; x<=numberOfVolumesPerAxisInPatch; x+=2)
    {
       for (int y=0; y<numberOfVolumesPerAxisInPatch; y++)
       {
          const int leftVoxelInImage     = x-1 + y * numberOfVolumesPerAxisInPatch;
          const int rightVoxelInImage    = x   + y * numberOfVolumesPerAxisInPatch;

          tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
          volumeX(0) +=       x * volumeH(0);
          volumeX(1) += (y+0.5) * volumeH(1);

          for (int unknown=0; unknown<unknowns; unknown++)
          {
             if (x>0)
             {
                Qouttest[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
             }
             if (x<numberOfVolumesPerAxisInPatch)
             {
                Qouttest[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
             }
          }
       }
    }
  }

  for (int shift=0; shift<2; shift++)
  {
     for (int y=shift; y<=numberOfVolumesPerAxisInPatch; y+=2)
     {
        for (int x=0; x<numberOfVolumesPerAxisInPatch; x++)
        {
           const int lowerVoxelInImage    = x + (y-1) * numberOfVolumesPerAxisInPatch;
           const int upperVoxelInImage    = x     + y * numberOfVolumesPerAxisInPatch;

           tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
           volumeX(0) += (x+0.5) * volumeH(0);
           volumeX(1) +=       y * volumeH(1);

           for (int unknown=0; unknown<unknowns; unknown++)
           {
              if (y>0)
              {
                 Qouttest[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
              }
              if (y<numberOfVolumesPerAxisInPatch)
              {
                 Qouttest[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
              }
           }
        }
     }
  }
  

  // Elementwise comparison

  for (unsigned int i=0;i<500;++i)
  {
     validateEquals(Qout[i], Qouttest[i]);
  }




  delete[] Qout;
  delete[] Qouttest;
  ::tarch::freeMemory(numericalFluxL, ::tarch::MemoryLocation::Heap);
  ::tarch::freeMemory(numericalFluxR, ::tarch::MemoryLocation::Heap);
}
