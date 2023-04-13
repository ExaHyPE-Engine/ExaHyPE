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
    // void initVectors();

  private:

    int _dofs;

    //Ax = b
    Mat A;

    Vec x, b;

    // static PetscData* instance;
};
