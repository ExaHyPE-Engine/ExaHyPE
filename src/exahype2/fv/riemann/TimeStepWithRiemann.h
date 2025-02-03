// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Enumerator.h"
#include "LoopBody.h"

#include "exahype2/Solver.h"
#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

#include "peano4/utils/Loop.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#include <algorithm>
#include <cstring>

namespace exahype2::fv::riemann {
  template <
    class SolverType,
    int  HaloSize,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateEigenvalues,
    bool EvaluateSource,
    bool EvaluateRiemann,
    bool EvaluateMaximumEigenvalueAfterTimeStep>
  static inline void timeStepWithRiemann(SolverType& solver, CellData<double, double>& patchData) InlineMethod;

  template <
    class SolverType,
    int  HaloSize,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateEigenvalues,
    bool EvaluateSource,
    bool EvaluateRiemann,
    bool EvaluateMaximumEigenvalueAfterTimeStep>
  static inline void timeStepWithRiemann(CellData<double, double>& patchData, peano4::utils::LoopPlacement loopPlacement) InlineMethod;
} // namespace exahype2::fv::riemann

#include "TimeStepWithRiemann.cpph"
