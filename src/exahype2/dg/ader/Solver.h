// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementRequests.h"
#include "exahype2/dg/Basis.h"
#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/Solver.h"
#include "exahype2/dg/ader/Predictor.h"
#include "exahype2/plotter/PatchPlotter.h"
#include "exahype2/terms/BoundaryConditions.h"
#include "exahype2/terms/InitialCondition.h"
#include "exahype2/terms/RefinementCriterion.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Regrid.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/plotter/Format.h"

namespace tarch::mpi {
  class ReductionRecord;
} // namespace tarch::mpi

namespace exahype2::dg::ader {
  template <typename Derived, typename StoragePrecision, typename PredictorPrecision, typename TimeAveragedPrecision, typename CorrectorPrecision, typename Limiter>
  class Solver;

  /**
   * The Limiter argument of a solver without a limiter.
   */
  struct NoLimiter final {};

  INLINE_METHOD constexpr double getCFLFactor(int order);

  template <typename SolverType, typename Storage, typename Compute>
  concept SolverConcept = exahype2::terms::HasInitialCondition<SolverType, Storage>
    and exahype2::terms::HasBoundaryConditionsWhereNeeded<SolverType, Compute>
    and exahype2::terms::HasRefinementCriterion<SolverType, Storage> and requires {
          { SolverType::DGOrder } -> std::convertible_to<int>;
          { SolverType::NumberOfUnknowns } -> std::convertible_to<int>;
          { SolverType::NumberOfAuxiliaryVariables } -> std::convertible_to<int>;
          { SolverType::PeriodicBoundaries[0] } -> std::convertible_to<bool>;
        };
} // namespace exahype2::dg::ader

/**
 * The ADER-DG solver family. Derived also declares NumberOfPointSources. Limiter is void or the
 * subcell limiter that checks every corrected cell and recomputes the rejected ones.
 */
TARGET_DECLARE
template <typename Derived, typename StoragePrecision, typename PredictorPrecision, typename TimeAveragedPrecision, typename CorrectorPrecision, typename Limiter>
class exahype2::dg::ader::Solver : public exahype2::dg::Solver<Derived, StoragePrecision, CorrectorPrecision> {
public:
  using Base = exahype2::dg::Solver<Derived, StoragePrecision, CorrectorPrecision>;

  template <typename Precision, typename Generated = Derived>
  using Quadrature = typename Generated::template Quadrature<Precision>;
  template <typename Precision, typename Generated = Derived>
  using DGMatrices = typename Generated::template DGMatrices<Precision>;

  using Base::NodesPerAxis;
  using Base::NodesPerFace;
  using Base::NumberOfData;
  using Base::UsesAdaptiveTimeStepping;

  static constexpr double PNPM       = exahype2::dg::ader::getCFLFactor(Derived::DGOrder);
  static constexpr bool   HasLimiter = not std::is_void_v<Limiter>;

  using LimiterType = std::conditional_t<HasLimiter, Limiter, exahype2::dg::ader::NoLimiter>;

  static constexpr double PicardResidualTolerance = 1e-7;

  Solver()                         = delete;
  Solver(const Solver&)            = delete;
  Solver(Solver&&)                 = delete;
  Solver& operator=(const Solver&) = delete;
  Solver& operator=(Solver&&)      = delete;
  ~Solver()                        = default;

  INLINE_METHOD Solver(
    double                                           timeStepFactor,
    const std::string&                               name,
    const std::vector<exahype2::plotter::DataField>& plottedFields
  );
  INLINE_METHOD Solver(
    double                                           timeStepFactor,
    const std::string&                               name,
    const std::vector<exahype2::plotter::DataField>& plottedFields,
    double                                           relaxationParameter,
    double                                           differencesScaling
  );

  INLINE_METHOD void allocate(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void initialise(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void performTimeStep(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void regrid(
    const peano4::CellConnectivity& cellConnectivity,
    const peano4::Regrid&           regrid,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void plot(
    const peano4::CellConnectivity& cellConnectivity,
    tarch::plotter::Format          format,
    const std::string&              outputDirectory
  );
  INLINE_METHOD void release();
  INLINE_METHOD void addLocalValuesToGlobalReduction(tarch::mpi::ReductionRecord& record);
  INLINE_METHOD void readValuesFromGlobalReduction(const tarch::mpi::ReductionRecord& record);

  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;
  [[nodiscard]] INLINE_METHOD double getTimeStepSizeFromCFL() const;
  [[nodiscard]] INLINE_METHOD double getFixedTimeStepSize() const;
  [[nodiscard]] INLINE_METHOD const LimiterType& getLimiter() const;
  [[nodiscard]] INLINE_METHOD int getNumberOfTroubledCells() const;

  template <typename Partner>
  INLINE_METHOD void coupleWith(
    Partner&                   partner,
    const std::vector<double>& fromPartnerMatrix,
    const std::vector<double>& toPartnerMatrix
  );

private:
  template <typename PartnerDerived, typename PartnerStorage, typename PartnerPredictorCompute, typename PartnerTimeAverageCompute, typename PartnerCorrectorCompute, typename PartnerLimiter>
  friend class exahype2::dg::ader::Solver;

  using LimiterSlot = std::conditional_t<HasLimiter, std::optional<Limiter>, exahype2::dg::ader::NoLimiter>;

  using Predictor = exahype2::dg::ader::Predictor<Derived, PredictorPrecision, TimeAveragedPrecision>;

  [[nodiscard]] static INLINE_METHOD exahype2::dg::BasisTables<PredictorPrecision> getBasisTables();
  [[nodiscard]] static INLINE_METHOD exahype2::dg::BasisTables<TimeAveragedPrecision> getTimeAveragedBasisTables();

  INLINE_METHOD void predict(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency               adjacency,
    double                          timeStepSize
  );
  template <typename DivergenceMarks>
  [[nodiscard]] INLINE_METHOD exahype2::dg::ader::PredictionCounts predictCells(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency               adjacency,
    double                          timeStepSize,
    const DivergenceMarks&          divergenceMarks
  );
  [[nodiscard]] INLINE_METHOD exahype2::dg::MaxEigenvalueStatistics correct(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency               adjacency,
    double                          timeStepSize
  );

  INLINE_METHOD void setCouplingMatrix(const std::vector<double>& fromPartnerMatrix);
  INLINE_METHOD void leaveTimeStepsToPartner();
  template <typename Partner>
  INLINE_METHOD void copyFaceHalvesFromPartner(
    const peano4::CellConnectivity& cellConnectivity,
    const Partner&                  partner,
    peano4::Adjacency               adjacency
  );
  INLINE_METHOD void rebuildCouplingFaces(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void reservePredictorScratch(const peano4::CellConnectivity& cellConnectivity);
  [[nodiscard]] static INLINE_METHOD int getPredictorTeamThreads();
  [[nodiscard]] static INLINE_METHOD int getPredictorThreadsPerCell();
  [[nodiscard]] static INLINE_METHOD int getNumberOfPredictorChunks(int cells);
  [[nodiscard]] static INLINE_METHOD int getNumberOfPredictorTeams(int cells);

  tarch::accelerator::Buffer<double> _pointSourceLocations{tarch::MemoryLocation::Device};

  tarch::accelerator::Buffer<PredictorPrecision> _predictorScratch{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<TimeAveragedPrecision> _predictorTimeAveragedScratch{tarch::MemoryLocation::Device};

  int _unconvergedPredictions{0};
  int _unconvergedPredictionsSlot{0};

  LimiterSlot _limiter{};

  std::function<void(const peano4::CellConnectivity&, exahype2::RefinementRequests&)> _coupledTimeStep{};
  bool                                                                                _drivenByPartner{false};

  tarch::accelerator::Buffer<int> _couplingFaces{tarch::MemoryLocation::Device};
  int                             _numberOfCouplingFaces{0};
  int                             _numberOfLocalCouplingFaces{0};

  tarch::accelerator::Buffer<CorrectorPrecision> _couplingMatrix{tarch::MemoryLocation::Device};
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/Solver.cpph"
