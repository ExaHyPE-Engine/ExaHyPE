#include "Data.h"

petsc::PetscData::PetscData(int argc, char** argv) {
  std::cout << "initialised petsc \n\n";
}

petsc::PetscData::PetscData(){
  std::cout << "used default (null) petsc initialiser\n";
}

void petsc::PetscData::initMatrixAndVectors( int dofs ){
  setDoFs( dofs );
  // init petsc
  PetscInitialize(PETSC_NULLPTR, PETSC_NULLPTR, PETSC_NULLPTR, PETSC_NULLPTR);

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
}

//storing doubles to be placed into A
void petsc::PetscData::insertDoubleAtIndexij( int i, int j, double val ){
  MatSetValue(_A, i, j, val, INSERT_VALUES);
}

void petsc::PetscData::incrementDoubleAtIndexij( int i, int j, double val ){
  MatSetValue(_A, i, j, val, ADD_VALUES);
}

void petsc::PetscData::insertValueAndRHS(double val, double rhs, int index){
  // insert "val" into _x 
  VecSetValue(_x, index, val, INSERT_VALUES);

  // insert "rhs" into _b
  VecSetValue(_b, index, rhs, INSERT_VALUES);
}


void petsc::PetscData::assemble(){
  
  //assemble matrices
  MatAssemblyBegin(_A,MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(_A,MAT_FINAL_ASSEMBLY);

  //assemble vectors
  VecAssemblyBegin(_x);
  VecAssemblyEnd(_x);
  VecAssemblyBegin(_b);
  VecAssemblyEnd(_b);

  //print out our contents
  // MatView(_A, PETSC_VIEWER_STDOUT_WORLD);

}

void petsc::PetscData::petscSolve(){
  std::cout << "\n\tsolving stage not implemented\n\n";
  MatMult(_A, _x, _b);
  std::cout << "matrix mult works fine\n\n";
}


void petsc::PetscData::setDoFs(int dofs){
  _dofs = dofs;
}

void petsc::PetscData::convertToStdVector(){
  //get size of vec
  PetscInt size;
  VecGetSize(_x, &size);

  _xAsStdVector.resize(size);

  //temp array to hold the values 
  PetscScalar y[size];

  //temp array to hold the indices we request
  PetscInt ix[size];
  for (int i=0; i<size; i++)
    ix[i] = i;

  //ask petsc
  VecGetValues( _x, size, ix, y );

  //put these into _xAsStdVector
  _xAsStdVector.insert( _xAsStdVector.begin(), &y[0], &y[size] );

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
