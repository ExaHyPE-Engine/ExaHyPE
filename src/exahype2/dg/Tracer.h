// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "DGUtils.h"

#include "peano4/Globals.h"
#include "peano4/datamanagement/CellMarker.h"

namespace exahype2::dg {
  template <typename Particle, int SourceIndex, int DestIndex>
  void projectValueOntoParticle(
    const peano4::datamanagement::CellMarker& marker,
    int                                       order,
    const double* NOALIAS                     QuadratureNodes1d,
    int                                       unknownsPerDoF,
    const double* NOALIAS                     Q,
    Particle&                                 particle
  );

  template <typename Particle, typename QStoreType>
  void projectAllValuesOntoParticle(
    const peano4::datamanagement::CellMarker& marker,
    int                                       order,
    const double* NOALIAS                     QuadratureNodes1d,
    int                                       unknownsPerDoF,
    const QStoreType* NOALIAS                 Q,
    Particle&                                 particle
  );
} // namespace exahype2::dg

#include "exahype2/dg/Tracer.cpph"
