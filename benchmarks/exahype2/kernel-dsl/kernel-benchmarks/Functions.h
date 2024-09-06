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

void Flux(
    ::exahype2::CellData& patchData,
    int patchIndex,
    int i,
    int j,
    int k,
    // volumeIndex,
    const exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
    const exahype2::enumerator::AoSLexicographicEnumerator& fluxEnumerator,
    int normal,
    double* __restrict__ tempFlux,
    const int n_vars,
    const int n_aux
    );

void Flux(
    ::exahype2::CellData& patchData,
    int patchIndex,
    int i,
    int j,
    // volumeIndex,
    const exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
    const exahype2::enumerator::AoSLexicographicEnumerator& fluxEnumerator,
    int normal,
    double* __restrict__ tempFlux,
    const int n_vars,
    const int n_aux
    );

void maxEigenvalue(
    ::exahype2::CellData& patchData,
    int patchIndex,
    int i,
    int j,
    int k,
    // volumeIndex,
    const exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
    const exahype2::enumerator::AoSLexicographicEnumerator& eigenvalueEnumerator,
    int normal,
    double* __restrict__ tempEigenvalueX,
    const int n_vars,
    const int n_aux
    );

void maxEigenvalue(
    ::exahype2::CellData& patchData,
    int patchIndex,
    int i,
    int j,
    // volumeIndex,
    const exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
    const exahype2::enumerator::AoSLexicographicEnumerator& eigenvalueEnumerator,
    int normal,
    double* __restrict__ tempEigenvalueX,
    const int n_vars,
    const int n_aux
    );


double max(double* a,double* b);