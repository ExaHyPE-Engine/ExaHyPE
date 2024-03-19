#include "CCZ4KernelTest.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"
#include "tarch/la/Vector.h"

#include "tarch/logging/Log.h"

#include "exahype2/fd/PatchUtils.h"
//#include "exahype2/CellData.h"
//#include "exahype2/fd/Functors.h"
#include "exahype2/fd/fd4/FD4.h"
#include "../applications/exahype2/ccz4/CCZ4Kernels.h"
#include "../applications/exahype2/ccz4/CCZ4Kernels.cpph"

#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "exahype2/enumerator/SoALexicographicEnumerator.h"
#include "exahype2/enumerator/AoSoALexicographicEnumerator.h"

#include <algorithm>
#include <iomanip>


exahype2::fd::tests::CCZ4KernelTest::CCZ4KernelTest():
  TestCase ("exahype2::fd::tests::CCZ4KernelTest") {
}


void exahype2::fd::tests::CCZ4KernelTest::run() {
  testMethod (AppleWithAppleTest);
}


void exahype2::fd::tests::CCZ4KernelTest::AppleWithAppleTest() {
  //in this test, the out layers inside the patch is not valid as the volumes 
  // in the halos can not update their first-derivatives. So only volumes with index between [3,26]^3
  // holds valid test data.

  // make no sense to test in 2d
  #if Dimensions==3
  constexpr int unknowns = 59;
  constexpr int auxiliaryVariables = 0;
  constexpr int numberOfDoFsPerAxisInCell = 30;
  constexpr int haloSize = 3;
  constexpr int numberOfDoFsPerAxisInCellWithHalos= 30+2*haloSize;

  const tarch::la::Vector<3, double> x({0.5, 0.5, 0.5});
  const tarch::la::Vector<3, double> cellSize({30.0/29.0, 30.0/29.0, 30.0/29.0});
 
  double oldQWithHalos[numberOfDoFsPerAxisInCellWithHalos*numberOfDoFsPerAxisInCellWithHalos*numberOfDoFsPerAxisInCellWithHalos*(unknowns+auxiliaryVariables)];
  double QOut[numberOfDoFsPerAxisInCell*numberOfDoFsPerAxisInCell*numberOfDoFsPerAxisInCell*(unknowns)];

  //random assignment here, keep the same with GRChombo.

  ::exahype2::CellData patchData(oldQWithHalos, x, cellSize, 0, 0.01, QOut);

  //set parameters
  static constexpr double CCZ4KOSigma=8.0;
  static constexpr double CCZ4itau = 1.0;
  static constexpr double CCZ4k1 = 0.1;
  static constexpr double CCZ4k2 = 0.0;
  static constexpr double CCZ4k3 = 0.5;
  static constexpr double CCZ4eta = 1.0;
  static constexpr double CCZ4f = 0.75;
  static constexpr double CCZ4g = 0.0;
  static constexpr double CCZ4xi = 1.0;
  static constexpr double CCZ4e = 1.0;
  static constexpr double CCZ4c = 1.0;
  static constexpr double CCZ4mu = 0.2;
  static constexpr double CCZ4ds = 1.0;
  static constexpr double CCZ4sk = 1.0;
  static constexpr double CCZ4bs = 1.0;
  static constexpr int CCZ4BBHType = 2;
  static constexpr int CCZ4LapseType = 1;
  static constexpr int CCZ4SO = 1;

  //then calculate the derivatives first
  //there is a potential seg error if number of auxiliary variables is not zero.
  //but without auxiliary variables everything should be fine. 
  ::exahype2::fd::fd4::reconstruct_first_derivatives(
    patchData,
    numberOfDoFsPerAxisInCell,
    haloSize,
    unknowns,
    auxiliaryVariables
  );

  //call the kernel
  ::exahype2::fd::fd4::timeStep_patchwise_heap_functors(
    patchData,
    numberOfDoFsPerAxisInCell,
    haloSize,
    unknowns,
    auxiliaryVariables,
    CCZ4KOSigma,
     false ,
     true ,
     true ,
    false, // do not copy the old solution. Abuse this to get the RHS directly
    ::exahype2::fd::fd4::DifferentialSourceTermVariant::CentralDifferencesWithLopsidedAdvection,
    [&](
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ F
    )->void {
      
    },
    [&](
      const double * __restrict__                  Q,
      const double * __restrict__     deltaQ,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ BTimesDeltaQ
    )->void {   
      double gradQSerialised[unknowns*3];
      for (int i=0; i<unknowns; i++) {
        gradQSerialised[i+0*unknowns] = 0.0;
        gradQSerialised[i+1*unknowns] = 0.0;
        gradQSerialised[i+2*unknowns] = 0.0;

        gradQSerialised[i+normal*unknowns] = deltaQ[i];
      }
      //for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
      //  BgradQ[i] = 0.0;
      //}
      applications::exahype2::ccz4::ncp(BTimesDeltaQ, Q, gradQSerialised, normal, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu, CCZ4SO);     
    },
    [&](
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeX,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    )->void {
      tarch::memset(S, 0, unknowns*sizeof(double));
      applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3, CCZ4SO);   
    }
  );

  //examine the output, notice voulme index should between [3,26]^3
  tarch::la::Vector<3, int> volumeIndex({15,15,15});
  tarch::la::Vector<3, double> volumeCoordinates({0,0,0});
  int checkingQ=0;
  exahype2::enumerator::AoSLexicographicEnumerator QOutEnumerator(1,numberOfDoFsPerAxisInCell,0,unknowns,0); //halosize and auxiliary variables are both 0,
  for (int i=0;i<3;i++){
    volumeCoordinates(i)  = patchData.cellCentre[0](i)-0.5*patchData.cellSize[0](i);
    volumeCoordinates(i) += (double(volumeIndex(i))+0.5)*patchData.cellSize[0](i)/QOutEnumerator._numberOfDoFsPerAxisInCell;
  }
  
  validateNumericalEqualsWithParams3(0, 1, 
    patchData.toString(), 
    patchData.QOut[0][QOutEnumerator(0, volumeIndex, checkingQ)],
    volumeCoordinates
  );

  #endif
}

