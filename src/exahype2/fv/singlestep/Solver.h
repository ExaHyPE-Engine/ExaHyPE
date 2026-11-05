// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/Solver.h"

namespace exahype2::fv::singlestep {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class Solver;
} // namespace exahype2::fv::singlestep

/**
 * The finite-volume solver family that advances every volume in one step from the volumes around
 * it, such as Godunov and MUSCL-Hancock.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::fv::singlestep::Solver : public exahype2::fv::Solver<Derived, StoragePrecision, ComputePrecision> {
public:
  using Base = exahype2::fv::Solver<Derived, StoragePrecision, ComputePrecision>;

  INLINE_METHOD Solver(
    double                                    timeStepFactor,
    std::string                               name,
    std::vector<exahype2::plotter::DataField> plottedFields
  );

  template <typename RuntimeParameters>
  INLINE_METHOD void performTimeStep(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests,
    const RuntimeParameters&        parameters
  );
  template <typename RuntimeParameters>
  INLINE_METHOD void plot(
    const peano4::CellConnectivity& cellConnectivity,
    tarch::plotter::Format          format,
    const std::string&              outputDirectory,
    const RuntimeParameters&        parameters
  );

private:
  template <typename RuntimeParameters>
  [[nodiscard]] INLINE_METHOD double updatePatches(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency               adjacency,
    double                          timeStepSize,
    const RuntimeParameters&        parameters
  );
};

#include "exahype2/fv/singlestep/Solver.cpph"
