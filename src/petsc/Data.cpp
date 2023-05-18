#include "Data.h"
static constexpr double pi = 3.14159265358979323846264338328;
static constexpr double oneOver2p2 = 1.0 / (2 * pi * pi);

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

// src/ksp/ksp/tutorials/ex7.c
void petsc::PetscData::petscSolve(){
  // for now, we stuff everything into this method
  // Vec         x, b, u; /* approx solution, RHS, exact solution */
  Vec         u; // exact soln
  // Mat         A;       /* linear system matrix */
  KSP         ksp;     /* KSP context */
  KSP        *subksp;  /* array of local KSP contexts on this processor */
  PC          pc;      /* PC context */
  PC          subpc;   /* PC context for subdomain */
  PetscReal   norm;    /* norm of solution error */
  // PetscInt    i, j, Ii, J, *blks, m = 4, n;
  // PetscMPIInt rank, size;
  // PetscInt    its, nlocal, first, Istart, Iend;
  // PetscScalar v, one = 1.0, none = -1.0;
  // PetscBool   isbjacobi;

  //don't know what this does
  PetscFunctionBeginUser;

  // //do the copy
  VecDuplicate(_b, &u);
  VecCopy(_b, u);

  //scale down by the eigenvalue
  // VecScale(u, oneOver2p2);
  // VecView(u, PETSC_VIEWER_STDOUT_WORLD);

  /*
     Create linear solver context
  */
  KSPCreate(PETSC_COMM_WORLD, &ksp);

  /*
     Set operators. Here the matrix that defines the linear system
     also serves as the preconditioning matrix.
  */
  KSPSetOperators(ksp, _A, _A);
  
  // KSPGetPC(ksp, &pc);
  // PCSetType(pc, PCBJACOBI);

  //set runtime options
  KSPSetFromOptions(ksp);

  //the rest comes from the block with isbjacobi. we just do this once
  KSPSetUp(ksp);
  KSPGetPC(ksp, &pc);

  //inside second else block
  PCSetType(pc, PCBJACOBI);
  KSPSetType(ksp, KSPCG);
  KSPSetTolerances(ksp, 1.e-7, PETSC_DEFAULT, PETSC_DEFAULT, PETSC_DEFAULT);

  //solve against the "exact" solution u
  KSPSolve(ksp, _b, _x);

  //check error
  //the line below is why we were getting negatives! _x = _x - _b  
  // VecAXPY(_x, -1, _b);
  // VecNorm(_x, NORM_2, &norm);

  PetscInt its;
  KSPGetIterationNumber(ksp, &its);
  PetscPrintf(PETSC_COMM_WORLD, "Iterations %" PetscInt_FMT "\n", its);


  KSPDestroy(&ksp);
  return;
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
  // MatDestroy(&_A);
  // VecDestroy(&_x);
  // VecDestroy(&_b);
  // PetscFinalize();

  // std::cout << "destroyed objects\n\n";
}
