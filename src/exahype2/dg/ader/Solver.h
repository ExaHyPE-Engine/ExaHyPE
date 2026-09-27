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
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/plotter/Format.h"

namespace exahype2::dg::ader {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision, typename Limiter>
  class Solver;

  /** The limiter slot of a solver without a limiter. */
  struct NoLimiter final {};

  constexpr double getCFLFactor(int order);

  /**
   * An ADER-DG solver declares the terms every predictor and corrector sweep calls and the
   * constants the node loops and the boundary update read. Its boundary conditions run in the
   * compute precision.
   */
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
 * Advances the cells of one ADER-DG solver on the partition: The space-time predictor of every
 * cell writes its face states and face fluxes onto its own halves of its faces, the Riemann
 * problems of the faces turn them into numerical fluxes, and the corrector adds the face
 * integrals. Derived is the generated solver class as for exahype2::dg::Solver and adds
 * NumberOfPointSources, the number of point sources of the scheme. Limiter is void or the
 * subcell limiter that judges every corrected candidate and recomputes the rejected cells.
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
   * iterates falls below this value.
   */
  static constexpr double PicardResidualTolerance = 1e-7;

  Solver()                         = delete;
  Solver(const Solver&)            = delete;
  Solver(Solver&&)                 = delete;
  Solver& operator=(const Solver&) = delete;
  Solver& operator=(Solver&&)      = delete;
  ~Solver()                        = default;

  Solver(
    double                                           timeStepFactor,
    const std::string&                               name,
    const std::vector<exahype2::plotter::DataField>& plottedFields
  );
  Solver(
    double                                           timeStepFactor,
    const std::string&                               name,
    const std::vector<exahype2::plotter::DataField>& plottedFields,
    double                                           relaxationParameter,
    double                                           differencesScaling
  );

  void allocate(const peano4::Partition& partition);
  void initialise(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  void performTimeStep(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  void regrid(const peano4::Partition& partition, const peano4::Regrid& regrid, exahype2::RefinementRequests& requests);
  void plot(const peano4::Partition& partition, tarch::plotter::Format format, const std::string& outputDirectory);
  void release();

  [[nodiscard]] tarch::AllocatedBytes getAllocatedBytes() const;
  [[nodiscard]] double getTimeStepSizeFromCFL() const;
  [[nodiscard]] double getFixedTimeStepSize() const;
  [[nodiscard]] const LimiterType& getLimiter() const;
  [[nodiscard]] int getNumberOfTroubledCells() const;

  template <typename Partner>
  void coupleWith(
    Partner&                   partner,
    const std::vector<double>& fromPartnerMatrix,
    const std::vector<double>& toPartnerMatrix
  );

private:
  /** The coupled time step drives the partner, a solver of another order or precision, through its private phases. */
  template <typename PartnerDerived, typename PartnerStorage, typename PartnerCompute, typename PartnerLimiter>
  friend class exahype2::dg::ader::Solver;

  using LimiterSlot = std::conditional_t<HasLimiter, std::optional<Limiter>, exahype2::dg::ader::NoLimiter>;

  // Predictor::ScratchSize reads the PDE terms of Derived, so only function bodies name it:
  // Derived is complete only once a function body is instantiated.
  using Predictor = exahype2::dg::ader::Predictor<Derived, ComputePrecision>;

  [[nodiscard]] static exahype2::dg::BasisTables<ComputePrecision> getBasisTables();

  void predict(const peano4::Partition& partition, double timeStepSize);
  template <typename DivergenceMarks>
  [[nodiscard]] int predictCells(
    const peano4::Partition& partition,
    double                   timeStepSize,
    const DivergenceMarks&   divergenceMarks
  );
  [[nodiscard]] exahype2::dg::MaxEigenvalueStatistics correct(
    const peano4::Partition& partition,
    peano4::Dependence       dependence,
    double                   timeStepSize
  );

  void setCouplingMatrix(const std::vector<double>& fromPartnerMatrix);
  void leaveTimeStepsToPartner();
  template <typename Partner>
  void copyFaceHalvesFromPartner(
    const peano4::Partition& partition,
    const Partner&           partner,
    peano4::Dependence       dependence
  );
  void rebuildCouplingFaces(const peano4::Partition& partition);
  void reservePredictorScratch(const peano4::Partition& partition);

  /** DIMENSIONS coordinates per point source, in the memory location of the kernels. */
  tarch::accelerator::Buffer<double> _pointSourceLocations{tarch::MemoryLocation::Device};

  /** One predictor scratch block and one Picard block per scratch chunk of the predictor dispatch. */
  tarch::accelerator::Buffer<ComputePrecision> _predictorScratch{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<ComputePrecision> _picardScratch{tarch::MemoryLocation::Device};

  LimiterSlot _limiter{};

  /** Runs the time step of this solver and of its coupling partner; empty without a partner. */
  std::function<void(const peano4::Partition&, exahype2::RefinementRequests&)> _coupledTimeStep{};
  bool                                                                         _drivenByPartner{false};

  /**
   * Two entries per coupling face: The face and the side of the cell this solver owns. The local
   * faces come first, then the faces whose partner half an exchange fills.
   */
  tarch::accelerator::Buffer<int> _couplingFaces{tarch::MemoryLocation::Device};
  int                             _numberOfCouplingFaces{0};
  int                             _numberOfLocalCouplingFaces{0};

  /** Maps the face nodes of the partner onto the face nodes of this solver, partner node major. */
  tarch::accelerator::Buffer<ComputePrecision> _couplingMatrix{tarch::MemoryLocation::Device};
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/Solver.cpph"
