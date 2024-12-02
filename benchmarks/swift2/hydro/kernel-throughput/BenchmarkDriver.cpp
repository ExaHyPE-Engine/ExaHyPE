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


void initParticles( vertexdata::HydroPartSet*  workItemParticleSet, int numberOfWorkItems, int numberOfParticles ) {
  int currentNumberOfParticles = 0;
  for (int currentWorkItem=0; currentWorkItem<numberOfWorkItems; currentWorkItem++) {
    for (auto& p: workItemParticleSet[currentWorkItem]) {
      initParticle(*p, numberOfParticles, currentNumberOfParticles);
      currentNumberOfParticles++;
    }
  }
}


void runCPUBenchmark(int numberOfParticles) {
    const peano4::datamanagement::VertexMarker  vertexMarker(gridTraversalEvent);
    const peano4::datamanagement::CellMarker    cellMarker(gridTraversalEvent);

    ::tarch::timing::Measurement         timingBaselineForceKernel;
    ::tarch::timing::Measurement         timingBaselineDensityKernel;
    ::tarch::timing::Measurement         timingBaselineKick1Kernel;
    ::tarch::timing::Measurement         timingBaselineKick2Kernel;
    ::tarch::timing::Measurement         timingBaselineDriftKernel;

    int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;
    numberOfWorkItems += (numberOfParticles % Benchmark_WorkItemSize==0) ? 0 : 1;
    vertexdata::HydroPartSet*  workItemParticleSet = new vertexdata::HydroPartSet[numberOfWorkItems];

    int currentNumberOfParticles = 0;
    for (int currentWorkItem=0; currentWorkItem<numberOfWorkItems; currentWorkItem++) {
      int numberOfParticlesForThisWorkItem = std::min( Benchmark_WorkItemSize, numberOfParticles-currentNumberOfParticles );
      globaldata::HydroPart*    particles = tarch::allocateMemory<globaldata::HydroPart>(numberOfParticlesForThisWorkItem, tarch::MemoryLocation::Heap);
      for (int i=0; i<numberOfParticlesForThisWorkItem; i++) {
        workItemParticleSet[currentWorkItem].addParticle(particles+i);
      }
      currentNumberOfParticles += numberOfParticlesForThisWorkItem;
    }

    logInfo( "runBenchmark(int)", "===============================");
    logInfo( "runBenchmark(int)", " Experiment: " << numberOfParticles << " particles" );
    logInfo( "runBenchmark(int)", " No of work items: " << numberOfWorkItems );
    logInfo( "runBenchmark(int)", "===============================");

    for (int sample=0; sample<Benchmark_Samples; sample++) {
        if (Benchmark_Testforce) {
            initParticles(workItemParticleSet, numberOfWorkItems, numberOfParticles);
            for (int workItem=0; workItem<numberOfWorkItems; workItem++) {
                ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::resetSmoothingLengthIterationCounter<globaldata::HydroPart>);
                ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::resetAcceleration<globaldata::HydroPart>);
            }


            ::tarch::timing::Watch watch("", "runBenchmark(int)", false);

            std::vector<int> dummyEntries(TwoPowerD);
            std::vector<int> coalscedEntries(TwoPowerD);
            for (int i=0; i<TwoPowerD; i++) {
              dummyEntries[i]=-1;
            }

            #pragma omp parallel for schedule(static,1)
            for (int workItem=0; workItem<numberOfWorkItems; workItem++) {

              for (int i=0; i<Benchmark_WorkItemRepeats; i++) {
                ::swift2::kernels::forAllParticlePairs( cellMarker, workItemParticleSet[workItem], workItemParticleSet[workItem], dummyEntries, dummyEntries, ::swift2::kernels::legacy::forceKernelWithMasking<globaldata::HydroPart>);
              }
            }

            watch.stop();
            timingBaselineForceKernel.setValue( watch.getCalendarTime()/Benchmark_WorkItemRepeats );
        }

        if (Benchmark_Testdensity) {
            initParticles(workItemParticleSet, numberOfWorkItems, numberOfParticles);
            for (int workItem=0; workItem<numberOfWorkItems; workItem++) {
                ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::prepareDensity<globaldata::HydroPart>);
            }


            ::tarch::timing::Watch watch("", "runBenchmark(int)", false);

            std::vector<int> dummyEntries(TwoPowerD);
            std::vector<int> coalscedEnries(TwoPowerD);
            for (int i=0; i<TwoPowerD; i++) {
              dummyEntries[i]=-1;
            }

            #pragma omp parallel for schedule(static,1)
            for (int workItem=0; workItem<numberOfWorkItems; workItem++) {
              for (int i=0; i<Benchmark_WorkItemRepeats; i++) {
                ::swift2::kernels::forAllParticlePairs( cellMarker, workItemParticleSet[workItem], workItemParticleSet[workItem], dummyEntries, dummyEntries, ::swift2::kernels::legacy::densityKernelWithMasking<globaldata::HydroPart>);
              }
            }

            watch.stop();
            timingBaselineDensityKernel.setValue( watch.getCalendarTime()/Benchmark_WorkItemRepeats );
        }

        if (Benchmark_Testkick1) {
            initParticles(workItemParticleSet, numberOfWorkItems, numberOfParticles);

            ::tarch::timing::Watch watch("", "runBenchmark(int)", false);

            #pragma omp parallel for schedule(static,1)
            for (int workItem=0; workItem<numberOfWorkItems; workItem++) {
              for (int i=0; i<Benchmark_WorkItemRepeats; i++) {
                ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSizeWithMasking<globaldata::HydroPart>);
              }
            }

            watch.stop();
            timingBaselineKick1Kernel.setValue( watch.getCalendarTime()/Benchmark_WorkItemRepeats );
        }

        if (Benchmark_Testkick2) {
            initParticles(workItemParticleSet, numberOfWorkItems, numberOfParticles);

            ::tarch::timing::Watch watch("", "runBenchmark(int)", false);

            #pragma omp parallel for schedule(static,1)
            for (int workItem=0; workItem<numberOfWorkItems; workItem++) {
              for (int i=0; i<Benchmark_WorkItemRepeats; i++) {
                ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSizeWithMasking<globaldata::HydroPart>);
                ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::resetPredictedValuesWithMasking<globaldata::HydroPart>);
              }
            }

            watch.stop();
            timingBaselineKick2Kernel.setValue( watch.getCalendarTime()/Benchmark_WorkItemRepeats );
        }

        if (Benchmark_Testdrift) {
            initParticles(workItemParticleSet, numberOfWorkItems, numberOfParticles);

            ::tarch::timing::Watch watch("", "runBenchmark(int)", false);

            #pragma omp parallel for schedule(static,1)
            for (int workItem=0; workItem<numberOfWorkItems; workItem++) {
              ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::timestepping::resetMovedParticleMarkerWithMasking<globaldata::HydroPart>);
              ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], -1, ::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSizeWithMasking<globaldata::HydroPart>);
            }

            watch.stop();
            timingBaselineDriftKernel.setValue( watch.getCalendarTime()/Benchmark_WorkItemRepeats );
        }
    } // end of sample loop

    logInfo( "runBenchmark(int)", "density (baseline): " << timingBaselineDensityKernel.getValue() << "  " << timingBaselineDensityKernel.toString() );
    logInfo( "runBenchmark(int)", "force (baseline):   " << timingBaselineForceKernel.getValue() << "  " << timingBaselineForceKernel.toString() );
    logInfo( "runBenchmark(int)", "kick1 (baseline):   " << timingBaselineKick1Kernel.getValue() << "  " << timingBaselineKick1Kernel.toString() );
    logInfo( "runBenchmark(int)", "kick2 (baseline):   " << timingBaselineKick2Kernel.getValue() << "  " << timingBaselineKick2Kernel.toString() );
    logInfo( "runBenchmark(int)", "drift (baseline):   " << timingBaselineDriftKernel.getValue() << "  " << timingBaselineDriftKernel.toString() );

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

    for (int particles=Benchmark_MinParticles; particles<=Benchmark_MaxParticles; particles*=2) {
      runCPUBenchmark( particles );
    }


    peano4::shutdownSingletons();
    repositories::DataRepository::shutdownDatatypes();
    peano4::shutdownParallelEnvironment();

    return ExitCodeSuccess;
}
