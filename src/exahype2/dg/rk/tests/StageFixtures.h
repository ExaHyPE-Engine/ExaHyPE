// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/AdaptiveTimeStepping.h"
#include "exahype2/FixedTimeStepping.h"
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/rk/Solver.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceEnumerator.h"
#include "peano4/dataview/FaceMarker.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::rk::tests {
  constexpr int DgOrder                    = 1;
  constexpr int RkOrder                    = 2;
  constexpr int RkStages                   = 2;
  constexpr int NumberOfUnknowns           = 1;
  constexpr int NumberOfAuxiliaryVariables = 0;
  constexpr int NumberOfDofsPerAxis        = DgOrder + 1;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr int NodesPerFace               = exahype2::dg::NodesPerFace<NumberOfDofsPerAxis>;
  constexpr int NodesPerCell               = exahype2::dg::NodesPerCell<NumberOfDofsPerAxis>;

  constexpr int StateElementsPerFace = NodesPerFace * NumberOfData;
  constexpr int FluxElementsPerFace  = NodesPerFace * NumberOfUnknowns;
  constexpr int ValuesPerStage       = NodesPerCell * NumberOfUnknowns;

  constexpr int SubdivisionFactor     = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell       = peano4::sfc::ActiveCurve::ChildrenPerCell;
  constexpr int TransferMatrixEntries = SubdivisionFactor * NumberOfDofsPerAxis * NumberOfDofsPerAxis;

  constexpr double MaxEigenvalue        = 4.0;
  constexpr double ReportedTimeStepSize = 0.1;

  struct StageTerms;
  class TestSolver;
  class AdaptiveTestSolver;
  class CellLabel;
  class FaceLabel;

  template <int Values>
  class ValuePatch;

  class Quadrature;
  class DGMatrices;
  class ButcherTableau;

  template <typename Element>
  INLINE_METHOD peano4::dataview::FaceEnumerator<Element> makeFaceEnumerator(Element* pool);

  //! Fills the tables and opens the grid initialisation of the solver.
  template <typename SolverType>
  INLINE_METHOD void enterGridInitialisation(SolverType& solver);

  //! Fills the tables and leaves the solver in the sweep that follows the grid initialisation.
  template <typename SolverType>
  INLINE_METHOD void enterTimeStepping(SolverType& solver);

  //! Closes the sweep with one cell reported and opens the next one.
  template <typename SolverType>
  INLINE_METHOD void advanceOneSweep(SolverType& solver);

  tarch::la::Vector<DIMENSIONS, int>    nodeIndexOf(int cellNode);
  tarch::la::Vector<DIMENSIONS, int>    positionOfChild(int child);
  tarch::la::Vector<DIMENSIONS, double> makeCellSize();
  int                                   cellNodeOnFaceLine(int direction, int faceNode, int normalIndex);
  peano4::dataview::CellMarker          makeCellMarker();
  peano4::dataview::CellMarker          makeChildCellMarker(int child);
  peano4::dataview::FaceMarker          makeFaceMarker(int faceNumber);
  peano4::dataview::FaceMarker          makeSubfaceMarker(int faceNumber, int subface);
} // namespace exahype2::dg::rk::tests

/**
 * The terms of a solver with one unknown, whose flux scales the state with the axis and adds the
 * time stamp so a test tells which time stamp a kernel passed, and whose Riemann solver writes
 * both states into the fluxes and records the arguments of its last call.
 */
struct exahype2::dg::rk::tests::StageTerms {
  static inline std::bitset<DIMENSIONS> PeriodicBoundaries;

  static inline int    riemannSolverCalls{0};
  static inline int    riemannSolverFaceNumber{-1};
  static inline int    riemannSolverDirection{-1};
  static inline bool   riemannSolverBoundary{false};
  static inline double riemannSolverTimeStamp{0.0};

  static INLINE_METHOD void flux(
    const double*                                state,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    double*                                      fluxValues
  );

  static INLINE_METHOD double maxEigenvalue(
    const double*                                state,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction
  );

  static INLINE_METHOD void boundaryConditions(
    const double*                                inside,
    double*                                      outside,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       timeStamp,
    int                                          direction
  );

  static INLINE_METHOD void riemannSolver(
    double*                                      leftFlux,
    double*                                      rightFlux,
    const double*                                leftState,
    const double*                                rightState,
    double                                       timeStamp,
    double                                       timeStepSize,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          direction,
    bool                                         isBoundary,
    int                                          faceNumber
  );

  //! Clears the record of the Riemann solver and opens every boundary of the domain.
  static INLINE_METHOD void reset();
};

//! A solver of the Runge-Kutta family that takes the time step size a test sets.
class exahype2::dg::rk::tests::TestSolver final : public exahype2::dg::rk::Solver<
                                                    exahype2::dg::rk::tests::TestSolver,
                                                    DgOrder,
                                                    RkOrder,
                                                    RkStages,
                                                    NumberOfUnknowns,
                                                    NumberOfAuxiliaryVariables,
                                                    exahype2::FixedTimeStepping>,
                                                  public exahype2::dg::rk::tests::StageTerms {
public:
  TestSolver();
  TestSolver(const TestSolver&)            = delete;
  TestSolver(TestSolver&&)                 = delete;
  TestSolver& operator=(const TestSolver&) = delete;
  TestSolver& operator=(TestSolver&&)      = delete;
  ~TestSolver() override                   = default;

  void startSimulation() override;
  void finishSimulation() override;

  void setTimeStepSize(double timeStepSize);
};

//! A solver of the Runge-Kutta family that derives its time step size from the reported eigenvalue.
class exahype2::dg::rk::tests::AdaptiveTestSolver final : public exahype2::dg::rk::Solver<
                                                            exahype2::dg::rk::tests::AdaptiveTestSolver,
                                                            DgOrder,
                                                            RkOrder,
                                                            RkStages,
                                                            NumberOfUnknowns,
                                                            NumberOfAuxiliaryVariables,
                                                            exahype2::AdaptiveTimeStepping>,
                                                          public exahype2::dg::rk::tests::StageTerms {
public:
  AdaptiveTestSolver();
  AdaptiveTestSolver(const AdaptiveTestSolver&)            = delete;
  AdaptiveTestSolver(AdaptiveTestSolver&&)                 = delete;
  AdaptiveTestSolver& operator=(const AdaptiveTestSolver&) = delete;
  AdaptiveTestSolver& operator=(AdaptiveTestSolver&&)      = delete;
  ~AdaptiveTestSolver() override                           = default;

  void startSimulation() override;
  void finishSimulation() override;

protected:
  [[nodiscard]] double minTerminalTime() const override;
};

//! Stands in for the generated cell label.
class exahype2::dg::rk::tests::CellLabel final {
public:
  [[nodiscard]] INLINE_METHOD double getTimeStamp() const;
  INLINE_METHOD void                 setTimeStamp(double value);
  [[nodiscard]] INLINE_METHOD double getTimeStepSize() const;
  INLINE_METHOD void                 setTimeStepSize(double value);
  [[nodiscard]] INLINE_METHOD bool   getHasUpdated() const;
  INLINE_METHOD void                 setHasUpdated(bool value);
  [[nodiscard]] INLINE_METHOD bool   getRequiresCriterionEvaluation() const;
  INLINE_METHOD void                 setRequiresCriterionEvaluation(bool value);

private:
  double _timeStamp{0.0};
  double _timeStepSize{0.0};
  bool   _hasUpdated{false};
  bool   _requiresCriterionEvaluation{false};
};

//! Stands in for the generated face label, whose entries belong to the two sides of the face.
class exahype2::dg::rk::tests::FaceLabel final {
public:
  [[nodiscard]] INLINE_METHOD double getUpdatedTimeStamp(int side) const;
  INLINE_METHOD void                 setUpdatedTimeStamp(int side, double value);
  [[nodiscard]] INLINE_METHOD double getNewTimeStamp(int side) const;
  INLINE_METHOD void                 setNewTimeStamp(int side, double value);
  [[nodiscard]] INLINE_METHOD double getOldTimeStamp(int side) const;
  INLINE_METHOD void                 setOldTimeStamp(int side, double value);
  [[nodiscard]] INLINE_METHOD bool   getUpdated(int side) const;
  INLINE_METHOD void                 setUpdated(int side, bool value);
  [[nodiscard]] INLINE_METHOD bool   getRequiresDataTransfer(int side) const;
  INLINE_METHOD void                 setRequiresDataTransfer(int side, bool value);
  [[nodiscard]] INLINE_METHOD bool   getBoundary() const;
  INLINE_METHOD void                 setBoundary(bool value);
  [[nodiscard]] INLINE_METHOD bool   getIsHanging() const;
  INLINE_METHOD void                 setIsHanging(bool value);

private:
  double _updatedTimeStamp[2]{0.0, 0.0};
  double _newTimeStamp[2]{0.0, 0.0};
  double _oldTimeStamp[2]{0.0, 0.0};
  bool   _updated[2]{false, false};
  bool   _requiresDataTransfer[2]{true, true};
  bool   _boundary{false};
  bool   _isHanging{false};
};

//! Holds the values of one face or one cell the way a generated patch record does.
template <int Values>
class exahype2::dg::rk::tests::ValuePatch final {
public:
  static constexpr int Cardinality = Values;

  [[nodiscard]] INLINE_METHOD double*       data();
  [[nodiscard]] INLINE_METHOD const double* data() const;

  INLINE_METHOD void fill(double value);

private:
  double _values[Values]{};
};

/**
 * Quadrature tables of unit weights, so a face integral and a Riemann problem reduce to the
 * values the caller passes in.
 */
class exahype2::dg::rk::tests::Quadrature final {
public:
  static inline double QuadraturePoints1D[NumberOfDofsPerAxis]{0.25, 0.75};
  static inline double QuadratureWeightsOnFace[NodesPerFace]{};
  static inline double InverseQuadratureWeightsInCell[NodesPerCell]{};

  static void initialise();
};

/**
 * Operator tables whose volume operators vanish, whose face coefficients pick the node at either
 * end of an axis, and whose transfer tables run over the child intervals with a prolongation that
 * scales interval b by b + 1 so a test tells the intervals apart, a cell interpolation that copies
 * the coarse node and a restriction that averages the children.
 */
class exahype2::dg::rk::tests::DGMatrices final {
public:
  static inline double MassMatrixDiagonal1D[NumberOfDofsPerAxis]{};
  static inline double StiffnessOperator1D[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{};
  static inline double DerivativeOperator1D[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{};
  static inline double BasisFunctionValuesLeft1D[NumberOfDofsPerAxis]{};
  static inline double BasisFunctionValuesRight1D[NumberOfDofsPerAxis]{};
  static inline double ProlongationMatrix1D[TransferMatrixEntries]{};
  static inline double InterpolationMatrix1D[TransferMatrixEntries]{};
  static inline double WeightedRestrictionMatrix1D[TransferMatrixEntries]{};

  static void initialise();
};

//! A two-stage tableau whose first row vanishes, so the first stage state is the cell state.
class exahype2::dg::rk::tests::ButcherTableau final {
public:
  static inline double ButcherMatrix[RkStages * RkStages]{0.0, 0.0, 1.0, 0.0};
  static inline double FinalEstimateWeights[RkStages]{1.0, 0.0};
  static inline double RelativeTimeStepSizes[RkStages]{0.25, 0.5};
};

#include "exahype2/dg/rk/tests/StageFixtures.cpph"
