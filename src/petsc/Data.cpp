#include "Data.h"

petsc::PetscData::PetscData(int argc, char** argv) {
  PetscInitialize(&argc, &argv, PETSC_NULL, PETSC_NULL);
  std::cout << "initialised petsc \n\n";
}

petsc::PetscData::PetscData(){
  PetscInitialize(PETSC_NULL, PETSC_NULL, PETSC_NULL, PETSC_NULL);
  std::cout << "used default (null) petsc initialiser\n\n";
}

void petsc::PetscData::initMatrix(){

  //matrix size, set it to number of dofs
  PetscInt n = _dofs;
  PetscInt val = 1;

  MatCreate(PETSC_COMM_WORLD, &A);
  MatSetSizes(A, PETSC_DECIDE, PETSC_DECIDE, n, n);
  MatSetFromOptions(A);
  MatSetUp(A);

  for (PetscInt i = 0; i < n; i++){
    MatSetValue(A, i, i, val, INSERT_VALUES);
  }
  
  //assemble
  MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);

  //print out our contents
  MatView(A, PETSC_VIEWER_STDOUT_WORLD);

}

// void petsc::PetscData::initVectors( std::vector< std::pair<double, double> > valuesAndRhs ){
void petsc::PetscData::initVectors(){
  PetscInt n = _dofs;
  PetscInt val = 1;

  //create both vectors
  VecCreate(PETSC_COMM_WORLD, &x);
  VecCreate(PETSC_COMM_WORLD, &b);

  VecSetSizes(x, PETSC_DECIDE, n);
  VecSetSizes(b, PETSC_DECIDE, n);

  VecSetFromOptions(x);
  VecSetFromOptions(b);

  VecSetUp(x);
  VecSetUp(b);

  for (PetscInt i = 0; i < n; i++ ){
    VecSetValue(x, i, val, ADD_VALUES);
    VecSetValue(b, i, val, ADD_VALUES);
  }

  //assemble both
  VecAssemblyBegin(x);
  VecAssemblyEnd(x);
  VecAssemblyBegin(b);
  VecAssemblyEnd(b);

  // VecView(x, PETSC_VIEWER_STDOUT_WORLD);
  // VecView(b, PETSC_VIEWER_STDOUT_WORLD);
}

petsc::PetscData::~PetscData(){
  //destroy petsc objects, or else we get a memory leak

  //COMMENTING OUT FOR NOW AS IT CAUSES PETSC ERROR
  // MatDestroy(&A);
  // PetscFinalize();

  // std::cout << "destroyed objects\n\n";
}

void petsc::PetscData::setDoFs(int dofs){
  _dofs = dofs;
}
