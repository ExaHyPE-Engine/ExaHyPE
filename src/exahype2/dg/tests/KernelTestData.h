// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::tests {
  struct KernelTestSetup;
  struct StubSolver;
  struct DeviceDeleter;
  template <typename T>
  class DeviceBuffer;
  struct KernelOperators;
  struct BasisTableSet;
  class DeviceBasisTables;
  struct CellChain;

  std::vector<double> getRamp(std::size_t count, double offset, double increment);

  TARGET_DECLARE
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getFaceCentreOf(
    int           cell,
    int           faceNumber,
    const double* cellCentres,
    const double* cellSizes
  );
  TARGET_DECLARE_END
} // namespace exahype2::dg::tests

TARGET_DECLARE
/**
 * Sizes of the cells, faces and stages the kernel tests run with.
 */
struct exahype2::dg::tests::KernelTestSetup final {
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
  static constexpr int    CoordinateCardinality        = NumberOfCells * DIMENSIONS;
  static constexpr int    FaceSlotCardinality          = NumberOfCells * peano4::FacesPerCell;
  static constexpr double TimeStamp                    = 0.75;
  static constexpr double TimeStepSize                 = 0.125;
};
TARGET_DECLARE_END

TARGET_DECLARE
/**
 * A solver whose terms depend on the state and the direction, so a wrong offset changes the
 * result.
 */
struct exahype2::dg::tests::StubSolver final {
  static constexpr int DGOrder                    = KernelTestSetup::DofsPerAxis - 1;
  static constexpr int NumberOfUnknowns           = KernelTestSetup::NumberOfUnknowns;
  static constexpr int NumberOfAuxiliaryVariables = KernelTestSetup::NumberOfAuxiliaryVariables;

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
 * A deleter that frees device memory, so a unique pointer can own a device buffer.
 */
struct exahype2::dg::tests::DeviceDeleter final {
  void operator()(void* data) const;
};

/**
 * A device copy of a host vector. A build without a GPU backend allocates it in host memory.
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

  [[nodiscard]] INLINE_METHOD T* getData() const;

  INLINE_METHOD void copyBack(std::vector<T>& target) const;

private:
  const std::size_t                       _count;
  const std::unique_ptr<T, DeviceDeleter> _data;
};

/**
 * Operator tables with distinct, non-symmetric entries, so an index mix-up cannot cancel out.
 */
struct exahype2::dg::tests::KernelOperators final {
  std::vector<double> quadratureNodes              = std::vector<double>(KernelTestSetup::DofsPerAxis);
  std::vector<double> leftFaceCoefficients         = std::vector<double>(KernelTestSetup::DofsPerAxis);
  std::vector<double> rightFaceCoefficients        = std::vector<double>(KernelTestSetup::DofsPerAxis);
  std::vector<double> derivativeOperator           = std::vector<double>(KernelTestSetup::OperatorCardinality);
  std::vector<double> faceQuadratureWeights        = std::vector<double>(KernelTestSetup::NodesPerFace);
  std::vector<double> inverseCellQuadratureWeights = std::vector<double>(KernelTestSetup::NodesPerCell);

  KernelOperators();
};

/**
 * Operator tables of a three-node Gauss-Legendre basis, so a kernel that acts on a polynomial
 * of that degree returns the exact result.
 */
struct exahype2::dg::tests::BasisTableSet final {
  std::vector<double> quadraturePoints             = std::vector<double>(KernelTestSetup::DofsPerAxis);
  std::vector<double> quadratureWeights            = std::vector<double>(KernelTestSetup::DofsPerAxis);
  std::vector<double> inverseQuadratureWeights     = std::vector<double>(KernelTestSetup::DofsPerAxis);
  std::vector<double> cellQuadratureWeights        = std::vector<double>(KernelTestSetup::NodesPerCell);
  std::vector<double> inverseCellQuadratureWeights = std::vector<double>(KernelTestSetup::NodesPerCell);
  std::vector<double> stiffnessOperator            = std::vector<double>(KernelTestSetup::OperatorCardinality);
  std::vector<double> stiffnessOperatorTransposed  = std::vector<double>(KernelTestSetup::OperatorCardinality);
  std::vector<double> derivativeOperator           = std::vector<double>(KernelTestSetup::OperatorCardinality);
  std::vector<double> inversePicardOperator        = std::vector<double>(KernelTestSetup::OperatorCardinality);
  std::vector<double> leftFaceCoefficients         = std::vector<double>(KernelTestSetup::DofsPerAxis);
  std::vector<double> rightFaceCoefficients        = std::vector<double>(KernelTestSetup::DofsPerAxis);

  BasisTableSet();

  [[nodiscard]] exahype2::dg::BasisTables<double> getView() const;
};

/**
 * Device copies of a BasisTableSet.
 */
class exahype2::dg::tests::DeviceBasisTables final {
public:
  explicit DeviceBasisTables(const BasisTableSet& tables);
  DeviceBasisTables(const DeviceBasisTables& other)            = delete;
  DeviceBasisTables(DeviceBasisTables&& other)                 = delete;
  DeviceBasisTables& operator=(const DeviceBasisTables& other) = delete;
  DeviceBasisTables& operator=(DeviceBasisTables&& other)      = delete;
  ~DeviceBasisTables()                                         = default;

  [[nodiscard]] exahype2::dg::BasisTables<double> getView() const;

private:
  const DeviceBuffer<double> _quadraturePoints;
  const DeviceBuffer<double> _quadratureWeights;
  const DeviceBuffer<double> _inverseQuadratureWeights;
  const DeviceBuffer<double> _cellQuadratureWeights;
  const DeviceBuffer<double> _inverseCellQuadratureWeights;
  const DeviceBuffer<double> _stiffnessOperator;
  const DeviceBuffer<double> _stiffnessOperatorTransposed;
  const DeviceBuffer<double> _derivativeOperator;
  const DeviceBuffer<double> _inversePicardOperator;
  const DeviceBuffer<double> _leftFaceCoefficients;
  const DeviceBuffer<double> _rightFaceCoefficients;
};

/**
 * A chain of cells along the first axis, in which two neighbouring cells hold the face between
 * them and every other face belongs to one cell. The work list stores every face once with one of
 * its cells and the number of the face in that cell.
 */
struct exahype2::dg::tests::CellChain final {
  std::vector<double>        cellCentres = std::vector<double>(KernelTestSetup::CoordinateCardinality);
  std::vector<double>        cellSizes   = std::vector<double>(KernelTestSetup::CoordinateCardinality);
  std::vector<std::uint64_t> faceIndices = std::vector<std::uint64_t>(KernelTestSetup::FaceSlotCardinality);
  std::vector<std::uint64_t> workFaceIndices{};
  std::vector<std::uint8_t>  workFaceNumbers{};
  std::vector<std::uint64_t> workCellIndices{};
  int                        numberOfUniqueFaces{0};

  CellChain();

  [[nodiscard]] int getNumberOfWorkFaces() const;
  [[nodiscard]] int getNumberOfFaceStateEntries() const;
  [[nodiscard]] int getNumberOfFaceFluxEntries() const;
};

#include "exahype2/dg/tests/KernelTestData.cpph"
