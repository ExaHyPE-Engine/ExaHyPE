#pragma once

#include "exahype2/dg/DGUtils.h"
#include "repositories/SolverRepository.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

namespace benchmarks {
  namespace exahype2 {
    namespace kernelbenchmarks {
      constexpr int NumberOfInputEntriesPerCell
        = (AderSolver::Order + 1) * (AderSolver::Order + 1)
#if Dimensions == 3
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
        double*                                     Q,
        const tarch::la::Vector<Dimensions, double> CellCenter,
        const tarch::la::Vector<Dimensions, double> CellSize
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
              kernels::AderSolver::Quadrature<double>::nodes
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
       */
      inline void reportRuntime(
        const std::string&                kernelIdentificator,
        const tarch::timing::Measurement& timingComputeKernel,
        const tarch::timing::Measurement& timingKernelLaunch,
        int                               numberOfCells,
        tarch::logging::Log               _log
      ) {
        std::stringstream ss;
        ss << "\n";
        ss << kernelIdentificator << ":\n\t";
        ss << "total compute time: " << timingComputeKernel.getValue() << " |\n\t";
        ss << "average compute time per cell: " << (timingComputeKernel.getValue() / numberOfCells) << " |\n\t";
        ss << timingComputeKernel.toString() << " |\n\t";
        ss << "total time including launch time: " << timingKernelLaunch.getValue() << " |\n\t";
        ss << "average total time per cell: " << (timingKernelLaunch.getValue() / numberOfCells);
        ss << " |\n\t" << timingKernelLaunch.toString();
        logInfo("reportRuntime()", ss.str());
      }
    } // namespace kernelbenchmarks
  }   // namespace exahype2
} // namespace benchmarks
