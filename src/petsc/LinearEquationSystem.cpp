#include "LinearEquationSystem.h"

#include <petscmat.h>
#include <petscvec.h>


tarch::logging::Log  petsc::LinearEquationSystem::_log( "petsc::LinearEquationSystem" );


petsc::LinearEquationSystem::LinearEquationSystem():
  _dofs(-1) {
}


petsc::LinearEquationSystem::~LinearEquationSystem() {
  if (_dofs>0) {
    //tidy up _A
    MatDestroy(&_A);

    //tidy up _x and _b
    VecDestroy(&_x);
    VecDestroy(&_b);
  }
}


void petsc::LinearEquationSystem::printMatrix() {
  MatView(_A, PETSC_VIEWER_STDOUT_WORLD);
}

void petsc::LinearEquationSystem::printMatrix(std::string filename) {
  PetscViewer viewer;
  PetscViewerASCIIOpen(PETSC_COMM_WORLD, filename.c_str(), &viewer);
  PetscViewerPushFormat(viewer, PETSC_VIEWER_ASCII_XML);
  MatView(_A, viewer);
  PetscViewerDestroy(&viewer);
}

void petsc::LinearEquationSystem::printRhs() {
  VecView(_b, PETSC_VIEWER_STDOUT_WORLD);
}

void petsc::LinearEquationSystem::printRhs(std::string filename) {
  PetscViewer viewer;
  PetscViewerASCIIOpen(PETSC_COMM_WORLD, filename.c_str(), &viewer);
  PetscViewerPushFormat(viewer, PETSC_VIEWER_ASCII_XML);
  VecView(_b, viewer);
  PetscViewerDestroy(&viewer);
}
void petsc::LinearEquationSystem::printX() {
  VecView(_x, PETSC_VIEWER_STDOUT_WORLD);
}

void petsc::LinearEquationSystem::printX(std::string filename) {
  PetscViewer viewer;
  PetscViewerASCIIOpen(PETSC_COMM_WORLD, filename.c_str(), &viewer);
  PetscViewerPushFormat(viewer, PETSC_VIEWER_ASCII_XML);
  VecView(_x, viewer);
  PetscViewerDestroy(&viewer);
}

void petsc::LinearEquationSystem::init( int dofs ){
  logTraceInWith1Argument( "initMatrixAndVectors()", dofs );

  _dofs = dofs;

  //matrix size, set it to number of dofs
  PetscInt n = _dofs;

  MatCreate(PETSC_COMM_WORLD, &_A);
  MatSetSizes(_A, PETSC_DECIDE, PETSC_DECIDE, n, n);
  MatSetFromOptions(_A);
  MatSetUp(_A);

  //next, do same with the two vectors...
  //create both vectors
  VecCreate(PETSC_COMM_WORLD, &_x);
  VecCreate(PETSC_COMM_WORLD, &_b);

  VecSetSizes(_x, PETSC_DECIDE, n);
  VecSetSizes(_b, PETSC_DECIDE, n);

  VecSetFromOptions(_x);
  VecSetFromOptions(_b);

  VecSetUp(_x);
  VecSetUp(_b);

  logTraceOut( "initMatrixAndVectors()" );
}


int petsc::LinearEquationSystem::getDoFs() const{
  return _dofs;
}


double petsc::LinearEquationSystem::get(int i) const {
  assertion1( i>=0, i );

  //check if we have populated it yet
  PetscInt    ix[] = {i};
  PetscScalar result[1];
  VecGetValues(
    _x,
    1, // size,
    ix,
    result
  );
  return result[0];
}


void petsc::LinearEquationSystem::insert(int row, int col, double val){
  MatSetValue(_A, row, col, val, INSERT_VALUES);
}


//storing doubles to be placed into A
void petsc::LinearEquationSystem::increment( int row, int col, double val ){
  MatSetValue(_A, row, col, val, ADD_VALUES);
}


void petsc::LinearEquationSystem::insert(int row, double val){
  // insert "val" into _x 
  VecSetValue(_x, row, 0.0, INSERT_VALUES);

  // insert "rhs" into _b
  VecSetValue(_b, row, val, INSERT_VALUES);
}


void petsc::LinearEquationSystem::increment(int row, double val){
  // insert "val" into _x
  VecSetValue(_x, row, 0.0, INSERT_VALUES);

  // insert "rhs" into _b
  VecSetValue(_b, row, val, ADD_VALUES);
}


void petsc::LinearEquationSystem::solve(){
  //assemble matrices
  MatAssemblyBegin(_A,MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(_A,MAT_FINAL_ASSEMBLY);

  //assemble vectors
  VecAssemblyBegin(_x);
  VecAssemblyEnd(_x);
  VecAssemblyBegin(_b);
  VecAssemblyEnd(_b);

  // for now, we stuff everything into this method
  Vec         u; // exact soln
  KSP         ksp;     /* KSP context */
  PC          pc;      /* PC context */
  PetscReal   norm;    /* norm of solution error */

  /*
     Create linear solver context
  */
  KSPCreate(PETSC_COMM_WORLD, &ksp);

  /*
     Set operators. Here the matrix that defines the linear system
     also serves as the preconditioning matrix.
  */
  KSPSetOperators(ksp, _A, _A);
  KSPGetPC(ksp, &pc);

  //set runtime options
  KSPSetFromOptions(ksp);

  PCSetType(pc, PCNONE); //no preconditioner
  KSPSetType(ksp, KSPGMRES);
  KSPSetUp(ksp);
  KSPSetTolerances(ksp, 1.e-7, PETSC_DEFAULT, PETSC_DEFAULT, PETSC_DEFAULT);

  //solve against the "exact" solution u
  KSPSolve(ksp, _b, _x);

  KSPConvergedReason reason;
  KSPGetConvergedReason(ksp, &reason);

  PetscInt its;
  KSPGetIterationNumber(ksp, &its);
  logInfo("petscSolve()", "number of iterations was " << (int)its);
  logInfo("petscSolve()", "converged for reason " << KSPConvergedReasons[reason]);

  //tidy up temp objects
  KSPDestroy(&ksp);

}
