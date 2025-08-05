// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Restriction.h"

namespace {
  [[maybe_unused]] tarch::logging::Log _log("toolbox::blockstructured");
} // namespace

void toolbox::blockstructured::internal::projectCells_AoS(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  std::function<void(
    tarch::la::Vector<DIMENSIONS, int>    coarseVolume,
    tarch::la::Vector<DIMENSIONS, int>    fineVolume,
    tarch::la::Vector<DIMENSIONS, double> coarseVolumeCentre,
    tarch::la::Vector<DIMENSIONS, double> fineVolumeCentre,
    double                                coarseVolumeH,
    double                                fineVolumeH
  )>                                        update
) {
  logTraceInWith2Arguments(
    "projectCells_AoS(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch
  );

  const double volumeHCoarse = marker.h()(0)
                               / static_cast<double>(numberOfDoFsPerAxisInPatch)
                               * 3.0;
  const double volumeHFine = marker.h()(0)
                             / static_cast<double>(numberOfDoFsPerAxisInPatch);

  tarch::la::Vector<DIMENSIONS, double> leftBottomOfFineCell = marker.getOffset(
  );
  tarch::la::Vector<DIMENSIONS, double> leftBottomOfCoarseCell
    = marker.getOffset()
      - tarch::la::multiplyComponents(
        marker.h(),
        tarch::la::convertScalar<
          double>(marker.getRelativePositionWithinFatherCell())
      );

  dfor(kFine, numberOfDoFsPerAxisInPatch) {
    tarch::la::Vector<DIMENSIONS, int>
      kCoarse = (kFine
                 + numberOfDoFsPerAxisInPatch
                     * marker.getRelativePositionWithinFatherCell())
                / 3;

    tarch::la::Vector<DIMENSIONS, double>
      volumeXCoarse = leftBottomOfCoarseCell
                      + volumeHCoarse * tarch::la::convertScalar<double>(kCoarse)
                      + 0.5
                          * tarch::la::Vector<DIMENSIONS, double>(volumeHCoarse);
    tarch::la::Vector<DIMENSIONS, double>
      volumeXFine = leftBottomOfFineCell
                    + volumeHFine * tarch::la::convertScalar<double>(kFine)
                    + 0.5 * tarch::la::Vector<DIMENSIONS, double>(volumeHFine);

    update(
      kCoarse,
      kFine,
      volumeXCoarse,
      volumeXFine,
      volumeHCoarse,
      volumeHFine
    );
  }

  logTraceOut("projectCells_AoS(...)");
}
