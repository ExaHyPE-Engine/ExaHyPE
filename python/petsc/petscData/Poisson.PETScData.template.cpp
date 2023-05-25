#include "PETScData.h"
/**
* todo - change this class to use CLASS_NAME, cf the abstract poisson tempaltes etc
*/

{{NAMESPACE | join("::")}}::PetscData::PetscData() {}
{{NAMESPACE | join("::")}}::PetscData::~PetscData() {}

void {{NAMESPACE | join("::")}}::PetscData::initMatrixAndVectors( int dofs ){
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

void {{NAMESPACE | join("::")}}::PetscData::setDoFs(int dofs){
  _dofs = dofs;
}

int {{NAMESPACE | join("::")}}::PetscData::getDoFs() const{
  return _dofs;
}

void {{NAMESPACE | join("::")}}::PetscData::convertToStdVector(){
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

std::vector<double> {{NAMESPACE | join("::")}}::PetscData::getX() const{
  return _xAsStdVector;
}

double {{NAMESPACE | join("::")}}::PetscData::getX(int i) const{
  //check if we have populated it yet
  if ( !_xAsStdVector.size() )
    return -1;
  return _xAsStdVector[i];
}

void {{NAMESPACE | join("::")}}::PetscData::insertDoubleAtIndexij(int i, int j, double val){
  MatSetValue(_A, i, j, val, ADD_VALUES);
}

//storing doubles to be placed into A
void {{NAMESPACE | join("::")}}::PetscData::incrementDoubleAtIndexij( int i, int j, double val ){
  MatSetValue(_A, i, j, val, INSERT_VALUES);
}

void {{NAMESPACE | join("::")}}::PetscData::insertValueAndRHS(double val, double rhs, int index){
  // insert "val" into _x 
  VecSetValue(_x, index, val, INSERT_VALUES);

  // insert "rhs" into _b
  VecSetValue(_b, index, rhs, INSERT_VALUES);
}

void {{NAMESPACE | join("::")}}::PetscData::assemble(){
  
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

void {{NAMESPACE | join("::")}}::PetscData::petscSolve(){
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
