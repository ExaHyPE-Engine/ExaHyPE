#include "petsc.h"

#include "tarch/logging/Log.h"

#include <petscsys.h>
/*
  4: #include <petscbag.h>
  5: #include <petscviewer.h>
*/


namespace {
  tarch::logging::Log _log( "petsc" );
}

void petsc::initParallelEnvironment([[maybe_unused]] int* argc, [[maybe_unused]] char*** argv) {
  PetscInitialize(argc, argv, (char *)0/*PETSC_NULLPTR*/, PETSC_NULLPTR);
  logInfo( "initParallelEnvironment(...)", "PETSc is initialised" );
}
