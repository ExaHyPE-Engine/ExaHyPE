// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::tests {
  struct DeviceKernelSetup;
  struct StubSolver;
  struct DeviceDeleter;
  template <typename T>
  class DeviceBuffer;
  struct KernelOperators;
  struct CellChain;

  std::vector<double> makeRamp(std::size_t count, double offset, double increment);

  TARGET_DECLARE
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> faceCentreOf(
    int           cell,
    int           faceNumber,
    const double* cellCentres,
    const double* cellSizes
  );
  TARGET_DECLARE_END
} // namespace exahype2::dg::tests

TARGET_DECLARE
/**
 * Sizes of the cells, faces and stages the device kernel tests run with.
 */
struct exahype2::dg::tests::DeviceKernelSetup final {
  static constexpr int    RungeKuttaSteps              = 3;
  static constexpr int    RungeKuttaStep               = 1;
  static constexpr int    DofsPerAxis                  = 3;
  static constexpr int    NumberOfUnknowns             = 2;
  static constexpr int    NumberOfAuxiliaryVariables   = 1;
  static constexpr int    NumberOfData                 = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  static constexpr int    NodesPerFace                 = DofsPerAxis * ((DIMENSIONS == 3) ? DofsPerAxis : 1);
  static constexpr int    NodesPerCell                 = NodesPerFace * DofsPerAxis;
  static constexpr int    CellStateCardinality         = NodesPerCell * NumberOfData;
  static constexpr int    StageCardinality             = NodesPerCell * NumberOfUnknowns;
  static constexpr int    RightHandSideCardinality     = RungeKuttaSteps * StageCardinality;
  static constexpr int    RungeKuttaStepOffset         = RungeKuttaStep * StageCardinality;
  static constexpr int    FaceSideStateCardinality     = NodesPerFace * NumberOfData;
  static constexpr int    FaceSideFluxCardinality      = NodesPerFace * NumberOfUnknowns;
  static constexpr int    FaceStateCardinality         = 2 * FaceSideStateCardinality;
  static constexpr int    FaceFluxCardinality          = 2 * FaceSideFluxCardinality;
  static constexpr int    NumberOfCells                = 8;
  static constexpr int    AllCellStatesCardinality     = NumberOfCells * CellStateCardinality;
  static constexpr int    AllRightHandSidesCardinality = NumberOfCells * RightHandSideCardinality;
  static constexpr int    OperatorCardinality          = DofsPerAxis * DofsPerAxis;
  static constexpr int    ButcherCardinality           = RungeKuttaSteps * RungeKuttaSteps;
  static constexpr int    CoordinateCardinality        = NumberOfCells * DIMENSIONS;
  static constexpr int    FaceSlotCardinality          = NumberOfCells * peano4::FacesPerCell;
  static constexpr double TimeStamp                    = 0.75;
  static constexpr double TimeStepSize                 = 0.125;
};
TARGET_DECLARE_END

TARGET_DECLARE
/**
 * Every callback depends on its state and on the direction it is asked about,
 * so a cell or face read at the wrong offset produces a different number.
 */
struct exahype2::dg::tests::StubSolver final {
  static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
    const double* NOALIAS                        insideState,
    double* NOALIAS                              outsideState,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    int                                          direction
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const double* NOALIAS                        state,
    const double* const NOALIAS                  stateGradients[DIMENSIONS],
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
    const double* NOALIAS                        state,
    const double* NOALIAS                        stateGradient,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void multiplyMaterialParameterMatrix(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    double* NOALIAS                              values
  );
};
TARGET_DECLARE_END

/**
 * Frees device memory, so a unique pointer can own a device buffer.
 */
struct exahype2::dg::tests::DeviceDeleter final {
  void operator()(void* data) const;
};

/**
 * Owns one buffer in device memory for the lifetime of a test. A build without
 * a GPU backend allocates and copies in plain host memory, so the tests run the
 * same code path everywhere.
 */
template <typename T>
class exahype2::dg::tests::DeviceBuffer final {
public:
  INLINE_METHOD explicit DeviceBuffer(const std::vector<T>& hostData);
  DeviceBuffer(const DeviceBuffer& other)            = delete;
  DeviceBuffer(DeviceBuffer&& other)                 = delete;
  DeviceBuffer& operator=(const DeviceBuffer& other) = delete;
  DeviceBuffer& operator=(DeviceBuffer&& other)      = delete;
  ~DeviceBuffer()                                    = default;

  INLINE_METHOD T* data() const;

  INLINE_METHOD void copyBack(std::vector<T>& target) const;

private:
  const std::size_t                       _count;
  const std::unique_ptr<T, DeviceDeleter> _data;
};

/**
 * Operator tables with distinct, non-symmetric entries, so an index mix-up
 * cannot cancel out.
 */
struct exahype2::dg::tests::KernelOperators final {
  std::vector<double> quadratureNodes              = std::vector<double>(DeviceKernelSetup::DofsPerAxis);
  std::vector<double> massMatrixDiagonal           = std::vector<double>(DeviceKernelSetup::DofsPerAxis);
  std::vector<double> leftFaceCoefficients         = std::vector<double>(DeviceKernelSetup::DofsPerAxis);
  std::vector<double> rightFaceCoefficients        = std::vector<double>(DeviceKernelSetup::DofsPerAxis);
  std::vector<double> stiffnessMatrix              = std::vector<double>(DeviceKernelSetup::OperatorCardinality);
  std::vector<double> derivativeOperator           = std::vector<double>(DeviceKernelSetup::OperatorCardinality);
  std::vector<double> faceQuadratureWeights        = std::vector<double>(DeviceKernelSetup::NodesPerFace);
  std::vector<double> inverseCellQuadratureWeights = std::vector<double>(DeviceKernelSetup::NodesPerCell);
  std::vector<double> rungeKuttaCoefficients       = std::vector<double>(DeviceKernelSetup::ButcherCardinality);

  KernelOperators();
};

/**
 * A chain of cells along the first axis, where two neighbours hold the same
 * face and a parallel dispatch could write it twice while the faces along the
 * other axes belong to one cell each, together with a work list that holds
 * every unique face once with one owning cell and the face number under which
 * that cell sees it.
 */
struct exahype2::dg::tests::CellChain final {
  std::vector<double>        cellCentres = std::vector<double>(DeviceKernelSetup::CoordinateCardinality);
  std::vector<double>        cellSizes   = std::vector<double>(DeviceKernelSetup::CoordinateCardinality);
  std::vector<std::uint64_t> faceIndices = std::vector<std::uint64_t>(DeviceKernelSetup::FaceSlotCardinality);
  std::vector<std::uint64_t> workFaceIndices{};
  std::vector<std::uint8_t>  workFaceNumbers{};
  std::vector<std::uint64_t> workCellIndices{};
  int                        numberOfUniqueFaces{0};

  CellChain();

  [[nodiscard]] int numberOfWorkFaces() const;
  [[nodiscard]] int faceStateEntries() const;
  [[nodiscard]] int faceFluxEntries() const;
};

#include "exahype2/dg/tests/DeviceKernelFixtures.cpph"
