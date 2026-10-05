// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2::dg::ader {
  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  struct Layout;

  template <typename Precision>
  struct BasisTables;

  template <
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateDiffusiveFlux,
    bool EvaluateDiffusiveSource>
  struct NonlinearPredictorScratch;

  template <
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateNonconservativeProduct,
    bool EvaluatePointSources>
  struct LinearPredictorScratch;
} // namespace exahype2::dg::ader

/**
 * Node counts and padded strides of one cell. A padded stride rounds the value count of a
 * node up to a multiple of the vector width, and every padding lane holds zero.
 */
template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
struct exahype2::dg::ader::Layout final {
  static constexpr int NumberOfData = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  static constexpr int PaddedNumberOfUnknowns =
    ((NumberOfUnknowns + PEANO_WITH_SIMDLEN - 1) / PEANO_WITH_SIMDLEN) * PEANO_WITH_SIMDLEN;
  static constexpr int PaddedNumberOfData =
    ((NumberOfData + PEANO_WITH_SIMDLEN - 1) / PEANO_WITH_SIMDLEN) * PEANO_WITH_SIMDLEN;
  static constexpr int NodesPerAxisIn3D = (DIMENSIONS == 3) ? NumberOfDofsPerAxis : 1;
  static constexpr int NodesPerFace     = NumberOfDofsPerAxis * NodesPerAxisIn3D;
  static constexpr int NodesPerCell     = NodesPerFace * NumberOfDofsPerAxis;
  static constexpr int SpaceTimeNodes   = NodesPerCell * NumberOfDofsPerAxis;
};

/**
 * Operator tables of one polynomial basis, packed row major with NumberOfDofsPerAxis columns.
 * The quadrature points of the reference interval double as the time nodes of the predictor.
 */
template <typename Precision>
struct exahype2::dg::ader::BasisTables final {
  const Precision* quadraturePoints{nullptr};
  const Precision* quadratureWeights{nullptr};
  const Precision* inverseQuadratureWeights{nullptr};
  const Precision* cellQuadratureWeights{nullptr};
  const Precision* inverseCellQuadratureWeights{nullptr};
  /**
   * Entry [i][j] holds the weight of node j times the derivative of basis function i at node j.
   */
  const Precision* stiffnessOperator{nullptr};
  const Precision* stiffnessOperatorTransposed{nullptr};
  /**
   * Entry [i][j] holds the derivative of basis function j at node i.
   */
  const Precision* derivativeOperator{nullptr};
  /**
   * Inverse of the temporal operator the Picard iteration solves.
   */
  const Precision* inversePicardOperator{nullptr};
  const Precision* leftFaceCoefficients{nullptr};
  const Precision* rightFaceCoefficients{nullptr};
};

/**
 * Extents and offsets of the scratch block of the nonlinear predictor. A term the solver switches
 * off has extent zero.
 */
template <
  int  NumberOfDofsPerAxis,
  int  NumberOfUnknowns,
  int  NumberOfAuxiliaryVariables,
  bool EvaluateFlux,
  bool EvaluateNonconservativeProduct,
  bool EvaluateSource,
  bool EvaluateDiffusiveFlux,
  bool EvaluateDiffusiveSource>
struct exahype2::dg::ader::NonlinearPredictorScratch final {
  using CellLayout = Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  static constexpr bool AccumulatesSource = EvaluateSource or EvaluateDiffusiveSource or EvaluateNonconservativeProduct;
  static constexpr bool NeedsStateGradient =
    EvaluateNonconservativeProduct or EvaluateDiffusiveFlux or EvaluateDiffusiveSource;

  static constexpr int SpaceTimeStateSize = CellLayout::PaddedNumberOfData * CellLayout::SpaceTimeNodes;
  static constexpr int RightHandSideSize  = CellLayout::PaddedNumberOfUnknowns * CellLayout::SpaceTimeNodes;
  static constexpr int StateGradientSize =
    NeedsStateGradient ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell * DIMENSIONS : 0;
  static constexpr int UpdatedTimeSliceSize = NumberOfDofsPerAxis * CellLayout::PaddedNumberOfUnknowns;
  static constexpr int FluxSize =
    EvaluateFlux ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell * DIMENSIONS : 0;
  static constexpr int TimeAveragedFluxSize = FluxSize;
  static constexpr int TimeAveragedSourceSize =
    AccumulatesSource ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell : 0;
  static constexpr int VolumeCoefficientsSize = NumberOfDofsPerAxis * NumberOfDofsPerAxis;

  static constexpr int SpaceTimeStateOffset     = 0;
  static constexpr int RightHandSideOffset      = SpaceTimeStateOffset + SpaceTimeStateSize;
  static constexpr int StateGradientOffset      = RightHandSideOffset + RightHandSideSize;
  static constexpr int UpdatedTimeSliceOffset   = StateGradientOffset + StateGradientSize;
  static constexpr int FluxOffset               = UpdatedTimeSliceOffset + UpdatedTimeSliceSize;
  static constexpr int TimeAveragedFluxOffset   = FluxOffset + FluxSize;
  static constexpr int TimeAveragedSourceOffset = TimeAveragedFluxOffset + TimeAveragedFluxSize;
  static constexpr int VolumeCoefficientsOffset = TimeAveragedSourceOffset + TimeAveragedSourceSize;
  static constexpr int Size                     = VolumeCoefficientsOffset + VolumeCoefficientsSize;
};

/**
 * Extents and offsets of the scratch block of the linear predictor. A term the solver switches off
 * has extent zero.
 */
template <
  int  NumberOfDofsPerAxis,
  int  NumberOfUnknowns,
  int  NumberOfAuxiliaryVariables,
  bool EvaluateNonconservativeProduct,
  bool EvaluatePointSources>
struct exahype2::dg::ader::LinearPredictorScratch final {
  using CellLayout = Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  static constexpr int StateSize             = CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell;
  static constexpr int TimeAveragedStateSize = CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell;
  static constexpr int FluxSize = DIMENSIONS * CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns;
  /**
   * The nonconservative product differentiates one line of nodes at a time.
   */
  static constexpr int StateGradientSize =
    EvaluateNonconservativeProduct ? CellLayout::PaddedNumberOfUnknowns * NumberOfDofsPerAxis : 0;
  static constexpr int PointSourceSize = EvaluatePointSources
    ? (NumberOfDofsPerAxis + 1) * CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns
    : 0;

  static constexpr int StateOffset             = 0;
  static constexpr int TimeAveragedStateOffset = StateOffset + StateSize;
  static constexpr int FluxOffset              = TimeAveragedStateOffset + TimeAveragedStateSize;
  static constexpr int StateGradientOffset     = FluxOffset + FluxSize;
  static constexpr int PointSourceOffset       = StateGradientOffset + StateGradientSize;
  static constexpr int Size                    = PointSourceOffset + PointSourceSize;
};
TARGET_DECLARE_END
