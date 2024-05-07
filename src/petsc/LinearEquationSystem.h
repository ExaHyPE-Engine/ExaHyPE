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

#include <numeric> //for std iota


namespace petsc {
  class LinearEquationSystem;
}

/**
* Class to capture all of the PETSc objects needed by some experiements.
* The default constructor will set the number of "dofs" to be -1. The
* initialisation of most of the PETSc objects are deferred until the 
* true number of dofs is known, so as not to interfere with Peano's
* MPI handing.
* 
* This class supports creating a (square) matrix (member variable _A),
* a RHS vector (_b), and a member variable _x, which will have the 
* solution written into it. 
* 
* The intended workflow is as follows:
*   - Call the constructor at some early stage during the main function
*   - Use Peano to establish how many true degrees of freedom you wish to have
*   - Send this number into the init() routine. This will set the sizes of the PETSc matrices and vectors
*   - Use the increment() function to insert values into the matrix and rhs vector. There is an overload for each. The solution vector currently is set to all zeros.
*   - Handover to PETSc to solve by calling the solve() routine. We currently use no preconditioner and a GMRES solver. These options can be seen in the body of solve()
*   - Use the get() routine to inspect the solution vector at each index.
*
*  At present, we only support a handful of preconditioners and solvers, each of which can yield
*  quite different results. More info on which preconditioners are supported can be found [here](https://petsc.org/main/manualpages/PC/) 
*  and solvers can be found [here](https://petsc.org/main/manual/ksp/) (see table 6).
*/
class petsc::LinearEquationSystem{
  public:
    /**
    * Trivial constructor. Sets the dofs to be -1. Modify this in init()
    */
    LinearEquationSystem();
    virtual ~LinearEquationSystem();

    /**
    * Overload to print the matrix to terminal
    */
    void printMatrix();
    /**
    * Overload to print the matrix to file
    * \param std::string filename
    */
    void printMatrix(std::string filename);

    /**
    * Overload to print the rhs to terminal
    */
    void printRhs();
    /**
    * Overload to print the rhs to file
    * \param std::string filename
    */
    void printRhs(std::string filename);

    /**
    * Overload to print the solution to terminal
    */
    void printX();
    /**
    * Overload to print the solution to terminal
    * \param std::string filename
    */
    void printX(std::string filename);

    /**
    * Returns number of DoFs in the system. Equivalent to number 
    * of rows in the matrix, and number of entries in the solution/rhs vector
    */
    int getDoFs() const;

    /**
     * method to intialise the matrix and two vectors
     * each we _dofs entries
     * \param int dofs
    */
    void init( int dofs );

    /**
    * Getter method to inspect the value of the solution vector at a 
    * given index. Only call after solve() has completed.
    * \param int i
    */
    double get(int i) const;

    /**
     * Overload to insert val into the matrix, at given row, column position
     * \param int row - row index of where we insert
     * \param int col - col index of where we insert
     * \param double val - value to insert
    */
    void insert(int row, int col, double val);

    /**
     * Overload to increment existing val in the matrix, at given row, column position
     * \param int row - row index of where we insert
     * \param int col - col index of where we insert
     * \param double val - value to insert
    */  
    void increment(int row, int col, double val);

    /**
     * Overload to insert val into the rhs vector, at given row position
     * \param int row - row index of where we insert
     * \param double val - value to insert
    */
    void insert(int row, double val);

    /**
     * Overload to increment existing val in the rhs vector, at given row position
     * \param int row - row index of where we insert
     * \param double val - value to insert
    */
    void increment(int row, double val);


    /**
    * Method to insert a tarch::la::Matrix into the PETSc matrix. 
    * The (0,0) entry of the supplied matrix will be placed at (row, col)
    * in the PETSc matrix. We put the implementation in the header
    * so that the templates work.
    * \param int row - starting row where we wish to insert
    * \param int col - starting col where we wish to insert
    * \param const tarch::la::Matrix<rows,columns,double>& stencil 
    */
    template <int rows, int columns>
    void insertMatrix(int row, int col, const tarch::la::Matrix<rows,columns,double>& stencil){
      // get a array of row indices we want to insert into, beginning
      // with the argument "row"
      int rowIndices[ stencil.rows() ];
      std::iota(rowIndices, rowIndices + stencil.rows(), row);

      // same for columns
      int colIndices[ stencil.cols() ];
      std::iota(colIndices, colIndices + stencil.cols(), col);

      // send to petsc
      MatSetValues( _A, stencil.rows(), rowIndices, stencil.cols(), colIndices, stencil.data(), ADD_VALUES );

    }

    /**
    * Enum class to allow us to customise different 
    * different petsc preconditioner types. See documentation
    * of this class for more info on what preconditioners
    * are supported within petsc.
    */
    enum class PreconditionerType: int 
    {
      None, LU, JACOBI
    };

    /**
    * Enum class to allow us to customise different 
    * different petsc solver types. See documentation
    * of this class for more info on what solvers
    * are supported within petsc.
    */
    enum class KSPSolverType: int
    {
      GMRES, CG
    };

    /**
    * Method to hand over to petsc and solve.
    */
    void solve(PreconditionerType pctype = PreconditionerType::None, KSPSolverType ksptype = KSPSolverType::GMRES);

    /**
    * Method to clean up PETSc objects. Call this in some late stage 
    * during the main function. We would ideally put these in the 
    * destructor of this class, but the destructor is typically called
    * after Peano has already cleaned up its MPI routines. This causes
    * a (trivial) MPI error at the very end of execution. For now, we
    * put this stuff here so as not to cause any issues.
    */
    void cleanUpPetscObjects();


    PCType  getPreconditionerTypeFromEnum(PreconditionerType type);
    KSPType getKSPSolverTypeFromEnum(KSPSolverType type);

  private:
    static tarch::logging::Log  _log;

    int _dofs;

    Mat  _A;
    Vec  _x;
    Vec  _b;

};
