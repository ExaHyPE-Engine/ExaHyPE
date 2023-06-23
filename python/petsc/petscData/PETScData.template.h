/**
 *
 * for now we use the project name as a prefix for
 * the template
 *
 * */


#pragma once

#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/logging/LogFilterFileReader.h"
#include "tarch/la/Matrix.h"

#include <petscksp.h>
#include <vector>

extern tarch::logging::Log _log;

namespace {{NAMESPACE | join("::")}}{
  class PetscData;
}

class {{NAMESPACE | join("::")}}::PetscData{
  public:
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
    * method to insert values into matrix based on stencil
    * \param indices - the indices of the DoFs in question
    * \param values  - logically 2d array of doubles
              the length of values should be equal to indices.size() * indices.size()
              intention is to call this function with tarch::la::Matrix::data(), which
              will return a pointer to the first value in this matrix. Ideally this matrix
              will be the intended stencil for this cell/face/vertex
    */
    void insertLocalMatrix( const std::vector< int >& rowIndices, const std::vector< int >& colIndices, const double* values );

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

    /**
    * method to tidy up _A, _x and _b
    */
    void petscDestroy();


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