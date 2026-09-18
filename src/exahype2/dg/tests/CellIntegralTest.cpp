// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellIntegralTest.h"

#include "exahype2/CellData.h"
#include "exahype2/dg/BoundaryConditions.h"
#include "exahype2/dg/CellIntegral.h"
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/Riemann.h"
#include "exahype2/dg/rusanov/Rusanov.h"
#include "exahype2/dg/tests/TestUtils.h"
#include "peano4/Loop.h"

exahype2::dg::tests::CellIntegralTest::CellIntegralTest() :
  TestCase("exahype2::dg::tests::CellIntegralTest"),
  _orderFourQuadraturePoints1D{0.04691007703066802, 0.23076534494715845, 0.5, 0.7692346550528415, 0.9530899229693319},
  _orderFourMassMatrixDiagonal1D{
    0.11846344252809464,
    0.23931433524968335,
    0.2844444444444444,
    0.2393143352496833,
    0.11846344252809476
  },
  _orderFourStiffnessOperator1D{
    -1.2005181447816875017764459698810242116451263427734375,
    -0.459606063929972219472830374797922559082508087158203125,
    0.1713312197562774918946360003246809355914592742919921875,
    -0.11698479961911900648630080468137748539447784423828125,
    0.130728401276001104935886587554705329239368438720703125,
    1.824799516391880427335081549244932830333709716796875,
    -0.362969592101977378550969888237887062132358551025390625,
    -0.81657642230598626031934372804244048893451690673828125,
    0.4444344937740536405357261173776350915431976318359375,
    -0.464471255981286024194787387386895716190338134765625,
    -0.95802422631547734521717529787565581500530242919921875,
    1.15002535018688423207322557573206722736358642578125,
    -0.0000000000000003789561257387200741506709440429594952952620993784360603484628882,
    -1.150025350186884676162435425794683396816253662109375,
    0.95802422631547756726178022290696389973163604736328125,
    0.464471255981286079705938618644722737371921539306640625,
    -0.44443449377405397360263350492459721863269805908203125,
    0.81657642230598648236394865307374857366085052490234375,
    0.362969592101977489573272350753541104495525360107421875,
    -1.824799516391880427335081549244932830333709716796875,
    -0.13072840127600116044703781881253235042095184326171875,
    0.11698479961911904811966422812474775128066539764404296875,
    -0.1713312197562775196502116159535944461822509765625,
    0.459606063929972219472830374797922559082508087158203125,
    1.2005181447816877238210508949123322963714599609375
  },
  _orderFourDerivativeOperator1D{
    -10.1340811913090842466544927447102963924407958984375,    15.4039041703444912201348415692336857318878173828125,
    -8.0870875087753280041624748264439404010772705078125,     3.920798231666558830710300753707997500896453857421875,
    -1.1035337019266335811806811761925928294658660888671875,  -1.9205120472639156670169313656515441834926605224609375,
    -1.516706434335750142139431773102842271327972412109375,   4.80550130432859834428427348029799759387969970703125,
    -1.8571160532876695992143822877551428973674774169921875,  0.488833230558736009374598552312818355858325958251953125,
    0.602336319455663016242397134192287921905517578125,       -2.870776484669482986333832741365768015384674072265625,
    -0.000000000000001332267629550187848508358001708984375,   2.870776484669483874512252441491000354290008544921875,
    -0.60233631945566312726469959670794196426868438720703125, -0.48883323055873584284114485853933729231357574462890625,
    1.8571160532876682669467527375672943890094757080078125,   -4.80550130432860012064111288054846227169036865234375,
    1.5167064343357505862286416231654584407806396484375,      1.9205120472639156670169313656515441834926605224609375,
    1.1035337019266331370914713261299766600131988525390625,   -3.920798231666557942531881053582765161991119384765625,
    8.087087508775329780519314226694405078887939453125,       -15.4039041703444912201348415692336857318878173828125,
    10.13408119130908602301133214496076107025146484375
  },
  _orderTwoQuadraturePoints1D{0.1127016653792583, 0.5, 0.8872983346207417},
  _orderTwoMassMatrixDiagonal1D{0.2777777777777777, 0.44444444444444436, 0.27777777777777773},
  _orderTwoStiffnessOperator1D{
    -1.0758287072798378,
    -0.5737753105492469,
    0.35860956909327923,
    1.4344382763731172,
    6.459277139373494e-17,
    -1.4344382763731172,
    -0.35860956909327923,
    0.5737753105492469,
    1.0758287072798378
  },
  _orderTwoDerivativeOperator1D{
    -3.872983346207417,
    5.163977794943222,
    -1.2909944487358056,
    -1.2909944487358056,
    0.0,
    1.2909944487358056,
    1.2909944487358056,
    -5.163977794943222,
    3.872983346207417
  } {}

void exahype2::dg::tests::CellIntegralTest::run() {
  testMethod(runEulerOrder2OnStationarySetup);
  testMethod(runEulerOrder4OnStationarySetup);
  testMethod(runReduceMaxScaledEigenvalue);
}

/**
 * Verifies two order-two Euler volume residuals.
 */
void exahype2::dg::tests::CellIntegralTest::runEulerOrder2OnStationarySetup() {
  constexpr int Unknowns           = 5;
  constexpr int AuxiliaryVariables = 0;
  constexpr int Order              = 2;

#if DIMENSIONS == 2
  const tarch::la::Vector<2, double> x({0.0, 0.0});
  const tarch::la::Vector<2, double> cellSize({1.0, 1.0});
  constexpr int                      NumberOfDoFs = (Order + 1) * (Order + 1);
#else
  const tarch::la::Vector<3, double> x({0.0, 0.0, 0.0});
  const tarch::la::Vector<3, double> cellSize({1.0, 1.0, 1.0});
  constexpr int                      NumberOfDoFs = (Order + 1) * (Order + 1) * (Order + 1);
#endif
  const double t  = 0.0;
  const double dt = 0.01;

  double QIn[NumberOfDoFs * (Unknowns + AuxiliaryVariables)];
  double QOut[NumberOfDoFs * (Unknowns + AuxiliaryVariables)];

  for (int dof = 0; dof < NumberOfDoFs; dof++) {
    QIn[dof * (Unknowns + AuxiliaryVariables) + 0] = 0.1;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 1] = 0.0;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 2] = 0.0;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 3] = 0.0;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 4] = 0.0;
  }

  ::exahype2::CellData cellData(QIn, x, cellSize, t, dt, QOut);

  cellIntegralPatchwiseInSituGaussLegendreFunctors(
    cellData,
    Order,
    Unknowns,
    AuxiliaryVariables,
    [&](
      const double* NOALIAS                        Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* NOALIAS                              F
    ) -> void { eulerFlux(Q, x, t, dt, normal, F); },
    nullptr, // ncp
    nullptr, // source
    nullptr, // pointSources
    _orderTwoQuadraturePoints1D,
    _orderTwoMassMatrixDiagonal1D,
    _orderTwoStiffnessOperator1D,
    _orderTwoDerivativeOperator1D,
    true,  // flux
    false, // ncp
    false, // source
    false  // pointSources
  );

  for (int dof = 0; dof < NumberOfDoFs; dof++) {
    validateNumericalEqualsWithParams2(
      QOut[dof * (Unknowns + AuxiliaryVariables) + 0],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
    validateNumericalEqualsWithParams2(
      QOut[dof * (Unknowns + AuxiliaryVariables) + 1],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
    validateNumericalEqualsWithParams2(
      QOut[dof * (Unknowns + AuxiliaryVariables) + 2],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
    validateNumericalEqualsWithParams2(
      QOut[dof * (Unknowns + AuxiliaryVariables) + 3],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
    validateNumericalEqualsWithParams2(
      QOut[dof * (Unknowns + AuxiliaryVariables) + 4],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
  }

  for (int dof = 0; dof < NumberOfDoFs; dof++) {
    QIn[dof * (Unknowns + AuxiliaryVariables) + 0] = 0.1;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 1] = 0.0;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 2] = 0.0;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 3] = 0.0;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 4] = 4.0;
  }

  cellIntegralPatchwiseInSituGaussLegendreFunctors(
    cellData,
    Order,
    Unknowns,
    AuxiliaryVariables,
    [&](
      const double* NOALIAS                        Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* NOALIAS                              F
    ) -> void { eulerFlux(Q, x, t, dt, normal, F); },
    nullptr, // ncp
    nullptr, // source
    nullptr, // pointSources
    _orderTwoQuadraturePoints1D,
    _orderTwoMassMatrixDiagonal1D,
    _orderTwoStiffnessOperator1D,
    _orderTwoDerivativeOperator1D,
    true,  // flux
    false, // ncp
    false, // source
    false  // pointSources
  );

  peano4::dForEach(Order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
    const auto dofLinearised = peano4::dLinearised(dof, Order + 1);
    validateNumericalEqualsWithParams2(
      QOut[dofLinearised * (Unknowns + AuxiliaryVariables) + 0],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
    if (dof(0) < (Order + 1) / 2) {
      validateWithParams2(
        QOut[dofLinearised * (Unknowns + AuxiliaryVariables) + 1] < 0.0,
        dof,
        plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
      );
    } else if (dof(0) > (Order + 1) / 2) {
      validateWithParams2(
        QOut[dofLinearised * (Unknowns + AuxiliaryVariables) + 1] > 0.0,
        dof,
        plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
      );
    }
    if (dof(1) < (Order + 1) / 2) {
      validateWithParams2(
        QOut[dofLinearised * (Unknowns + AuxiliaryVariables) + 2] < 0.0,
        dof,
        plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
      );
    } else if (dof(1) > (Order + 1) / 2) {
      validateWithParams2(
        QOut[dofLinearised * (Unknowns + AuxiliaryVariables) + 2] > 0.0,
        dof,
        plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
      );
    }
    validateNumericalEqualsWithParams2(
      QOut[dofLinearised * (Unknowns + AuxiliaryVariables) + 4],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
  });
}

/** Verifies that the order-four kernel preserves a constant zero-pressure Euler state. */
void exahype2::dg::tests::CellIntegralTest::runEulerOrder4OnStationarySetup() {
  constexpr int Unknowns           = 5;
  constexpr int AuxiliaryVariables = 0;
  constexpr int Order              = 4;

#if DIMENSIONS == 2
  const tarch::la::Vector<2, double> x({0.0, 0.0});
  const tarch::la::Vector<2, double> cellSize({1.0, 1.0});
  constexpr int                      NumberOfDoFs = (Order + 1) * (Order + 1);
#else
  const tarch::la::Vector<3, double> x({0.0, 0.0, 0.0});
  const tarch::la::Vector<3, double> cellSize({1.0, 1.0, 1.0});
  constexpr int                      NumberOfDoFs = (Order + 1) * (Order + 1) * (Order + 1);
#endif
  const double t  = 0.0;
  const double dt = 0.01;

  double QIn[NumberOfDoFs * (Unknowns + AuxiliaryVariables)];
  double QOut[NumberOfDoFs * (Unknowns + AuxiliaryVariables)];

  for (int dof = 0; dof < NumberOfDoFs; dof++) {
    QIn[dof * (Unknowns + AuxiliaryVariables) + 0] = 0.1;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 1] = 0.0;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 2] = 0.0;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 3] = 0.0;
    QIn[dof * (Unknowns + AuxiliaryVariables) + 4] = 0.0;
  }

  ::exahype2::CellData cellData(QIn, x, cellSize, t, dt, QOut);

  cellIntegralPatchwiseInSituGaussLegendreFunctors(
    cellData,
    Order,
    Unknowns,
    AuxiliaryVariables,
    [&](
      const double* NOALIAS                        Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* NOALIAS                              F
    ) -> void { eulerFlux(Q, x, t, dt, normal, F); },
    nullptr, // ncp
    nullptr, // source
    nullptr, // pointSources
    _orderFourQuadraturePoints1D,
    _orderFourMassMatrixDiagonal1D,
    _orderFourStiffnessOperator1D,
    _orderFourDerivativeOperator1D,
    true,  // flux
    false, // ncp
    false, // source
    false  // pointSources
  );

  for (int dof = 0; dof < NumberOfDoFs; dof++) {
    validateNumericalEqualsWithParams2(
      QOut[dof * (Unknowns + AuxiliaryVariables) + 0],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
    validateNumericalEqualsWithParams2(
      QOut[dof * (Unknowns + AuxiliaryVariables) + 1],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
    validateNumericalEqualsWithParams2(
      QOut[dof * (Unknowns + AuxiliaryVariables) + 2],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
    validateNumericalEqualsWithParams2(
      QOut[dof * (Unknowns + AuxiliaryVariables) + 3],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
    validateNumericalEqualsWithParams2(
      QOut[dof * (Unknowns + AuxiliaryVariables) + 4],
      0.0,
      dof,
      plotCell(QOut, Order, Unknowns, AuxiliaryVariables)
    );
  }
}

/** Verifies the distinction between per-cell scaled and global physical eigenvalues. */
void exahype2::dg::tests::CellIntegralTest::runReduceMaxScaledEigenvalue() {
  constexpr int    Unknowns           = 1;
  constexpr int    AuxiliaryVariables = 0;
  constexpr int    Order              = 2;
  constexpr double CellWidth          = 0.5;

#if DIMENSIONS == 2
  const tarch::la::Vector<2, double> x({0.0, 0.0});
  const tarch::la::Vector<2, double> cellSize({CellWidth, CellWidth});
  constexpr int                      NumberOfDoFs = (Order + 1) * (Order + 1);
#else
  const tarch::la::Vector<3, double> x({0.0, 0.0, 0.0});
  const tarch::la::Vector<3, double> cellSize({CellWidth, CellWidth, CellWidth});
  constexpr int                      NumberOfDoFs = (Order + 1) * (Order + 1) * (Order + 1);
#endif

  double QIn[NumberOfDoFs * Unknowns];
  double QOut[NumberOfDoFs * Unknowns];
  for (int dof = 0; dof < NumberOfDoFs; dof++) {
    QIn[dof]  = 0.0;
    QOut[dof] = 1.0;
  }
  QOut[NumberOfDoFs / 2] = 2.0;

  ::exahype2::CellData cellData(QIn, x, cellSize, 0.0, 0.01, QOut);

  const double maxPhysicalEigenvalue = reduceMaxScaledEigenvaluePatchwiseFunctors(
    cellData,
    Order,
    Unknowns,
    AuxiliaryVariables,
    [](const double* NOALIAS Q, const tarch::la::Vector<DIMENSIONS, double>& x, double t, double dt, int normal)
      -> double { return Q[0] * (normal + 1.0); },
    _orderTwoQuadraturePoints1D
  );

  // The scaled eigenvalue sums the directional eigenvalues and divides by the cell width,
  // while the physical maximum is the largest directional eigenvalue.
  constexpr double DirectionSum = DIMENSIONS == 2 ? 3.0 : 6.0;
  validateNumericalEquals(cellData.maxEigenvalue[0], 2.0 * DirectionSum / CellWidth);
  validateNumericalEquals(maxPhysicalEigenvalue, 2.0 * DIMENSIONS);
}
