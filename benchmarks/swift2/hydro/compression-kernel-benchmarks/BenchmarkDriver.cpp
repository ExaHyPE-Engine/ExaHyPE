#include "Benchmarks-main.h"
#include "Constants.h"

#include "tarch/NonCriticalAssertions.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"

#include "peano4/peano.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "repositories/DataRepository.h"
#include "repositories/StepRepository.h"
#include "repositories/GlobalState.h"

#include "peano4/UnitTests.h"
#include "tarch/UnitTests.h"
#include "swift2/UnitTests.h"
#include "toolbox/particles/UnitTests.h"

#include "swift2/UserInterface.h"

#include "Constants.h"

#include "tarch/logging/LogFilterFileReader.h"

#include "observers/CreateGrid.h"
#include "observers/InitialConditions.h"
#include "observers/Plot.h"


#include "observers/StepHydroPartSPH_Drift_sweep0.h"

#include "observers/DummyStepHydroPartPredictHydro_sweep1.h"

#include "observers/StepHydroPartPredictHydro_sweep2.h"

#include "observers/StepHydroPartSPH_DensityLoopWithConstantSearchRadius_sweep3.h"

#include "observers/StepHydroPartSPH_ForceLoop_sweep4.h"

#include "observers/StepHydroPartSPH_Kick2_sweep5.h"

#include "observers/StepHydroPartSPH_ReduceGlobalQuantities_sweep6.h"

#include "observers/StepHydroPartSPH_Kick1_sweep7.h"



#include "observers/InitHydroPartSPH_DensityLoopWithConstantSearchRadius_sweep0.h"

#include "observers/InitHydroPartSPH_ForceLoop_sweep1.h"

#include "observers/InitHydroPartSPH_Kick2_sweep2.h"

#include "swift2/scenario/NohProblem.h"

#include "tarch/timing/Measurement.h"
#include "tarch/multicore/otter.h"


tarch::logging::Log _log("::");


using namespace benchmarks::swift2::hydro::aos_vs_soa_kernel_benchmarks;


/**
 * See initGridTraversalEvent()
 */
peano4::grid::GridTraversalEvent      gridTraversalEvent;


/**
 * Initialise the particles with some meaningful information
 */
void initParticles( globaldata::HydroPart* particles, int numberOfParticles ) {
    int particlesPerAxis = static_cast<int>( std::ceil( std::pow(numberOfParticles, 1.0/Dimensions ) ) + 1.0 );
    logInfo( "initParticles(...)", "map onto " << particlesPerAxis << "^d grid as initial layout" );
    const double h = 1.0 / particlesPerAxis;
    for (int i=0; i<numberOfParticles; i++) {
        tarch::la::Vector<Dimensions,double> x = {
                (i % particlesPerAxis) * h
                , (i % (particlesPerAxis*particlesPerAxis) ) * h
                #if Dimensions==3
                , (i % (particlesPerAxis*particlesPerAxis*particlesPerAxis) ) * h
                #endif
        };
        particles[i].setX(x);
        ::swift2::scenario::initNohProblemIC(particles[i], 1.0, particlesPerAxis, Dimensions );
    }
}


void runCPUBenchmark(int numberOfParticles) {
    const int Samples = 16;
    logInfo( "runBenchmark(int)", "===============================");
    logInfo( "runBenchmark(int)", "  " << numberOfParticles << " particles (" << Samples << " samples)" );
    logInfo( "runBenchmark(int)", "===============================");

    const peano4::datamanagement::VertexMarker  vertexMarker(gridTraversalEvent);
    const peano4::datamanagement::CellMarker    cellMarker(gridTraversalEvent);

    ::tarch::timing::Measurement         timingForceKernel;
    ::tarch::timing::Measurement         timingDensityKernel;
    ::tarch::timing::Measurement         timingKick1Kernel;
    ::tarch::timing::Measurement         timingKick2Kernel;
    ::tarch::timing::Measurement         timingDriftKernel;

    // Our generic kernels require a sequence of pointers over the actual
    // particles. In our setup, we need one set per core.
    globaldata::HydroPart*    particles = tarch::allocateMemory<globaldata::HydroPart>(numberOfParticles, tarch::MemoryLocation::Heap);

    initParticles( particles, numberOfParticles );

    const int numberOfThreads = tarch::multicore::Core::getInstance().getNumberOfThreads();
    vertexdata::HydroPartSet  particleSet[numberOfThreads];

    int numberOfParticlesPerThread = numberOfParticles / numberOfThreads;
    if ( (numberOfParticles % numberOfParticlesPerThread)!=0 ) {
        numberOfParticlesPerThread++;
    }

    for (int i=0; i<numberOfParticles; i++) {
        // we somehow will need to initialise particles with something meaningful. Notably the positions
        // have to be spread out I'd think.
        // @todo
        particleSet[i / numberOfParticlesPerThread].addParticle(particles+i);
    }

    for (int sample=0; sample<Samples; sample++) {
//        #pragma omp parallel
//        #pragma omp master
        {

//        {
//            for (int thread=0; thread<numberOfThreads; thread++) {
//                ::swift2::kernels::forAllParticles( vertexMarker, particleSet[thread], ::swift2::kernels::legacy::resetSmoothingLengthIterationCounter<globaldata::HydroPart>);
//                //::swift2::kernels::forAllParticles( vertexMarker, particleSet[thread], ::swift2::kernels::legacy::prepareHydroForce<globaldata::HydroPart>);
//                ::swift2::kernels::forAllParticles( vertexMarker, particleSet[thread], ::swift2::kernels::legacy::resetAcceleration<globaldata::HydroPart>);
//            }
//
//            ::tarch::timing::Watch watch("", "runBenchmark(int)", false);
//
//#pragma omp parallel for
//            for (int thread=0; thread<numberOfThreads; thread++) {
//                ::swift2::kernels::forAllParticlePairs( cellMarker, particleSet[thread], particleSet[thread], ::swift2::kernels::legacy::forceKernel<globaldata::HydroPart>);
//            }
//
//            watch.stop();
//            timingForceKernel.setValue( watch.getCalendarTime() );
//        }

//        {
//            for (int thread=0; thread<numberOfThreads; thread++) {
//                ::swift2::kernels::forAllParticles( vertexMarker, particleSet[thread], ::swift2::kernels::legacy::prepareDensity<globaldata::HydroPart>);
//            }
//
//            ::tarch::timing::Watch watch("", "runBenchmark(int)", false);
//
//#pragma omp parallel for
//            for (int thread=0; thread<numberOfThreads; thread++) {
//                ::swift2::kernels::forAllParticlePairs( cellMarker, particleSet[thread], particleSet[thread], ::swift2::kernels::legacy::densityKernel<globaldata::HydroPart>);
//            }
//
//            watch.stop();
//            timingDensityKernel.setValue( watch.getCalendarTime() );
//        }

        {
            ::tarch::timing::Watch watch("", "runBenchmark(int)", false);

#pragma omp parallel for schedule(static,1)
            for (int thread=0; thread<numberOfThreads; thread++) {
                ::swift2::kernels::forAllParticles( vertexMarker, particleSet[thread], ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize<globaldata::HydroPart>);
            }

            watch.stop();
            timingKick1Kernel.setValue( watch.getCalendarTime() );
        }

        {
            ::tarch::timing::Watch watch("", "runBenchmark(int)", false);

#pragma omp parallel for schedule(static,1)
            for (int thread=0; thread<numberOfThreads; thread++) {
                ::swift2::kernels::forAllParticles( vertexMarker, particleSet[thread], ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize<globaldata::HydroPart>);
                ::swift2::kernels::forAllParticles( vertexMarker, particleSet[thread], ::swift2::kernels::legacy::resetPredictedValues<globaldata::HydroPart>);
            }

            watch.stop();
            timingKick2Kernel.setValue( watch.getCalendarTime() );
        }

        {
            ::tarch::timing::Watch watch("", "runBenchmark(int)", false);

#pragma omp parallel for schedule(static,1)
            for (int thread=0; thread<numberOfThreads; thread++) {
                ::swift2::kernels::forAllParticles( vertexMarker, particleSet[thread], ::swift2::timestepping::resetMovedParticleMarker<globaldata::HydroPart>);
            }

            watch.stop();
            timingDriftKernel.setValue( watch.getCalendarTime() );
        }
    }
    }

    logInfo( "runBenchmark(int)", "density kernel: " << timingDensityKernel.getValue() << "  " << timingDensityKernel.toString() );
    logInfo( "runBenchmark(int)", "force kernel:   " << timingForceKernel.getValue() << "  " << timingForceKernel.toString() );
    logInfo( "runBenchmark(int)", "kick1 kernel:   " << timingKick1Kernel.getValue() << "  " << timingKick1Kernel.toString() );
    logInfo( "runBenchmark(int)", "kick2 kernel:   " << timingKick2Kernel.getValue() << "  " << timingKick2Kernel.toString() );
    logInfo( "runBenchmark(int)", "drift kernel:   " << timingDriftKernel.getValue() << "  " << timingDriftKernel.toString() );

    tarch::freeMemory(particles, tarch::MemoryLocation::Heap);
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
            benchmarks::swift2::hydro::aos_vs_soa_kernel_benchmarks::DomainOffset,
            benchmarks::swift2::hydro::aos_vs_soa_kernel_benchmarks::DomainSize,
            benchmarks::swift2::hydro::aos_vs_soa_kernel_benchmarks::PeriodicBC
    );

    benchmarks::swift2::hydro::aos_vs_soa_kernel_benchmarks::repositories::DataRepository::initDatatypes();

    initGridTraversalEvent();

    if (not swift2::parseCommandLineArguments(argc,argv) ) {
        logError("main()", "Invalid command line arguments.");
        return ExitCodeInvalidArguments;
    }

    #if PeanoDebug>=2
    tarch::tests::TestCase* peanoCoreTests = peano4::getUnitTests();
  peanoCoreTests->run();
  if (peanoCoreTests->getNumberOfErrors() != 0) {
    logError("main()", "Peano4 core unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }
  delete peanoCoreTests;

  tarch::tests::TestCase* tarchTests = tarch::getUnitTests();
  tarchTests->run();
  if (tarchTests->getNumberOfErrors() != 0) {
    logError("main()", "technical architecture (tarch) unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }
  delete tarchTests;

  tarch::tests::TestCase* swiftTests = swift2::getUnitTests();
  swiftTests->run();
  if (swiftTests->getNumberOfErrors() != 0) {
    logError("main()", "Swift unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }
  delete swiftTests;

  tarch::tests::TestCase* particlesTests = toolbox::particles::getUnitTests();
  particlesTests->run();
  if (particlesTests->getNumberOfErrors() != 0) {
    logError("main()", "toolbox particles unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }
  delete particlesTests;
    #endif

    tarch::logging::Statistics::getInstance().clear();


    const int numberOfThreads = tarch::multicore::Core::getInstance().getNumberOfThreads();

    logInfo( "runBenchmark(int)", "===============================");
    logInfo( "runBenchmark(int)", " threads=" << numberOfThreads );
    logInfo( "runBenchmark(int)", " sizeof(Particle)=" << sizeof(globaldata::HydroPart) );
    logInfo( "runBenchmark(int)", "===============================");

    for (int particles=1; particles<=MaxParticles; particles*=2) {
        if (particles>=numberOfThreads) {
            runCPUBenchmark( particles );
        }
    }


    peano4::shutdownSingletons();
    benchmarks::swift2::hydro::aos_vs_soa_kernel_benchmarks::repositories::DataRepository::shutdownDatatypes();
    peano4::shutdownParallelEnvironment();

    return ExitCodeSuccess;
}
