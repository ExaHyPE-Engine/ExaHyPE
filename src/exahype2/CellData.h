// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_CELL_DATA_H_
#define _EXAHYPE2_CELL_DATA_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>


namespace exahype2 {
    /**
     * Representation of a number of patches which contains all information
     * that's required to process them (given you know the semantics of the
     * data).
     *
     * This struct is a mixture of AoS and SoA. All data besides the Qin and
     * result are SoA. The reason for this "inconsistent" model is that we
     * want to transfer the patch data to accelerators quickly, while we also
     * want to avoid to copy too much stuff around. So we leave the big input
     * fields and basically manage just pointers to that. All scalar or small
     * vector data are converted in SoA which makes it fairly straightforward
     * later to move them over to an accelerator.
     *
     * ## Memory responsibility
     *
     * The PatchData object is a meta object. It does not administer any major
     * heap data itself. That is, its QIn data has to be redirected to the
     * reconstructed data, i.e. the patch data including the halos, by the user.
     * This happens for example in applyKernelToCell() where we create a
     * PatchData object over both allocated input and output data.
     *
     *
     * @todo Doesn't belong into fv. The whole Generic header belongs into the
     * directory above.
     *
     * @todo The enclave task should use this one instead of its local fields
     */
    struct CellData {
      double**   QIn;
      tarch::la::Vector<Dimensions,double>*   patchCentre;
      tarch::la::Vector<Dimensions,double>*   patchSize;
      double*    t;
      double*    dt;
      int*       id;
      const int  numberOfCells;
      double**   QOut;
      double*    maxEigenvalue;

      /**
       * Construct patch data object for one single cell.
       */
      CellData(
        double*   QIn,
        const tarch::la::Vector<Dimensions,double>&   patchCentre_,
        const tarch::la::Vector<Dimensions,double>&   patchSize_,
        double    t,
        double    dt,
        double*   QOut
      );
      CellData( int numberOfPatches_ );
      ~CellData();

      std::string toString() const;
    } ;
}

#endif
