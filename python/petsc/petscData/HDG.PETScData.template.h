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

#include <petscksp.h>
// #include <vector>

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

    
    
}