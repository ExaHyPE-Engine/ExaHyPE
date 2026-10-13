// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/particles/Forces.h"
#include "exahype2/particles/ParticleSet.h"
#include "peano4/CellConnectivity.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg {
  template <int NodesPerAxis, int NumberOfData, typename Storage, int... FieldIndices>
  INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFields(
    exahype2::particles::InterpolationScheme     scheme,
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const double* NOALIAS                        quadratureNodes,
    const Storage* NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    exahype2::particles::Particle&               particle
  );

  template <int NodesPerAxis, int NumberOfData, typename Storage, int... FieldIndices>
  INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFields(
    exahype2::particles::FieldList<FieldIndices...> fields,
    exahype2::particles::InterpolationScheme        scheme,
    const tarch::la::Vector<DIMENSIONS, double>&    centre,
    const tarch::la::Vector<DIMENSIONS, double>&    width,
    const double* NOALIAS                           quadratureNodes,
    const Storage* NOALIAS                          Q,
    const tarch::la::Vector<DIMENSIONS, double>&    position,
    exahype2::particles::Particle&                  particle
  );

  template <int... Fields>
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD bool listsValue(
    exahype2::particles::FieldList<Fields...> fields,
    int                                       value
  );

  template <int NodesPerAxis, typename Storage>
  INLINE_METHOD GPU_CALLABLE_METHOD double evaluateFacePolynomial(
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellWidth,
    int                                          normal,
    const double* NOALIAS                        quadratureNodes,
    int                                          valuesPerNode,
    const Storage* NOALIAS                       values,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    int                                          value
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

namespace exahype2::dg {
  template <int NodesPerAxis, int NumberOfData, int NumberOfParticleData, typename Fields, typename Pool>
  struct TraceFaceValues;

  template <int NodesPerAxis, int NumberOfData, int NumberOfParticleData, typename Fields, typename Pool>
  struct TraceParticles;

  template <int NodesPerAxis, int NumberOfData, int NumberOfParticleData, int... DifferenceFields, typename Pool>
  INLINE_METHOD void traceFaceValues(
    const peano4::CellConnectivity&                         cellConnectivity,
    const Pool&                                             faceStates,
    const double* NOALIAS                                   quadratureNodes,
    exahype2::particles::ParticleSet<NumberOfParticleData>& particles,
    int                                                     normal,
    double                                                  position
  );

  template <int NodesPerAxis, int NumberOfData, int NumberOfParticleData, int... FieldIndices, typename Pool>
  INLINE_METHOD void traceParticles(
    const peano4::CellConnectivity&                         cellConnectivity,
    const Pool&                                             solution,
    const double* NOALIAS                                   quadratureNodes,
    double                                                  timeStepSize,
    exahype2::particles::ParticleSet<NumberOfParticleData>& particles,
    const exahype2::particles::Forces&                      forces,
    const tarch::la::Vector<DIMENSIONS, double>&            domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>&            domainWidth,
    int                                                     periodicAxes
  );
} // namespace exahype2::dg

TARGET_DECLARE
/**
 * Writes the face values at the position of one particle on the plane into the particle. A field
 * of Fields receives the jump across the face, every other field the average of both sides.
 */
template <int NodesPerAxis, int NumberOfData, int NumberOfParticleData, typename Fields, typename Pool>
struct exahype2::dg::TraceFaceValues final {
  peano4::CellConnectivity                                   cellConnectivity{};
  typename Pool::ConstView                                   faceStates{};
  const double*                                              quadratureNodes{nullptr};
  exahype2::particles::ParticleSetView<NumberOfParticleData> particles{};
  int                                                        normal{0};
  double                                                     position{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int index) const;
};

/**
 * Interpolates the fields of Fields to one particle and moves the particle by timeStepSize. With
 * samplesOnly the kernel stops after the interpolation.
 */
template <int NodesPerAxis, int NumberOfData, int NumberOfParticleData, typename Fields, typename Pool>
struct exahype2::dg::TraceParticles final {
  peano4::CellConnectivity                                   cellConnectivity{};
  typename Pool::ConstView                                   solution{};
  const double*                                              quadratureNodes{nullptr};
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

#include "exahype2/dg/Tracing.cpph"
