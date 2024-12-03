#include "Benchmarks-main.h"
#include "Constants.h"

#include "tarch/NonCriticalAssertions.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"

#include "peano4/peano.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "repositories/DataRepository.h"
#include "repositories/StepRepository.h"
#include "repositories/GlobalState.h"

#include "swift2/UserInterface.h"

#include "Constants.h"

#include "swift2/kernels/ParticleSetIterators.h"
#include "swift2/kernels/cppiterators/ParticleSetIterators.h"
#include "swift2/kernels/ompiterators/ParticleSetIterators.h"


#include "swift2/kernels/legacy/Density.h"
#include "swift2/kernels/legacy/HydroForce.h"
#include "swift2/kernels/legacy/Leapfrog.h"
#include "swift2/kernels/legacy/SmoothingLength.h"

#include "swift2/timestepping/Leapfrog.h"
#include "swift2/timestepping/TimeStepping.h"

#include "NohProblem.h"

#include "tarch/timing/Measurement.h"
#include "tarch/multicore/otter.h"


#include "globaldata/HydroPart.h"
#include "vertexdata/HydroPartSet.h"




tarch::logging::Log _log("::");


using namespace benchmarks::swift2::hydro::kernel_throughput;


/**
 * See initGridTraversalEvent()
 */
peano4::grid::GridTraversalEvent      gridTraversalEvent;

vertexdata::HydroPartSet*             workItemParticleSet;


/**
 * Initialise the particles with some meaningful information
 *
 * We basically spread them out evenly (so there's no division by
 * zero if we compute distances) and then use the Noh2d setup to
 * give each particle a meaningful value.
 */
void initParticle( globaldata::HydroPart& particle, int numberOfParticles, int index ) {
  int particlesPerAxis = static_cast<int>( std::ceil( std::pow(numberOfParticles, 1.0/Dimensions ) ) + 1.0 );
  const double h = 1.0 / particlesPerAxis;
  tarch::la::Vector<Dimensions,double> x = {
                (index % particlesPerAxis) * h
                , (index % (particlesPerAxis*particlesPerAxis) ) * h
                #if Dimensions==3
                , (index % (particlesPerAxis*particlesPerAxis*particlesPerAxis) ) * h
                #endif
  };
  particle.setX(x);
  initNohProblemIC(particle, 1.0, particlesPerAxis, Dimensions );
}


void initParticles( int numberOfParticles ) {
  const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;

  int currentNumberOfParticles = 0;
  for (int currentWorkItem=0; currentWorkItem<numberOfWorkItems; currentWorkItem++) {
    for (auto& p: workItemParticleSet[currentWorkItem]) {
      initParticle(*p, numberOfParticles, currentNumberOfParticles);
      currentNumberOfParticles++;
    }
  }
}


/**
 * Prepare the force benchmark
 *
 * To prepare for the force benchmark, we run through the following steps:
 *
 * - Initialise the particles through an initParticles() call. This initialises
 *   the workItemParticleSet sequence.
 * - invoke resetSmoothingLengthIterationCounter() and resetAcceleration()  on
 *   the particles. For this, we run over all the workItemParticleSets and,
 *   within each set, use the forAllParticles function to initialise the
 *   particles.
 */
void prepareForceBenchmark(int numberOfParticles) {
  initParticles(numberOfParticles);

  const peano4::datamanagement::VertexMarker  vertexMarker(gridTraversalEvent);
  const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;

  for (int workItem=0; workItem<numberOfWorkItems; workItem++) {
      ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::resetSmoothingLengthIterationCounter<globaldata::HydroPart>);
      ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::resetAcceleration<globaldata::HydroPart>);
  }
}


/**
 * Create coalesced particle indicator
 *
 * Whenever we enter a cell, we are given all the particles associated to the
 * @f$ 2^d @f$ vertices of the cell as one big container. If we work with
 * coalesced storage schemes, these particles will technically be stored as
 * @f$ 2^d @f$ continuous sequences. In the benchmark, all particles of one
 * work item are held
 * in one continuous sequence, which is a special case of this. Nevertheless,
 * we create a vector of integeres, which pretends that they were split up further.
 *
 * So if chunk has 18 entries, it might, in 2d, holds the entries 5,5,5,3.
 *
 */
std::vector<int> createCoalescedMemoryIndices() {
  std::vector<int> coalescedEntries(TwoPowerD);
  int remainingParticlesToAssignToChunk = Benchmark_WorkItemSize;
  int chunkSize = std::max(1,Benchmark_WorkItemSize / TwoPowerD);
  chunkSize += (Benchmark_WorkItemSize % TwoPowerD == 0) ? 0 : 1;
  for (int i=0; i<TwoPowerD; i++) {
    coalescedEntries[i]                = chunkSize;
    remainingParticlesToAssignToChunk -= chunkSize;
    chunkSize                          = std::min( chunkSize, remainingParticlesToAssignToChunk );
  }

  return coalescedEntries;
}



void allocateParticles(int numberOfParticles) {
  const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;
  assertionEquals( numberOfParticles % Benchmark_WorkItemSize, 0);
  workItemParticleSet = new vertexdata::HydroPartSet[numberOfWorkItems];

  int currentNumberOfParticles = 0;
  for (int currentWorkItem=0; currentWorkItem<numberOfWorkItems; currentWorkItem++) {
    int numberOfParticlesForThisWorkItem = std::min( Benchmark_WorkItemSize, numberOfParticles-currentNumberOfParticles );
    // @todo We might want to allocate in managed memory
    globaldata::HydroPart*    particles = tarch::allocateMemory<globaldata::HydroPart>(numberOfParticlesForThisWorkItem, tarch::MemoryLocation::Heap);
    for (int i=0; i<numberOfParticlesForThisWorkItem; i++) {
      workItemParticleSet[currentWorkItem].addParticle(particles+i);
    }
    currentNumberOfParticles += numberOfParticlesForThisWorkItem;
  }
}


void assessKernel(
  std::function< void(int) >  kernelCallInLoop,
  const std::string&          name,
  int                         numberOfParticles
) {
  const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;

  ::tarch::timing::Measurement         measurement;
  for (int sample=0; sample<Benchmark_Samples; sample++) {
    ::tarch::timing::Watch watch("", "runBenchmark(int)", false);
    #pragma omp parallel for schedule(static,1)
    for (int workItem=0; workItem<numberOfWorkItems; workItem++) {
      for (int i=0; i<Benchmark_WorkItemRepeats; i++) {
        kernelCallInLoop(i);
      }
    }
    watch.stop();
    measurement.setValue( watch.getCalendarTime()/Benchmark_WorkItemRepeats/numberOfParticles );
  }
  logInfo( "assessKernel(int)", name << ":\t" << measurement.getValue() << "  " << measurement.toString() );
}


void runBenchmark(int numberOfParticles) {
    const peano4::datamanagement::VertexMarker  vertexMarker(gridTraversalEvent);
    const peano4::datamanagement::CellMarker    cellMarker(gridTraversalEvent);

    allocateParticles(numberOfParticles);

    const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;

    logInfo( "runBenchmark(int)", "===============================");
    logInfo( "runBenchmark(int)", " Experiment: " << numberOfParticles << " particles" );
    logInfo( "runBenchmark(int)", " No of work items: " << numberOfWorkItems );
    logInfo( "runBenchmark(int)", "===============================");

    if (Benchmark_Testforce) {
      prepareForceBenchmark(numberOfParticles);
      assessKernel(
          [&](int workItem) -> void {
            ::swift2::kernels::forAllParticlePairs( cellMarker, workItemParticleSet[workItem], workItemParticleSet[workItem], createCoalescedMemoryIndices(), createCoalescedMemoryIndices(), ::swift2::kernels::legacy::forceKernelWithMasking<globaldata::HydroPart>);
          },
          "force (baseline)",
          numberOfParticles
      );
      assessKernel(
          [&](int workItem) -> void {
            ::swift2::kernels::coalesced::forAllParticlePairs( cellMarker, workItemParticleSet[workItem], workItemParticleSet[workItem], createCoalescedMemoryIndices(), createCoalescedMemoryIndices(), ::swift2::kernels::legacy::forceKernelWithMasking<globaldata::HydroPart>);
          },
          "force (coalesced,native)",
          numberOfParticles
      );
    }

    if (Benchmark_Testdensity) {
      initParticles(numberOfParticles);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticlePairs( cellMarker, workItemParticleSet[workItem], workItemParticleSet[workItem], createCoalescedMemoryIndices(), createCoalescedMemoryIndices(), ::swift2::kernels::legacy::densityKernelWithMasking<globaldata::HydroPart>);
        },
        "density (baseline)",
        numberOfParticles
      );
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::coalesced::forAllParticlePairs( cellMarker, workItemParticleSet[workItem], workItemParticleSet[workItem], createCoalescedMemoryIndices(), createCoalescedMemoryIndices(), ::swift2::kernels::legacy::densityKernelWithMasking<globaldata::HydroPart>);
        },
        "density (coalesced,native)",
        numberOfParticles
      );
    }

    if (Benchmark_Testkick1) {
      initParticles(numberOfParticles);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSizeWithMasking<globaldata::HydroPart>);
        },
        "kick1 (baseline)",
        numberOfParticles
      );
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::coalesced::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSizeWithMasking<globaldata::HydroPart>);
        },
        "kick1 (coalesced,native)",
        numberOfParticles
      );
    }

    if (Benchmark_Testkick2) {
      initParticles(numberOfParticles);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSizeWithMasking<globaldata::HydroPart>);
          ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::resetPredictedValuesWithMasking<globaldata::HydroPart>);
        },
        "kick2 (baseline)",
        numberOfParticles
      );
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::coalesced::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSizeWithMasking<globaldata::HydroPart>);
          ::swift2::kernels::coalesced::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::resetPredictedValuesWithMasking<globaldata::HydroPart>);
        },
        "kick2 (coalesced,native)",
        numberOfParticles
      );
    }

    if (Benchmark_Testdrift) {
      initParticles(numberOfParticles);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::timestepping::resetMovedParticleMarkerWithMasking<globaldata::HydroPart>);
          ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSizeWithMasking<globaldata::HydroPart>);
        },
        "drift (baseline)",
        numberOfParticles
      );
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::coalesced::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::timestepping::resetMovedParticleMarkerWithMasking<globaldata::HydroPart>);
          ::swift2::kernels::coalesced::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSizeWithMasking<globaldata::HydroPart>);
        },
        "drift (coalesced,native)",
        numberOfParticles
      );
    } // end of sample loop

    for (int currentWorkItem=0; currentWorkItem<numberOfWorkItems; currentWorkItem++) {
      tarch::freeMemory(*workItemParticleSet[currentWorkItem].begin(), tarch::MemoryLocation::Heap);
    }
    delete[] workItemParticleSet;
}



/**
 * Initialise the global variable gridTraversalEvent
 *
 * We work with one big traversal event which represents the unit square/cube.
 * It is not refined, and none of the adjacent vertices are hanging. The
 * traversal event is not used directly, but the markers are derived from it
 * later throughout the benchmark run.
 */
void initGridTraversalEvent() {
    gridTraversalEvent = peano4::grid::GridTraversalEvent(
            tarch::la::Vector< Dimensions, double >(0.0), //         __x,
            tarch::la::Vector< Dimensions, double >(1.0), //         __h,
            0,                    // std::bitset< TwoPowerD >        __hasBeenRefined,
            0,                    // std::bitset< TwoPowerD >        __willBeRefined,
            0,                    // std::bitset< TwoPowerD >        __isVertexLocal,
            0,                    // std::bitset< TwoPowerD >        __isParentVertexLocal,
            0,                    // std::bitset< TwoPowerD >        __isVertexParentOfSubtree,
            0,                    // std::bitset< TwoTimesD >        __isFaceLocal,
            true,                 // bool    __isCellLocal,
            true,                 // bool    __isParentCellLocal,
            0,                    // std::bitset< TwoPowerD >        __isVertexAdjacentToParallelDomainBoundary,
            0,                    // std::bitset< TwoTimesD >        __isFaceAdjacentToParallelDomainBoundary,
            tarch::la::Vector<TwoPowerD,int>(1),    //  __numberOfAdjacentTreesPerVertex
            std::bitset< ThreePowerD >().all(), //       __isAdjacentCellLocal,
            tarch::la::Vector< TwoPowerD, int >(0), //     __vertexDataFrom,
            tarch::la::Vector< TwoPowerD, int >(0), //     __vertexDataTo,
            tarch::la::Vector< TwoTimesD, int >(0), //     __faceDataFrom,
            tarch::la::Vector< TwoTimesD, int >(0), //     __faceDataTo,
            0,                    // int     __cellData,
            tarch::la::Vector< Dimensions, int >(0), //    __relativePositionToFather,
            0,                    // int     __invokingSpacetree,
            true                  // __invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing
    );
}



int main(int argc, char** argv) {
    const int ExitCodeSuccess          = 0;
    const int ExitCodeUnitTestsFailed  = 1;
    const int ExitCodeInvalidArguments = 2;

    peano4::initParallelEnvironment(&argc,&argv);
    tarch::multicore::initSmartMPI();
    peano4::fillLookupTables();
    tarch::initNonCriticalAssertionEnvironment();

    peano4::initSingletons(
      DomainOffset,
      DomainSize,
      PeriodicBC
    );

    repositories::DataRepository::initDatatypes();

    initGridTraversalEvent();

    if (not swift2::parseCommandLineArguments(argc,argv) ) {
        logError("main()", "Invalid command line arguments.");
        return ExitCodeInvalidArguments;
    }

    const int numberOfThreads = tarch::multicore::Core::getInstance().getNumberOfThreads();

    logInfo( "runBenchmark(int)", "===============================");
    logInfo( "runBenchmark(int)", " threads=" << numberOfThreads );
    logInfo( "runBenchmark(int)", " sizeof(Particle)=" << sizeof(globaldata::HydroPart) );
    logInfo( "runBenchmark(int)", " max particles=" << Benchmark_MaxParticles );
    logInfo( "runBenchmark(int)", " samples=" << Benchmark_Samples );
    logInfo( "runBenchmark(int)", " size of work item (cell size)=" << Benchmark_WorkItemSize );
    logInfo( "runBenchmark(int)", " kernel repetitions per work item=" << Benchmark_WorkItemRepeats );
    logInfo( "runBenchmark(int)", "===============================");

    int particles=std::max(Benchmark_MinParticles,Benchmark_WorkItemSize);
    while (particles<=Benchmark_MaxParticles) {
      int previousParticles = particles;
      particles += (particles % Benchmark_WorkItemSize);
      runBenchmark( particles );
      particles = previousParticles*2 < particles ? particles*2 : previousParticles*2;
    }

    peano4::shutdownSingletons();
    repositories::DataRepository::shutdownDatatypes();
    peano4::shutdownParallelEnvironment();

    return ExitCodeSuccess;
}
