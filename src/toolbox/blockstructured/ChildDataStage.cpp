// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "toolbox/blockstructured/ChildDataStage.h"

/**
 * The stage is defined in the .cpph, so nothing in the core library
 * instantiates it and a mistake in a body would first surface when a solver
 * uses it. Instantiating it here compiles it along with the library, for the
 * two widths a solver may hold its patches in.
 */
template class toolbox::blockstructured::ChildDataStage<double>;
template class toolbox::blockstructured::ChildDataStage<float>;
