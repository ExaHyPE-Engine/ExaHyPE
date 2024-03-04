#include "CellIntegralTest.h"

#include <cmath>
#include <iostream>
#include <fstream>

#include "../DGUtils.h"
#include "../CellIntegral.h"
#include "../Riemann.h"
#include "../BoundaryConditions.h"
#include "../rusanov/Rusanov.h"
#include "TestUtils.h"
#include "exahype2/CellData.h"

/*
 * We use precomputed values for lagrange polynomials as basis function
 * on Gauss-Legendre nodes
 */
exahype2::dg::tests::CellIntegralTest::CellIntegralTest():
  TestCase ("exahype2::dg::tests::CellIntegralTest"),
  _Order4_QuadraturePoints1d { 0.04691007703066802,0.23076534494715845,0.5,0.7692346550528415,0.9530899229693319},
  _Order4_MassMatrixDiagonal1d { 0.11846344252809464,0.23931433524968335,0.2844444444444444,0.2393143352496833,0.11846344252809476},
  _Order4_StiffnessOperator1d {
    -1.2005181447816875017764459698810242116451263427734375,-0.459606063929972219472830374797922559082508087158203125,0.1713312197562774918946360003246809355914592742919921875,-0.11698479961911900648630080468137748539447784423828125,0.130728401276001104935886587554705329239368438720703125,
    1.824799516391880427335081549244932830333709716796875,-0.362969592101977378550969888237887062132358551025390625,-0.81657642230598626031934372804244048893451690673828125,0.4444344937740536405357261173776350915431976318359375,-0.464471255981286024194787387386895716190338134765625,
    -0.95802422631547734521717529787565581500530242919921875,1.15002535018688423207322557573206722736358642578125,-0.0000000000000003789561257387200741506709440429594952952620993784360603484628882,-1.150025350186884676162435425794683396816253662109375,0.95802422631547756726178022290696389973163604736328125,
    0.464471255981286079705938618644722737371921539306640625,-0.44443449377405397360263350492459721863269805908203125,0.81657642230598648236394865307374857366085052490234375,0.362969592101977489573272350753541104495525360107421875,-1.824799516391880427335081549244932830333709716796875,
    -0.13072840127600116044703781881253235042095184326171875,0.11698479961911904811966422812474775128066539764404296875,-0.1713312197562775196502116159535944461822509765625,0.459606063929972219472830374797922559082508087158203125,1.2005181447816877238210508949123322963714599609375
  },
  _Order4_DerivativeOperator1d {
    -10.1340811913090842466544927447102963924407958984375,15.4039041703444912201348415692336857318878173828125,-8.0870875087753280041624748264439404010772705078125,3.920798231666558830710300753707997500896453857421875,-1.1035337019266335811806811761925928294658660888671875,
    -1.9205120472639156670169313656515441834926605224609375,-1.516706434335750142139431773102842271327972412109375,4.80550130432859834428427348029799759387969970703125,-1.8571160532876695992143822877551428973674774169921875,0.488833230558736009374598552312818355858325958251953125,
    0.602336319455663016242397134192287921905517578125,-2.870776484669482986333832741365768015384674072265625,-0.000000000000001332267629550187848508358001708984375,2.870776484669483874512252441491000354290008544921875,-0.60233631945566312726469959670794196426868438720703125,
    -0.48883323055873584284114485853933729231357574462890625,1.8571160532876682669467527375672943890094757080078125,-4.80550130432860012064111288054846227169036865234375,1.5167064343357505862286416231654584407806396484375,1.9205120472639156670169313656515441834926605224609375,
    1.1035337019266331370914713261299766600131988525390625,-3.920798231666557942531881053582765161991119384765625,8.087087508775329780519314226694405078887939453125,-15.4039041703444912201348415692336857318878173828125,10.13408119130908602301133214496076107025146484375
  },
  _Order4_BasisFunctionValuesLeft1d { 1.5514080490943134,-0.8931583920000723,0.5333333333333335,-0.2679416522233877,0.07635866179581295},
  _Order2_QuadraturePoints1d{0.1127016653792583,0.5,0.8872983346207417},
  _Order2_MassMatrixDiagonal1d{0.2777777777777777,0.44444444444444436,0.27777777777777773},
  _Order2_StiffnessOperator1d{-1.0758287072798378,-0.5737753105492469,0.35860956909327923,1.4344382763731172,6.459277139373494e-17,-1.4344382763731172,-0.35860956909327923,0.5737753105492469,1.0758287072798378},
  _Order2_DerivativeOperator1d{-3.872983346207417,5.163977794943222,-1.2909944487358056,-1.2909944487358056,0.0,1.2909944487358056,1.2909944487358056,-5.163977794943222,3.872983346207417} {}

void exahype2::dg::tests::CellIntegralTest::run() {
  testMethod(runEulerOrder2OnStationarySetup);
  testMethod(runEulerOrder4OnStationarySetup);
}

void exahype2::dg::tests::CellIntegralTest::runEulerOrder2OnStationarySetup() {
  //using euler equations as example
  constexpr int unknowns = 5;
  constexpr int auxiliaryVariables = 0;
  constexpr int order = 2;

  #if Dimensions==2
  const tarch::la::Vector<2, double> x({0.0, 0.0});
  const tarch::la::Vector<2, double> cellSize({1.0, 1.0});
  constexpr int numberOfDoFs = (order+1)*(order+1);
  #else
  const tarch::la::Vector<3, double> x({0.0, 0.0, 0.0});
  const tarch::la::Vector<3, double> cellSize({1.0, 1.0, 1.0});
  constexpr int numberOfDoFs = (order+1)*(order+1)*(order+1);
  #endif
  //const double dx = 0.01;
  const double t  = 0.0;
  const double dt = 0.01;

  double QIn[numberOfDoFs * (unknowns+auxiliaryVariables)];
  double QOut[numberOfDoFs * (unknowns+auxiliaryVariables)];

  for (int dof=0; dof<numberOfDoFs; dof++) {
    QIn[dof*(unknowns+auxiliaryVariables)+0] = 0.1;
    QIn[dof*(unknowns+auxiliaryVariables)+1] = 0.0;
    QIn[dof*(unknowns+auxiliaryVariables)+2] = 0.0;
    QIn[dof*(unknowns+auxiliaryVariables)+3] = 0.0;
    QIn[dof*(unknowns+auxiliaryVariables)+4] = 0.0;
  }

  ::exahype2::CellData cellData(
      QIn,
      x,
      cellSize,
      t,
      dt,
      QOut
      );

  cellIntegral_patchwise_in_situ_GaussLegendre_functors(
    cellData,
    order,
    unknowns,
    auxiliaryVariables,
    [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
        ) -> void {
        eulerFlux(Q,x,t,dt,normal,F);
      },
    nullptr, //ncp
    nullptr, //source
    nullptr, //pointSources
    _Order2_QuadraturePoints1d,
    _Order2_MassMatrixDiagonal1d, //massMatrixDiagonal
    _Order2_StiffnessOperator1d,
    _Order2_DerivativeOperator1d,
    true,     //flux
    false,    //ncp
    false,    //source
    false);   //pointSources


  for (int dof=0; dof<numberOfDoFs; dof++) {
    validateNumericalEqualsWithParams2( QOut[dof*(unknowns+auxiliaryVariables)+0], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    validateNumericalEqualsWithParams2( QOut[dof*(unknowns+auxiliaryVariables)+1], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    validateNumericalEqualsWithParams2( QOut[dof*(unknowns+auxiliaryVariables)+2], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    validateNumericalEqualsWithParams2( QOut[dof*(unknowns+auxiliaryVariables)+3], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    validateNumericalEqualsWithParams2( QOut[dof*(unknowns+auxiliaryVariables)+4], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
  }

  for (int dof=0; dof<numberOfDoFs; dof++) {
    QIn[dof*(unknowns+auxiliaryVariables)+0] = 0.1;
    QIn[dof*(unknowns+auxiliaryVariables)+1] = 0.0;
    QIn[dof*(unknowns+auxiliaryVariables)+2] = 0.0;
    QIn[dof*(unknowns+auxiliaryVariables)+3] = 0.0;
    QIn[dof*(unknowns+auxiliaryVariables)+4] = 4.0;
  }

  cellIntegral_patchwise_in_situ_GaussLegendre_functors(
    cellData,
    order,
    unknowns,
    auxiliaryVariables,
    [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
        ) -> void {
        eulerFlux(Q,x,t,dt,normal,F);
      },
    nullptr, //ncp
    nullptr, //source
    nullptr, //pointSources
    _Order2_QuadraturePoints1d,
    _Order2_MassMatrixDiagonal1d, //massMatrixDiagonal
    _Order2_StiffnessOperator1d,
    _Order2_DerivativeOperator1d,
    true,     //flux
    false,    //ncp
    false,    //source
    false);   //pointSources


  dfor(dof,order+1) {
    int dofLinearised = peano4::utils::dLinearised(dof,order+1);
    validateNumericalEqualsWithParams2( QOut[dofLinearised*(unknowns+auxiliaryVariables)+0], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    if (dof(0)<(order+1)/2) {
      validateWithParams2( QOut[dofLinearised*(unknowns+auxiliaryVariables)+1]<0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    }
    else if (dof(0)>(order+1)/2) {
      validateWithParams2( QOut[dofLinearised*(unknowns+auxiliaryVariables)+1]>0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    }
    if (dof(1)<(order+1)/2) {
      validateWithParams2( QOut[dofLinearised*(unknowns+auxiliaryVariables)+2]<0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    }
    else if (dof(1)>(order+1)/2) {
      validateWithParams2( QOut[dofLinearised*(unknowns+auxiliaryVariables)+2]>0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    }
    validateNumericalEqualsWithParams2( QOut[dofLinearised*(unknowns+auxiliaryVariables)+4], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
  }
}


void exahype2::dg::tests::CellIntegralTest::runEulerOrder4OnStationarySetup() {
  //using euler equations as example
  constexpr int unknowns = 5;
  constexpr int auxiliaryVariables = 0;
  constexpr int order = 4;

  #if Dimensions==2
  const tarch::la::Vector<2, double> x({0.0, 0.0});
  const tarch::la::Vector<2, double> cellSize({1.0, 1.0});
  constexpr int numberOfDoFs = (order+1)*(order+1);
  #else
  const tarch::la::Vector<3, double> x({0.0, 0.0, 0.0});
  const tarch::la::Vector<3, double> cellSize({1.0, 1.0, 1.0});
  constexpr int numberOfDoFs = (order+1)*(order+1)*(order+1);
  #endif
  //const double dx = 0.01;
  const double t  = 0.0;
  const double dt = 0.01;

  double QIn[numberOfDoFs * (unknowns+auxiliaryVariables)];
  double QOut[numberOfDoFs * (unknowns+auxiliaryVariables)];

  for (int dof=0; dof<numberOfDoFs; dof++) {
    QIn[dof*(unknowns+auxiliaryVariables)+0] = 0.1;
    QIn[dof*(unknowns+auxiliaryVariables)+1] = 0.0;
    QIn[dof*(unknowns+auxiliaryVariables)+2] = 0.0;
    QIn[dof*(unknowns+auxiliaryVariables)+3] = 0.0;
    QIn[dof*(unknowns+auxiliaryVariables)+4] = 0.0;
  }

  ::exahype2::CellData cellData(
      QIn,
      x,
      cellSize,
      t,
      dt,
      QOut
      );

  cellIntegral_patchwise_in_situ_GaussLegendre_functors(
    cellData,
    order,
    unknowns,
    auxiliaryVariables,
    [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
        ) -> void {
        eulerFlux(Q,x,t,dt,normal,F);
      },
    nullptr, //ncp
    nullptr, //source
    nullptr, //pointSources
    _Order4_QuadraturePoints1d,
    _Order4_MassMatrixDiagonal1d, //massMatrixDiagonal
    _Order4_StiffnessOperator1d,
    _Order4_DerivativeOperator1d,
    true,     //flux
    false,    //ncp
    false,    //source
    false);   //pointSources


  for (int dof=0; dof<numberOfDoFs; dof++) {
    validateNumericalEqualsWithParams2( QOut[dof*(unknowns+auxiliaryVariables)+0], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    validateNumericalEqualsWithParams2( QOut[dof*(unknowns+auxiliaryVariables)+1], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    validateNumericalEqualsWithParams2( QOut[dof*(unknowns+auxiliaryVariables)+2], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    validateNumericalEqualsWithParams2( QOut[dof*(unknowns+auxiliaryVariables)+3], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
    validateNumericalEqualsWithParams2( QOut[dof*(unknowns+auxiliaryVariables)+4], 0.0, dof, plotCell( QOut, order, unknowns, auxiliaryVariables ) );
  }
}


/*
void exahype2::dg::tests::CellIntegralTest::runDGEuler(){
  validate(false);

  file_stream << "Starting Euler test in " << Dimensions << " dimensions\n\n";

  // geometry
#if Dimensions==2
  const tarch::la::Vector<2, double> x({0.0, 0.0});
  const tarch::la::Vector<2, double> cellSize({1.0, 1.0});
#else
  const tarch::la::Vector<3, double> x({0.0, 0.0, 0.0});
  const tarch::la::Vector<3, double> cellSize({1.0, 1.0, 1.0});
#endif
  const double dx = 0.01;
  const double t  = 0.0;
  const double dt = 0.01;

  //using euler equations as example
  const int unknowns = 5;
  const int auxiliaryVariables = 0;
  const int order = 4;

  //Parameter initialisation and memory allocation
  const int nodesPerAxis  = order+1;
  const int nodesPerCell  = getNodesPerCell(nodesPerAxis);
  const int nodesPerFace  = nodesPerCell/nodesPerAxis;
  const int strideQ       = unknowns+auxiliaryVariables;

  //values in the cell at the beginning and end
  double* QIn = new double[nodesPerCell*strideQ];
  double* QOut = new double[nodesPerCell*strideQ];
  //values on the faces
  double* QLeft = new double[2*nodesPerFace*strideQ];
  double* QRight = new double[2*nodesPerFace*strideQ];
  double* QDown = new double[2*nodesPerFace*strideQ];
  double* QUp = new double[2*nodesPerFace*strideQ];
#if Dimensions==3
  double* QFront = new double[2*nodesPerFace*strideQ];
  double* QBack = new double[2*nodesPerFace*strideQ];
#endif

  //values of the rusanov fluxes on the faces
  double* QHullForRiemann = new double[2*Dimensions*2*nodesPerFace*unknowns];


  ::exahype2::CellData cellData(
      QIn,
      x,
      cellSize,
      t,
      dt,
      QOut
      );


  //setting initial conditions

  file_stream << "Initial conditions: \n";
  for(int i=0; i<nodesPerCell; i++){
    eulerInitial(&QIn[i*strideQ]);
    file_stream << "node " << i << " [";
    for(int var=0; var<strideQ; var++){
      file_stream << QIn[i*strideQ+var] << ", ";
    }
    file_stream << "]\n";
  }

  //for later comparisons
  double QInit[strideQ];
  eulerInitial(QInit);

    cellIntegral_patchwise_in_situ_GaussLegendre_functors(
      cellData,
      order,
      unknowns,
      auxiliaryVariables,
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
          ) -> void {
          eulerFlux(Q,x,t,dt,normal,F);
        },
      nullptr, //ncp
      nullptr, //source
      nullptr, //pointSources
      QuadraturePoints1d,
      MassMatrixDiagonal1d, //massMatrixDiagonal
      StiffnessOperator1d,
      DerivativeOperator1d,
      true,     //flux
      false,    //ncp
      false,    //source
      false);   //pointSources


  file_stream << "\nComputed volume integral values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << QOut[i*unknowns+var] << ", ";
    }
    file_stream << "]\n";
  }
  file_stream << "\n";

  // Step 2), the values computed in step 1) are extrapolated to the surface boundary
  projectVolumetricDataOntoFaces(
    QIn,
    order,
    unknowns,
    auxiliaryVariables,
    BasisFunctionValuesLeft1d,
    QLeft,
    QRight,
    QDown,
    QUp
#if Dimensions==3
    ,QFront,
    QBack
#endif
  );

  double* F[2*Dimensions];

  F[0] = QLeft;
  F[1] = QRight;
  F[2] = QDown;
  F[3] = QUp;
#if Dimensions==3
  F[4] = QFront;
  F[5] = QBack;
#endif


  // *  project data onto opposite face as well to trade with "identical" cell,
  // *  this essentially simulates the exchange of surface data with the
  // *  neighbouring cells which Peano would take care of for us normally
  projectVolumetricDataOntoFaces(
    QIn,
    order,
    unknowns,
    auxiliaryVariables,
    BasisFunctionValuesLeft1d,
    QRight,
    QLeft,
    QUp,
    QDown
#if Dimensions==3
    ,QBack,
    QFront
#endif
  );


  for(int f=0; f<2*Dimensions; f++){

    double* currentFace = F[f];
    file_stream << "\nFace " << f << ": \n";

    for(int node=0; node<2*nodesPerFace; node++){
      file_stream << "node " << node << ": [";
      for(int var=0; var<strideQ; var++){
        file_stream << (std::abs(currentFace[node*strideQ+var])<0.00001 ? 0.0 : currentFace[node*strideQ+var]) << ", ";
      }
      file_stream << "]\n";
    }

  }


  // The faces are assembled into one pointer construct for ease of access in the
  // next steps. In addition information about whether the faces are at the boundary
  // is provided since at the boundary the flux must be computed with boundary values
  // instead of using the extrapolated solution from the neighbour.
  double* QHull[Dimensions*2];
#if Dimensions==2
  QHull[0] = QLeft;
  QHull[2] = QRight;
  QHull[1] = QDown;
  QHull[3] = QUp;
#elif Dimensions==3
  QHull[0] = QLeft;
  QHull[3] = QRight;
  QHull[1] = QDown;
  QHull[4] = QUp;
  QHull[2] = QBack;
  QHull[5] = QFront;
#endif


  for(int i=0; i<2*Dimensions*2*nodesPerFace*strideQ; i++){
    QHullForRiemann[i] = 0.0;
  }


  for(int face=0; face<2*Dimensions; face++){

    rusanov::solveRiemannProblem_pointwise_in_situ(
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
          ) -> void {
          eulerFlux(Q,x,t,dt,normal,F);
      },
      nullptr,
      [&](
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double                                      dt,
        int                                         normal
        ) -> double {
          return eulerEigenvalue(Q,x,t,dt,normal);
      },
      x,
      cellSize,
      t,
      dt,
      order,
      unknowns,
      auxiliaryVariables,
      face, //faceNumber
      QuadraturePoints1d,
      true, //useFlux
      false, //useNCP
      QHull[face], //TODO
      &QHullForRiemann[face*2*nodesPerFace*unknowns]
      );

  }


  file_stream << "\nRusanov Fluxes per Face:\n";
  for(int f=0; f<2*Dimensions; f++){
    file_stream << "At face " << f << "\n";
    for(int i=0; i<2*nodesPerFace; i++){
      file_stream << "at node " << i << ": [";
      for(int var=0; var<unknowns; var++){
        file_stream << (std::abs(QHullForRiemann[(f*2*nodesPerFace+i)*unknowns+var])<0.000001 ? 0.0 : QHullForRiemann[(f*2*nodesPerFace+i)*unknowns+var]) << ", ";
      }
      file_stream << "]\n";
    }
  }


  //  The contributions from the computed rusanov flux are then added to the nodes
  //  within the cell.
  integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
#if Dimensions==2
    &QHullForRiemann[(0*2+0)*nodesPerFace*unknowns], //left
    &QHullForRiemann[(2*2+0)*nodesPerFace*unknowns], //right
    &QHullForRiemann[(1*2+0)*nodesPerFace*unknowns], //down
    &QHullForRiemann[(3*2+0)*nodesPerFace*unknowns], //up
#else
    &QHullForRiemann[(0*2+0)*nodesPerFace*unknowns], //left
    &QHullForRiemann[(3*2+0)*nodesPerFace*unknowns], //right
    &QHullForRiemann[(1*2+0)*nodesPerFace*unknowns], //down
    &QHullForRiemann[(4*2+0)*nodesPerFace*unknowns], //up
    &QHullForRiemann[(2*2+0)*nodesPerFace*unknowns], //front
    &QHullForRiemann[(5*2+0)*nodesPerFace*unknowns], //back
#endif
    order,
    unknowns,
    auxiliaryVariables,
    cellSize,
    BasisFunctionValuesLeft1d,
    MassMatrixDiagonal1d,
    QOut
  );



  file_stream << "\nValues after Riemann Integral are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << (std::abs(QOut[i*unknowns+var]) <0.0000001 ? 0.0 : QOut[i*unknowns+var]) << ", ";
    }
    file_stream << "]\n";
  }


  multiplyWithInvertedMassMatrix_GaussLegendre(
    cellData,
    order,
    unknowns,
    auxiliaryVariables,
    MassMatrixDiagonal1d
  );

  //Finally we check that after all these steps have been taken QOut, which should be the sum of the volume and face contributions, is zero

  file_stream << "\nFinal values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << (std::abs(QOut[i*unknowns+var]) <0.0000001 ? 0.0 : QOut[i*unknowns+var]) << ", ";
//      validateNumericalEquals(QOut[i*strideQ+var], 0.0);
    }
    file_stream << "]\n";
  }


  delete[] QIn;
  delete[] QOut;
  delete[] QLeft;
  delete[] QRight;
  delete[] QDown;
  delete[] QUp;
#if Dimensions==3
  delete[] QFront;
  delete[] QBack;
#endif
  delete[] QHullForRiemann;

}//runDGEuler



void exahype2::dg::tests::CellIntegralTest::runDGElastic(){


  file_stream << "\n\nStarting Elastic test in " << Dimensions << " dimensions\n\n";

  // geometry
#if Dimensions==2
  const tarch::la::Vector<2, double> x({0.0, 0.0});
  const tarch::la::Vector<2, double> cellSize({1.0, 1.0});
#else
  const tarch::la::Vector<3, double> x({0.0, 0.0, 0.0});
  const tarch::la::Vector<3, double> cellSize({1.0, 1.0, 1.0});
#endif
  const double dx = 0.01;
  const double t  = 0.0;
  const double dt = 0.01;

  //using euler equations as example
  const int unknowns = 9;
  const int auxiliaryVariables = 3;
  const int order = 4;

  //Parameter initialisation and memory allocation
  const int nodesPerAxis  = order+1;
  const int nodesPerCell  = getNodesPerCell(nodesPerAxis);
  const int nodesPerFace  = nodesPerCell/nodesPerAxis;
  const int strideQ       = unknowns+auxiliaryVariables;

  //values in the cell at the beginning and end
  double* QIn = new double[nodesPerCell*strideQ];
  double* QOut = new double[nodesPerCell*unknowns];
  //values on the faces
  double* QLeft = new double[2*nodesPerFace*strideQ];
  double* QRight = new double[2*nodesPerFace*strideQ];
  double* QDown = new double[2*nodesPerFace*strideQ];
  double* QUp = new double[2*nodesPerFace*strideQ];
#if Dimensions==3
  double* QFront = new double[2*nodesPerFace*strideQ];
  double* QBack = new double[2*nodesPerFace*strideQ];
#endif

  //values of the rusanov fluxes on the faces
  double* QHullForRiemann = new double[2*Dimensions*2*nodesPerFace*strideQ];


  ::exahype2::CellData cellData(
      QIn,
      x,
      cellSize,
      t,
      dt,
      QOut
      );


  //setting initial conditions

  file_stream << "Initial conditions: \n";
  for(int i=0; i<nodesPerCell; i++){
    elasticInitial(&QIn[i*strideQ]);
    file_stream << "node " << i << " [";
    for(int var=0; var<strideQ; var++){
      file_stream << QIn[i*strideQ+var] << ", ";
    }
    file_stream << "]\n";
  }

  //for later comparisons
  double QInit[strideQ];
  elasticInitial(QInit);

  cellIntegral_patchwise_in_situ_GaussLegendre_functors(
    cellData,
    order,
    unknowns,
    auxiliaryVariables,
    [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
        ) -> void {
        elasticFlux(Q,x,t,dt,normal,F);
    },
    [&](
        const double * __restrict__                  Q,
        const double * __restrict__                  dQdx,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        BgradQ
      ) -> void{
      elasticNonConservativeProduct(Q, dQdx, x, t, dt, normal, BgradQ);
    },
    nullptr, //source
    nullptr, //pointSources
    QuadraturePoints1d,
    MassMatrixDiagonal1d, //massMatrixDiagonal
    StiffnessOperator1d,
    DerivativeOperator1d,
    true,     //flux
    true,    //ncp
    false,    //source
    false);   //pointSources

  file_stream << "\nComputed volume integral values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << (std::abs(QOut[i*unknowns+var]) <0.0000001 ? 0.0 : QOut[i*unknowns+var]) << ", ";
    }
    file_stream << "]\n";
  }
  file_stream << "\n";

  // Step 2), the values computed in step 1) are extrapolated to the surface boundary
  projectVolumetricDataOntoFaces(
    QIn,
    order,
    unknowns,
    auxiliaryVariables,
    BasisFunctionValuesLeft1d,
    QLeft,
    QRight,
    QDown,
    QUp
#if Dimensions==3
    ,QFront,
    QBack
#endif
  );

  double* F[2*Dimensions];

  F[0] = QLeft;
  F[1] = QRight;
  F[2] = QDown;
  F[3] = QUp;
#if Dimensions==3
  F[4] = QFront;
  F[5] = QBack;
#endif


  //   project data onto opposite face as well to trade with "identical" cell,
  //   this essentially simulates the exchange of surface data with the
  //   neighbouring cells which Peano would take care of for us normally
  projectVolumetricDataOntoFaces(
    QIn,
    order,
    unknowns,
    auxiliaryVariables,
    BasisFunctionValuesLeft1d,
    QRight,
    QLeft,
    QUp,
    QDown
#if Dimensions==3
    ,QBack,
    QFront
#endif
  );


  for(int f=0; f<2*Dimensions; f++){

    double* currentFace = F[f];
    file_stream << "\nFace " << f << ": \n";

    for(int node=0; node<2*nodesPerFace; node++){
      file_stream << "node " << node << ": [";
      for(int var=0; var<strideQ; var++){
        file_stream << (std::abs(currentFace[node*strideQ+var])<0.00001 ? 0.0 : currentFace[node*strideQ+var]) << ", ";
      }
      file_stream << "]\n";
    }

  }


  // The faces are assembled into one pointer construct for ease of access in the
  // next steps. In addition information about whether the faces are at the boundary
  // is provided since at the boundary the flux must be computed with boundary values
  // instead of using the extrapolated solution from the neighbour.
  double* QHull[Dimensions*2];
#if Dimensions==2
  QHull[0] = QLeft;
  QHull[2] = QRight;
  QHull[1] = QDown;
  QHull[3] = QUp;
#elif Dimensions==3
  QHull[0] = QLeft;
  QHull[3] = QRight;
  QHull[1] = QDown;
  QHull[4] = QUp;
  QHull[2] = QBack;
  QHull[5] = QFront;
#endif


  for(int i=0; i<2*Dimensions*2*nodesPerFace*unknowns; i++){
    QHullForRiemann[i] = 0.0;
  }


  for(int face=0; face<2*Dimensions; face++){

    rusanov::solveRiemannProblem_pointwise_in_situ(
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
          ) -> void {
          elasticFlux(Q,x,t,dt,normal,F);
      },
      [&](
          const double * __restrict__                  Q,
          const double * __restrict__                  dQdx,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        BgradQ
        ) -> void{
        elasticNonConservativeProduct(Q, dQdx, x, t, dt, normal, BgradQ);
      },
      [&](
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double                                      dt,
        int                                         normal
        ) -> double {
          return elasticEigenvalue(Q,x,t,dt,normal);
      },
      x,
      cellSize,
      t,
      dt,
      order,
      unknowns,
      auxiliaryVariables,
      face, //faceNumber
      QuadraturePoints1d,
      true, //useFlux
      true, //useNCP
      QHull[face], //TODO
      &QHullForRiemann[face*2*nodesPerFace*unknowns]
      );

  }


  file_stream << "\nRusanov Fluxes per Face:\n";
  for(int f=0; f<2*Dimensions; f++){
    file_stream << "At face " << f << "\n";
    for(int i=0; i<2*nodesPerFace; i++){
      file_stream << "at node " << i << ": [";
      for(int var=0; var<unknowns; var++){
        file_stream << (std::abs(QHullForRiemann[(f*2*nodesPerFace+i)*unknowns+var])<0.000001 ? 0.0 : QHullForRiemann[(f*2*nodesPerFace+i)*unknowns+var]) << ", ";
      }
      file_stream << "]\n";
    }
  }


  //  The contributions from the computed rusanov flux are then added to the nodes
  //  within the cell.
  integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
#if Dimensions==2
    &QHullForRiemann[(0*2+0)*nodesPerFace*unknowns], //left
    &QHullForRiemann[(2*2+0)*nodesPerFace*unknowns], //right
    &QHullForRiemann[(1*2+0)*nodesPerFace*unknowns], //down
    &QHullForRiemann[(3*2+0)*nodesPerFace*unknowns], //up
#else
    &QHullForRiemann[(0*2+0)*nodesPerFace*unknowns], //left
    &QHullForRiemann[(3*2+0)*nodesPerFace*unknowns], //right
    &QHullForRiemann[(1*2+0)*nodesPerFace*unknowns], //down
    &QHullForRiemann[(4*2+0)*nodesPerFace*unknowns], //up
    &QHullForRiemann[(2*2+0)*nodesPerFace*unknowns], //front
    &QHullForRiemann[(5*2+0)*nodesPerFace*unknowns], //back
#endif
    order,
    unknowns,
    auxiliaryVariables,
    cellSize,
    BasisFunctionValuesLeft1d,
    MassMatrixDiagonal1d,
    QOut
  );



  file_stream << "\nValues after Riemann Integral are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << (std::abs(QOut[i*unknowns+var]) <0.0000001 ? 0.0 : QOut[i*unknowns+var]) << ", ";
    }
    file_stream << "]\n";
  }


  multiplyWithInvertedMassMatrix_GaussLegendre(
    cellData,
    order,
    unknowns,
    auxiliaryVariables,
    MassMatrixDiagonal1d
  );

  //Finally we check that after all these steps have been taken QOut, which should be the sum of the volume and face contributions, is zero

  file_stream << "\nFinal values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << (std::abs(QOut[i*unknowns+var]) <0.0000001 ? 0.0 : QOut[i*unknowns+var]) << ", ";
//      validateNumericalEquals(QOut[i*strideQ+var], 0.0);
    }
    file_stream << "]\n";
  }


  delete[] QIn;
  delete[] QOut;
  delete[] QLeft;
  delete[] QRight;
  delete[] QDown;
  delete[] QUp;
#if Dimensions==3
  delete[] QFront;
  delete[] QBack;
#endif

  delete[] QHullForRiemann;


} //runDGElastic
*/

