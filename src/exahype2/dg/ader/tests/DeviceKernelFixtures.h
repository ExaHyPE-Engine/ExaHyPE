// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/ader/PredictorScratch.h"
#include "exahype2/dg/tests/DeviceKernelFixtures.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader::tests {
  struct DeviceKernelSetup;
  struct StubSolver;
  struct BasisTableSet;
  class DeviceBasisTables;
} // namespace exahype2::dg::ader::tests

/**
 * Sizes and coefficients of the predictor device tests.
 */
TARGET_DECLARE
struct exahype2::dg::ader::tests::DeviceKernelSetup final {
  static constexpr int    DofsPerAxis             = 3;
  static constexpr int    Unknowns                = 2;
  static constexpr int    AuxiliaryVariables      = 1;
  static constexpr int    NumberOfCells           = 8;
  static constexpr int    NumberOfPointSources    = 2;
  static constexpr double TimeStamp               = 0.75;
  static constexpr double TimeStepSize            = 0.01;
  static constexpr double PicardTolerance         = 1.0e-10;
  static constexpr double FluxSpeed               = 0.5;
  static constexpr double DiffusionSpeed          = 0.05;
  static constexpr double ProductSpeed            = 0.25;
  static constexpr double SourceStrength          = 0.5;
  static constexpr double DiffusiveSourceStrength = 0.1;

  using CellLayout = exahype2::dg::ader::Layout<DofsPerAxis, Unknowns, AuxiliaryVariables>;

  static constexpr int CellStateCardinality     = CellLayout::NodesPerCell * CellLayout::NumberOfData;
  static constexpr int FaceStateCardinality     = CellLayout::NodesPerFace * CellLayout::NumberOfData;
  static constexpr int FaceFluxCardinality      = CellLayout::NodesPerFace * Unknowns;
  static constexpr int AllCellStatesCardinality = NumberOfCells * CellStateCardinality;
  static constexpr int AllFaceStatesCardinality = NumberOfCells * peano4::FacesPerCell * FaceStateCardinality;
  static constexpr int AllFaceFluxesCardinality = NumberOfCells * peano4::FacesPerCell * FaceFluxCardinality;
  static constexpr int LocationCardinality      = NumberOfPointSources * DIMENSIONS;
  static constexpr int AllIndicesCardinality    = NumberOfCells * NumberOfPointSources;
  static constexpr int OperatorCardinality      = DofsPerAxis * DofsPerAxis;
};
TARGET_DECLARE_END

TARGET_DECLARE
/**
 * Every callback depends on state, position and direction, so a wrong offset changes the result.
 */
struct exahype2::dg::ader::tests::StubSolver final {
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

  static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
    const double* NOALIAS                        state,
    const double* NOALIAS                        stateGradient,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void multiplyMaterialParameterMatrix(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void pointSource(
    const double* NOALIAS state,
    const double* NOALIAS nodePosition,
    double                timeStamp,
    double                timeStepSize,
    double* NOALIAS       forceVector,
    int                   pointSource
  );
};
TARGET_DECLARE_END

/**
 * Operator tables of a three-node Gauss-Legendre basis.
 */
struct exahype2::dg::ader::tests::BasisTableSet final {
  std::vector<double> quadraturePoints             = std::vector<double>(DeviceKernelSetup::DofsPerAxis);
  std::vector<double> quadratureWeights            = std::vector<double>(DeviceKernelSetup::DofsPerAxis);
  std::vector<double> inverseQuadratureWeights     = std::vector<double>(DeviceKernelSetup::DofsPerAxis);
  std::vector<double> cellQuadratureWeights        = std::vector<double>(DeviceKernelSetup::CellLayout::NodesPerCell);
  std::vector<double> inverseCellQuadratureWeights = std::vector<double>(DeviceKernelSetup::CellLayout::NodesPerCell);
  std::vector<double> stiffnessOperator            = std::vector<double>(DeviceKernelSetup::OperatorCardinality);
  std::vector<double> stiffnessOperatorTransposed  = std::vector<double>(DeviceKernelSetup::OperatorCardinality);
  std::vector<double> derivativeOperator           = std::vector<double>(DeviceKernelSetup::OperatorCardinality);
  std::vector<double> inversePicardOperator        = std::vector<double>(DeviceKernelSetup::OperatorCardinality);
  std::vector<double> leftFaceCoefficients         = std::vector<double>(DeviceKernelSetup::DofsPerAxis);
  std::vector<double> rightFaceCoefficients        = std::vector<double>(DeviceKernelSetup::DofsPerAxis);

  BasisTableSet();

  [[nodiscard]] exahype2::dg::ader::BasisTables<double> view() const;
};

/**
 * Device copies of a BasisTableSet.
 */
class exahype2::dg::ader::tests::DeviceBasisTables final {
public:
  explicit DeviceBasisTables(const BasisTableSet& tables);
  DeviceBasisTables(const DeviceBasisTables& other)            = delete;
  DeviceBasisTables(DeviceBasisTables&& other)                 = delete;
  DeviceBasisTables& operator=(const DeviceBasisTables& other) = delete;
  DeviceBasisTables& operator=(DeviceBasisTables&& other)      = delete;
  ~DeviceBasisTables()                                         = default;

  [[nodiscard]] exahype2::dg::ader::BasisTables<double> view() const;

private:
  const exahype2::dg::tests::DeviceBuffer<double> _quadraturePoints;
  const exahype2::dg::tests::DeviceBuffer<double> _quadratureWeights;
  const exahype2::dg::tests::DeviceBuffer<double> _inverseQuadratureWeights;
  const exahype2::dg::tests::DeviceBuffer<double> _cellQuadratureWeights;
  const exahype2::dg::tests::DeviceBuffer<double> _inverseCellQuadratureWeights;
  const exahype2::dg::tests::DeviceBuffer<double> _stiffnessOperator;
  const exahype2::dg::tests::DeviceBuffer<double> _stiffnessOperatorTransposed;
  const exahype2::dg::tests::DeviceBuffer<double> _derivativeOperator;
  const exahype2::dg::tests::DeviceBuffer<double> _inversePicardOperator;
  const exahype2::dg::tests::DeviceBuffer<double> _leftFaceCoefficients;
  const exahype2::dg::tests::DeviceBuffer<double> _rightFaceCoefficients;
};

#include "exahype2/dg/ader/tests/DeviceKernelFixtures.cpph"
