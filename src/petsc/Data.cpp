#include "Data.h"

petsc::PetscData::PetscData(int argc, char** argv) {
  std::cout << "initialised petsc \n\n";
}

petsc::PetscData::PetscData(){
  std::cout << "used default (null) petsc initialiser\n\n";
}

void petsc::PetscData::initMatrixAndVectors( int dofs ){
  setDoFs( dofs );
  // init petsc
  PetscInitialize(PETSC_NULLPTR, PETSC_NULLPTR, PETSC_NULLPTR, PETSC_NULLPTR);

  //matrix size, set it to number of dofs
  PetscInt n = _dofs;

  //reserve enough space in the temporary vector to hold the values of A
  // _AasStdVector.resize( _dofs * _dofs );

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


void petsc::PetscData::assembleA(){

  // std::cout << "\n\tNumber of elements is " << _AasStdMap.size() << "\n\n";

  // for (PetscInt i = 0; i < 100; i++){
  //   for (PetscInt j = 0; j < 100; j++){
  //     PetscScalar val = getTempDoubleAtIndexij(i,j);
  //     MatSetValue(_A, i, j, val, INSERT_VALUES);
  //   }
  // }
  
  //assemble
  MatAssemblyBegin(_A,MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(_A,MAT_FINAL_ASSEMBLY);

  std::cout << "\nsucessful matrix construction\n\n";

  //print out our contents
  // MatView(_A, PETSC_VIEWER_STDOUT_WORLD);

}

// void petsc::PetscData::assembleMatrix(){



// }

// void petsc::PetscData::assembleVectors( std::vector< std::pair<double, double> > valuesAndRhs ){
//   PetscInt n = _dofs;


//   for (PetscInt i = 0; i < n; i++ ){
//     std::pair<double, double> localVal = valuesAndRhs.at(i);
//     VecSetValue(x, i, localVal.first, ADD_VALUES);
//     VecSetValue(b, i, localVal.second, ADD_VALUES);
//   }

//   //assemble both
//   VecAssemblyBegin(x);
//   VecAssemblyEnd(x);
//   VecAssemblyBegin(b);
//   VecAssemblyEnd(b);

//   //convert our newly initialised vector to a standard vector
//   convertToStdVector();
// }


void petsc::PetscData::setDoFs(int dofs){
  _dofs = dofs;
}

void petsc::PetscData::convertToStdVector(){
  //get size of vec
  PetscInt size;
  VecGetSize(_x, &size);

  //make a temp array to hold the values of the vec
  const PetscScalar* petscArray;

  //get a read-only ptr to the petsc array
  VecGetArrayRead(_x, &petscArray);

  //TODO - work out why _xAsStdVector[i] = petscArray[i]; causes segfault
  for (int i = 0; i < size; i++) {
    _xAsStdVector.push_back( petscArray[i] );
  }

  //return the read-only ptr to petsc
  VecRestoreArrayRead(_x, &petscArray);

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
