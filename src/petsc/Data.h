#pragma once

#include <petscsys.h>
#include <petscmat.h>
#include <petscvec.h>
#include <vector>

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

    // void initMatrix();

    /**
     * For now, we pass in the values given to the Poisson class
     * during the InitVertexDof stage. Once we get to the step where
     * we Init Petsc, we send those values over here. See Poisson-main.cpp
    */
    // void initVectors( std::vector< std::pair<double, double> > valuesAndRhs );
    
    //method to convert petsc vec to std array
    //for now, just for the solution vector
    void convertToStdVector();

    /**
     * getter methods to retrieve values inside the solution vector
    */
    std::vector<double> getX() const;
    double getX(int i) const;


  private:
    int _dofs;

    //Ax = b
    Mat A;
    Vec x, b;

    /**
     * initialise a vector to store x for now, so we can interrogate them 
     * when we want to map solution back onto mesh
    */
    std::vector<double> _xAsStdVector;

};
