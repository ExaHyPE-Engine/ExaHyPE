// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/LimiterStatus.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::limiting::tests {
  struct TestCellLabel;
  struct TestFaceLabel;

  template <int NumberOfObservables>
  struct TestVertexLabel;

  template <typename Element, int Cardinality>
  struct TestEnumerator;

  template <typename Value, int Cardinality>
  struct TestPatch;

  template <bool Adaptive>
  struct TestSolver;

  struct TestGeometry;
} // namespace exahype2::dg::limiting::tests

/**
 * Stands in for the generated cell label.
 */
struct exahype2::dg::limiting::tests::TestCellLabel final {
  exahype2::dg::LimiterStatus limiterStatus{exahype2::dg::LimiterStatus::Regular};
  exahype2::dg::LimiterStatus previousLimiterStatus{exahype2::dg::LimiterStatus::Regular};
  bool                        wasTroubledInPreviousStep{false};
  bool                        predictorDiverged{false};
  double                      timeStamp{0.0};
  double                      timeStepSize{0.0};

  [[nodiscard]] INLINE_METHOD exahype2::dg::LimiterStatus getLimiterStatus() const;
  INLINE_METHOD void                                      setLimiterStatus(exahype2::dg::LimiterStatus value);
  [[nodiscard]] INLINE_METHOD exahype2::dg::LimiterStatus getPreviousLimiterStatus() const;
  INLINE_METHOD void                                      setPreviousLimiterStatus(exahype2::dg::LimiterStatus value);
  [[nodiscard]] INLINE_METHOD bool                        getWasTroubledInPreviousStep() const;
  INLINE_METHOD void                                      setWasTroubledInPreviousStep(bool value);
  [[nodiscard]] INLINE_METHOD bool                        getPredictorDiverged() const;
  INLINE_METHOD void                                      setPredictorDiverged(bool value);
  [[nodiscard]] INLINE_METHOD double                      getTimeStamp() const;
  [[nodiscard]] INLINE_METHOD double                      getTimeStepSize() const;
};

/**
 * Stands in for the generated face label.
 */
struct exahype2::dg::limiting::tests::TestFaceLabel final {
  exahype2::dg::LimiterStatus limiterStatus[2]{
    exahype2::dg::LimiterStatus::Regular,
    exahype2::dg::LimiterStatus::Regular
  };
  exahype2::dg::LimiterStatus limiterStatusSnapshot[2]{
    exahype2::dg::LimiterStatus::Regular,
    exahype2::dg::LimiterStatus::Regular
  };
  tarch::la::Vector<2, int> aboveHanging{0};
  bool                      isHanging{false};
  bool                      boundary{false};

  [[nodiscard]] INLINE_METHOD exahype2::dg::LimiterStatus getLimiterStatus(int side) const;
  INLINE_METHOD void                                      setLimiterStatus(int side, exahype2::dg::LimiterStatus value);
  [[nodiscard]] INLINE_METHOD exahype2::dg::LimiterStatus getLimiterStatusSnapshot(int side) const;
  INLINE_METHOD void               setLimiterStatusSnapshot(int side, exahype2::dg::LimiterStatus value);
  INLINE_METHOD void               raiseLimiterStatus(int side, exahype2::dg::LimiterStatus value);
  [[nodiscard]] INLINE_METHOD bool getAboveHanging(int side) const;
  [[nodiscard]] INLINE_METHOD bool getIsHanging() const;
  [[nodiscard]] INLINE_METHOD bool getBoundary() const;
};

/**
 * Stands in for the generated vertex label that carries the previous-solution observable extrema.
 */
template <int NumberOfObservables>
struct exahype2::dg::limiting::tests::TestVertexLabel final {
  double dmpMinimum[NumberOfObservables]{};
  double dmpMaximum[NumberOfObservables]{};

  [[nodiscard]] INLINE_METHOD double getDmpMinimum(int observable) const;
  INLINE_METHOD void                 setDmpMinimum(int observable, double value);
  [[nodiscard]] INLINE_METHOD double getDmpMaximum(int observable) const;
  INLINE_METHOD void                 setDmpMaximum(int observable, double value);
};

/**
 * Stands in for peano4::dataview::Enumerator.
 */
template <typename Element, int Cardinality>
struct exahype2::dg::limiting::tests::TestEnumerator final {
  Element* elements{nullptr};

  INLINE_METHOD Element& operator()(int index) const;
};

/**
 * Stands in for a generated patch record.
 */
template <typename Value, int Cardinality>
struct exahype2::dg::limiting::tests::TestPatch final {
  using DoFType = Value;

  Value values[Cardinality]{};

  [[nodiscard]] INLINE_METHOD Value*       data();
  [[nodiscard]] INLINE_METHOD const Value* data() const;
};

/**
 * A one-unknown solver with the terms the limiter pipeline evaluates. It counts its flux
 * evaluations, so a test tells how often the subcell scheme advanced.
 */
template <bool Adaptive>
struct exahype2::dg::limiting::tests::TestSolver final {
  static constexpr int    DGOrder                    = 1;
  static constexpr int    NumberOfUnknowns           = 1;
  static constexpr int    NumberOfAuxiliaryVariables = 0;
  static constexpr bool   UsesAdaptiveTimeStepping   = Adaptive;
  static constexpr int    NumberOfDofsPerAxis        = DGOrder + 1;
  static constexpr int    NumberOfSubcellsPerAxis    = 2 * DGOrder + 1;
  static constexpr int    NumberOfNodes              = peano4::power(NumberOfDofsPerAxis, DIMENSIONS);
  static constexpr int    NumberOfSubcells           = peano4::power(NumberOfSubcellsPerAxis, DIMENSIONS);
  static constexpr double AdvectionSpeed             = 1.0;

  inline static std::bitset<DIMENSIONS> PeriodicBoundaries{};
  inline static int                     numberOfFluxEvaluations{0};

  double maxPhysicalEigenvalue{0.0};
  double maxScaledEigenvalue{0.0};
  int    numberOfTroubledCells{0};

  INLINE_METHOD void setMaxEigenvalue(double value);
  INLINE_METHOD void setMaxScaledEigenvalue(double value);
  INLINE_METHOD void markCellAsTroubled();

  INLINE_METHOD static void initialCondition(
    double*                                      state,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize
  );

  INLINE_METHOD static void mapDmpObservables(double* observables, const double* state);

  INLINE_METHOD static double dmpRelaxationParameter(int observable, double specifiedRelaxationParameter);

  INLINE_METHOD static bool isPhysicallyAdmissible(
    const double*                                state,
    int                                          numberOfStates,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  );

  template <typename ComputePrecision>
  INLINE_METHOD static void flux(
    const ComputePrecision*                      state,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal,
    ComputePrecision*                            fluxes
  );

  template <typename ComputePrecision>
  INLINE_METHOD static double maxEigenvalue(
    const ComputePrecision*                      state,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal
  );

  template <typename ComputePrecision>
  INLINE_METHOD static void boundaryConditions(
    const ComputePrecision*                      stateInside,
    ComputePrecision*                            stateOutside,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       timeStamp,
    int                                          normal
  );
};

/**
 * Builds the marker of a local unrefined cell of the given centre and width.
 */
struct exahype2::dg::limiting::tests::TestGeometry final {
  INLINE_METHOD static peano4::dataview::CellMarker cellMarker(double centre, double width);
};

#include "exahype2/dg/limiting/tests/LimiterFixtures.cpph"
