// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_TIME_STEPPING_H_
#define _EXAHYPE2_TIME_STEPPING_H_


#include "tarch/la/ScalarOperations.h"

#include "peano4/datamanagement/FaceEnumerator.h"

#include <utility>


namespace exahype2 {
  /**
   *
   * @param cellLabel
   */
  template <typename CellLabel, typename FaceLabel>
  bool runTimeStepOnCell(
    const CellLabel& cellLabel,
    const peano4::datamanagement::FaceEnumerator< FaceLabel >& faceLabelEnumerator
  ) {
    double cellTimeStamp =  cellLabel.getTimeStamp();

    bool updateCell = true;

    for (int d=0; d<Dimensions; d++) {
      updateCell &= tarch::la::greaterEquals(
        faceLabelEnumerator(d).getNewTimeStamp(0),
        cellTimeStamp
      );
      updateCell &= tarch::la::greaterEquals(
        faceLabelEnumerator(d+Dimensions).getNewTimeStamp(1),
        cellTimeStamp
      );
    }

    return updateCell;
  }


  /**
   * @param cellTimeStamp This is an in-out parameter, as we can adopt the stamp size to
   *   avoid the accummulation of rounding errors.
   * @return Weight of (old,new) data.
   */
  std::pair<double,double> getInterpolationWeights( double oldTimeStampOnFace, double newTimeStampOnFace, double& cellTimeStamp );
}

#endif
