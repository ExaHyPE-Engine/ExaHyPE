#include <iostream>
#include "exahype2/UserInterface.h"
#include "observers/CreateGrid.h"
#include "observers/CreateGridAndConvergeLoadBalancing.h"
#include "observers/CreateGridButPostponeRefinement.h"
#include "observers/InitGrid.h"
#include "observers/PlotSolution.h"
#include "observers/TimeStep.h"
#include "peano4/peano.h"
#include "repositories/DataRepository.h"
#include "repositories/SolverRepository.h"
#include "repositories/StepRepository.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/Device.h"
#include "tarch/logging/CommandLineLogger.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/otter.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include "tasks/FVRusanovSolverEnclaveTask.h"
#include "toolbox/loadbalancing/loadbalancing.h"


// void Flux( //2d
//     ::exahype2::CellData& patchData,
//     int patchIndex,
//     int i,
//     int j,
//     const exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
//     const exahype2::enumerator::AoSLexicographicEnumerator& fluxEnumerator,
//     int normal,
//     double* __restrict__ tempFlux,
//     const int n_vars,
//     const int n_aux
//     ) {
//     tarch::la::Vector<2, int> volume({i,j});
//     exahype2::fv::rusanov::loopbodies::computeFlux<n_vars, n_aux>(
//           patchData.QIn[patchIndex],
//           QInEnumerator,
//           [&]( //Fluxfunctor
//             const double* __restrict__ Q,
//             const ::tarch::la::Vector<Dimensions, double>& x, //
//             const ::tarch::la::Vector<Dimensions, double>& h, //
//             double                                         t,
//             double                                         dt,
//             int                                            normal,
//             double* __restrict__ F
//             ) -> void {
//                 benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfFVRusanovSolver.flux(Q, x, h, t, dt, normal, F);
//             },
//           patchData.cellCentre[patchIndex],
//           patchData.cellSize[patchIndex],
//           patchIndex,
//           tarch::la::slice<Dimensions>(volume, 0),
//           patchData.t[patchIndex],
//           patchData.dt[patchIndex],
//           normal,
//           tempFlux,
//           fluxEnumerator
//         );
// }

// void Flux( //2d
//     ::exahype2::CellData& patchData,
//     int patchIndex,
//     int i,
//     int j,
//     int k,
//     const exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
//     const exahype2::enumerator::AoSLexicographicEnumerator& fluxEnumerator,
//     int normal,
//     double* __restrict__ tempFlux,
//     const int n_vars,
//     const int n_aux
//     ) {
//     tarch::la::Vector<3, int> volume({i,j,k});
//     exahype2::fv::rusanov::loopbodies::computeFlux<n_vars, n_aux>(
//           patchData.QIn[patchIndex],
//           QInEnumerator,
//           [&]( //Fluxfunctor
//             const double* __restrict__ Q,
//             const ::tarch::la::Vector<Dimensions, double>& x,
//             const ::tarch::la::Vector<Dimensions, double>& h,
//             double                                         t,
//             double                                         dt,
//             int                                            normal,
//             double* __restrict__ F
//             ) -> void {
//                 benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfFVRusanovSolver.flux(Q, x, h, t, dt, normal, F);
//             },
//           patchData.cellCentre[patchIndex],
//           patchData.cellSize[patchIndex],
//           patchIndex,
//           tarch::la::slice<Dimensions>(volume, 0),
//           patchData.t[patchIndex],
//           patchData.dt[patchIndex],
//           normal,
//           tempFlux,
//           fluxEnumerator
//         );
// }

// void maxEigenvalue( //2d
//     ::exahype2::CellData& patchData,
//     int patchIndex,
//     int i,
//     int j,
//     const exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
//     const exahype2::enumerator::AoSLexicographicEnumerator& eigenvalueEnumerator,
//     int normal,
//     double* __restrict__ tempEigenvalueX,
//     const int n_vars,
//     const int n_aux
//     ) {
//     tarch::la::Vector<2, int> volume({i,j});
//     exahype2::fv::rusanov::loopbodies::computeMaxEigenvalue<n_vars, n_aux>(
//         patchData.QIn[patchIndex],
//         QInEnumerator,
//         [&]( //EigenFunctor
//             const double* __restrict__ Q,
//             const ::tarch::la::Vector<Dimensions, double>& x,
//             const ::tarch::la::Vector<Dimensions, double>& h,
//             double t,
//             double dt,
//             int normal) -> double { return benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfFVRusanovSolver.maxEigenvalue(Q, x, h, t, dt, normal); },
//         patchData.cellCentre[patchIndex],
//         patchData.cellSize[patchIndex],
//         patchIndex,
//         tarch::la::slice<Dimensions>(volume, 0), // volumeindex
//         patchData.t[patchIndex],
//         patchData.dt[patchIndex],
//         normal,
//         tempEigenvalueX,
//         eigenvalueEnumerator
//     );
// }

// void maxEigenvalue( //3d
//     ::exahype2::CellData& patchData,
//     int patchIndex,
//     int i,
//     int j,
//     int k,
//     const exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
//     const exahype2::enumerator::AoSLexicographicEnumerator& eigenvalueEnumerator,
//     int normal,
//     double* __restrict__ tempEigenvalueX,
//     const int n_vars,
//     const int n_aux
//     ) {
//     tarch::la::Vector<3, int> volume({i,j,k});
//     exahype2::fv::rusanov::loopbodies::computeMaxEigenvalue<n_vars, n_aux>(
//         patchData.QIn[patchIndex],
//         QInEnumerator,
//         [&]( //EigenFunctor
//             const double* __restrict__ Q,
//             const ::tarch::la::Vector<Dimensions, double>& x,
//             const ::tarch::la::Vector<Dimensions, double>& h,
//             double t,
//             double dt,
//             int normal) -> double { return benchmarks::exahype2::kernelbenchmarks::repositories::instanceOfFVRusanovSolver.maxEigenvalue(Q, x, h, t, dt, normal); },
//         patchData.cellCentre[patchIndex],
//         patchData.cellSize[patchIndex],
//         patchIndex,
//         tarch::la::slice<Dimensions>(volume, 0), // volumeindex
//         patchData.t[patchIndex],
//         patchData.dt[patchIndex],
//         normal,
//         tempEigenvalueX,
//         eigenvalueEnumerator
//     );
// }

double max(double* a, double* b) {
    return std::max(*a,*b);
};