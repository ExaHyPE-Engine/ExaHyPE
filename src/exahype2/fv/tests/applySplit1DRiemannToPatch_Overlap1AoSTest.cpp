
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
  
  // These two values are not used in this test. So I comment them
  // out to eliminate compiler warnings.
  //tarch::la::Vector<Dimensions,double> patchCentre = {0.018519,0.018519};
  //tarch::la::Vector<Dimensions,double> patchSize   = {0.037037,0.037037};
  tarch::la::Vector<Dimensions,double> volumeH     = {0.01,0.01};


  int unknowns = 5;
  int auxiliaryVariables = 0;
  double dt = 0.1;

  double * Qout= new double[500];
  double * Qcmp= new double[500];
  for (unsigned int i=0;i<500;++i) 
  {
     Qout[i]=0.0;
     Qcmp[i]=1.0*i;
  }

  for (int y=0; y<numberOfVolumesPerAxisInPatch; y++)
  {
    for (int x=0; x<=numberOfVolumesPerAxisInPatch; x++)
    {
      const int leftVoxelInImage     =  x - 1 + y * numberOfVolumesPerAxisInPatch;
      const int rightVoxelInImage    =  x     + y * numberOfVolumesPerAxisInPatch;


      for (int unknown=0; unknown<unknowns; unknown++)
      {
        if (x>0)
        { 
           Qout[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
           //printf("loopA,%d,%d,%d,%d,%d,%d\n", x, y, leftVoxelInImage, rightVoxelInImage, unknown, leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown);
           Qout[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
        }
        if (x<numberOfVolumesPerAxisInPatch)
        {
           Qout[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
           //printf("loopA,%d,%d,%d,%d,%d,%d\n", x, y, leftVoxelInImage, rightVoxelInImage, unknown, rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown);
           Qout[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
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


      for (int unknown=0; unknown<unknowns; unknown++) {
        if (y>0)
        {
          Qout[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
           //printf("loopB,%d,%d,%d,%d,%d,%d\n", x, y, upperVoxelInImage, lowerVoxelInImage, unknown, lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown);
          Qout[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
        }
        if (y<numberOfVolumesPerAxisInPatch)
        {
          Qout[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
           //printf("loopB,%d,%d,%d,%d,%d,%d\n", x, y, upperVoxelInImage, lowerVoxelInImage, unknown, upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown);
          Qout[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
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


          for (int unknown=0; unknown<unknowns; unknown++)
          {
             if (x>0)
             {
                Qouttest[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
              //printf("loopC,%d,%d,%d,%d,%d,%d\n", x, y, leftVoxelInImage, rightVoxelInImage, unknown, leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown);
                Qouttest[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
             }
             if (x<numberOfVolumesPerAxisInPatch)
             {
                Qouttest[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
              //printf("loopC,%d,%d,%d,%d,%d,%d\n", x, y, leftVoxelInImage, rightVoxelInImage, unknown, rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown);
                Qouttest[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
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


           for (int unknown=0; unknown<unknowns; unknown++)
           {
              if (y>0)
              {
                 Qouttest[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
           //printf("loopD,%d,%d,%d,%d,%d,%d\n", x, y, upperVoxelInImage, lowerVoxelInImage, unknown, lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown);
                Qouttest[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
              }
              if (y<numberOfVolumesPerAxisInPatch)
              {
                 Qouttest[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
           //printf("loopD,%d,%d,%d,%d,%d,%d\n", x, y, upperVoxelInImage, lowerVoxelInImage, unknown, upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown);
                Qouttest[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
              }
           }
        }
     }
  }
  
  // Done with routine 2
  double * Qouttest2= new double[500];
  for (unsigned int i=0;i<500;++i) Qouttest2[i]=0.0;
      for (int shift = 0; shift < 2; shift++)
      {
        #ifdef SharedOMP
        #pragma omp parallel for collapse(2)
        #endif
        for (int x = shift; x <= numberOfVolumesPerAxisInPatch; x += 2)
        {
          for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
          {
            const int leftVoxelInImage     = x - 1 + y * numberOfVolumesPerAxisInPatch;
            const int rightVoxelInImage    = x     + y * numberOfVolumesPerAxisInPatch;

            for (int unknown = 0; unknown < unknowns; unknown++)
            {
              if (x > 0)
              {
                Qouttest2[leftVoxelInImage * (unknowns + auxiliaryVariables) + unknown]  -= dt / volumeH (0) * numericalFluxL[unknown];
                Qouttest2[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
              }
              if (x < numberOfVolumesPerAxisInPatch)
              {
                Qouttest2[rightVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * numericalFluxR[unknown];
                Qouttest2[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
              }
            }
          }
        }
      }

       //Iterate over other normal
      for (int shift = 0; shift < 2; shift++)
      {
        #ifdef SharedOMP
        #pragma omp parallel for collapse(2)
        #endif
        for (int y = shift; y <= numberOfVolumesPerAxisInPatch; y += 2)
        {
          for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
          {
            const int lowerVoxelInImage    = x      + (y - 1) *      numberOfVolumesPerAxisInPatch ;
            const int upperVoxelInImage    = x      +       y *      numberOfVolumesPerAxisInPatch ;

            for (int unknown = 0; unknown < unknowns; unknown++)
            {
              if (y > 0)
              {
                Qouttest2[lowerVoxelInImage * (unknowns + auxiliaryVariables) + unknown] -= dt / volumeH (0) * numericalFluxL[unknown];
                Qouttest2[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
              }
              if (y < numberOfVolumesPerAxisInPatch)
              {
                Qouttest2[upperVoxelInImage * (unknowns + auxiliaryVariables) + unknown] += dt / volumeH (0) * numericalFluxR[unknown];
                Qouttest2[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += Qcmp[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ];
              }
            }
          }
        }
      }


  // Elementwise comparison

  for (unsigned int i=0;i<500;++i)
  {
     //printf("%d: %f vs %f\n", i, Qout[i], Qouttest[i]);
     validateEquals(Qout[i],     Qouttest[i]);
     //printf("%f vs %f\n", Qouttest[i], Qouttest2[i]);
     validateEquals(Qouttest[i], Qouttest2[i]);
  }




  delete[] Qout;
  delete[] Qouttest;
  ::tarch::freeMemory(numericalFluxL, ::tarch::MemoryLocation::Heap);
  ::tarch::freeMemory(numericalFluxR, ::tarch::MemoryLocation::Heap);
}
