#include <limits>
#include <fstream>
#include <string>

#include "sod-shocktube-benchmark-main.h"
#include "Constants.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"
#include "tarch/timing/Watch.h"
#include "tarch/timing/Measurement.h"
#include "tarch/tests/TreeTestCaseCollection.h"

#include "peano4/peano.h"

#include "repositories/DataRepository.h"
#include "repositories/StepRepository.h"
#include "repositories/SolverRepository.h"

#include "observers/CreateGrid.h"
#include "observers/CreateGridButPostponeRefinement.h"
#include "observers/CreateGridAndConvergeLoadBalancing.h"
#include "observers/InitGrid.h"
#include "observers/PlotSolution.h"
#include "observers/TimeStep.h"

#include "toolbox/loadbalancing/loadbalancing.h"


#include "exahype2/NonCriticalAssertions.h"
#include "exahype2/UserInterface.h"

#include "tasks/euler_fv_sodEnclaveTask.h"

#include "config.h"

#if defined(SharedTBB)
#include <tbb/task_group.h>
#endif

using namespace benchmarks::exahype2::euler::sod_shocktube;


tarch::logging::Log _log("::");


tarch::timing::Measurement   gridConstructionMeasurement;
tarch::timing::Measurement   timeStepMeasurement;
tarch::timing::Measurement   plotMeasurement;

double* validOutcome = nullptr;


#if Dimensions==2
constexpr int NumberOfInputEntries  = (euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch+2)*(euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch+2) * euler_fv_sod::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch+0)*(euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch+0) * euler_fv_sod::NumberOfUnknowns;
#else
constexpr int NumberOfInputEntries  = (euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch+2)*(euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch+2)*(euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch+2) * euler_fv_sod::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch+0)*(euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch+0)*(euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch+0) * euler_fv_sod::NumberOfUnknowns;
#endif


/**
 * Set input data
 */
void initInputData( double* Q, int total_patches, int patch_id) {

  bool isRight = ( patch_id < total_patches/2 ) ? false : true; //check if patch is on right or left; check if it works for 3D
  for (int i=0; i<NumberOfInputEntries/euler_fv_sod::NumberOfUnknowns; i++) {
    #if Dimensions==2
    if(isRight){
      Q[i*4+0] = 0.125;     // density
      Q[i*4+1] = 0.0;         // x-momentum
      Q[i*4+2] = 0.0;         // y-momentum
      Q[i*4+3] = 0.1/0.4; // pressure = 0.1; energy = pressure / (gamma-1); gamma = 1.4; kinetic energy is zero in inital state
    }else{
      Q[i*4+0] = 1.0;       // density
      Q[i*4+1] = 0.0;         // x-momentum
      Q[i*4+2] = 0.0;         // y-momentum
      Q[i*4+3] = 1.0/0.4; // pressure = 1; energy = pressure / (gamma-1); gamma = 1.4; kinetic energy is zero in inital state
    }
    #else
    if(isRight){
      Q[i*5+0] = 0.125;     // density
      Q[i*5+1] = 0;         // x-momentum
      Q[i*5+2] = 0;         // y-momentum
      Q[i*5+3] = 0;         // z-momentum
      Q[i*5+4] = 0.1 / 0.4; // pressure = 0.1; energy = pressure / (gamma-1); gamma = 1.4; kinetic energy is zero in inital state
    }else{
      Q[i*5+0] = 1.0;       // density 
      Q[i*5+1] = 0;         // x-momentum
      Q[i*5+2] = 0;         // y-momentum
      Q[i*5+3] = 0;         // z-momentum
      Q[i*5+4] = 1.0 / 0.4; // pressure = 1; energy = pressure / (gamma-1); gamma = 1.4; kinetic energy is zero in inital state
    }
    #endif
  }
}


/**
 * Make a persistent snapshot of a solution and assume, from hereon, that
 * this snapshot is the valid data. You can call this routine as often as
 * you want. Only the very first call will trigger a snapshot.
 */
void storeOutcome( const double* Q ) {
  if (validOutcome==nullptr) {
    validOutcome = new double[ NumberOfOutputEntries ];
    std:memcpy(validOutcome,Q,sizeof(double)*NumberOfOutputEntries);
    logInfo( "storeOutcome(double*)", "bookmarked reference solution" );
  }
}


/**
 * Works if and only if storeOutcome has been performed
 * before.
 */
void validateOutcome( const double* Q, int patchIndex ) {
  int index = 0;
  int errors = 0;
  double maxDifference = 0.0;

  std::string name = "Patch_"+std::to_string(patchIndex)+".csv";
  std::ofstream file;
  file.open(name);
  file<<"Analytical,Exahype\n";

  for (int i=0; i<NumberOfOutputEntries; i++) {
    if (not tarch::la::equals( Q[i], validOutcome[i], benchmarks::exahype2::euler::sod_shocktube::Accuracy) ) {
      errors++;
    }
    maxDifference = std::max(maxDifference, std::abs(Q[i]-validOutcome[i]) );
    //std::cout<<Q[i]<<","<<validOutcome[i]<<"\n";
    file.close();
  }
  
  if (errors>0) {
    logError( "validateOutcome(...)", "outcomes in patch " << patchIndex << " differ in " << errors << " entries with max difference of " << maxDifference << " (admissible accuracy=" << benchmarks::exahype2::euler::sod_shocktube::Accuracy << ")"  );
    logError( "validateOutcome(...)", "terminate immediately" );
    //exit(-1);
  }
}

/**
 * Helper function for sod_shocktube analytical solution calculation
 */
double f(double p4, double p1, double p5, double rho1, double rho5, double gamma)
{
    // shock tube equation

    // local variables
    double z, c1, c5, gm1, gp1, g2, fact;

    z = (p4 / p5 - 1.0);
    c1 = std::sqrt(gamma * p1 / rho1);
    c5 = std::sqrt(gamma * p5 / rho5);

    gm1 = gamma - 1.0;
    gp1 = gamma + 1.0;
    g2 = 2.0 * gamma;

    fact = gm1 / g2 * (c5 / c1) * z / std::sqrt(1.0 + gp1 / g2 * z);
    fact = std::pow(1.0 - fact, g2 / gm1);

    return p1 * fact - p4;
}

/**
* Function to calculate analytical solution of sod shocktube
*/
void analytical_solution(std::vector<double>& rho, std::vector<double>&  u, std::vector<double>& p, double t, double size)
{
        // solves the exact riemann problem
        // original code from bruce fryxell: https://cococubed.com/code_pages/exact_riemann.shtml

        // declare variables
        int npts = rho.size();
        int itmax, iter, i;
        std::vector<double> x(npts);
        double rhol, pl, ul, rhor, pr, ur, gamma, xi, xl, xr,
            rho1, p1, u1, rho5, p5, u5, p40, p41, f0, eps, f1, p4,
            error, z, c5, gm1, gp1, gmfac1, gmfac2, fact, u4, rho4, w,
            p3, u3, rho3, c1, c3, xsh, xcd, xft, xhd, dx;


        // set initial conditions

        // state at left of discontinuity
        rhol = 1.0;
        pl = 1.0;
        ul = 0.0;

        // state at right of discontinuity
        rhor = 0.125;
        pr = 0.1;
        ur = 0.0;

        //  equation of state
        gamma = 1.4;

        // location of discontinuity at t = 0
        xi = 0.5*size;

        // spatial interval over which to compute solution
        xl = 0.0;
        xr = size;

        // begin solution
        rho1 = rhol;
        p1 = pl;
        u1 = ul;
        rho5 = rhor;
        p5 = pr;
        u5 = ur;

        // solve for post-shock pressure by secant method
        // initial guesses

        p40 = p1;
        p41 = p5;
        f0 = f(p40, p1, p5, rho1, rho5, gamma);

        // maximum number of iterations and maxium allowable relative error
        itmax = 200;
        eps = std::numeric_limits<double>::min();
        iter = 0;
        while (iter < itmax)
        {
            f1 = f(p41, p1, p5, rho1, rho5, gamma);
            if (f1 == f0)
                break;
            p4 = p41 - (p41 - p40) * f1 / (f1 - f0);
            error = std::abs(p4 - p41) / p41;
            if (error < eps)
                break;
            p40 = p41;
            p41 = p4;
            f0 = f1;
            iter++;
        }

        // compute post-shock density and velocity
        z = (p4 / p5 - 1.0);
        c5 = std::sqrt(gamma * p5 / rho5);

        gm1 = gamma - 1.0;
        gp1 = gamma + 1.0;
        gmfac1 = 0.5 * gm1 / gamma;
        gmfac2 = 0.5 * gp1 / gamma;

        fact = std::sqrt(1.0 + gmfac2 * z);

        u4 = c5 * z / (gamma * fact);
        rho4 = rho5 * (1.0 + gmfac2 * z) / (1.0 + gmfac1 * z);

        // shock speed
        w = c5 * fact;

        // compute values at foot of rarefaction
        p3 = p4;
        u3 = u4;
        rho3 = rho1 * std::pow(p3 / p1, 1.0 / gamma);

        // compute positions of waves
        c1 = sqrt(gamma * p1 / rho1);
        c3 = sqrt(gamma * p3 / rho3);

        xsh = xi + w * t;
        xcd = xi + u3 * t;
        xft = xi + (u3 - c3) * t;
        xhd = xi - c1 * t;

        // compute solution as a function of position
        dx = (xr - xl) / (npts - 1);
        for (int i = 0; i < npts; i++)
            x[i] = xl + dx * (i + 0.5);

        for (int i = 0; i < npts; i++)
        {
            if (x[i] < xhd)
            {
                rho[i] = rho1;
                p[i] = p1;
                u[i] = u1;
            }
            else if (x[i] < xft)
            {
                u[i] = 2.0 / gp1 * (c1 + (x[i] - xi) / t);
                fact = 1.0 - 0.5 * gm1 * u[i] / c1;
                rho[i] = rho1 * std::pow(fact, (2.0 / gm1));
                p[i] = p1 * std::pow(fact, 2.0 * gamma / gm1);
            }
            else if (x[i] < xcd)
            {
                rho[i] = rho3;
                p[i] = p3;
                u[i] = u3;
            }
            else if (x[i] < xsh)
            {
                rho[i] = rho4;
                p[i] = p4;
                u[i] = u4;
            }
            else
            {
                rho[i] = rho5;
                p[i] = p5;
                u[i] = u5;
            }
        }
}

constexpr double TimeStamp    = 0.0;
constexpr double TimeStepSize = 1e-6;
constexpr double CellSize     = 0.1;
constexpr double CellOffset   = 0.0;

/**
 * Run the benchmark for one particular number of patches per fused kernel call.
 */
void runBenchmarks(int patches) {
  peano4::grid::GridTraversalEvent event;
  event.setX(CellOffset);
  event.setH(CellSize);
  peano4::datamanagement::CellMarker marker(event);

  logInfo( "main()", "========================================" );
  logInfo( "main()", "  Number of patches fused: " << patches   );
  logInfo( "main()", "========================================" );

  {
    std::vector<double*> inputData;
    std::vector<double*> outputData;
    std::vector<double> rho_analytical(patches*euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch);
    std::vector<double> p_analytical(patches*euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch);
    std::vector<double> u_analytical(patches*euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch);

    // Calculate analytical solution
    analytical_solution(rho_analytical, u_analytical, p_analytical, TimeStepSize*10, patches*CellSize);

    for (int i=0; i<patches; i++) {
      inputData.push_back( new double[ NumberOfInputEntries ] );
      outputData.push_back( new double[ NumberOfOutputEntries ] );
      initInputData( inputData[i], patches, i );
    }

    exahype2::enumerator::AoSLexicographicEnumerator QOutEnumerator(1,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch,0,euler_fv_sod::NumberOfUnknowns,0);

    tarch::timing::Watch watch( "::", "main()", false );
    for (int i=0; i<patches; i++) {
      for(int x=0; x<euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch; x++){
        for(int y=0; y<euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch; y++){
          #if Dimensions==2
          tarch::la::Vector<2,int> k = volumeIndex2d(x,y);
          outputData[i][QOutEnumerator(i,k,0)] = rho_analytical[i*NumberOfOutputEntries+x]; // density
          outputData[i][QOutEnumerator(i,k,1)] = u_analytical[i*NumberOfOutputEntries+x]* rho_analytical[i*NumberOfOutputEntries+x]; // x-momentum
          outputData[i][QOutEnumerator(i,k,2)] = 0; // y-momentum
          outputData[i][QOutEnumerator(i,k,3)] = p_analytical[i*NumberOfOutputEntries+x]/0.4+0.5*u_analytical[i*NumberOfOutputEntries+x]*u_analytical[i*NumberOfOutputEntries+x]; // energy
          #else
            for(int z=0; z<euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch; z++){
              tarch::la::Vector<3,int> k = volumeIndex3d(x,y,z);
              outputData[i][QOutEnumerator(i,k,0)] = rho_analytical[i*NumberOfOutputEntries+x]; // density
              outputData[i][QOutEnumerator(i,k,1)] = u_analytical[i*NumberOfOutputEntries+x]* rho_analytical[i*NumberOfOutputEntries+x]; // x-momentum
              outputData[i][QOutEnumerator(i,k,2)] = 0; // y-momentum
              outputData[i][QOutEnumerator(i,k,3)] = 0; // z-momentum
              outputData[i][QOutEnumerator(i,k,4)] = p_analytical[i*NumberOfOutputEntries+x]/0.4+0.5*u_analytical[i*NumberOfOutputEntries+x]*u_analytical[i*NumberOfOutputEntries+x]; // energy
            }
          #endif
        }
      }
    }
    watch.stop();

    for (int i=0; i<patches; i++) {
      storeOutcome( outputData[i] );
      //validateOutcome( outputData[i],i );
      delete[] outputData[i];
      delete[] inputData[i];
    }

    logInfo("main()", "completed measurements for baseline on host: "<< watch.getCalendarTime() << " / " << (watch.getCalendarTime()/patches/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch) );
  }


  auto testFusedVariant = [&]( std::function<void(
    exahype2::CellData&     patchData,
    bool                    evaluateFlux,
    bool                    evaluateNonconservativeProduct,
    bool                    evaluateSource,
    bool                    evaluateMaximumEigenvalueAfterTimeStep
    )> kernel, std::string markerName, bool computeCFL ) {
    ::exahype2::CellData patchData(patches);
    for (int i=0; i<patches; i++) {
      patchData.QIn[i]           = new double[ NumberOfInputEntries ];
      patchData.t[i]             = TimeStamp;
      patchData.dt[i]            = TimeStepSize;
      patchData.QOut[i]          = new double[ NumberOfOutputEntries ];
      patchData.cellCentre[i]   = tarch::la::Vector<Dimensions,double>(CellOffset+0.5*CellSize);
      patchData.cellSize[i]     = tarch::la::Vector<Dimensions,double>(CellSize);
      initInputData( patchData.QIn[i], patches, i );
    }

    tarch::timing::Watch watch( "::", "main()", false );
    kernel(
      patchData,
      true ,
      false ,
      false ,
      computeCFL
    );
    watch.stop();

    for (int i=0; i<patches; i++) {
      validateOutcome( patchData.QOut[i],i );
      delete[] patchData.QIn[i];
      delete[] patchData.QOut[i];
    }

    logInfo("main()", "completed measurements for " << markerName << " on host: " << watch.getCalendarTime() << " / " << (watch.getCalendarTime()/patches/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch) );
  };


  auto testFusedVariantOnDevice = [&](
    int device,
    std::function<void(
      int                     device,
      exahype2::CellData&     patchData,
      bool                    evaluateFlux,
      bool                    evaluateNonconservativeProduct,
      bool                    evaluateSource,
      bool                    evaluateMaximumEigenvalueAfterTimeStep
    )> kernel, std::string markerName, bool computeCFL ) {

    constexpr int MaxHostTasks = 5;
    ::exahype2::CellData patchData[MaxHostTasks] = { patches, patches, patches, patches, patches };

    for (int hostTask=0; hostTask<MaxHostTasks; hostTask++)
    for (int i=0; i<patches; i++) {
      patchData[hostTask].QIn[i]   = new double[ NumberOfInputEntries ];
      patchData[hostTask].t[i]     = TimeStamp;
      patchData[hostTask].dt[i]    = TimeStepSize;
      patchData[hostTask].QOut[i]  = new double[ NumberOfOutputEntries ];
      patchData[hostTask].cellCentre[i]     = tarch::la::Vector<Dimensions,double>(CellOffset+0.5*CellSize);
      patchData[hostTask].cellSize[i]       = tarch::la::Vector<Dimensions,double>(CellSize);
      initInputData( patchData[hostTask].QIn[i], patches, i );
    }

    tarch::timing::Watch singleKernelWatch( "::", "main()", false );
    kernel(
      device,
      patchData[0],
      true ,
      false ,
      false ,
      computeCFL
    );
    singleKernelWatch.stop();
    logInfo("main()", "completed measurements for single thread on device " << device << ", " << markerName << ": " << singleKernelWatch.getCalendarTime() << " / " << (singleKernelWatch.getCalendarTime()/patches/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch) );

    tarch::timing::Watch parallelKernelWatch( "::", "main()", false );
    #if defined(GPUOffloadingOMP) and defined(SharedOMP)
    #pragma omp parallel for
    for (int hostTask=1; hostTask<MaxHostTasks; hostTask++) {
      kernel(
        device,
        patchData[hostTask],
        true ,
        false ,
        false ,
        computeCFL
      );
    }
    #endif

    #if defined(GPUOffloadingSYCL) and defined(SharedTBB)
    ::tbb::task_group taskGroup;
    for (int hostTask=1; hostTask<MaxHostTasks; hostTask++) {
      taskGroup.run(
        [&]() -> void {
          kernel(
            device,
            patchData[hostTask],
            true ,
            false ,
            false ,
            computeCFL
          );
        }
      );
    }
    taskGroup.wait();
    #endif
    parallelKernelWatch.stop();
    logInfo("main()", "completed measurements for " << (MaxHostTasks-1) << " threads on device " << device << ", " << markerName << ": " << parallelKernelWatch.getCalendarTime() << " / " << (parallelKernelWatch.getCalendarTime()/patches/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch/euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch/(MaxHostTasks-1)) );

    for (int hostTask=0; hostTask<MaxHostTasks; hostTask++)
    for (int i=0; i<patches; i++) {
      validateOutcome( patchData[hostTask].QOut[i],i );
      delete[] patchData[hostTask].QIn[i];
      delete[] patchData[hostTask].QOut[i];
    }

  };

  // #if Dimension==2
  // if (patches<=64) {
  // #else
  // if (patches<=4) {
  // #endif
  //   testFusedVariant(
  //     ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_call_stack_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
  //     "call stack, patch-wise, AoS",   
  //   );
  //   testFusedVariant(
  //     ::exahype2::fv::rusanov::timeStepWithRusanov_batched_call_stack_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
  //     "call stack, batched, AoS", true
  //   );
  // }

  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, euler_fv_sod::NumberOfUnknowns, 
    0,exahype2::enumerator::AoSLexicographicEnumerator>,"heap, patch-wise, AoS", true
  );
  // testFusedVariant(
  //   ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, euler_fv_sod::NumberOfUnknowns, 
  //   0,exahype2::enumerator::AoSLexicographicEnumerator>,"heap, batched, AoS", true
  // );
  // testFusedVariant(
  //   ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, euler_fv_sod::NumberOfUnknowns, 
  //   0,exahype2::enumerator::AoSoALexicographicEnumerator>,"heap, patch-wise, AoSoA", true
  // );
  // testFusedVariant(
  //   ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, euler_fv_sod::NumberOfUnknowns, 
  //   0,exahype2::enumerator::AoSoALexicographicEnumerator>, "heap, batched, AoSoA", true
  // );
  // testFusedVariant(
  //   ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, euler_fv_sod::NumberOfUnknowns, 
  //   0,exahype2::enumerator::SoALexicographicEnumerator>,"heap, patch-wise, SoA", true
  // );
  // testFusedVariant(
  //   ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, euler_fv_sod::NumberOfUnknowns, 
  //   0,exahype2::enumerator::SoALexicographicEnumerator>,"heap, batched, SoA", true
  // );

  // testFusedVariant(
  //   ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_in_situ_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, euler_fv_sod::NumberOfUnknowns,
  //   0>,"heap, patch-wise, in-situ", true
  // );
  // testFusedVariant(
  //   ::exahype2::fv::rusanov::timeStepWithRusanov_batched_in_situ_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, euler_fv_sod::NumberOfUnknowns,
  //   0>,"heap, batched, in-situ", true
  // );

  // #if defined(GPUOffloadingOMP)
  // if ( tarch::multicore::Core::getInstance().getNumberOfGPUs()==0 ) {
  //   logWarning( "main()", "no device found" );
  // }
  // for (int device=0; device<tarch::multicore::Core::getInstance().getNumberOfGPUs(); device++) {
  //   logInfo( "main()", "device " << device << " (first kernel triggered by one thread, then kernel triggered by four threads if multithreading enabled)" );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
  //     "omp, patch-wise, AoS", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
  //     "omp, patch-wise, AoSoA", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
  //     "omp, patch-wise, SoA", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
  //     "omp, batched, AoS", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
  //     "omp, batched, AoSoA", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
  //     "omp, batched, SoA", true
  //   );
  // }
  // #endif

  // #if defined(GPUOffloadingCPP)
  // if ( tarch::multicore::Core::getInstance().getNumberOfGPUs()==0 ) {
  //   logWarning( "main()", "no device found" );
  // }
  // for (int device=0; device<tarch::multicore::Core::getInstance().getNumberOfGPUs(); device++) {
  //   logInfo( "main()", "device " << device << " (first kernel triggered by one thread, then kernel triggered by four threads if multithreading enabled)" );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
  //     "cpp, patch-wise, AoS", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
  //     "cpp, batched, AoS", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
  //     "cpp, patch-wise, AoSoA", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
  //     "cpp, batched, AoSoA", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
  //     "cpp, patch-wise, SoA", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
  //     "cpp, batched, SoA", true
  //   );
  // }
  // #endif

  // #if defined(GPUOffloadingSYCL)
  // if ( tarch::multicore::Core::getInstance().getNumberOfGPUs()==0 ) {
  //   logWarning( "main()", "no device found" );
  // }
  // for (int device=0; device<tarch::multicore::Core::getInstance().getNumberOfGPUs(); device++) {
  //   logInfo( "main()", "device " << device << " (first kernel triggered by one thread, then kernel triggered by four threads if multithreading enabled)" );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
  //     "sycl, task-graph, AoS", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
  //     "sycl, batched, AoS", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
  //     "sycl, task-graph, AoSoA", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
  //     "sycl, batched, AoSoA", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
  //     "sycl, task-graph, SoA", true
  //   );
  //   testFusedVariantOnDevice(
  //     device,
  //     ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_heap_static_calls<euler_fv_sod,euler_fv_sod::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
  //     "sycl, batched, SoA", true
  //   );
  // }
  // #endif
}


int main(int argc, char** argv) {
  const int ExitCodeSuccess          = 0;
  const int ExitCodeInvalidArguments = 2;
  const int ExitCodeInvalidBuild     = 3;

  peano4::initParallelEnvironment(&argc,&argv);
  exahype2::initNonCritialAssertionEnvironment();
  tarch::multicore::initSmartMPI();
  peano4::fillLookupTables();

  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 and tarch::multicore::Core::getInstance().getNumberOfThreads()<=1 ) {
    logError("main()", "MPI runs without multithreading are not supported currently.");
    return ExitCodeInvalidBuild;
  }

  if (not exahype2::parseCommandLineArguments(argc,argv) ) {
    logError("main()", "Invalid command line arguments.");
    return ExitCodeInvalidArguments;
  }

  benchmarks::exahype2::euler::sod_shocktube::repositories::DataRepository::initDatatypes();

  repositories::startSimulation();

  otterTraceInitialise();

  tarch::logging::Statistics::getInstance().clear();

  const bool isGlobalMaster     =     tarch::mpi::Rank::getInstance().isGlobalMaster();
  const bool isPeanoComputeNode = not tarch::mpi::Rank::getInstance().isGlobalMaster();


  #if defined(GPUOffloadingSYCL)
  if ( tarch::multicore::Core::getInstance().getNumberOfGPUs()>0 ) {
    logWarning( "main()", "you might have to set ZE_SERIALIZE=1|2|3 before you start the benchmark" );
  }
  #endif

  // #if Dimensions==2
  // for (int patches=2; patches<=1024*2*2*2*2; patches*=2) {
  // #else
  // for (int patches=2; patches<=1024*2; patches*=2) {
  // #endif
  //   std::stringstream msg;
  //   msg << "benchmark-" << patches;
  //   otterPhaseSwitch( msg.str().c_str() );

  runBenchmarks(2);
  // }

  otterTraceFinalise();

//  repositories::finishSimulation();

  benchmarks::exahype2::euler::sod_shocktube::repositories::DataRepository::shutdownDatatypes();
  tarch::multicore::shutdownSmartMPI();
  exahype2::shutdownNonCritialAssertionEnvironment();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}