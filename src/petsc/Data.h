#pragma once

#include <petscsys.h>
#include <petscmat.h>
#include <petscvec.h>
#include <vector>

#include <iostream>

//the above only works because i have $PETSC_DIR configured already

namespace petsc{
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

    void initMatrix();
    void initVectors( std::vector< std::pair<double, double> > valuesAndRhs );
    
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

    // static PetscData* instance;
};
