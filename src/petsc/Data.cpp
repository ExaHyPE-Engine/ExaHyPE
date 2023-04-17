#include "Data.h"

petsc::PetscData::PetscData(int argc, char** argv) {
  std::cout << "initialised petsc \n\n";
}

petsc::PetscData::PetscData(){
  std::cout << "used default (null) petsc initialiser\n\n";
}

void petsc::PetscData::initMatrix(){
  PetscInitialize(PETSC_NULL, PETSC_NULL, PETSC_NULL, PETSC_NULL);

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
  // MatView(A, PETSC_VIEWER_STDOUT_WORLD);

}

void petsc::PetscData::initVectors( std::vector< std::pair<double, double> > valuesAndRhs ){
  PetscInt n = _dofs;

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
    std::pair<double, double> localVal = valuesAndRhs.at(i);
    VecSetValue(x, i, localVal.first, ADD_VALUES);
    VecSetValue(b, i, localVal.second, ADD_VALUES);
  }

  //assemble both
  VecAssemblyBegin(x);
  VecAssemblyEnd(x);
  VecAssemblyBegin(b);
  VecAssemblyEnd(b);

  //convert our newly initialised vector to a standard vector
  convertToStdVector();
}


void petsc::PetscData::setDoFs(int dofs){
  _dofs = dofs;
}

void petsc::PetscData::convertToStdVector(){
  //get size of vec
  PetscInt size;
  VecGetSize(x, &size);

  //make a temp array to hold the values of the vec
  const PetscScalar* petscArray;

  //get a read-only ptr to the petsc array
  VecGetArrayRead(x, &petscArray);

  //TODO - work out why _xAsStdVector[i] = petscArray[i]; causes segfault
  for (int i = 0; i < size; i++) {
    _xAsStdVector.push_back( petscArray[i] );
  }

  //return the read-only ptr to petsc
  VecRestoreArrayRead(x, &petscArray);

}

std::vector<double> petsc::PetscData::getX() const{
  return _xAsStdVector;
}

double petsc::PetscData::getX(int i) const{
  //check if we have populated it yet
  if ( !_xAsStdVector.size() )
    return -1;
  return _xAsStdVector[i];
}

petsc::PetscData::~PetscData(){
  //destroy petsc objects, or else we get a memory leak

  //COMMENTING OUT FOR NOW AS IT CAUSES PETSC ERROR
  // MatDestroy(&A);
  // PetscFinalize();

  // std::cout << "destroyed objects\n\n";
}
