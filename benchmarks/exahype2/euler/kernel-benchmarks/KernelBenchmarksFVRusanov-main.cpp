#include "KernelBenchmarksFVRusanov-main.h"
#include "Constants.h"
#include "exahype2/fv/rusanov/LoopBody.h"
#include "exahype2/fv/rusanov/sycl/GPUCellData.h"
#include "observers/CreateGrid.h"
#include "observers/CreateGridAndConvergeLoadBalancing.h"
#include "observers/CreateGridButPostponeRefinement.h"
#include "observers/InitGrid.h"
#include "observers/PlotSolution.h"
#include "observers/TimeStep.h"

#include "repositories/DataRepository.h"
#include "repositories/SolverRepository.h"
#include "repositories/StepRepository.h"

#include "tasks/FVRusanovSolverEnclaveTask.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/Device.h"
#include "tarch/timing/Watch.h"
#include "tarch/timing/Measurement.h"

#include "peano4/peano.h"

#include "exahype2/NonCriticalAssertions.h"
#include "exahype2/UserInterface.h"

#include "toolbox/loadbalancing/loadbalancing.h"

#include "config.h"

//#ifdef UseFenv
//#include "tarch/FenvWrapper.h"
//#endif

#if defined(SharedTBB)
#include <tbb/task_group.h>
#endif

using namespace benchmarks::exahype2::kernelbenchmarks;

static_assert(Accuracy >= std::numeric_limits<double>::epsilon() || Accuracy == 0.0);

tarch::logging::Log _log("::");

tarch::timing::Measurement gridConstructionMeasurement;
tarch::timing::Measurement timeStepMeasurement;
tarch::timing::Measurement plotMeasurement;

double validMaxEigenvalue = 0;
double* validOutcome = nullptr;
double minTime = 0;
constexpr int N = 33554432*2;

#if Dimensions==2
constexpr int NumberOfInputEntries  = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch+2)*(FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch+2) * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch+0)*(FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch+0) * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfFiniteVolumesPerPatch = FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch*FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch;
#else
constexpr int NumberOfInputEntries  = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch+2)*(FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch+2)*(FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch+2) * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch+0)*(FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch+0)*(FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch+0) * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfFiniteVolumesPerPatch = FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch*FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch*FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch;
#endif

/**
 * Set input data
 *
 * We really don't care if this makes any sense. Just make the data reasonably
 * smooth and ensure that all is positive, as there's for example density among
 * the values.
 */
void initInputData( double* Q ) {
  /*for (int i=0; i<NumberOfInputEntries/FVRusanovSolver::NumberOfUnknowns; i++) {
    Q[i*FVRusanovSolver::NumberOfUnknowns+0] = 1.0;
    Q[i*FVRusanovSolver::NumberOfUnknowns+1] = std::sin( 1.0 * i / (NumberOfInputEntries) * tarch::la::PI );
    Q[i*FVRusanovSolver::NumberOfUnknowns+2] = std::sin( 2.0 * i / (NumberOfInputEntries) * tarch::la::PI );
    #if Dimensions == 3
      Q[i*FVRusanovSolver::NumberOfUnknowns+3] = std::sin( 3.0 * i / (NumberOfInputEntries) * tarch::la::PI );
      Q[i*FVRusanovSolver::NumberOfUnknowns+4] = 1.1 + std::sin( i / (NumberOfInputEntries) * tarch::la::PI );
    #else
      Q[i*FVRusanovSolver::NumberOfUnknowns+3] = 1.1 + std::sin( i / (NumberOfInputEntries) * tarch::la::PI );
    #endif
  }*/
  for (int i=0; i<NumberOfInputEntries; i++) {
    Q[i] = std::sin( 1.0 * i / (NumberOfInputEntries) * tarch::la::PI );
  }
}

/**
 * Make a persistent snapshot of a solution and assume, from hereon, that
 * this snapshot is the valid data. You can call this routine as often as
 * you want. Only the very first call will trigger a snapshot.
 */
void storeOutcome(const double* Q, const double maxEigenvalue) {
  if (validOutcome == nullptr) {
    validOutcome = new double[NumberOfOutputEntries];
    std::memcpy(validOutcome, Q, sizeof(double) * NumberOfOutputEntries);
    logInfo("storeOutcome(double*)", "bookmarked reference solution");
  }
  validMaxEigenvalue = maxEigenvalue;
}

/**
 * Works if and only if storeOutcome has been performed before.
 *
 * @return Tuple of maximum difference and total number of errors.
 */
std::tuple<double, int> validateOutcome(double* Q, int patchIndex, const double maxEigenvalue) {
  int index = 0;
  int errors = 0;
  double maxDifference = 0.0;
  for (int i=0; i<NumberOfOutputEntries; i++) {
    if (not tarch::la::equals(Q[i], validOutcome[i], Accuracy)) {
      errors++;
      if (patchIndex == 0) {
        std::cout.precision(16);
        std::cout << std::fixed << "Q[" << i << "]!=validOutcome[" << i << "]: " << Q[i] << "!=" << validOutcome[i] << std::endl;
      }
    }
    maxDifference = std::max(maxDifference, std::abs(Q[i]-validOutcome[i]) );
    index++;
  }
  if (not tarch::la::equals( maxEigenvalue, validMaxEigenvalue, Accuracy)) {
    std::cout << std::fixed << " maxEigenvalue[" << patchIndex << "]!=validMaxEigenvalue[" << patchIndex << "]: " << maxEigenvalue << "!=" << validMaxEigenvalue << std::endl;
    errors++;
  }

  return { maxDifference, errors };
}

constexpr double TimeStamp    = 0.5;
constexpr double TimeStepSize = 1e-6;
constexpr double CellSize     = 0.1;
constexpr double CellOffset   = 4.0;



struct SoALexicographicEnumerator {
      SoALexicographicEnumerator(int numberOfPatches, int numberOfVolumesPerAxisInPatch, int haloSize, int unknowns, int numberOfAuxiliaryVariables):
        _numberOfPatches(numberOfPatches),
        _numberOfVolumesPerAxisInPatch(numberOfVolumesPerAxisInPatch),
        _haloSize(haloSize),
        _unknowns(unknowns),
        _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables){}

      /**
       * Access an index
       *
       * The index always refers to the interior of the patches. So you can use negative
       * indices if you want.
       */
      int operator()(
        int patchIndex,
        const tarch::la::Vector<Dimensions,int>&  volumeIndex,
        int unknown
      ) const InlineMethod {
        if constexpr (Dimensions==2) {
          return                         unknown * _numberOfPatches * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
               + (patchIndex % _numberOfPatches) *                    (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
               +      (volumeIndex(1)+_haloSize) *                                                                   (_numberOfVolumesPerAxisInPatch+2*_haloSize)
               +      (volumeIndex(0)+_haloSize);
        }
        else {
          return                         unknown * _numberOfPatches *  (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
               + (patchIndex % _numberOfPatches) *                     (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
               +      (volumeIndex(2)+_haloSize) *                                                                    (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
               +      (volumeIndex(1)+_haloSize) *                                                                                                                   (_numberOfVolumesPerAxisInPatch+2*_haloSize)
               +      (volumeIndex(0)+_haloSize);
        }
      }

      #if !defined(GPUOffloadingOMP)
      std::string toString() const;
      #endif


      /**
       * Returns the total size.
       */
      int size() const InlineMethod {
        if constexpr (Dimensions==2) {
          return _numberOfPatches
              * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
              * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
              * (_unknowns+_numberOfAuxiliaryVariables);
        }
        else {
          return _numberOfPatches
              * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
              * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
              * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
              * (_unknowns+_numberOfAuxiliaryVariables);
        }
      }


      const int _numberOfPatches;
      const int _numberOfVolumesPerAxisInPatch;
      const int _haloSize;
      const int _unknowns;
      const int _numberOfAuxiliaryVariables;
    };





struct AoSLexicographicEnumerator {
  public:
	AoSLexicographicEnumerator(int numberOfCells, int numberOfDoFsPerAxisInCell, int haloSize, int unknowns, int numberOfAuxiliaryVariables):
	  _numberOfCells(numberOfCells),
	  _numberOfDoFsPerAxisInCell(numberOfDoFsPerAxisInCell),
	  _haloSize(haloSize),
	  _unknowns(unknowns),
	  _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables){}

	/**
	 * Access an index
	 *
	 * The index always refers to the interior of the patches. So you can use negative
	 * indices if you want.
	 *
	 * @todo modulo Operator muss ich noch dokumentieren
	 */
	int operator()(
	  int cellIndex,
	  const tarch::la::Vector<Dimensions,int>&  volumeIndex,
	  int unknown
	) const InlineMethod {
	  if constexpr (Dimensions==2) {
		return (cellIndex % _numberOfCells) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
			 +      (volumeIndex(1)+_haloSize) *                                                (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
			 +      (volumeIndex(0)+_haloSize) *                                                                                               (_unknowns+_numberOfAuxiliaryVariables)
			 +                        unknown;
	  }
	  else {
		return (cellIndex % _numberOfCells) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
			 +      (volumeIndex(2)+_haloSize) *                                                (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
			 +      (volumeIndex(1)+_haloSize) *                                                                                               (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
			 +      (volumeIndex(0)+_haloSize) *                                                                                                                                              (_unknowns+_numberOfAuxiliaryVariables)
			 +                        unknown;
	  }
	}

	#if !defined(GPUOffloadingOMP)
	std::string toString() const;
	#endif

	int size() const InlineMethod {
	  if constexpr (Dimensions==2) {
		return _numberOfCells
			* (_numberOfDoFsPerAxisInCell+2*_haloSize)
			* (_numberOfDoFsPerAxisInCell+2*_haloSize)
			* (_unknowns+_numberOfAuxiliaryVariables);
	  }
	  else {
		return _numberOfCells
			* (_numberOfDoFsPerAxisInCell+2*_haloSize)
			* (_numberOfDoFsPerAxisInCell+2*_haloSize)
			* (_numberOfDoFsPerAxisInCell+2*_haloSize)
			* (_unknowns+_numberOfAuxiliaryVariables);
	  }
	}

	int numberOfDofsPerCell() const InlineMethod {
	  if constexpr (Dimensions==2) {
		return (_numberOfDoFsPerAxisInCell+2*_haloSize)
			 * (_numberOfDoFsPerAxisInCell+2*_haloSize);
	  }
	  else {
		return (_numberOfDoFsPerAxisInCell+2*_haloSize)
			 * (_numberOfDoFsPerAxisInCell+2*_haloSize)
			 * (_numberOfDoFsPerAxisInCell+2*_haloSize);
	  }
	}

	const int _numberOfCells;
	const int _numberOfDoFsPerAxisInCell;
	const int _haloSize;
	const int _unknowns;
	const int _numberOfAuxiliaryVariables;
};




struct AoSoALexicographicEnumerator {
  public:
	AoSoALexicographicEnumerator(int numberOfPatches, int numberOfVolumesPerAxisInPatch, int haloSize, int unknowns, int numberOfAuxiliaryVariables):
	  _numberOfPatches(numberOfPatches),
	  _numberOfVolumesPerAxisInPatch(numberOfVolumesPerAxisInPatch),
	  _haloSize(haloSize),
	  _unknowns(unknowns),
	  _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables){}

	/**
	 * Access an index
	 *
	 * The index always refers to the interior of the patches. So you can use negative
	 * indices if you want.
	 *
	 */
	int operator()(
	  int patchIndex,
	  const tarch::la::Vector<Dimensions,int>&  volumeIndex,
	  int unknown
	) const InlineMethod {
	  if constexpr (Dimensions==2) {
		return (patchIndex % _numberOfPatches) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
			 + unknown * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
			 + (volumeIndex(1)+_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
			 + (volumeIndex(0)+_haloSize);
	  }
	  else {
		return (patchIndex % _numberOfPatches) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
			 + unknown * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
			 + (volumeIndex(2)+_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
			 + (volumeIndex(1)+_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
			 + (volumeIndex(0)+_haloSize);
	  }
	}

#if !defined(GPUOffloadingOMP)
	std::string toString() const;
#endif

	/**
	 * Returns the total size.
	 */
	int size() const InlineMethod {
	  if constexpr (Dimensions==2) {
		return _numberOfPatches
			* (_numberOfVolumesPerAxisInPatch+2*_haloSize)
			* (_numberOfVolumesPerAxisInPatch+2*_haloSize)
			* (_unknowns+_numberOfAuxiliaryVariables);
	  }
	  else {
		return _numberOfPatches
			* (_numberOfVolumesPerAxisInPatch+2*_haloSize)
			* (_numberOfVolumesPerAxisInPatch+2*_haloSize)
			* (_numberOfVolumesPerAxisInPatch+2*_haloSize)
			* (_unknowns+_numberOfAuxiliaryVariables);
	  }
	}


	const int _numberOfPatches;
	const int _numberOfVolumesPerAxisInPatch;
	const int _haloSize;
	const int _unknowns;
	const int _numberOfAuxiliaryVariables;
};

struct GPUCellData {
          double*   QIn;
          tarch::la::Vector<Dimensions,double>*   cellCentre;
          tarch::la::Vector<Dimensions,double>*   cellSize;
          double*    t;
          double*    dt;
          int        numberOfCells;
          double*   QOut;
          double*    maxEigenvalue;

          GPUCellData()  = delete;

          GPUCellData(const GPUCellData& copy) = default;

          ~GPUCellData() = default;


          GPUCellData(
			  const ::exahype2::CellData& hostObject,
			  const AoSLexicographicEnumerator& inEnumerator,
			  const AoSLexicographicEnumerator& outEnumerator,
			  ::sycl::queue& queue
			) {
			  numberOfCells = hostObject.numberOfCells;
			  ::std::vector<::sycl::event> copyEvent(5);
			  cellCentre    = ::sycl::malloc_device< tarch::la::Vector<Dimensions,double> >(numberOfCells, queue);
			  cellSize      = ::sycl::malloc_device< tarch::la::Vector<Dimensions,double> >(numberOfCells, queue);
			  QIn          = ::sycl::malloc_device< double >(numberOfCells*inEnumerator.size(), queue);
			  QOut          = ::sycl::malloc_device< double >(numberOfCells*outEnumerator.size(), queue);
			  t             = ::sycl::malloc_device< double >(numberOfCells, queue);
			  dt            = ::sycl::malloc_device< double >(numberOfCells, queue);
			  maxEigenvalue = ::sycl::malloc_device< double >(numberOfCells, queue);
			  double* tmpQIn = new double[numberOfCells * inEnumerator.size()];
			  for (int i = 0; i < numberOfCells; i++) {
				  ::std::memcpy(&tmpQIn[i * inEnumerator.size()], hostObject.QIn[i], inEnumerator.size()*sizeof(double));
			  }
			  copyEvent[0] = queue.memcpy(cellCentre, hostObject.cellCentre, numberOfCells*sizeof(tarch::la::Vector<Dimensions,double>));
			  copyEvent[1] = queue.memcpy(cellSize, hostObject.cellSize, numberOfCells*sizeof(tarch::la::Vector<Dimensions,double>));
			  copyEvent[2] = queue.memcpy(QIn, tmpQIn, numberOfCells*inEnumerator.size()*sizeof(double));
			  copyEvent[3] = queue.memcpy(t, hostObject.t, numberOfCells*sizeof(double));
			  copyEvent[4] = queue.memcpy(dt, hostObject.dt, numberOfCells*sizeof(double));
			  ::sycl::event::wait(copyEvent);
			  delete[] tmpQIn;
			}


          void destroy(
			  ::exahype2::CellData& hostObject,
			  const AoSLexicographicEnumerator& outEnumerator,
			  bool copyEigenvaluesBack,
			  ::sycl::queue& queue
			) {
			  double* tmpQOut = new double[numberOfCells*outEnumerator.size()];
			  ::std::vector<::sycl::event> copyEvent(2);
			  copyEvent[0] = queue.memcpy(tmpQOut, QOut, numberOfCells*outEnumerator.size()*sizeof(double));
			  if (copyEigenvaluesBack) {
				copyEvent[1] = queue.memcpy( hostObject.maxEigenvalue, maxEigenvalue, numberOfCells*sizeof(double) );
			  }
			  ::sycl::event::wait(copyEvent);
			  for (int i = 0; i < numberOfCells; i++) {
				  ::std::memcpy(hostObject.QOut[i], &tmpQOut[i*outEnumerator.size()], outEnumerator.size()*sizeof(double));
			  }

			  delete[] tmpQOut;
			  ::sycl::free( cellCentre, queue);
			  ::sycl::free( cellSize, queue);
			  ::sycl::free( QIn, queue);
			  ::sycl::free( QOut, queue);
			  ::sycl::free( t, queue);
			  ::sycl::free( dt, queue);
			  ::sycl::free( maxEigenvalue, queue);
			}         
        };


#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
template <class QInEnumeratorType, class QOutEnumeratorType>
void copySolution_LoopBody(
  const double* __restrict__                QIn,
  const QInEnumeratorType&                  QInEnumerator,
  int                                       patchIndex,
  const tarch::la::Vector<Dimensions,int>&  volumeIndex,
  int                                       unknown,
  double* __restrict__                      QOut,
  const QOutEnumeratorType&                 QOutEnumerator
) {
  QOut[ QOutEnumerator(patchIndex,volumeIndex,unknown) ] = QIn[QInEnumerator(patchIndex,volumeIndex,unknown)];
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif


KeywordToAvoidDuplicateSymbolsForInlinedFunctions double maxEigenvalue(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        int                                          numberOfUnknowns,
        int                                          numberOfAuxiliaryVariables,
        double                                       gamma = 1.4
      ) InlineMethod {
        #ifdef GPUOffloadingOff
        assertion(normal>=0);
        assertion(normal<Dimensions);

        if (Q[0]<=0.0 or Q[0]!=Q[0]) {
          ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "Q[0]>0", "density negative (Q[0]=" + std::to_string( Q[0] ) + ") at x=" + toString(faceCentre) );
        }
        #endif

        const double irho = 1./ std::abs( Q[0] );
        #if Dimensions==3
        const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
        #else
        const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
        #endif

        #ifdef GPUOffloadingOff
        nonCriticalAssertion9(  tarch::la::greaterEquals(p,0.0), Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
        #endif
        const double c   = std::sqrt(gamma * std::abs(p) * irho);

        const double u_n = Q[normal + 1] * irho;
        const double result = std::max( std::abs(u_n - c), std::abs(u_n + c) );

        #ifdef GPUOffloadingOff
        nonCriticalAssertion9( result>=0.0, result, p, u_n, irho, c, faceCentre, volumeH, t, normal );
        #endif

        return result;
      }

template <typename Solver, typename QMaxEigenvalueEnumeratorType, int unknowns, int auxiliaryVariables>
void computeMaxEigenvalue_LoopBody(
  const double* __restrict__                                         QIn,
  const AoSLexicographicEnumerator&  QInEnumerator,
  const tarch::la::Vector<Dimensions,double>&                        patchCentre,
  const tarch::la::Vector<Dimensions,double>&                        patchSize,
  int                                                                patchIndex,
  const tarch::la::Vector<Dimensions,int>&                           volumeIndex,
  double                                                             t,
  double                                                             dt,
  int                                                                normal,
  double* __restrict__                                               QMaxEigenvalue,
  const QMaxEigenvalueEnumeratorType&                                QMaxEigenvalueEnumerator
) {
    QMaxEigenvalue[ QMaxEigenvalueEnumerator(patchIndex,volumeIndex,0) ] =
      maxEigenvalue(
        QIn + QInEnumerator(patchIndex,volumeIndex,0),
        ::exahype2::fv::getVolumeCentre( patchCentre, patchSize, QInEnumerator._numberOfDoFsPerAxisInCell, volumeIndex ),
        ::exahype2::fv::getVolumeSize( patchSize, QInEnumerator._numberOfDoFsPerAxisInCell ),
        t,
        dt,
        normal,
		unknowns,
		auxiliaryVariables
      );
      
}

template <typename Solver, int unknowns, int auxiliaryVariables>
double reduceMaxEigenvalue_LoopBody(
  const double* __restrict__                   QOut,
  const AoSLexicographicEnumerator&  QOutEnumerator,
  const tarch::la::Vector<Dimensions,double>&  patchCentre,
  const tarch::la::Vector<Dimensions,double>&  patchSize,
  int                                          patchIndex,
  const tarch::la::Vector<Dimensions,int>&     volumeIndex,
  double                                       t,
  double                                       dt
) {
  double result = 0.0;
  for (int normal=0; normal<Dimensions; normal++) {
    // actual computation via user's functor
    result = std::max(
     result,
      maxEigenvalue(
        QOut + QOutEnumerator(patchIndex,volumeIndex,0),
        ::exahype2::fv::getVolumeCentre( patchCentre, patchSize, QOutEnumerator._numberOfDoFsPerAxisInCell, volumeIndex ),
        ::exahype2::fv::getVolumeSize( patchSize, QOutEnumerator._numberOfDoFsPerAxisInCell ),
        t,
        dt,
        normal,
		unknowns,
		auxiliaryVariables
      )
    );
  }

  return result;
}


KeywordToAvoidDuplicateSymbolsForInlinedFunctions void flux(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        int                                          numberOfUnknowns,
        int                                          numberOfAuxiliaryVariables,
        double * __restrict__                        F,
        double                                       gamma = 1.4
      ) InlineMethod {
        #ifdef GPUOffloadingOff
        assertion4( normal>=0, faceCentre, volumeH, t, normal );
        assertion4( normal<Dimensions, faceCentre, volumeH, t, normal);
        #endif

        const double irho = 1./Q[0];
        #if Dimensions==3
        const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
        #else
        const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
        #endif

        const double coeff = irho*Q[normal+1];
        F[0] = coeff*Q[0];
        F[1] = coeff*Q[1];
        F[2] = coeff*Q[2];
        F[normal+1] += p;

        #if Dimensions==3
        F[3] = coeff*Q[3];
        F[4] = coeff*Q[4] + coeff*p;
        #else
        F[3] = coeff*Q[3] + coeff*p;
        #endif
      }

template <typename Solver, typename QFluxEnumeratorType, int unknowns, int auxiliaryVariables>
void computeFlux_LoopBody(
  const double* __restrict__                                         QIn,
  const AoSLexicographicEnumerator&  QInEnumerator,
  const tarch::la::Vector<Dimensions,double>&                        patchCentre,
  const tarch::la::Vector<Dimensions,double>&                        patchSize,
  int                                                                patchIndex,
  const tarch::la::Vector<Dimensions,int>&                           volumeIndex,
  double                                                             t,
  double                                                             dt,
  int                                                                normal,
  double* __restrict__                                               QFlux,
  const QFluxEnumeratorType&  QFluxEnumerator
) {
  flux(
    QIn + QInEnumerator(patchIndex,volumeIndex,0),
    ::exahype2::fv::getVolumeCentre( patchCentre, patchSize, QInEnumerator._numberOfDoFsPerAxisInCell, volumeIndex ),
    ::exahype2::fv::getVolumeSize( patchSize, QInEnumerator._numberOfDoFsPerAxisInCell ),
    t,
    dt,
    normal,
	unknowns,
	auxiliaryVariables,
    QFlux + QFluxEnumerator(patchIndex,volumeIndex,0)
  );
  
}
template <typename QFluxEnumeratorType, typename QOutEnumeratorType>
void updateSolutionWithFlux_LoopBody(
    const double* __restrict__                    tempFluxX,
    const double* __restrict__                    tempFluxY,
    const double* __restrict__                    tempFluxZ,
    const QFluxEnumeratorType&                    fluxEnumerator,
    const tarch::la::Vector<Dimensions,double>&   patchCentre,
    const tarch::la::Vector<Dimensions,double>&   patchSize,
    int                                           patchIndex,
    const tarch::la::Vector<Dimensions,int>&      volumeIndex,
    int                                           unknown,
    double                                        dt,
    double* __restrict__                          QOut,
    const QOutEnumeratorType&                     QOutEnumerator
) {
  auto updateAlongOneDirection = [=](const double* __restrict__ tempFlux, const int normal) {
    tarch::la::Vector<Dimensions,int> leftVolume = volumeIndex;
    tarch::la::Vector<Dimensions,int> rightVolume = volumeIndex;
    normal == 0 ? (leftVolume(0)--, rightVolume(0)++) :
    normal == 1 ? (leftVolume(1)--, rightVolume(1)++) :
                  (leftVolume(2)--, rightVolume(2)++);

    const double fluxLeft  = 0.5 * tempFlux[fluxEnumerator(patchIndex, leftVolume,unknown)]
                           + 0.5 * tempFlux[fluxEnumerator(patchIndex, volumeIndex,unknown)];
    const double fluxRight = 0.5 * tempFlux[fluxEnumerator(patchIndex, volumeIndex,unknown)]
                           + 0.5 * tempFlux[fluxEnumerator(patchIndex, rightVolume,unknown)];

    #if !defined(GPUOffloadingOMP) and !defined(GPUOffloadingSYCL)
    assertionNumericalEquals6(
      fluxLeft - fluxRight, 0.5 * tempFlux[fluxEnumerator(patchIndex, leftVolume,unknown)]-0.5 * tempFlux[fluxEnumerator(patchIndex, rightVolume,unknown)],
      tempFlux[fluxEnumerator(patchIndex, leftVolume,unknown)],
      tempFlux[fluxEnumerator(patchIndex, volumeIndex,unknown)],
      tempFlux[fluxEnumerator(patchIndex, rightVolume,unknown)],
      patchIndex, volumeIndex, unknown
    );
    #endif
    QOut[ QOutEnumerator(patchIndex, volumeIndex, unknown) ] += dt / patchSize(normal) * QOutEnumerator._numberOfDoFsPerAxisInCell * (fluxLeft - fluxRight);
    /*QOut[ ] += dt / patchSize(normal) * QOutEnumerator._numberOfDoFsPerAxisInCell * 0.5 * (tempFlux[fluxEnumerator(patchIndex, leftVolume, unknown)]-tempFlux[fluxEnumerator(patchIndex, rightVolume,unknown)]);*/
  };

                     updateAlongOneDirection(tempFluxX,0);
                     updateAlongOneDirection(tempFluxY,1);
  if (Dimensions==3) updateAlongOneDirection(tempFluxZ,2);
}


#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
template <typename QInEnumeratorType, typename QMaxEigenvalueEnumeratorType, typename QOutEnumeratorType>
void updateSolutionWithEigenvalueDamping_LoopBody(
  const double* __restrict__                   QIn,
  const QInEnumeratorType &                    QInEnumerator,
  const double* __restrict__                   tempMaxEigenvalueX,
  const double* __restrict__                   tempMaxEigenvalueY,
  const double* __restrict__                   tempMaxEigenvalueZ,
  const QMaxEigenvalueEnumeratorType &         eigenvalueEnumerator,
  const tarch::la::Vector<Dimensions,double>&  patchCentre,
  const tarch::la::Vector<Dimensions,double>&  patchSize,
  int                                          patchIndex,
  const tarch::la::Vector<Dimensions,int>&     volumeIndex,
  int                                          unknown,
  double                                       dt,
  double* __restrict__                         QOut,
  const QOutEnumeratorType &                   QOutEnumerator
) {
  auto updateAlongOneDirection = [=](const double* __restrict__ tempMaxEigenvalue, int normal) {
    tarch::la::Vector<Dimensions,int> leftVolume   = volumeIndex;
    tarch::la::Vector<Dimensions,int> rightVolume  = volumeIndex;
    normal == 0 ? (leftVolume(0)--, rightVolume(0)++) :
    normal == 1 ? (leftVolume(1)--, rightVolume(1)++) :
                  (leftVolume(2)--, rightVolume(2)++);

    double leftVolumeValue   = tempMaxEigenvalue[eigenvalueEnumerator(patchIndex, leftVolume,  0)];
    double centerVolumeValue = tempMaxEigenvalue[eigenvalueEnumerator(patchIndex, volumeIndex, 0)];
    double rightVolumeValue  = tempMaxEigenvalue[eigenvalueEnumerator(patchIndex, rightVolume, 0)];

    const double lambdaLeft  = tempMaxEigenvalue[eigenvalueEnumerator(patchIndex, leftVolume,0)] > tempMaxEigenvalue[eigenvalueEnumerator(patchIndex, volumeIndex,0)] 
                             ? tempMaxEigenvalue[eigenvalueEnumerator(patchIndex, leftVolume,0)] 
                             : tempMaxEigenvalue[eigenvalueEnumerator(patchIndex, volumeIndex,0)];
    const double lambdaRight = tempMaxEigenvalue[eigenvalueEnumerator(patchIndex, rightVolume,0)] > tempMaxEigenvalue[eigenvalueEnumerator(patchIndex, volumeIndex,0)]
                             ? tempMaxEigenvalue[eigenvalueEnumerator(patchIndex, rightVolume,0)] 
                             : tempMaxEigenvalue[eigenvalueEnumerator(patchIndex, volumeIndex,0)];
    double fluxLeft  = - lambdaLeft * (QIn[QInEnumerator(patchIndex, volumeIndex,unknown)] - QIn[QInEnumerator(patchIndex, leftVolume,unknown)]);
    double fluxRight = - lambdaRight * (QIn[QInEnumerator(patchIndex, rightVolume,unknown)] - QIn[QInEnumerator(patchIndex, volumeIndex,unknown)]);
    QOut[ QOutEnumerator(patchIndex, volumeIndex,unknown) ] +=  dt / patchSize(normal) * QOutEnumerator._numberOfDoFsPerAxisInCell * (fluxLeft - fluxRight)*0.5;

    #if !defined(GPUOffloadingOMP) and  !defined(GPUOffloadingSYCL)
    assertion7(
      QOut[ QOutEnumerator(patchIndex, volumeIndex,unknown) ]==QOut[ QOutEnumerator(patchIndex, volumeIndex,unknown) ],
      dt, unknown, volumeIndex, normal,
      fluxLeft, fluxRight,
      QOutEnumerator(patchIndex, volumeIndex,unknown)
    );
    #endif
  };

                     updateAlongOneDirection(tempMaxEigenvalueX,0);
                     updateAlongOneDirection(tempMaxEigenvalueY,1);
  if (Dimensions==3) updateAlongOneDirection(tempMaxEigenvalueZ,2);
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif


template <
  typename Solver,
  std::size_t  numberOfVolumesPerAxisInPatch,
  std::size_t  haloSize,
  std::size_t  unknowns,
  std::size_t  auxiliaryVariables,
  typename TempDataEnumerator
>
void timeStepWithRusanov_patchwise_static_calls(
  int                     targetDevice,
  ::exahype2::CellData&   hostPatchData,
  bool                    evaluateFlux,
  bool                    evaluateNonconservativeProduct,
  bool                    evaluateSource,
  bool                    evaluateMaximumEigenvalueAfterTimeStep
) {
  #if defined(GPUOffloadingSYCL)
  ::sycl::property_list prop_list{::sycl::property::queue::enable_profiling()};
  ::sycl::queue queue{::sycl::gpu_selector_v, prop_list};
  //::sycl::queue& queue = tarch::accelerator::getSYCLQueue(targetDevice);
  constexpr int solversHaloSize        = 1;

  AoSLexicographicEnumerator QInEnumerator       (                        1, numberOfVolumesPerAxisInPatch, haloSize, unknowns, auxiliaryVariables);
  AoSLexicographicEnumerator QOutEnumerator      (                        1, numberOfVolumesPerAxisInPatch,        0, unknowns, auxiliaryVariables);
  TempDataEnumerator fluxEnumerator      (hostPatchData.numberOfCells, numberOfVolumesPerAxisInPatch,   solversHaloSize, unknowns, 0);
  TempDataEnumerator ncpEnumerator       (hostPatchData.numberOfCells, numberOfVolumesPerAxisInPatch+1, solversHaloSize, unknowns, 0);
  TempDataEnumerator eigenvalueEnumerator(hostPatchData.numberOfCells, numberOfVolumesPerAxisInPatch,   solversHaloSize,        1, 0);

  double* tempFluxX                   = evaluateFlux ?                                     ::sycl::malloc_device<double>(fluxEnumerator.size(), queue) : nullptr;
  double* tempFluxY                   = evaluateFlux ?                                     ::sycl::malloc_device<double>(fluxEnumerator.size(), queue) : nullptr;
  double* tempFluxZ                   = evaluateFlux and Dimensions==3 ?                   ::sycl::malloc_device<double>(fluxEnumerator.size(), queue) : nullptr;
  double* tempNonconservativeProductX = evaluateNonconservativeProduct ?                   ::sycl::malloc_device<double>(ncpEnumerator.size(), queue) : nullptr;
  double* tempNonconservativeProductY = evaluateNonconservativeProduct ?                   ::sycl::malloc_device<double>(ncpEnumerator.size(), queue) : nullptr;
  double* tempNonconservativeProductZ = evaluateNonconservativeProduct and Dimensions==3 ? ::sycl::malloc_device<double>(ncpEnumerator.size(), queue) : nullptr;
  double* tempEigenvalueX             = ::sycl::malloc_device<double>(eigenvalueEnumerator.size(), queue);
  double* tempEigenvalueY             = ::sycl::malloc_device<double>(eigenvalueEnumerator.size(), queue);
  double* tempEigenvalueZ             = ::sycl::malloc_device<double>(eigenvalueEnumerator.size(), queue);

  const std::size_t numberOfPatches = hostPatchData.numberOfCells;
  GPUCellData  patchData( hostPatchData, QInEnumerator, QOutEnumerator, queue );
  ::sycl::range<3> totalWorkItem{numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch+2*solversHaloSize, (unknowns+auxiliaryVariables)*numberOfPatches};
  ::sycl::range<3> workgroupSize{numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch+2*solversHaloSize, unknowns+auxiliaryVariables};
  ::sycl::event event = queue.submit(
    [&](::sycl::handler &handle) {
      handle.parallel_for(
      ::sycl::nd_range<3> {totalWorkItem, workgroupSize},
      [=] (::sycl::nd_item<3> i) {        
	    if (Dimensions==2) {
                  int patchIndex    = i.get_group(2);
                  int y             = i.get_local_id(0);
                  int x             = i.get_local_id(1);
				  int unknown       = i.get_local_id(2);

				  if (x < numberOfVolumesPerAxisInPatch) {
                    copySolution_LoopBody(
                      patchData.QIn + patchIndex * QInEnumerator.size(),
                      QInEnumerator,
                      patchIndex,
                      volumeIndex2d(x,y),
                      unknown,
                      patchData.QOut + patchIndex * QOutEnumerator.size(),
                      QOutEnumerator
                    );
					}
				  }
                
        if (Dimensions==2) {
			      int patchIndex = i.get_group(2);
                  int y          = i.get_local_id(0);
                  int x          = i.get_local_id(1)-solversHaloSize;
				  int unknown       = i.get_local_id(2);
                  
				  if (unknown == 0) {
                    computeMaxEigenvalue_LoopBody<Solver, TempDataEnumerator, unknowns, auxiliaryVariables>(
                      patchData.QIn + patchIndex * QInEnumerator.size(),
                      QInEnumerator,
                      patchData.cellCentre[patchIndex],
                      patchData.cellSize[patchIndex],
                      patchIndex,
                      volumeIndex2d(x,y),
                      patchData.t[patchIndex], patchData.dt[patchIndex],
                      0,
                      tempEigenvalueX,
                      eigenvalueEnumerator
                    );
				  
                    computeMaxEigenvalue_LoopBody<Solver, TempDataEnumerator, unknowns, auxiliaryVariables>(
                      patchData.QIn + patchIndex * QInEnumerator.size(),
                      QInEnumerator,
                      patchData.cellCentre[patchIndex],
                      patchData.cellSize[patchIndex],
                      patchIndex,
                      volumeIndex2d(y,x),
                      patchData.t[patchIndex], patchData.dt[patchIndex],
                      1,
                      tempEigenvalueY,
                      eigenvalueEnumerator
                    );
				  }

				  i.barrier();

                  if (unknown < unknowns && x < static_cast<int>(numberOfVolumesPerAxisInPatch-solversHaloSize)) {
                    x += solversHaloSize;
					updateSolutionWithEigenvalueDamping_LoopBody(
                      patchData.QIn + patchIndex * QInEnumerator.size(),
                      QInEnumerator,
                      tempEigenvalueX,
                      tempEigenvalueY,
                      tempEigenvalueZ,
                      eigenvalueEnumerator,
                      patchData.cellCentre[patchIndex],
                      patchData.cellSize[patchIndex],
                      patchIndex,
                      volumeIndex2d(x,y),
                      unknown,
                      patchData.dt[patchIndex],
                      patchData.QOut + patchIndex * QOutEnumerator.size(),
                      QOutEnumerator
                    );
				  }
        }

        if (evaluateFlux and Dimensions==2) {
			      int patchIndex = i.get_group(2);
                  int x          = i.get_local_id(1)-solversHaloSize;
                  int y          = i.get_local_id(0);
				  int unknown       = i.get_local_id(2);
				  
				  if(unknown == 0) {
                    computeFlux_LoopBody<Solver, TempDataEnumerator, unknowns, auxiliaryVariables>(
                      patchData.QIn + patchIndex * QInEnumerator.size(),
                      QInEnumerator,
                      patchData.cellCentre[patchIndex],
                      patchData.cellSize[patchIndex],
                      patchIndex,
                      volumeIndex2d(x,y),
                      patchData.t[patchIndex],
                      patchData.dt[patchIndex],
                      0, // normal
                      tempFluxX,
                      fluxEnumerator
                    );
                  
                    computeFlux_LoopBody<Solver, TempDataEnumerator, unknowns, auxiliaryVariables>(
                      patchData.QIn + patchIndex * QInEnumerator.size(),
                      QInEnumerator,
                      patchData.cellCentre[patchIndex],
                      patchData.cellSize[patchIndex],
                      patchIndex,
                      volumeIndex2d(y,x),
                      patchData.t[patchIndex],
                      patchData.dt[patchIndex],
                      1, // normal
                      tempFluxY,
                      fluxEnumerator
                    );
                  }

				  i.barrier();

                  if (x < static_cast<int>(numberOfVolumesPerAxisInPatch-solversHaloSize) && unknown < unknowns) {
                    x += solversHaloSize;
					updateSolutionWithFlux_LoopBody(
                      tempFluxX, tempFluxY, tempFluxZ,
                      fluxEnumerator,
                      patchData.cellCentre[patchIndex],
                      patchData.cellSize[patchIndex],
                      patchIndex,
                      volumeIndex2d(x,y),
                      unknown,
                      patchData.dt[patchIndex],
                      patchData.QOut + patchIndex * QOutEnumerator.size(),
                      QOutEnumerator
                    );
                  }
        }
		i.barrier();
        if (evaluateMaximumEigenvalueAfterTimeStep and Dimensions==2) {
			      int patchIndex = i.get_group(2);
                  if (i.get_local_id(0) == 0 && i.get_local_id(1) == 0 && i.get_local_id(2) == 0) {
				    double newMaxEigenvalue = 0.0;
                    for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++) {
                      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
                        newMaxEigenvalue = std::max(
                          newMaxEigenvalue,reduceMaxEigenvalue_LoopBody<Solver, unknowns, auxiliaryVariables>( 
                          patchData.QOut + patchIndex * QOutEnumerator.size(),
                          QOutEnumerator,
                          patchData.cellCentre[patchIndex],
                          patchData.cellSize[patchIndex],
                          patchIndex,
                          volumeIndex2d(x,y),
                          patchData.t[patchIndex],
                          patchData.dt[patchIndex]
                          )
                        );
                      }  
				    }
                    patchData.maxEigenvalue[patchIndex] = newMaxEigenvalue;
				  }
				}
            });
    }
  );
  event.wait();
  auto start = event.get_profiling_info<::sycl::info::event_profiling::command_start>();
  auto stop = event.get_profiling_info<::sycl::info::event_profiling::command_end>();
  auto duration = static_cast<double>(stop - start) / NSEC_IN_SEC;
  ::std::cout << "Patchwise kernel execution time: " << duration << " sec" << ::std::endl;
  patchData.destroy(  hostPatchData, QOutEnumerator, evaluateMaximumEigenvalueAfterTimeStep, queue  );

  if (tempFluxX!=nullptr)                   ::sycl::free( tempFluxX, queue);
  if (tempFluxY!=nullptr)                   ::sycl::free( tempFluxY, queue);
  if (tempFluxZ!=nullptr)                   ::sycl::free( tempFluxZ, queue);
  if (tempNonconservativeProductX!=nullptr) ::sycl::free( tempNonconservativeProductX, queue);
  if (tempNonconservativeProductY!=nullptr) ::sycl::free( tempNonconservativeProductY, queue);
  if (tempNonconservativeProductZ!=nullptr) ::sycl::free( tempNonconservativeProductZ, queue);
  if (tempEigenvalueX!=nullptr)             ::sycl::free( tempEigenvalueX, queue);
  if (tempEigenvalueY!=nullptr)             ::sycl::free( tempEigenvalueY, queue);
  if (tempEigenvalueZ!=nullptr)             ::sycl::free( tempEigenvalueZ, queue);
  #else
  assertionMsg(false, "this routine should only be used in combination with SYCL offloading, as it has a device parameter" );
  #endif
}

template <
  typename Solver,
  std::size_t  numberOfVolumesPerAxisInPatch,
  std::size_t  haloSize,
  std::size_t  unknowns,
  std::size_t  auxiliaryVariables,
  typename TempDataEnumerator
>
void timeStepWithRusanov_batched_static_calls(
  int                     targetDevice,
  ::exahype2::CellData&   hostPatchData,
  bool                    evaluateFlux,
  bool                    evaluateNonconservativeProduct,
  bool                    evaluateSource,
  bool                    evaluateMaximumEigenvalueAfterTimeStep
) {
  #if defined(GPUOffloadingSYCL)
  ::sycl::property_list prop_list{::sycl::property::queue::enable_profiling()};
  ::sycl::queue queue{::sycl::gpu_selector_v, prop_list};
  //sycl::queue& queue = tarch::accelerator::getSYCLQueue(targetDevice);

  constexpr int solversHaloSize        = 1;

  AoSLexicographicEnumerator QInEnumerator       (                        1, numberOfVolumesPerAxisInPatch, haloSize, unknowns, auxiliaryVariables);
  AoSLexicographicEnumerator QOutEnumerator      (                        1, numberOfVolumesPerAxisInPatch,        0, unknowns, auxiliaryVariables);
  TempDataEnumerator fluxEnumerator      (hostPatchData.numberOfCells, numberOfVolumesPerAxisInPatch,   solversHaloSize, unknowns, 0);
  TempDataEnumerator ncpEnumerator       (hostPatchData.numberOfCells, numberOfVolumesPerAxisInPatch+1, solversHaloSize, unknowns, 0);
  TempDataEnumerator eigenvalueEnumerator(hostPatchData.numberOfCells, numberOfVolumesPerAxisInPatch,   solversHaloSize,        1, 0);

  double* tempFluxX                   = evaluateFlux ?                                     ::sycl::malloc_device<double>(fluxEnumerator.size(), queue) : nullptr;
  double* tempFluxY                   = evaluateFlux ?                                     ::sycl::malloc_device<double>(fluxEnumerator.size(), queue) : nullptr;
  double* tempFluxZ                   = evaluateFlux and Dimensions==3 ?                   ::sycl::malloc_device<double>(fluxEnumerator.size(), queue) : nullptr;
  double* tempNonconservativeProductX = evaluateNonconservativeProduct ?                   ::sycl::malloc_device<double>(ncpEnumerator.size(), queue) : nullptr;
  double* tempNonconservativeProductY = evaluateNonconservativeProduct ?                   ::sycl::malloc_device<double>(ncpEnumerator.size(), queue) : nullptr;
  double* tempNonconservativeProductZ = evaluateNonconservativeProduct and Dimensions==3 ? ::sycl::malloc_device<double>(ncpEnumerator.size(), queue) : nullptr;
  double* tempEigenvalueX             = ::sycl::malloc_device<double>(eigenvalueEnumerator.size(), queue);
  double* tempEigenvalueY             = ::sycl::malloc_device<double>(eigenvalueEnumerator.size(), queue);
  double* tempEigenvalueZ             = ::sycl::malloc_device<double>(eigenvalueEnumerator.size(), queue);

  const std::size_t numberOfPatches = hostPatchData.numberOfCells;
  GPUCellData  patchData( hostPatchData, QInEnumerator, QOutEnumerator, queue );

  ::sycl::event copySolutionEvent;
   if (Dimensions==2) {
    copySolutionEvent = queue.submit(
      [&](::sycl::handler &handle) {
        handle.parallel_for(
          ::sycl::range<3>{numberOfPatches,numberOfVolumesPerAxisInPatch,numberOfVolumesPerAxisInPatch},
          [=] (::sycl::item<3> i) {
            int patchIndex = i[0];
            int x          = i[1];
            int y          = i[2];
            for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
              copySolution_LoopBody(
                patchData.QIn + patchIndex * QInEnumerator.size(),
                QInEnumerator,
                patchIndex,
                volumeIndex2d(x,y),
                unknown,
                patchData.QOut + patchIndex * QOutEnumerator.size(),
                QOutEnumerator
              );
            }
          }
        );
      }
    );
  }
  
  ::sycl::event updateWithEigenvalue;
  if (Dimensions==2) {
    ::sycl::event computeMaxEigenvalueX = queue.submit(
      [&](::sycl::handler &handle) {
        handle.parallel_for(
          ::sycl::range<3>{numberOfPatches,numberOfVolumesPerAxisInPatch,numberOfVolumesPerAxisInPatch+2*solversHaloSize},
          [=] (::sycl::item<3> i) {
            int patchIndex = i[0];
            int y          = i[1];
            int x          = i[2]-solversHaloSize;

            computeMaxEigenvalue_LoopBody<Solver, TempDataEnumerator, unknowns, auxiliaryVariables>(
                patchData.QIn + patchIndex * QInEnumerator.size(),
                QInEnumerator,
                patchData.cellCentre[patchIndex],
                patchData.cellSize[patchIndex],
                patchIndex,
                volumeIndex2d(x,y),
                patchData.t[patchIndex], patchData.dt[patchIndex],
                0,
                tempEigenvalueX,
                eigenvalueEnumerator
            );
          }
        );
      }
    );

    ::sycl::event computeMaxEigenvalueY = queue.submit(
      [&](::sycl::handler &handle) {
        handle.parallel_for(
          ::sycl::range<3>{numberOfPatches,numberOfVolumesPerAxisInPatch+2*solversHaloSize,numberOfVolumesPerAxisInPatch},
          [=] (::sycl::item<3> i) {
            int patchIndex = i[0];
            int y          = i[1]-solversHaloSize;
            int x          = i[2];

            computeMaxEigenvalue_LoopBody<Solver, TempDataEnumerator, unknowns, auxiliaryVariables>(
                patchData.QIn + patchIndex * QInEnumerator.size(),
                QInEnumerator,
                patchData.cellCentre[patchIndex],
                patchData.cellSize[patchIndex],
                patchIndex,
                volumeIndex2d(x,y),
                patchData.t[patchIndex], patchData.dt[patchIndex],
                1,
                tempEigenvalueY,
                eigenvalueEnumerator
            );
          }
        );
      }
    );

    updateWithEigenvalue = queue.submit(
      [&](::sycl::handler &handle) {
        handle.depends_on(copySolutionEvent);
        handle.depends_on(computeMaxEigenvalueX);
        handle.depends_on(computeMaxEigenvalueY);

        handle.parallel_for(
          ::sycl::range<3>{numberOfPatches,numberOfVolumesPerAxisInPatch,numberOfVolumesPerAxisInPatch},
          [=] (::sycl::item<3> i) {
            int patchIndex = i[0];
            int y          = i[1];
            int x          = i[2];

            for (int unknown = 0; unknown < unknowns; unknown++) {
              updateSolutionWithEigenvalueDamping_LoopBody(
                patchData.QIn + patchIndex * QInEnumerator.size(),
                QInEnumerator,
                tempEigenvalueX,
                tempEigenvalueY,
                tempEigenvalueZ,
                eigenvalueEnumerator,
                patchData.cellCentre[patchIndex],
                patchData.cellSize[patchIndex],
                patchIndex,
                volumeIndex2d(x,y),
                unknown,
                patchData.dt[patchIndex],
                patchData.QOut + patchIndex * QOutEnumerator.size(),
                QOutEnumerator
              );
            }
          }
        );
      }
    );
  }
 
  ::sycl::event updateWithFlux;
  if (evaluateFlux and Dimensions==2) {
    ::sycl::event computeFluxX = queue.submit(
      [&](::sycl::handler &handle) {
        handle.parallel_for(
          ::sycl::range<3>{numberOfPatches,numberOfVolumesPerAxisInPatch,numberOfVolumesPerAxisInPatch+2*solversHaloSize},
          [=] (::sycl::item<3> i) {
            int patchIndex = i[0];
            int y          = i[1];
			int x = i[2]-solversHaloSize;
           // for (int x = -solversHaloSize; x < static_cast<int>(numberOfVolumesPerAxisInPatch+solversHaloSize); x++) {
              computeFlux_LoopBody<Solver, TempDataEnumerator, unknowns, auxiliaryVariables>(
                patchData.QIn + patchIndex * QInEnumerator.size(),
                QInEnumerator,
                patchData.cellCentre[patchIndex],
                patchData.cellSize[patchIndex],
                patchIndex,
                volumeIndex2d(x,y),
                patchData.t[patchIndex],
                patchData.dt[patchIndex],
                0, // normal
                tempFluxX,
                fluxEnumerator
              );
           // }
          }
        );
      }
    );

    ::sycl::event computeFluxY = queue.submit(
      [&](::sycl::handler &handle) {
        handle.parallel_for(
          ::sycl::range<3>{numberOfPatches,numberOfVolumesPerAxisInPatch,numberOfVolumesPerAxisInPatch+2*solversHaloSize},
          [=] (::sycl::item<3> i) {
            int patchIndex = i[0];
            int x          = i[1];
			int y = i[2]-solversHaloSize;

            //for (int y = -solversHaloSize; y < static_cast<int>(numberOfVolumesPerAxisInPatch+solversHaloSize);  y++) {
              computeFlux_LoopBody<Solver, TempDataEnumerator, unknowns, auxiliaryVariables>(
                patchData.QIn + patchIndex * QInEnumerator.size(),
                QInEnumerator,
                patchData.cellCentre[patchIndex],
                patchData.cellSize[patchIndex],
                patchIndex,
                volumeIndex2d(x,y),
                patchData.t[patchIndex],
                patchData.dt[patchIndex],
                1, // normal
                tempFluxY,
                fluxEnumerator
              );
            //}
          }
        );
      }
    );

    updateWithFlux = queue.submit(
      [&](::sycl::handler &handle) {
        handle.depends_on( updateWithEigenvalue );
        handle.depends_on( computeFluxX );
        handle.depends_on( computeFluxY );

        handle.parallel_for(
          ::sycl::range<3>{numberOfPatches,numberOfVolumesPerAxisInPatch,numberOfVolumesPerAxisInPatch},
          [=] (::sycl::item<3> i) {
            int patchIndex = i[0];
            int x          = i[1];
            int y          = i[2];
            for (int unknown=0; unknown<unknowns; unknown++) {
              updateSolutionWithFlux_LoopBody(
                tempFluxX, tempFluxY, tempFluxZ,
                fluxEnumerator,
                patchData.cellCentre[patchIndex],
                patchData.cellSize[patchIndex],
                patchIndex,
                volumeIndex2d(x,y),
                unknown,
                patchData.dt[patchIndex],
                patchData.QOut + patchIndex * QOutEnumerator.size(),
                QOutEnumerator
              );
            }
          }
        );
      }
    );
  }
  
  ::sycl::event evalEigen;
  if (evaluateMaximumEigenvalueAfterTimeStep and Dimensions==2) {
    evalEigen = queue.submit(
      [&](::sycl::handler &handle) {
        handle.depends_on( updateWithEigenvalue );
        handle.depends_on( updateWithFlux );

        handle.parallel_for(
          ::sycl::range<1>{numberOfPatches},
          [=] (::sycl::item<1> i) {
            int patchIndex = i[0];

            double newMaxEigenvalue = 0.0;
            for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
            for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
              newMaxEigenvalue = std::max(
                newMaxEigenvalue,
                reduceMaxEigenvalue_LoopBody<Solver, unknowns, auxiliaryVariables>(
                  patchData.QOut + patchIndex * QOutEnumerator.size(),
                  QOutEnumerator,
                  patchData.cellCentre[patchIndex],
                  patchData.cellSize[patchIndex],
                  patchIndex,
                  volumeIndex2d(x,y),
                  patchData.t[patchIndex],
                  patchData.dt[patchIndex]
                )
              );
            }

            patchData.maxEigenvalue[patchIndex] = newMaxEigenvalue;
          }
        );
      }
    );//.wait();
	evalEigen.wait();
  }
    else {
    updateWithEigenvalue.wait();
    updateWithFlux.wait();
  }
  auto start = copySolutionEvent.get_profiling_info<::sycl::info::event_profiling::command_start>();
  auto stop = evalEigen.get_profiling_info<::sycl::info::event_profiling::command_end>();
  auto duration = static_cast<double>(stop - start) / NSEC_IN_SEC;
  ::std::cout << "Batched kernel execution time: " << duration << "sec" << ::std::endl;

  patchData.destroy(  hostPatchData, QOutEnumerator, evaluateMaximumEigenvalueAfterTimeStep, queue  );

  if (tempFluxX!=nullptr)                   ::sycl::free( tempFluxX, queue);
  if (tempFluxY!=nullptr)                   ::sycl::free( tempFluxY, queue);
  if (tempFluxZ!=nullptr)                   ::sycl::free( tempFluxZ, queue);
  if (tempNonconservativeProductX!=nullptr) ::sycl::free( tempNonconservativeProductX, queue);
  if (tempNonconservativeProductY!=nullptr) ::sycl::free( tempNonconservativeProductY, queue);
  if (tempNonconservativeProductZ!=nullptr) ::sycl::free( tempNonconservativeProductZ, queue);
  if (tempEigenvalueX!=nullptr)             ::sycl::free( tempEigenvalueX, queue);
  if (tempEigenvalueY!=nullptr)             ::sycl::free( tempEigenvalueY, queue);
  if (tempEigenvalueZ!=nullptr)             ::sycl::free( tempEigenvalueZ, queue);
  #else
  assertionMsg(false, "this routine should only be used in combination with OpenMP offloading, as it has a device parameter" );
  #endif
}
/**
 * Run the benchmark for one particular number of patches
 *
 * This operation benchmarks exclusively the host performance. This happens in
 * two steps: We first assess the baseline performance, i.e. the normal kernel
 * call, and then we look if the stateless optimisation pays off.
 *
 * These two steps are ran on multiple threads. In OpenMP for example, there is
 * one parallel loop around the two steps which invokes the same benchmark per
 * loop entry. However, only the first thread always will report its runtime.
 * If you are not bandwidth-bound, the data you obtain should thus be
 * independent of the number of threads used.
 *
 * @param numberOfPatches Number of patches to study
 */
void runBenchmarksOnHost(int numberOfPatches) {
  peano4::grid::GridTraversalEvent event;
  event.setX(CellOffset);
  event.setH(CellSize);
  peano4::datamanagement::CellMarker marker(event);

  /**
   * Allocate the test data, run the kernel, and time the execution.
   *
   * If the user has specified an accuracy, check if the outcome agrees with
   * the baseline data up to this accuracy. If the routine is invoked on
   * thread 0, dump the time spent in the kernel.
   */
  auto runBaseline = [&](
    int currentThread,
    int patches
  ) -> void {
    std::vector<double*> inputData;
    std::vector<double*> outputData;
    for (int i=0; i<patches; i++) {
      inputData.push_back( tarch::allocateMemory<double>(NumberOfInputEntries, tarch::MemoryLocation::Heap) );
      outputData.push_back( new double[ NumberOfOutputEntries ] );
      initInputData( inputData[i] );
    }

    tarch::timing::Watch watch( "::runBenchmarksOnHost", "runBaseline(...)", false );
    double outputMaxEigenvalue = 0;
    for (int i=0; i<patches; i++) {
      outputMaxEigenvalue = tasks::FVRusanovSolverEnclaveTask::applyKernelToCell(
        marker,
        TimeStamp,
        TimeStepSize,
        inputData[i],
        outputData[i]
      );
    }
    watch.stop();

    if (currentThread==0) {
      logInfo("main()", "baseline: "
        << watch.getCalendarTime() << " / "
        << (watch.getCalendarTime()/patches/NumberOfFiniteVolumesPerPatch) );
    }

    if constexpr (Accuracy>0.0) {
      int errors = 0;
      double maxDifference = 0.0;
      for (int i=0; i<patches; i++) {
        storeOutcome( outputData[i],outputMaxEigenvalue );
        auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome( outputData[i],i ,outputMaxEigenvalue);
        errors += errorsPerPatch;
        maxDifference = std::max(maxDifference, maxDifferencePerPatch);
      }

      if (errors > 0) {
        logError("main()", "max difference of outcome from baseline for all patches is "
          << maxDifference << " (admissible accuracy=" << Accuracy << ")" << " for " << errors << " entries");
        std::abort();
      }
    }

    for (int i=0; i<patches; i++) {
      // inputData is freed automatically by the kernel
      delete[] outputData[i];
    }
  };

  /**
   * @see runBaseline
   */
  auto runStatelessVariantOnHost = [&]( std::function<void(
    exahype2::CellData&     patchData,
    bool                    evaluateFlux,
    bool                    evaluateNonconservativeProduct,
    bool                    evaluateSource,
    bool                    evaluateMaximumEigenvalueAfterTimeStep
    )> kernel,
    const std::string&      markerName,
    int                     currentThread,
    int                     patches
  ) -> void {
    ::exahype2::CellData patchData(patches);
    for (int i=0; i<patches; i++) {
      patchData.QIn[i]           = tarch::allocateMemory<double>(NumberOfInputEntries, tarch::MemoryLocation::Heap);
      patchData.t[i]             = TimeStamp;
      patchData.dt[i]            = TimeStepSize;
      patchData.QOut[i]          = new double[ NumberOfOutputEntries ];
      patchData.cellCentre[i]   = tarch::la::Vector<Dimensions,double>(CellOffset+0.5*CellSize);
      patchData.cellSize[i]     = tarch::la::Vector<Dimensions,double>(CellSize);
      initInputData( patchData.QIn[i] );
    }

    tarch::timing::Watch watch( "::runBenchmarksOnHost", "runStatelessVariantOnHost(...)", false );
    kernel(
      patchData,
      true ,
      false ,
      false ,
      true   //computeCFL
    );
    watch.stop();

    if (currentThread==0) {
      logInfo("main()", "stateless kernels on host with "
        <<  markerName << ": "
        << watch.getCalendarTime() << " / "
        << (watch.getCalendarTime()/patches/NumberOfFiniteVolumesPerPatch) );
    }

    if constexpr (Accuracy>0.0) {
      int errors = 0;
      double maxDifference = 0.0;
      for (int i=0; i<patches; i++) {
        auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome( patchData.QOut[i],i,patchData.maxEigenvalue[i]);
        errors += errorsPerPatch;
        maxDifference = std::max(maxDifference, maxDifferencePerPatch);
      }

      if (errors > 0) {
        logError("main()", "max difference of outcome from "
          << markerName << " for all patches is " << maxDifference << " (admissible accuracy=" << Accuracy << ")" << " for " << errors << " entries");
        std::abort();
      }
    }

    for (int i=0; i<patches; i++) {
      tarch::freeMemory( patchData.QIn[i], tarch::MemoryLocation::Heap );
      delete[] patchData.QOut[i];
    }
  };

  auto numberOfPatchesPerThread = numberOfPatches;
  if constexpr (WithScaling) {
    numberOfPatchesPerThread /= tarch::multicore::Core::getInstance().getNumberOfThreads();
  }

  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runBaseline(thread, numberOfPatchesPerThread);
    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSLexicographicEnumerator>,
      "heap, patch-wise, AoS", thread, numberOfPatchesPerThread
    );
    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSoALexicographicEnumerator>,
      "heap, patch-wise, AoSoA", thread, numberOfPatchesPerThread
    );
    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,SoALexicographicEnumerator>,
      "heap, patch-wise, SoA", thread, numberOfPatchesPerThread
    );
    /*runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_in_situ_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables>,
      "heap, patch-wise, in-situ", thread, numberOfPatchesPerThread
    );*/

    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSLexicographicEnumerator>,
      "heap, batched, AoS", thread, numberOfPatchesPerThread
    );
    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSoALexicographicEnumerator>,
      "heap, batched, AoSoA", thread, numberOfPatchesPerThread
    );
    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,SoALexicographicEnumerator>,
      "heap, batched, SoA", thread, numberOfPatchesPerThread
    );
    /*runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_batched_in_situ_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables>,
      "heap, batched, in-situ", thread, numberOfPatchesPerThread
    );*/
  }
}

void runStatelessVariantOnDevice(
  int                       patches,
  int                       device,
  std::function<void(
    int                     device,
    exahype2::CellData&     patchData,
    bool                    evaluateFlux,
    bool                    evaluateNonconservativeProduct,
    bool                    evaluateSource,
    bool                    evaluateMaximumEigenvalueAfterTimeStep
  )> kernel,
  const std::string&      markerName,
  int                     currentThread
) {
  ::exahype2::CellData patchData(patches);
  for (int i=0; i<patches; i++) {
    patchData.QIn[i]            = tarch::allocateMemory<double>(NumberOfInputEntries, tarch::MemoryLocation::Heap);
    patchData.t[i]              = TimeStamp;
    patchData.dt[i]             = TimeStepSize;
    patchData.QOut[i]           = new double[ NumberOfOutputEntries ];
    patchData.cellCentre[i]     = tarch::la::Vector<Dimensions,double>(CellOffset+0.5*CellSize);
    patchData.cellSize[i]       = tarch::la::Vector<Dimensions,double>(CellSize);
    initInputData( patchData.QIn[i] );
  }

  std::vector<double> timings;
  for (unsigned int k = 0; k < TimeSteps; k++) {
    tarch::timing::Watch watch( "::runBenchmarksOnDevice", "runStatelessVariantOnDevice(...)", false );
    kernel(
      device,
      patchData,
      true ,
      false ,
      false ,
      true   //computeCFL
    );
    watch.stop();
    timings.push_back(watch.getCalendarTime());
  }

  if constexpr (Accuracy > 0.0) {
    int errors = 0;
    double maxDifference = 0.0;
    for (int i=0; i<patches; i++) {
      auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome( patchData.QOut[i],i,patchData.maxEigenvalue[i]);
      errors += errorsPerPatch;
      maxDifference = std::max(maxDifference, maxDifferencePerPatch);
    }

    if (errors > 0) {
      logError("main()", "max difference of outcome from "
        << markerName << " for all patches is " << maxDifference << " (admissible accuracy=" << Accuracy << ")" << " for " << errors << " entries");
      std::abort();
    }
  }

  for (int i=0; i<patches; i++) {
    tarch::freeMemory( patchData.QIn[i], tarch::MemoryLocation::Heap );
    delete[] patchData.QOut[i];
  }

  if (currentThread==0) {
    logInfo("main()", "stateless kernels on device " << device << " with "
      << markerName << ": "
      << *std::minmax_element(timings.begin() + 1, timings.end()).first << " / "
      << (*std::minmax_element(timings.begin() + 1, timings.end()).first/patches/NumberOfFiniteVolumesPerPatch)
      << ", efficiency: "
      << ((minTime/N) / (*std::minmax_element(timings.begin() + 1, timings.end()).first/patches/NumberOfFiniteVolumesPerPatch)) * 100.0 << " [%]"
    );
  }
};

#if defined(GPUOffloadingSYCL)
void submitSYCLKernels(int patches, int device, int thread) {
  tarch::accelerator::getSYCLQueue(thread).submit([&](auto &h) {
    h.host_task([=]() {/*
      runStatelessVariantOnDevice(
        patches, device,
        timeStepWithRusanov_taskgraph_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSLexicographicEnumerator>,
        "sycl, task-graph, AoS", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        timeStepWithRusanov_taskgraph_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSoALexicographicEnumerator>,
        "sycl, task-graph, AoSoA", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        timeStepWithRusanov_taskgraph_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,SoALexicographicEnumerator>,
        "sycl, task-graph, SoA", thread
      );*//*
      runStatelessVariantOnDevice(
        patches, device,
        timeStepWithRusanov_patchwise_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSLexicographicEnumerator>,
        "sycl, patch-wise, AoS", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        timeStepWithRusanov_patchwise_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSoALexicographicEnumerator>,
        "sycl, patch-wise, AoSoA", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        timeStepWithRusanov_patchwise_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,SoALexicographicEnumerator>,
        "sycl, patch-wise, SoA", thread
      );*/
      runStatelessVariantOnDevice(
        patches, device,
        timeStepWithRusanov_batched_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSLexicographicEnumerator>,
        "sycl, batched, AoS", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        timeStepWithRusanov_batched_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSoALexicographicEnumerator>,
        "sycl, batched, AoSoA", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        timeStepWithRusanov_batched_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,SoALexicographicEnumerator>,
        "sycl, batched, SoA", thread
      );
    });
  }).wait();
}
#endif

/**
 * This is the heart of the overall benchmark. We first of all invoke
 * as many threads as we are supposed to use, and we then make each
 * (logical) thread issue GPU offloading calls. Only the first thread
 * reports its runtime results (so we don't get a mess).
 *
 * @see runBenchmarksOnHost::runBaseline()
 */
void runBenchmarksOnDevice(int numberOfPatches) {
  peano4::grid::GridTraversalEvent event;
  event.setX(CellOffset);
  event.setH(CellSize);
  peano4::datamanagement::CellMarker marker(event);

  const int device = 0;
  auto numberOfPatchesPerThread = numberOfPatches;
  if constexpr (WithScaling) {
    numberOfPatchesPerThread /= tarch::multicore::Core::getInstance().getNumberOfThreads();
  }

#if defined(SharedOMP) and defined(GPUOffloadingOMP)
  std::vector<double> timings;
  double* a = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Heap);
  double* b = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Heap);
  #pragma omp target enter data map(alloc : a [0:N], b [0:N])
  for (unsigned int k = 0; k < TimeSteps; k++) {
    tarch::timing::Watch watch( "::runBenchmarksOnDevice", "measureBandwidth", false );
    //#pragma omp target loop
    #pragma omp target teams distribute parallel for simd
    for (int i = 0; i < N; i++) {
      b[i] = a[i];
    }
    watch.stop();
    timings.push_back(watch.getCalendarTime());
  }
  #pragma omp target exit data map(release : a [0:N], b [0:N])
  tarch::freeMemory( a, tarch::MemoryLocation::Heap );
  tarch::freeMemory( b, tarch::MemoryLocation::Heap );
  minTime = *std::minmax_element(timings.begin() + 1, timings.end()).first;
  const double bandwidth = 1.0E-9 * 2 * sizeof(double) * N / minTime;
  logInfo("main()", "achievable bandwidth on device " << device << ": "
    << bandwidth << " [GB/s], "
    << minTime << " / "
    << (minTime/N)
  );

  tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSLexicographicEnumerator>,
      "omp, patch-wise, AoS, managed", thread
    );
  }
  tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSoALexicographicEnumerator>,
      "omp, patch-wise, AoSoA, managed", thread
    );
  }
  tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,SoALexicographicEnumerator>,
      "omp, patch-wise, SoA, managed", thread
    );
  }

  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSLexicographicEnumerator>,
      "omp, patch-wise, AoS, map", thread
    );
  }
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSoALexicographicEnumerator>,
      "omp, patch-wise, AoSoA, map", thread
    );
  }
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,SoALexicographicEnumerator>,
      "omp, patch-wise, SoA, map", thread
    );
  }

  tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSLexicographicEnumerator>,
      "omp, batched, AoS, managed", thread
    );
  }
  tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSoALexicographicEnumerator>,
      "omp, batched, AoSoA, managed", thread
    );
  }
  tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,SoALexicographicEnumerator>,
      "omp, batched, SoA, managed", thread
    );
  }

  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSLexicographicEnumerator>,
      "omp, batched, AoS, map", thread
    );
  }
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSoALexicographicEnumerator>,
      "omp, batched, AoSoA, map", thread
    );
  }
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,SoALexicographicEnumerator>,
      "omp, batched, SoA, map", thread
    );
  }
#elif defined(SharedTBB) and defined(GPUOffloadingSYCL)
  ::tbb::task_group taskGroup;
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    taskGroup.run(
      [=]() -> void {
        submitSYCLKernels(numberOfPatchesPerThread, device, thread);
      }
    );
  }
  taskGroup.wait();
#elif defined(SharedSYCL) and defined(GPUOffloadingSYCL)
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    tarch::multicore::getHostSYCLQueue().submit([&](auto &h) {
      h.host_task([=]() {
        submitSYCLKernels(numberOfPatchesPerThread, device, thread);
      });
    }
  }
  tarch::multicore::getHostSYCLQueue().wait();
#elif defined(SharedOMP) and defined(GPUOffloadingSYCL)
  ::tbb::task_group taskGroup;
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    taskGroup.run(
      [=]() -> void {
        submitSYCLKernels(numberOfPatchesPerThread, device, thread);
      }
    );
  }
#elif defined(GPUOffloadingSYCL)
  submitSYCLKernels(numberOfPatchesPerThread, device, 0);
#elif /*defined(SharedCPP) and*/ defined(GPUOffloadingCPP)
  // @TODO: Parallelise with C++ threads
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSLexicographicEnumerator>,
      "cpp, patch-wise, AoS", thread
    );
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSoALexicographicEnumerator>,
      "cpp, patch-wise, AoSoA", thread
    );
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,SoALexicographicEnumerator>,
      "cpp, patch-wise, SoA", thread
    );
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSLexicographicEnumerator>,
      "cpp, batched, AoS", thread
    );
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,AoSoALexicographicEnumerator>,
      "cpp, batched, AoSoA", thread
    );
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<FVRusanovSolver,FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables,SoALexicographicEnumerator>,
      "cpp, batched, SoA", thread
    );
  }
#endif
}

int main(int argc, char** argv) {
  constexpr int ExitCodeSuccess          = 0;
  constexpr int ExitCodeInvalidArguments = 2;
  constexpr int ExitCodeInvalidBuild     = 3;

  peano4::initParallelEnvironment(&argc,&argv);
  exahype2::initNonCritialAssertionEnvironment();
  tarch::multicore::initSmartMPI();
  peano4::fillLookupTables();

//#ifdef UseFenv
//  tarch::feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
//#endif

  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 and tarch::multicore::Core::getInstance().getNumberOfThreads()<=1 ) {
    logError("main()", "MPI runs without multithreading are not supported currently.");
    return ExitCodeInvalidBuild;
  }

  if (not exahype2::parseCommandLineArguments(argc,argv) ) {
    logError("main()", "Invalid command line arguments.");
    return ExitCodeInvalidArguments;
  }

  repositories::startSimulation();

  otterTraceInitialise();

  tarch::logging::Statistics::getInstance().clear();

  const bool isGlobalMaster     =     tarch::mpi::Rank::getInstance().isGlobalMaster();
  const bool isPeanoComputeNode = not tarch::mpi::Rank::getInstance().isGlobalMaster();

  #if defined(GPUOffloadingSYCL)
  if ( tarch::accelerator::Device::getInstance().getNumberOfDevices()>0 ) {
    logWarning( "main()", "you might have to set ZE_SERIALIZE=1|2|3 before you start the benchmark" );
  }
  #endif

  logInfo("main()", "Number of threads: " << tarch::multicore::Core::getInstance().getNumberOfThreads());
  logInfo("main()", "Number of finite volumes per axis per patch: " << FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch);
  logInfo("main()", "Dimensions: " << Dimensions);

  int numberOfPatches = 16384;
  if constexpr (WithScaling) {
    numberOfPatches = tarch::multicore::Core::getInstance().getNumberOfThreads();
  }
  for (; numberOfPatches <= MaxPatches; numberOfPatches *= 2) {
    std::stringstream msg;
    msg << "kernel-benchmarks-" << numberOfPatches << "-patches-" << tarch::multicore::Core::getInstance().getNumberOfThreads() << "-threads";
    otterPhaseSwitch( msg.str().c_str() );

    logInfo("main()", "========================================");
    logInfo("main()", "  Number of patches: " << numberOfPatches);
    logInfo("main()", "========================================");

    runBenchmarksOnHost(numberOfPatches);
    if (tarch::accelerator::Device::getInstance().getNumberOfDevices() > 0) {
      runBenchmarksOnDevice(numberOfPatches);
    }
  }

  if constexpr (Accuracy > 0.0) {
    logInfo("main()", "all kernels yield the same outcome up to a accuracy of " << Accuracy);
  } else {
    logInfo("main()", "no accuracy checks were performed");
  }

  otterTraceFinalise();

  delete[] validOutcome;
  exit(0);
  repositories::finishSimulation();
  repositories::DataRepository::shutdownDatatypes();
  tarch::multicore::shutdownSmartMPI();
  exahype2::shutdownNonCritialAssertionEnvironment();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
