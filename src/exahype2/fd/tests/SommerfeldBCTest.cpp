#include "SommerfeldBCTest.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"
#include "tarch/la/Vector.h"

#include "tarch/logging/Log.h"

#include "exahype2/fd/PatchUtils.h"
#include "exahype2/enumerator/FaceAoSLexicographicEnumerator.h"
#include "exahype2/fd/BoundaryConditions.h"

#include <algorithm>
#include <iomanip>


exahype2::fd::tests::SommerfeldBCTest::SommerfeldBCTest():
  TestCase ("exahype2::fd::tests::SommerfeldBCTest") {
}


void exahype2::fd::tests::SommerfeldBCTest::run() {
  testMethod (flatsolutiontest);
}


void exahype2::fd::tests::SommerfeldBCTest::flatsolutiontest() {

  //we consider facenumber 3, i.e. face on the x positive, patch size 2, evolving quantity 2. Thus the array has 2x2x2x6=48 entries 
  double Qold[] = {
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,
  };

  double Qnew[] = {
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
  };


  ::exahype2::fd::applySommerfeldConditions(
    [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal
    ) -> double {
        return 2.0;
    },
    [&](
        double * __restrict__                        Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  gridCellH
    ) -> void {
      Q[0] = 1.0;
      Q[1] = 2.0;
    },
    {1.0,0.0,0.0},
    {0.05,0.05,0.05},
    1.0,
    0.01,
    2,
    3,
    2,
    0,
    3, //facenumber
    {0.0,0.0,0.0},
    Qold,
    Qnew
  );

/*
  std::cout<<std::setprecision(1);
    for (int i=0;i<4;i++){
      for (int j=0;j<12;j++){
        std::cout<<Qnew[i*12+j] << " ";
      }
      std::cout<<"\n";
  }
*/



  //validateNumericalEquals( 1.0,  2.0 );//
}

