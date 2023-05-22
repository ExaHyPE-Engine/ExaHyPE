#pragma once

#include <petscksp.h>

#include <vector>
#include <map>

#include <iostream>


namespace petsc{
  /**
   * Class to store a matrix, solution vector and rhs vector.
   * Eventually we will implement solvers too. Currently we have methods
   * to initialise the matrix and also the vectors. 
   * The matrix is initialised to be a n x n matrix, where n is the number
   * of DoFs (subject to change). 
   * The vector x is initialised to be the "value", which ultimately comes from *(fineGridVertexPoisson.getValue().data())
   * The vector b is initialised to be the "rhs", which ultimately comes from *(fineGridVertexPoisson.getRhs().data())
   * 
   * These are just first implementation, but crucially we have initialising 
   * the PETSc world, formation of matrix and two vectors, and methods to 
   * pass values back to c++.
  */
  class PetscData;
}

class petsc::PetscData{
  public:
    // PetscData* getInstance();
    PetscData(int argc, char** argv);
    PetscData();
    virtual ~PetscData();

    int getDoFs() const;
    void setDoFs(int dofs);

    /**
     * method to intialise the matrix and two vectors
     * each we _dofs entries
     * \param int dofs
    */
    void initMatrixAndVectors( int dofs );
    
    //method to convert petsc vec to std array
    //for now, just for the solution vector
    void convertToStdVector();

    /**
     * getter methods to retrieve values inside the solution vector
    */
    std::vector<double> getX() const;
    double getX(int i) const;

    /**
     * method to send doubles to petsc as we receive them
    */
    void insertDoubleAtIndexij(int i, int j, double val);
    void incrementDoubleAtIndexij(int i, int j, double val);
    void applyStencilToA(int numRows, int rowIndices, int numCols, int colIndices, double values[], double scale);

    /**
    * method to insert value and rhs into the _x and _b vectors 
    * respectively. 
    */
    void insertValueAndRHS(double val, double rhs, int index);

    /**
     * Assemble vectors and matrices
    */
    void assemble();

    /**
    * method to hand over to petsc and solve.
    */
    void petscSolve();

  private:
    int _dofs;

    //Ax = b
    Mat _A;
    Vec _x, _b;

    /**
     * initialise a vector to store x for now, so we can interrogate them 
     * when we want to map solution back onto mesh
    */
    std::vector<double> _xAsStdVector;

};
