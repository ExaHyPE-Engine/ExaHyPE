// This file is part of the Peano's PETSc extension. For conditions of
// distribution and use, please see the copyright notice at
// www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/logging/LogFilterFileReader.h"
#include "tarch/la/Matrix.h"

#include <petscmat.h>
#include <petscsys.h>
#include <petscksp.h>


namespace petsc {
  class LinearEquationSystem;
}


class petsc::LinearEquationSystem{
  public:
    LinearEquationSystem();
    virtual ~LinearEquationSystem();

    void printMatrix();
    void printMatrix(std::string filename);
    void printRhs();
    void printRhs(std::string filename);
    void printX();
    void printX(std::string filename);

    int getDoFs() const;

    /**
     * method to intialise the matrix and two vectors
     * each we _dofs entries
     * \param int dofs
    */
    void init( int dofs );

    //method to convert petsc vec to std array
    //for now, just for the solution vector
//    void convertToStdVector();

    double get(int i) const;

    /**
     * method to send doubles to petsc as we receive them
    */
    void insert(int row, int col, double val);
    void increment(int row, int col, double val);

    void insert(int row, double val);
    void increment(int row, double val);
/*



    *
    * method to insert values into matrix based on stencil
    * \param indices - the indices of the DoFs in question
    * \param values  - logically 2d array of doubles
              the length of values should be equal to indices.size() * indices.size()
              intention is to call this function with tarch::la::Matrix::data(), which
              will return a pointer to the first value in this matrix. Ideally this matrix
              will be the intended stencil for this cell/face/vertex

      IMPLEMENTATION IS IN THE HEADER SO THE TEMPLATES WORK

//    template <int rows, int columns>
//    void insertLocalMatrix( const std::vector< int >& rowIndices, const std::vector< int >& colIndices, const tarch::la::Matrix<rows,columns,double>& stencil ){

  *
  in order of appearance, the params are:
    1. _A, our petsc matrix
    2. the number of rows we wish to insert into
    3. a pointer to the start of an array containing the indices of the rows we wish to insert into
      here we just use the address of the start of the std::vector we pass in
    4. the number of cols we wish to insert into
    5. a pointer to the start of an array containing the indices of the cols we wish to insert into
      here we use same as above
    6. a logically 2d array of doubles to insert. we get this from tarch::la::Matrix::data()
    7. insert policy. let's use ADD_VALUES (equivalent to +=)


    if (rowIndices.size() != stencil.rows() 
        or
        colIndices.size() != stencil.cols()
    )
      throw std::runtime_error("mismatched stencil!");
    MatSetValues( _A, rowIndices.size(), &rowIndices[0], colIndices.size(), &colIndices[0], stencil.data(), ADD_VALUES );
    }


    same as above, but dangerous

    void insertLocalMatrix( const int rows, const int* rowIndices, const int cols, const int* colIndices, const double* values ){
      MatSetValues( _A, rows, rowIndices, cols, colIndices, values, ADD_VALUES );
    }


  *
    inserting a vector of doubles into our rhs vector

    template <int cellDofs>
    void insertRHS( const tarch::la::Vector<cellDofs,double> dofs, const int& globalIndex ){
      
      tarch::la::Vector<cellDofs, int> indices;
      std::iota( indices.data(), indices.data() + indices.size(), globalIndex );


      the params here are, in order of appearance:
        - _b, our rhs vector
        - the number of entries we want to add
        - array containing each of the indices we insert into
        - array containing the data we insert there
        - insert policy. ADD_VALUES is equivalent to +=

      VecSetValues( _b, dofs.size(), indices.data(), dofs.data(), ADD_VALUES );
    }


    inserting vector of doubles to "u"

    template <int cellDofs>
    void insertX( const tarch::la::Vector<cellDofs,double> dofs, const int& globalIndex ){
      tarch::la::Vector<cellDofs, int> indices;
      std::iota( indices.data(), indices.data() + indices.size(), globalIndex );


      the params here are, in order of appearance:
        - _x, our solution vector
        - the number of entries we want to add
        - array containing each of the indices we insert into
        - array containing the data we insert there
        - insert policy. ADD_VALUES is equivalent to +=

      VecSetValues( _x, dofs.size(), indices.data(), dofs.data(), ADD_VALUES );
    
    }


    *
    * method to insert value and rhs into the _x and _b vectors 
    * respectively. 

    void insertValueAndRHS(double val, double rhs, int index);

*/
    /**
     * Assemble vectors and matrices
    */
//    void assemble();

    /**
    * method to hand over to petsc and solve.
    */
    void solve();

    /**
    * method to tidy up _A, _x and _b
    */
//    void petscDestroy();


  private:
    static tarch::logging::Log  _log;

    int _dofs;

    Mat  _A;
    Vec  _x;
    Vec  _b;

    /**
     * initialise a vector to store x for now, so we can interrogate them 
     * when we want to map solution back onto mesh
    */
//    std::vector<double> _xAsStdVector;

};
