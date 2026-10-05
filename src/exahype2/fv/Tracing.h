// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/particles/Forces.h"
#include "exahype2/particles/ParticleSet.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::fv {
  template <int PatchSize, int NumberOfData, typename Storage>
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double interpolatePiecewiseConstant(
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const Storage* NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    int                                          unknown
  );

  template <int PatchSize, int NumberOfData, typename Storage>
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double interpolatePiecewiseLinear(
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const Storage* NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    int                                          unknown
  );

  template <int PatchSize, int NumberOfData, typename Storage, int... FieldIndices>
  INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFields(
    exahype2::particles::InterpolationScheme     scheme,
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const Storage* NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    exahype2::particles::Particle&               particle
  );

  template <int PatchSize, int NumberOfData, typename Storage, int... FieldIndices>
  INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFields(
    exahype2::particles::FieldList<FieldIndices...> fields,
    exahype2::particles::InterpolationScheme        scheme,
    const tarch::la::Vector<DIMENSIONS, double>&    centre,
    const tarch::la::Vector<DIMENSIONS, double>&    width,
    const Storage* NOALIAS                          Q,
    const tarch::la::Vector<DIMENSIONS, double>&    position,
    exahype2::particles::Particle&                  particle
  );

  template <int PatchSize>
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> clampIntoPatch(
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const tarch::la::Vector<DIMENSIONS, double>& position
  );
} // namespace exahype2::fv
TARGET_DECLARE_END

namespace exahype2::fv {
  template <int PatchSize, int NumberOfData, int NumberOfParticleData, typename Fields, typename Pool>
  struct TraceParticles;

  template <int PatchSize, int NumberOfData, int NumberOfParticleData, int... FieldIndices, typename Pool>
  INLINE_METHOD void traceParticles(
    const peano4::Partition&                                partition,
    const Pool&                                             solution,
    double                                                  timeStepSize,
    exahype2::particles::ParticleSet<NumberOfParticleData>& particles,
    const exahype2::particles::Forces&                      forces,
    const tarch::la::Vector<DIMENSIONS, double>&            domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>&            domainWidth,
    int                                                     periodicAxes
  );
} // namespace exahype2::fv

TARGET_DECLARE
/**
 * Moves one particle through the patch of its cell: Interpolates the fields of Fields at the
 * particle position, applies the forces, advances the particle by timeStepSize, reflects it at
 * the walls of the axes that are not periodic and wraps it through the periodic ones. With
 * samplesOnly the kernel stops after the interpolation.
 */
template <int PatchSize, int NumberOfData, int NumberOfParticleData, typename Fields, typename Pool>
struct exahype2::fv::TraceParticles final {
  peano4::Partition                                          partition{};
  typename Pool::ConstView                                   solution{};
  double                                                     timeStepSize{0.0};
  exahype2::particles::ParticleSetView<NumberOfParticleData> particles{};
  exahype2::particles::Forces                                forces{};
  tarch::la::Vector<DIMENSIONS, double>                      domainOffset{};
  tarch::la::Vector<DIMENSIONS, double>                      domainWidth{};
  int                                                        periodicAxes{0};
  bool                                                       samplesOnly{false};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int index, int& numberOfMovedParticles) const;
};
TARGET_DECLARE_END

#include "exahype2/fv/Tracing.cpph"
