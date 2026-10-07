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
#include "peano4/Partition.h"
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
  template <typename Derived, typename StoragePrecision, typename ComputePrecision, typename Limiter>
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
template <typename Derived, typename StoragePrecision, typename ComputePrecision, typename Limiter>
class exahype2::dg::ader::Solver : public exahype2::dg::Solver<Derived, StoragePrecision, ComputePrecision> {
public:
  using Base = exahype2::dg::Solver<Derived, StoragePrecision, ComputePrecision>;

  // The defaulted parameter defers the lookup in Derived to the use, where Derived is complete.
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
  /**
   * The Picard iteration of the predictor stops once the Euclidean distance between two
   * iterates falls below this value. A cell stops at the rounding distance of its iterates in
   * ComputePrecision when that distance is larger.
   */
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

  INLINE_METHOD void allocate(const peano4::Partition& partition);
  INLINE_METHOD void initialise(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  INLINE_METHOD void performTimeStep(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  INLINE_METHOD void regrid(
    const peano4::Partition&      partition,
    const peano4::Regrid&         regrid,
    exahype2::RefinementRequests& requests
  );
  INLINE_METHOD void plot(
    const peano4::Partition& partition,
    tarch::plotter::Format   format,
    const std::string&       outputDirectory
  );
  INLINE_METHOD void release();
  INLINE_METHOD void contributeTo(tarch::mpi::ReductionRecord& record);
  INLINE_METHOD void adoptFrom(const tarch::mpi::ReductionRecord& record);

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
  /**
   * The coupled time step calls the private phases of a partner of another order or precision.
   */
  template <typename PartnerDerived, typename PartnerStorage, typename PartnerCompute, typename PartnerLimiter>
  friend class exahype2::dg::ader::Solver;

  using LimiterSlot = std::conditional_t<HasLimiter, std::optional<Limiter>, exahype2::dg::ader::NoLimiter>;

  // Predictor::ScratchSize reads the PDE terms of Derived. Only function bodies name it. Derived is
  // complete only once a function body is instantiated.
  using Predictor = exahype2::dg::ader::Predictor<Derived, ComputePrecision>;

  [[nodiscard]] static INLINE_METHOD exahype2::dg::BasisTables<ComputePrecision> getBasisTables();

  INLINE_METHOD void predict(const peano4::Partition& partition, peano4::Adjacency adjacency, double timeStepSize);
  template <typename DivergenceMarks>
  [[nodiscard]] INLINE_METHOD exahype2::dg::ader::PredictionCounts predictCells(
    const peano4::Partition& partition,
    peano4::Adjacency        adjacency,
    double                   timeStepSize,
    const DivergenceMarks&   divergenceMarks
  );
  [[nodiscard]] INLINE_METHOD exahype2::dg::MaxEigenvalueStatistics correct(
    const peano4::Partition& partition,
    peano4::Adjacency        adjacency,
    double                   timeStepSize
  );

  INLINE_METHOD void setCouplingMatrix(const std::vector<double>& fromPartnerMatrix);
  INLINE_METHOD void leaveTimeStepsToPartner();
  template <typename Partner>
  INLINE_METHOD void copyFaceHalvesFromPartner(
    const peano4::Partition& partition,
    const Partner&           partner,
    peano4::Adjacency        adjacency
  );
  INLINE_METHOD void rebuildCouplingFaces(const peano4::Partition& partition);
  INLINE_METHOD void reservePredictorScratch(const peano4::Partition& partition);
  [[nodiscard]] static INLINE_METHOD int getPredictorTeamThreads();
  [[nodiscard]] static INLINE_METHOD int getPredictorThreadsPerCell();
  [[nodiscard]] static INLINE_METHOD int getNumberOfPredictorChunks(int cells);
  [[nodiscard]] static INLINE_METHOD int getNumberOfPredictorTeams(int cells);

  tarch::accelerator::Buffer<double> _pointSourceLocations{tarch::MemoryLocation::Device};

  /**
   * One predictor scratch block per scratch chunk of the predictor dispatch.
   */
  tarch::accelerator::Buffer<ComputePrecision> _predictorScratch{tarch::MemoryLocation::Device};

  /**
   * The number of cells of this rank whose Picard iteration stopped at its sweep limit since the
   * last reduction.
   */
  int _unconvergedPredictions{0};
  int _unconvergedPredictionsSlot{0};

  LimiterSlot _limiter{};

  /**
   * Runs the time steps of this solver and its coupling partner. It is empty without a partner.
   */
  std::function<void(const peano4::Partition&, exahype2::RefinementRequests&)> _coupledTimeStep{};
  bool                                                                         _drivenByPartner{false};

  /**
   * Two entries per coupling face: The face and the side of this solver's cell. The local faces
   * come first, then the remote ones.
   */
  tarch::accelerator::Buffer<int> _couplingFaces{tarch::MemoryLocation::Device};
  int                             _numberOfCouplingFaces{0};
  int                             _numberOfLocalCouplingFaces{0};

  /**
   * The matrix that maps the face nodes of the partner onto those of this solver, with the partner
   * node as the major index.
   */
  tarch::accelerator::Buffer<ComputePrecision> _couplingMatrix{tarch::MemoryLocation::Device};
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/Solver.cpph"
