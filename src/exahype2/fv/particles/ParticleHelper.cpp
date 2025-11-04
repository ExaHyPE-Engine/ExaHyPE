#include "ParticleHelper.h"

// ================================================================================================
// Global Configuration Variables (for domain boundaries and periodicity)
// ================================================================================================

tarch::la::Vector<DIMENSIONS, double> exahype2::fv::particles::utils::domainOffset;
tarch::la::Vector<DIMENSIONS, double> exahype2::fv::particles::utils::domainSize;
tarch::la::Vector<DIMENSIONS, int>    exahype2::fv::particles::utils::isPeriodic;

/**
 * Set global domain parameters shared by all particles.
 *
 * @param offset   Lower corner of the domain (domain origin)
 * @param size     Extent/length of the domain in each dimension
 * @param periodic Flags indicating whether each dimension is periodic (1) or not (0)
 */
void exahype2::fv::particles::utils::setGlobalParameters(
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& size,
  const tarch::la::Vector<DIMENSIONS, int>&    periodic
) {
  domainOffset = offset;
  domainSize   = size;
  isPeriodic   = periodic;
}
