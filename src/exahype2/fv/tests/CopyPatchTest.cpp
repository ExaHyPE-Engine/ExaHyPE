#include "peano4/utils/Loop.h"

// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CopyPatchTest.h"

exahype2::fv::tests::CopyPatchTest::CopyPatchTest():
  TestCase ("exahype2::fv::tests::CopyPatchTest") {
}

void exahype2::fv::tests::CopyPatchTest::run() {
  testMethod (testMe);
}

void exahype2::fv::tests::CopyPatchTest::testMe() {
  int    unknowns(5);
  int    auxiliaryVariables(2);
  int    numberOfVolumesPerAxisInPatch(5);
  int    haloSize(1);
  double*  QinWithHalo;
  double* QOutWithoutHalo;
  dfor(k,numberOfVolumesPerAxisInPatch) {
    tarch::la::Vector<Dimensions,int>   source = k + tarch::la::Vector<Dimensions,int>(haloSize);
    int sourceSerialised      = peano4::utils::dLinearised(source,numberOfVolumesPerAxisInPatch+haloSize*2);
    int destinationSerialised = peano4::utils::dLinearised(k,numberOfVolumesPerAxisInPatch);
    std::cerr << k << " " <<  sourceSerialised << " " <<  destinationSerialised << " \n";
    for (int i=0; i<unknowns+auxiliaryVariables; i++) {
      //QOutWithoutHalo[destinationSerialised*(unknowns+auxiliaryVariables)+i] = QinWithHalo[sourceSerialised*(unknowns+auxiliaryVariables)+i];
    }
  }
 
  std::cerr << "Now the alternative implementation without dfor\n"; 
  int sourceSerialised      = numberOfVolumesPerAxisInPatch+haloSize*2+haloSize;
  int destinationSerialised = 0;
  for (int y=0; y<numberOfVolumesPerAxisInPatch; y++) {
    for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
       std::cerr << "[" << x <<","<< y << "] "<< sourceSerialised << " " <<  destinationSerialised << "\n";
      for (int i=0; i<unknowns+auxiliaryVariables; i++) {
        //QOutWithoutHalo[destinationSerialised*(unknowns+auxiliaryVariables)+i] = QinWithHalo[sourceSerialised*(unknowns+auxiliaryVariables)+i];
      }
      sourceSerialised++;
      destinationSerialised++;
    }
    sourceSerialised += 2*haloSize;
  }

}
