#include "Data.h"

petsc::PetscData::PetscData(int DoFs): _dofs(DoFs) {


  PetscInitialize(PETSC_NULL, PETSC_NULL, PETSC_NULL, PETSC_NULL);
  initMatrix();

}

// petsc::PetscData* petsc::PetscData::getInstance() { return instance; }

void petsc::PetscData::initMatrix(){

  //matrix size, set it to number of dofs
  PetscInt n = _dofs;

  Mat A; 
  MatCreate(PETSC_COMM_WORLD, &A);
  MatSetSizes(A, PETSC_DECIDE, PETSC_DECIDE, n, n);
  MatSetFromOptions(A);
  MatSetUp(A);

  for (PetscInt i = 0; i < n; i++){
    MatSetValue(A, i, i, 1, INSERT_VALUES);
  }
  
  //assemble
  MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);

  //print out our contents
  MatView(A, PETSC_VIEWER_STDOUT_WORLD);

}

petsc::PetscData::~PetscData(){
  //todo: destroy petsc objects, or we risk a memory leak

  // MatDestroy(&A);
  // PetscFinalize();
}
