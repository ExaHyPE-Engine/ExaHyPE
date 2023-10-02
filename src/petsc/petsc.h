// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/tests/TestCase.h"


namespace petsc {
  /**
   * Initialise PETSc
   *
   * Should be called early throughout the main. In line with PETSc's
   * documentation, maybe as the first thing at all. As PETSc "owns"
   * MPI itself, this routine could peano4::initParallelEnvironment(),
   * but our init does a little bit more, so please call this routine
   * directly after you have invoked Peano's parallel initialisation.
   *
   * @see https://petsc.org/release/manualpages/Sys/PetscInitialize/
   * @see peano4::initParallelEnvironment()
   */
  void initParallelEnvironment([[maybe_unused]] int* argc, [[maybe_unused]] char*** argv);

  void shutdownParallelEnvironment();
}

