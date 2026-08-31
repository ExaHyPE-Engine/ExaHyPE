// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackTopology.h"

bool peano4::grid::StackTopology::isInOutStack(const int stackNumber) {
  return stackNumber == static_cast<int>(BaseStack::Input);
}
