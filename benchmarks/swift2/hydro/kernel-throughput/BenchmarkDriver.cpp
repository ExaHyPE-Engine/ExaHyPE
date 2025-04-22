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

#include "swift2/kernels/legacy/Density.h"
#include "swift2/kernels/legacy/HydroForce.h"
#include "swift2/kernels/legacy/Leapfrog.h"
#include "swift2/kernels/legacy/SmoothingLength.h"

#include "swift2/timestepping/Leapfrog.h"
#include "swift2/timestepping/TimeStepping.h"

#include "swift2/kernels/ParticleSetIterators.h"


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
vertexdata::HydroPartSet*             scatteredWorkItemParticleSet;


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
  particle.setCellH(0.5);
  particle.setMoveState(globaldata::HydroPart::MoveState::NotMovedYet);
  particle.setParallelState(globaldata::HydroPart::ParallelState::Local);
  #if Dimensions==2
  particle.setV_full( {1e-4,1e-3} );
  #else
  particle.setV_full( {1e-4,1e-3,1e-2} );
  #endif

  initNohProblemIC(particle, 1.0, particlesPerAxis, Dimensions );

  std::bitset<TwoPowerD> flags;
  flags.set();
  particle.setContainedInAdjacentCell(flags);

  particle.setSmoothingLengthConverged(false);
  particle.setSmoothingLength(1.0/2.0);

  globaldata::HydroPart::getSpecies().setTimeStepSize(1e-5, 1e-3, false);
}


void initParticles( int numberOfParticles, bool initScatteredData ) {
  const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;

  if (initScatteredData) {
    int currentNumberOfParticles = 0;
    for (int currentWorkItem=0; currentWorkItem<numberOfWorkItems; currentWorkItem++) {
      for (auto& p: scatteredWorkItemParticleSet[currentWorkItem]) {
        initParticle(*p, numberOfParticles, currentNumberOfParticles);
        currentNumberOfParticles++;
      }
    }
  }
  else {
    int currentNumberOfParticles = 0;
    for (int currentWorkItem=0; currentWorkItem<numberOfWorkItems; currentWorkItem++) {
      for (auto& p: workItemParticleSet[currentWorkItem]) {
        initParticle(*p, numberOfParticles, currentNumberOfParticles);
        currentNumberOfParticles++;
      }
    }
  }
}


/**
 * Validate the scattered data against the continuous data
 *
 * We assume that the benchmark always assesses the generic kernel operating on
 * scattered data first and that there's exactly one run with this data set.
 * That means that we can assume that the scattered data is our validation data
 * and the kernels over the continuous dataset have to yield exactly the same
 * outcome. We validate this here.
 */
void validateOutcomes( int numberOfParticles ) {
  const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;

  int currentParticle = 0;
  for (int currentWorkItem=0; currentWorkItem<numberOfWorkItems; currentWorkItem++) {
    auto scatteredP  = scatteredWorkItemParticleSet[currentWorkItem].begin();
    auto continuousP = workItemParticleSet[currentWorkItem].begin();

    while (scatteredP != scatteredWorkItemParticleSet[currentWorkItem].end()) {
      if (
        not ((*scatteredP)->getX()(0)==(*scatteredP)->getX()(0))
        or
        not ((*scatteredP)->getX()(1)==(*scatteredP)->getX()(1))
        or
        not tarch::la::equals( (*scatteredP)->getX(), (*continuousP)->getX() )
      ) {
        std::cerr << "ERROR in x:" << std::endl
                  << "\t scatteredP  = " << (*scatteredP)->toString() << std::endl
                  << "\t continuousP = "    << (*continuousP)->toString() << std::endl;
        exit(-1);
      }
      if (
        not tarch::la::equals( (*scatteredP)->getSearchRadius(), (*continuousP)->getSearchRadius() )
      ) {
        std::cerr << "ERROR in search radius:" << std::endl
                  << "\t scatteredP  = " << (*scatteredP)->toString() << std::endl
                  << "\t continuousP = "    << (*continuousP)->toString() << std::endl;
        exit(-1);
      }
      if (
        ( (*scatteredP)->getContainedInAdjacentCell()!=(*continuousP)->getContainedInAdjacentCell() )
      ) {
        std::cerr << "ERROR in contained in adjacent cell flag:" << std::endl
                  << "\t scatteredP  = " << (*scatteredP)->toString() << std::endl
                  << "\t continuousP = "    << (*continuousP)->toString() << std::endl;
        exit(-1);
      }
      if (
        ( (*scatteredP)->getParallelState()!=(*continuousP)->getParallelState() )
      ) {
        std::cerr << "ERROR in contained in parallel state:" << std::endl
                  << "\t scatteredP  = " << (*scatteredP)->toString() << std::endl
                  << "\t continuousP = "    << (*continuousP)->toString() << std::endl;
        exit(-1);
      }
      if (
        ( (*scatteredP)->getMoveState()!=(*continuousP)->getMoveState() )
      ) {
        std::cerr << "ERROR in contained in move state:" << std::endl
                  << "\t scatteredP  = " << (*scatteredP)->toString() << std::endl
                  << "\t continuousP = "    << (*continuousP)->toString() << std::endl;
        exit(-1);
      }
      if (
        ( (*scatteredP)->getMass()!=(*continuousP)->getMass() )
      ) {
        std::cerr << "ERROR in contained in mass:" << std::endl
                  << "\t scatteredP  = " << (*scatteredP)->toString() << std::endl
                  << "\t continuousP = "    << (*continuousP)->toString() << std::endl;
        exit(-1);
      }

//      continuousP = (,x=[1.27116e+308,1.27116e+308],cellH=[0.5,0.5],
      // v=[0.707107,0.707107],a=[0,0],density=1,pressure=6.66667e-07,smoothingLength=0.5,u=1e-06,uDot=0,v_full=[0.707107,0.707107],u_full=1e-06,wcount=0,wcount_dh=0,f=0,hDot=0,rho_dh=0,smoothingLengthIterCount=0,smlIterLeftBound=0,smlIterRightBound=0,hasNoNeighbours=0,smoothingLengthConverged=0,cfl=0.01,initialTimeStepSize=0.01,adjustTimeStepSize=0,hydroDimensions=2,etaFactor=1.2348,smlMin=1e-06,smlMax=0.166667,smlTolerance=0.0001,smlMaxIterations=30,alphaAV=0.8,betaAV=3,balsara=0,rot_v=0,div_v=0,v_sig_AV=0.00210819,soundSpeed=0.00105409)


      scatteredP++;
      continuousP++;
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
void prepareForceBenchmark(int numberOfParticles, bool initScatteredData) {
  initParticles(numberOfParticles, initScatteredData);

  const peano4::datamanagement::VertexMarker  vertexMarker(gridTraversalEvent);
  const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;

  for (int workItem=0; workItem<numberOfWorkItems; workItem++) {
    if (initScatteredData) {
      ::swift2::kernels::forAllParticles<
        ::swift2::kernels::legacy::resetSmoothingLengthIterationCounter<globaldata::HydroPart>,
        ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
      >( vertexMarker, scatteredWorkItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::Generic );

      ::swift2::kernels::forAllParticles<
        ::swift2::kernels::legacy::resetAcceleration<globaldata::HydroPart>,
        ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
      >( vertexMarker, scatteredWorkItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::Generic);
    }
    else {
      ::swift2::kernels::forAllParticles<
        ::swift2::kernels::legacy::resetSmoothingLengthIterationCounter<globaldata::HydroPart>,
        ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
      >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::Generic );

      ::swift2::kernels::forAllParticles<
        ::swift2::kernels::legacy::resetAcceleration<globaldata::HydroPart>,
        ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
      >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::Generic);
    }
  }
}



std::vector<int> CoalescedMemoryIndices(TwoPowerD);
std::vector<int> ScatteredMemoryIndices(1);


/**
 * Create CoalescedMemory particle indicator
 *
 * Whenever we enter a cell, we are given all the particles associated to the
 * @f$ 2^d @f$ vertices of the cell as one big container. If we work with
 * CoalescedMemory storage schemes, these particles will technically be stored as
 * @f$ 2^d @f$ continuous sequences. In the benchmark, all particles of one
 * work item are held
 * in one continuous sequence, which is a special case of this. Nevertheless,
 * we create a vector of integeres, which pretends that they were split up further.
 *
 * So if chunk has 18 entries, it might, in 2d, holds the entries 5,5,5,3.
 *
 */
void createCoalescedMemoryIndices() {
  int remainingParticlesToAssignToChunk = Benchmark_WorkItemSize;
  int chunkSize = std::max(1,Benchmark_WorkItemSize / TwoPowerD);
  chunkSize += (Benchmark_WorkItemSize % TwoPowerD == 0) ? 0 : 1;
  for (int i=0; i<TwoPowerD; i++) {
    CoalescedMemoryIndices[i]          = chunkSize;
    remainingParticlesToAssignToChunk -= chunkSize;
    chunkSize                          = std::min( chunkSize, remainingParticlesToAssignToChunk );
  }

  ScatteredMemoryIndices[0] = -1;
}



void allocateParticles(int numberOfParticles) {
  const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;
  assertionEquals( numberOfParticles % Benchmark_WorkItemSize, 0);

  workItemParticleSet          = new vertexdata::HydroPartSet[numberOfWorkItems];
  scatteredWorkItemParticleSet = new vertexdata::HydroPartSet[numberOfWorkItems];

  int currentNumberOfParticles = 0;
  for (int currentWorkItem=0; currentWorkItem<numberOfWorkItems; currentWorkItem++) {
    int numberOfParticlesForThisWorkItem = std::min( Benchmark_WorkItemSize, numberOfParticles-currentNumberOfParticles );
    // @todo We might want to allocate in managed memory
    globaldata::HydroPart*    particles = tarch::allocateMemory<globaldata::HydroPart>(numberOfParticlesForThisWorkItem, tarch::MemoryLocation::Heap);
    for (int i=0; i<numberOfParticlesForThisWorkItem; i++) {
      workItemParticleSet[currentWorkItem].addParticle(particles+i);
      scatteredWorkItemParticleSet[currentWorkItem].addParticle( new globaldata::HydroPart() );
    }
    currentNumberOfParticles += numberOfParticlesForThisWorkItem;
  }
}


void deleteParticles(int numberOfParticles) {
  const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;
  assertionEquals( numberOfParticles % Benchmark_WorkItemSize, 0);

  for (int currentWorkItem=0; currentWorkItem<numberOfWorkItems; currentWorkItem++) {
    tarch::freeMemory(*workItemParticleSet[currentWorkItem].begin(), tarch::MemoryLocation::Heap);
    for (auto& p: scatteredWorkItemParticleSet[currentWorkItem]) {
      delete p;
    }
  }

  delete[] workItemParticleSet;
  delete[] scatteredWorkItemParticleSet;
}


/**
 *
 *
 * @param firstAssessCallForThisProgramPhase If it is true, we do not check
 *        the continuous data structure against the scattered one.
 */
void assessKernel(
  std::function< void(int) >  kernelCallInLoop,
  const std::string&          name,
  int                         numberOfParticles,
  bool                        firstAssessCallForThisProgramPhase
) {
  const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;

  ::tarch::timing::Measurement         measurement;
  for (int sample=0; sample<Benchmark_Samples; sample++) {
    ::tarch::timing::Watch watch("", "runBenchmark(int)", false);
    #pragma omp parallel for schedule(static,1)
    for (int i=0; i<Benchmark_WorkItemRepeats; i++) {
      for (int workItem=0; workItem<numberOfWorkItems; workItem++) {
        kernelCallInLoop(i);
      }
    }
    watch.stop();
    measurement.setValue( watch.getCalendarTime()/Benchmark_WorkItemRepeats/numberOfParticles );
  }
  #if defined(ValidateSWIFTKernelOutcomes)
  if (not firstAssessCallForThisProgramPhase) {
    validateOutcomes(numberOfParticles);
  }
  #endif
  logInfo( "assessKernel(int)", name << ":\t" << measurement.getValue() << "  " << measurement.toString() );
}


void runBenchmark(int numberOfParticles) {
    const peano4::datamanagement::VertexMarker  vertexMarker(gridTraversalEvent);
    const peano4::datamanagement::CellMarker    cellMarker(gridTraversalEvent);
    const std::vector<bool> localParticleContainedInCell(Benchmark_MaxParticles, true);

    allocateParticles(numberOfParticles);

    const int numberOfWorkItems = numberOfParticles / Benchmark_WorkItemSize;

    logInfo( "runBenchmark(int)", "===============================");
    logInfo( "runBenchmark(int)", " Experiment: " << numberOfParticles << " particles" );
    logInfo( "runBenchmark(int)", " No of work items: " << numberOfWorkItems );
    logInfo( "runBenchmark(int)", "===============================");


    if (Benchmark_Testforce) {
      prepareForceBenchmark(numberOfParticles, true);
      assessKernel(
          [&](int workItem) -> void {
          const swift2::CellMetaData cellMetaData(cellMarker, localParticleContainedInCell);

            ::swift2::kernels::forAllParticlePairs<
              ::swift2::kernels::legacy::forceKernel<globaldata::HydroPart>,
              ::swift2::kernels::alwaysUpdateInCellKernel<globaldata::HydroPart>,
              ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<globaldata::HydroPart>
            >( cellMetaData, scatteredWorkItemParticleSet[workItem], scatteredWorkItemParticleSet[workItem], ::swift2::kernels::kernelrealisation::Generic);
          },
          "force (scattered,generic)",
          numberOfParticles,
          true
      );

      prepareForceBenchmark(numberOfParticles, false);
      assessKernel(
          [&](int workItem) -> void {
          const swift2::CellMetaData cellMetaData(cellMarker, localParticleContainedInCell);

            ::swift2::kernels::forAllParticlePairs<
              ::swift2::kernels::legacy::forceKernel<globaldata::HydroPart>,
              ::swift2::kernels::alwaysUpdateInCellKernel<globaldata::HydroPart>,
              ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<globaldata::HydroPart>
            >( cellMetaData, workItemParticleSet[workItem], workItemParticleSet[workItem], ::swift2::kernels::kernelrealisation::Generic);
          },
          "force (continuous,generic)",
          numberOfParticles,
          false
      );

      prepareForceBenchmark(numberOfParticles, false);
      assessKernel(
          [&](int workItem) -> void {
          const swift2::CellMetaData cellMetaData(cellMarker, CoalescedMemoryIndices, CoalescedMemoryIndices, localParticleContainedInCell);

            ::swift2::kernels::forAllParticlePairs<
              ::swift2::kernels::legacy::forceKernel<globaldata::HydroPart>,
              ::swift2::kernels::alwaysUpdateInCellKernel<globaldata::HydroPart>,
              ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<globaldata::HydroPart>
            >( cellMetaData, workItemParticleSet[workItem], workItemParticleSet[workItem], ::swift2::kernels::kernelrealisation::CoalescedMemoryLocalActive);
          },
          "force (continuous,local-active-coalesced)",
          numberOfParticles,
          false
      );

      prepareForceBenchmark(numberOfParticles, false);
      assessKernel(
        [&](int workItem) -> void {
          const swift2::CellMetaData cellMetaData(cellMarker, CoalescedMemoryIndices, CoalescedMemoryIndices, localParticleContainedInCell);

          ::swift2::kernels::forAllParticlePairs<
            ::swift2::kernels::legacy::forceKernel<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInCellKernel<globaldata::HydroPart>,
            ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<globaldata::HydroPart>
            >( cellMetaData, workItemParticleSet[workItem], workItemParticleSet[workItem], ::swift2::kernels::kernelrealisation::CoalescedMemoryActiveLocal);
        },
        "force (continuous,active-local-coalesced)",
        numberOfParticles,
        false
      );

//      prepareForceBenchmark(numberOfParticles);
//      assessKernel(
//        [&](int workItem) -> void {
//          ::swift2::kernels::forAllParticlePairs( cellMarker, workItemParticleSet[workItem], workItemParticleSet[workItem], CoalescedMemoryIndices, CoalescedMemoryIndices, ::swift2::kernels::legacy::forceKernel<globaldata::HydroPart>, ::swift2::kernels::alwaysUpdateInCellKernel<globaldata::HydroPart>, ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<globaldata::HydroPart>, ::swift2::kernels::kernelrealisation::EvaluatePredicateInPrologueLocalActive);
//        },
//        "force (EvaluatePredicateInPrologueLocalActive)",
//        numberOfParticles
//      );
//      assessKernel(
//        [&](int workItem) -> void {
//          ::swift2::kernels::forAllParticlePairs( cellMarker, workItemParticleSet[workItem], workItemParticleSet[workItem], CoalescedMemoryIndices, CoalescedMemoryIndices, ::swift2::kernels::legacy::forceKernel<globaldata::HydroPart>, ::swift2::kernels::alwaysUpdateInCellKernel<globaldata::HydroPart>, ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<globaldata::HydroPart>, ::swift2::kernels::kernelrealisation::EvaluatePredicateInPrologueActiveLocal);
//        },
//        "force (EvaluatePredicateInPrologueActiveLocal)",
//        numberOfParticles
//      );
    }

    if (Benchmark_Testdensity) {
      initParticles(numberOfParticles, true);
      assessKernel(
        [&](int workItem) -> void {
          const swift2::CellMetaData cellMetaData(cellMarker, localParticleContainedInCell);

          ::swift2::kernels::forAllParticlePairs<
            ::swift2::kernels::legacy::densityKernel<globaldata::HydroPart>,
            ::swift2::kernels::localParticleCanBeUpdatedInCellKernel<globaldata::HydroPart>,
            ::swift2::kernels::legacy::densityKernelEvaluatePairInteractionPredicate<globaldata::HydroPart>
          >( cellMetaData, scatteredWorkItemParticleSet[workItem], scatteredWorkItemParticleSet[workItem], ::swift2::kernels::kernelrealisation::Generic);
        },
        "density (scattered,generic)",
        numberOfParticles,
        true
      );

      initParticles(numberOfParticles, false);
      assessKernel(
        [&](int workItem) -> void {
          const swift2::CellMetaData cellMetaData(cellMarker, localParticleContainedInCell);

          ::swift2::kernels::forAllParticlePairs<
            ::swift2::kernels::legacy::densityKernel<globaldata::HydroPart>,
            ::swift2::kernels::localParticleCanBeUpdatedInCellKernel<globaldata::HydroPart>,
            ::swift2::kernels::legacy::densityKernelEvaluatePairInteractionPredicate<globaldata::HydroPart>
          >( cellMetaData, workItemParticleSet[workItem], workItemParticleSet[workItem], ::swift2::kernels::kernelrealisation::Generic);
        },
        "density (continuous,generic)",
        numberOfParticles,
        false
      );

      initParticles(numberOfParticles, false);
      assessKernel(
        [&](int workItem) -> void {
          const swift2::CellMetaData cellMetaData(cellMarker, CoalescedMemoryIndices, CoalescedMemoryIndices, localParticleContainedInCell);

          ::swift2::kernels::forAllParticlePairs<
            ::swift2::kernels::legacy::densityKernel<globaldata::HydroPart>,
            ::swift2::kernels::localParticleCanBeUpdatedInCellKernel<globaldata::HydroPart>,
            ::swift2::kernels::legacy::densityKernelEvaluatePairInteractionPredicate<globaldata::HydroPart>
          >( cellMetaData, workItemParticleSet[workItem], workItemParticleSet[workItem], ::swift2::kernels::kernelrealisation::CoalescedMemoryLocalActive);
        },
        "density (continuous,local-active)",
        numberOfParticles,
        false
      );

      initParticles(numberOfParticles, false);
      assessKernel(
        [&](int workItem) -> void {
          const swift2::CellMetaData cellMetaData(cellMarker, CoalescedMemoryIndices, CoalescedMemoryIndices, localParticleContainedInCell);

          ::swift2::kernels::forAllParticlePairs<
            ::swift2::kernels::legacy::densityKernel<globaldata::HydroPart>,
            ::swift2::kernels::localParticleCanBeUpdatedInCellKernel<globaldata::HydroPart>,
            ::swift2::kernels::legacy::densityKernelEvaluatePairInteractionPredicate<globaldata::HydroPart>
          >( cellMetaData, workItemParticleSet[workItem], workItemParticleSet[workItem], ::swift2::kernels::kernelrealisation::CoalescedMemoryActiveLocal);
        },
        "density (continuous,active-local-coalesced)",
        numberOfParticles,
        false
      );

//      initParticles(numberOfParticles, false);
//      assessKernel(
//        [&](int workItem) -> void {
//          ::swift2::kernels::forAllParticlePairs( cellMarker, workItemParticleSet[workItem], workItemParticleSet[workItem], CoalescedMemoryIndices, CoalescedMemoryIndices, ::swift2::kernels::legacy::densityKernel<globaldata::HydroPart>, ::swift2::kernels::localParticleCanBeUpdatedInCellKernel<globaldata::HydroPart>, ::swift2::kernels::legacy::densityKernelEvaluatePairInteractionPredicate<globaldata::HydroPart>, ::swift2::kernels::kernelrealisation::EvaluatePredicateInPrologueLocalActive);
//        },
//        "density (EvaluatePredicateInPrologueLocalActive)",
//        numberOfParticles
//      );

//      initParticles(numberOfParticles);

//      assessKernel(
//        [&](int workItem) -> void {
//          ::swift2::kernels::forAllParticlePairs( cellMarker, workItemParticleSet[workItem], workItemParticleSet[workItem], CoalescedMemoryIndices, CoalescedMemoryIndices, ::swift2::kernels::legacy::densityKernel<globaldata::HydroPart>, ::swift2::kernels::localParticleCanBeUpdatedInCellKernel<globaldata::HydroPart>, ::swift2::kernels::legacy::densityKernelEvaluatePairInteractionPredicate<globaldata::HydroPart>, ::swift2::kernels::kernelrealisation::EvaluatePredicateInPrologueActiveLocal);
//        },
//        "density (EvaluatePredicateInPrologueActiveLocal)",
//        numberOfParticles
//      );
    }

    if (Benchmark_Testkick1) {
      initParticles(numberOfParticles, true);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
          >( vertexMarker, scatteredWorkItemParticleSet[workItem], -1, ::swift2::kernels::kernelrealisation::Generic);
        },
        "kick1 (scattered,generic)",
        numberOfParticles,
        true
      );

      initParticles(numberOfParticles, false);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
          >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::Generic);
        },
        "kick1 (continuous,generic)",
        numberOfParticles,
        false
      );

      initParticles(numberOfParticles, false);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
          >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::CoalescedMemory);
        },
        "kick1 (continuous,coalesced)",
        numberOfParticles,
        false
      );
//      assessKernel(
//        [&](int workItem) -> void {
//          ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize<globaldata::HydroPart>, ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>, ::swift2::kernels::kernelrealisation::EvaluatePredicateInPrologue);
//        },
//        "kick1 (EvaluatePredicateInPrologue)",
//        numberOfParticles
//      );
    }

    if (Benchmark_Testkick2) {
      initParticles(numberOfParticles, true);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
          >( vertexMarker, scatteredWorkItemParticleSet[workItem], -1, ::swift2::kernels::kernelrealisation::Generic);

          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::resetPredictedValues<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
          >( vertexMarker, scatteredWorkItemParticleSet[workItem], -1, ::swift2::kernels::kernelrealisation::Generic);
        },
        "kick2 (scattered,generic)",
        numberOfParticles,
        true
      );

      initParticles(numberOfParticles, false);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
          >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::Generic);

          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::resetPredictedValues<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
          >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::Generic);
        },
        "kick2 (continuous,generic)",
        numberOfParticles,
        false
      );

      initParticles(numberOfParticles, false);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
          >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::CoalescedMemory);

          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::resetPredictedValues<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
          >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::CoalescedMemory);
        },
        "kick2 (continuous,coalesced)",
        numberOfParticles,
        false
      );
//      assessKernel(
//        [&](int workItem) -> void {
//          ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize<globaldata::HydroPart>, ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>, ::swift2::kernels::kernelrealisation::EvaluatePredicateInPrologue);
//          ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::legacy::resetPredictedValues<globaldata::HydroPart>, ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>, ::swift2::kernels::kernelrealisation::EvaluatePredicateInPrologue);
//        },
//        "kick2 (EvaluatePredicateInPrologue)",
//        numberOfParticles
//      );
    }

    if (Benchmark_Testdrift) {
      initParticles(numberOfParticles, true);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles<
            ::swift2::timestepping::resetMovedParticleMarker<globaldata::HydroPart>,
            ::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSizeParticleUpdatePredicate<globaldata::HydroPart>
          >( vertexMarker, scatteredWorkItemParticleSet[workItem], -1, ::swift2::kernels::kernelrealisation::Generic);

          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSize<globaldata::HydroPart>,
            ::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSizeParticleUpdatePredicate<globaldata::HydroPart>
          >( vertexMarker, scatteredWorkItemParticleSet[workItem], -1, ::swift2::kernels::kernelrealisation::Generic);
        },
        "drift (scattered,generic)",
        numberOfParticles,
        true
      );

      initParticles(numberOfParticles, false);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles<
            ::swift2::timestepping::resetMovedParticleMarker<globaldata::HydroPart>,
            ::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSizeParticleUpdatePredicate<globaldata::HydroPart>
          >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::Generic);

          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSize<globaldata::HydroPart>,
            ::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSizeParticleUpdatePredicate<globaldata::HydroPart>
          >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::Generic);
        },
        "drift (continuous,generic)",
        numberOfParticles,
        false
      );

      /*
      initParticles(numberOfParticles);
      assessKernel(
        [&](int workItem) -> void {
          ::swift2::kernels::forAllParticles<
            ::swift2::timestepping::resetMovedParticleMarker<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
          >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::CoalescedMemory);

          ::swift2::kernels::forAllParticles<
            ::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSize<globaldata::HydroPart>,
            ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>
          >( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::kernelrealisation::CoalescedMemory);
        },
        "drift (continuous,coalesced)",
        numberOfParticles,
        false
      );
*/
//      assessKernel(
//        [&](int workItem) -> void {
//          ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::timestepping::resetMovedParticleMarker<globaldata::HydroPart>, ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>, ::swift2::kernels::kernelrealisation::EvaluatePredicateInPrologue);
//          ::swift2::kernels::forAllParticles( vertexMarker, workItemParticleSet[workItem], Benchmark_WorkItemSize, ::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSize<globaldata::HydroPart>, ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::HydroPart>, ::swift2::kernels::kernelrealisation::EvaluatePredicateInPrologue);
//        },
//        "drift (EvaluatePredicateInPrologue)",
//        numberOfParticles
//      );
    } // end of sample loop

    deleteParticles(numberOfParticles);
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

    #ifdef NO_PREDICATES
    printf("Predicates disabled\n");
    #else
    printf("Predicates enabled\n");
    #endif

    // DValidateSWIFTKernelOutcomes

    peano4::initSingletons(
      DomainOffset,
      DomainSize,
      PeriodicBC
    );

    repositories::DataRepository::initDatatypes();

    initGridTraversalEvent();
    createCoalescedMemoryIndices();

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
