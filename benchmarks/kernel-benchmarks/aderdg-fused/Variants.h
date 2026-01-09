#pragma once

/**
 *
 * There are a total of six variants based on different assumptions.
 *
 * The first assumption has to do with whether face data is defined per face, as in left and right values
 *  for a given face, or per cell, as in any given cell having its four (six in 3d) faces.
 *
 * The second assumption has to do with whether the data for the faces can be shared between
 *  neighbours or not, i.e. can two cells share a pointer or does data need to be copied from one to another.
 *
 * The third assumption is only relevant if two cells cannot share a pointer, and has to do with
 *  whether we can solve the Riemann problem once per face and share the solutions of this, or
 *  whether we can only share the previous faces data, and must perform the riemann solve on each
 *  side of the face independently
 *
 * The details of the variants are described in their respective files, but the concepts/assumptions
 * are as follows:
 *
 * 1. Face Data defined per cell, only contains a given cells' values
 *
 * 2. Face Data defined per cell, contains values from both sides of the face, but each cell
 *      has its own copy of the data which must therefore be transmitted between timesteps
 *
 * 3. Face Data defined per cell, contains values from both sides of the face, with said data
 *      shared between two adjacent cells, i.e. both have a pointer to the same data.
 *
 * 4. Face Data defined per face, but not shared between cells, i.e. the data must be copied
 *      between two instances of face data. Riemann solvers performed once per face.
 *
 * 5. Face Data defined per face, but not shared between cells. In this variant however the
 *      data between the faces is copied and the Riemann solver is then performed per cell.
 *
 * 6. Face Data is defined per face, and shared between cells. I.e. there exists one instance
 *      of face data which two cells share and on which the Riemann solver can be computed
 */

#include "tarch/la/Vector.h"

namespace variant1 {
  extern tarch::logging::Log _log;
  void                       runBenchmarks(
                          int                                         numberOfCells,
                          double                                      timeStamp,
                          double                                      timeStepSize,
                          const tarch::la::Vector<DIMENSIONS, double> cellCenter,
                          const tarch::la::Vector<DIMENSIONS, double> cellSize
                        );
} // namespace variant1

namespace variant2 {
  extern tarch::logging::Log _log;
  void                       runBenchmarks(
                          int                                         numberOfCells,
                          double                                      timeStamp,
                          double                                      timeStepSize,
                          const tarch::la::Vector<DIMENSIONS, double> cellCenter,
                          const tarch::la::Vector<DIMENSIONS, double> cellSize
                        );
} // namespace variant2

namespace variant3 {
  extern tarch::logging::Log _log;
  void                       runBenchmarks(
                          int                                         numberOfCells,
                          double                                      timeStamp,
                          double                                      timeStepSize,
                          const tarch::la::Vector<DIMENSIONS, double> cellCenter,
                          const tarch::la::Vector<DIMENSIONS, double> cellSize
                        );
} // namespace variant3

namespace variant4 {
  extern tarch::logging::Log _log;
  void                       runBenchmarks(
                          int                                         numberOfCells,
                          double                                      timeStamp,
                          double                                      timeStepSize,
                          const tarch::la::Vector<DIMENSIONS, double> cellCenter,
                          const tarch::la::Vector<DIMENSIONS, double> cellSize
                        );
} // namespace variant4

namespace variant5 {
  extern tarch::logging::Log _log;
  void                       runBenchmarks(
                          int                                         numberOfCells,
                          double                                      timeStamp,
                          double                                      timeStepSize,
                          const tarch::la::Vector<DIMENSIONS, double> cellCenter,
                          const tarch::la::Vector<DIMENSIONS, double> cellSize
                        );
} // namespace variant5

namespace variant6 {
  extern tarch::logging::Log _log;
  void                       runBenchmarks(
                          int                                         numberOfCells,
                          double                                      timeStamp,
                          double                                      timeStepSize,
                          const tarch::la::Vector<DIMENSIONS, double> cellCenter,
                          const tarch::la::Vector<DIMENSIONS, double> cellSize
                        );
} // namespace variant6
