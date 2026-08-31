// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::fv {
  void mapInnerNeighbourVoxelAlongBoundaryOntoAuxiliaryVariable(
    double* NOALIAS Q,
    int             unknowns,
    int             auxiliaryVariables,
    int             numberOfVolumesPerAxisInPatch,
    int             fromIndex,
    int             toIndex
  );
} // namespace exahype2::fv
