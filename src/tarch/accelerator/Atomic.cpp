// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Atomic.h"

template int  tarch::accelerator::updateAtomicMin(int& target, int value);
template int  tarch::accelerator::updateAtomicMax(int& target, int value);
template int  tarch::accelerator::incrementAtomic(int& target);
template int  tarch::accelerator::addAtomic(int& target, int value);
template void tarch::accelerator::storeAtomic(int& target, int value);

template double tarch::accelerator::updateAtomicMin(double& target, double value);
template double tarch::accelerator::updateAtomicMax(double& target, double value);
template double tarch::accelerator::incrementAtomic(double& target);
template double tarch::accelerator::addAtomic(double& target, double value);
template void   tarch::accelerator::storeAtomic(double& target, double value);
