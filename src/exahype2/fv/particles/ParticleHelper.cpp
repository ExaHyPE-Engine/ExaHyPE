// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleHelper.h"

tarch::la::Vector<DIMENSIONS, double> exahype2::fv::particles::utils::domainOffset;
tarch::la::Vector<DIMENSIONS, double> exahype2::fv::particles::utils::domainSize;
tarch::la::Vector<DIMENSIONS, int>    exahype2::fv::particles::utils::isPeriodic;

/**
 * Overwrites the domain corner, the domain extent and the periodicity every particle reads.
 *
 * @param offset Lower corner of the domain.
 * @param size Extent of the domain along each axis.
 * @param periodic Carries 1 for a periodic axis and 0 for a non-periodic one.
 */
void exahype2::fv::particles::utils::setGlobalParameters(
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& size,
  const tarch::la::Vector<DIMENSIONS, int>&    periodic
) {
  exahype2::fv::particles::utils::domainOffset = offset;
  exahype2::fv::particles::utils::domainSize   = size;
  exahype2::fv::particles::utils::isPeriodic   = periodic;
}
