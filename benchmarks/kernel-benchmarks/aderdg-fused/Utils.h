#pragma once

#include "exahype2/dg/DGUtils.h"
#include "repositories/SolverRepository.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

#include <fstream>

namespace benchmarks {
  namespace exahype2 {
    namespace kernelbenchmarks {
      constexpr int NumberOfInputEntriesPerCell
        = (AderSolver::Order + 1) * (AderSolver::Order + 1)
#if DIMENSIONS == 3
          * (AderSolver::Order + 1)
#endif
          * (AderSolver::NumberOfUnknowns + AderSolver::NumberOfAuxiliaryVariables);

      constexpr int NumberOfOutputEntriesPerCell = 0;

      /**
       * Set input data
       *
       * We use the values from the centralMost Cell in the domain
       * with an arbitrary cell size of 1/81 of the domain size.
       *
       * TODO: should we update this to actual initial conditions?
       */
      inline void initInputData(
        SolverPrecision*                            Q,
        const tarch::la::Vector<DIMENSIONS, double> CellCenter,
        const tarch::la::Vector<DIMENSIONS, double> CellSize
      ) {
        // for (int i = 0; i < NumberOfInputEntriesPerCell; i++) {
        //   Q[i] = std::sin(1.0 * i / (NumberOfInputEntriesPerCell) * tarch::la::PI);
        // }

        int linearisedIndex = 0;
        dfor(index, AderSolver::Order + 1) {
          repositories::instanceOfAderSolver.initialCondition(
            Q + linearisedIndex,
            ::exahype2::dg::getQuadraturePoint(
              CellCenter,
              CellSize,
              index,
              repositories::instanceOfAderSolver.Order + 1,
              kernels::AderSolver::Quadrature<SolverPrecision>::nodes
            ),
            CellSize,
            // index,
            true
          );
          linearisedIndex += AderSolver::NumberOfUnknowns + AderSolver::NumberOfAuxiliaryVariables;
        }
      }


      /**
       * Reports the runtime and throughput of the benchmarks
       *
       * First set of outputs refers to the time of the compute kernel itself.
       * Second set of outputs refers to the time including the kernel launch overhead.
       * The throughput is measured in the updates of volumes per second (dof/s).
       *
       * In the second part, this also writes the information about the runtime to a csv file
       */
      inline void reportRuntime(
        const std::string&                kernelIdentificator,
        const tarch::timing::Measurement& timingComputeKernel,
        const tarch::timing::Measurement& timingKernelLaunch,
        int                               numberOfCells,
        int                               numberOfThreads,
        tarch::logging::Log               _log
      ) {
        std::stringstream ss;
        ss << "\n";
        ss << kernelIdentificator << ":\n\t";
        ss << "total compute time: " << timingComputeKernel.getValue() << " |\n\t";
        ss << "average compute time per cell: " << (timingComputeKernel.getValue() / numberOfCells) << " |\n\t";
        ss << timingComputeKernel.toString() << " |\n\t";
        //ss << "total time including launch time: " << timingKernelLaunch.getValue() << " |\n\t";
        //ss << "average total time per cell: " << (timingKernelLaunch.getValue() / numberOfCells);
        //ss << " |\n\t" << timingKernelLaunch.toString();
        logInfo("reportRuntime()", ss.str());

        std::ostringstream snapshotFileName;
        snapshotFileName << "ader-benchmarks-prec-"
                         << typeid(SolverPrecision).name()
                         << ".csv";
        std::ofstream file( snapshotFileName.str(), std::ios_base::app );
        file  << kernelIdentificator << ","
              << numberOfThreads << ","
              << numberOfCells << ","
              << timingComputeKernel.getValue() << ","
              << timingKernelLaunch.getValue() << ","
              << (timingComputeKernel.getValue() / numberOfCells) << ","
              // << (timingKernelLaunch.getValue() / numberOfCells) << ","
              // << timingKernelLaunch.max() << ","
              // << timingKernelLaunch.min() << ","
              // << timingKernelLaunch.getStandardDeviation()
              << std::endl;
      }
    } // namespace kernelbenchmarks
  }   // namespace exahype2
} // namespace benchmarks
