#include <petscsys.h>
#include <petscmat.h>
#include <iostream>

//the above only works because i have $PETSC_DIR configured already

namespace petsc{
  class PetscData;
}

class petsc::PetscData{
  public:
    PetscData* getInstance();
    PetscData(int DoFs);

    virtual ~PetscData();

  private:

    int _dofs;
    void initMatrix();

    //petsc matrix itself
    Mat A;

    static PetscData* instance;
};
