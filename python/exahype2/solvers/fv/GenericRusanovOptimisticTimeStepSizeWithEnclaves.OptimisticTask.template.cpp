#include "{{CLASSNAME}}.h"

// user includes
{{INCLUDES}}

#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"

#include "peano4/utils/Loop.h"

#include "tarch/multicore/SmartScheduler.h"
#include "tarch/mpi/DoubleMessage.h"
#include "tarch/mpi/IntegerMessage.h"

#include <algorithm>


tarch::logging::Log                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );
#if defined(UseSmartMPI)
int                                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_optimisticTaskId(
  tarch::multicore::registerSmartMPITask(
    peano4::parallel::Tasks::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}"),
    [](smartmpi::ReceiverCallType type, int rank, int tag, MPI_Comm communicator) -> smartmpi::Task* {
      if (type==smartmpi::ReceiverCallType::ReceiveTask) {
        return receiveTask( rank, tag, communicator );
      }
      else {
        return receiveOutcome( rank, tag, communicator );
      }
    }
  )
);
#else
int                                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_optimisticTaskId(
  peano4::parallel::Tasks::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}")
);
#endif


double* {{NAMESPACE | join("::")}}::{{CLASSNAME}}::copyPatchData( double* __restrict__ patchData) {
  #if Dimensions==2
  constexpr int Number = {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}};
  #else
  constexpr int Number = {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}};
  #endif

  double* result = tarch::allocateMemory(Number,tarch::MemoryLocation::Heap );
  std::copy_n(patchData, Number, result);
  return result;
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double                                      predictedTimeStepSize,
  double* __restrict__                        patchData
):
  ::exahype2::EnclaveTask(
    _optimisticTaskId,
    marker,
    t+dt,
    predictedTimeStepSize,
    copyPatchData( patchData ),
    #if Dimensions==2
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
    {{NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_2D}},
    #else
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
    {{NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_3D}},
    #endif
    [&](double* reconstructedPatch, double* targetPatch, const ::peano4::datamanagement::CellMarker& marker, double t, double dt) -> void {
          {{PREPROCESS_RECONSTRUCTED_PATCH}}

          ::exahype2::fv::copyPatch(
            reconstructedPatch,
            targetPatch,
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            {{NUMBER_OF_VOLUMES_PER_AXIS}}-2,
            1 // halo size
          );

          {% if USE_SPLIT_LOOP %}
          #if Dimensions==2
          ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
          #else
          ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
          #endif
          {% else %}
          #if Dimensions==2
          ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
          #else
          ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
          #endif
          {% endif %}
            [&](
              const double* __restrict__                   QL,
              const double* __restrict__                   QR,
              const tarch::la::Vector<Dimensions,double>&  x,
              double                                       dx,
              double                                       t,
              double                                       dt,
              int                                          normal,
              double                                       FL[],
              double                                       FR[]
            ) -> void {
            ::exahype2::fv::splitRusanov1d(
              [] (
               const double * __restrict__ Q,
               const tarch::la::Vector<Dimensions,double>&  faceCentre,
               const tarch::la::Vector<Dimensions,double>&  volumeH,
               double                                       t,
               double                                       dt,
               int                                          normal,
               double * __restrict__                        F
              ) -> void {
                {% if FLUX_IMPLEMENTATION=="<none>" %}
                for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) F[i] = 0.0;
                {% else %}
                repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
                {% endif %}
              },
              [] (
                const double* __restrict__                   Q,
                const double * __restrict__                  deltaQ,
                const tarch::la::Vector<Dimensions,double>&  faceCentre,
                const tarch::la::Vector<Dimensions,double>&  volumeH,
                double                                       t,
                double                                       dt,
                int                                          normal,
                double                                       BgradQ[]
              ) -> void {
                {% if NCP_IMPLEMENTATION!="<none>" %}
                repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, normal, BgradQ );
                {% endif %}
              },
              [] (
                const double* __restrict__                   Q,
                const tarch::la::Vector<Dimensions,double>&  faceCentre,
                const tarch::la::Vector<Dimensions,double>&  volumeH,
                double                                       t,
                double                                       dt,
                int                                          normal
              ) -> double {
                return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
              },
              QL, QR, x, dx, t, dt, normal,
              {{NUMBER_OF_UNKNOWNS}},
              {{NUMBER_OF_AUXILIARY_VARIABLES}},
              FL,FR,
              {% if FLUX_IMPLEMENTATION=="<none>" %}
              true,
              {% else %}
              false,
              {% endif %}
              {% if NCP_IMPLEMENTATION=="<none>" %}
              true
              {% else %}
              false
              {% endif %}
            );
            },
          [&](
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       dx,
            double                                       t,
            double                                       dt,
            double * __restrict__                        S
          ) -> void {
            repositories::{{SOLVER_INSTANCE}}.sourceTerm(
              Q,
              x, dx, t, dt,
              S
            );
          },
            marker.x(),
            marker.h(),
            t,
            dt,
            {{NUMBER_OF_VOLUMES_PER_AXIS}}-2,
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            reconstructedPatch,
            targetPatch
          );

          double maxEigenvalue = ::exahype2::fv::maxEigenvalue_AoS(
            [] (
              const double * __restrict__                  Q,
              const tarch::la::Vector<Dimensions,double>&  faceCentre,
              const tarch::la::Vector<Dimensions,double>&  volumeH,
              double                                       t,
              double                                       dt,
              int                                          normal
            ) -> double {
              return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
            },
            marker.x(),
            marker.h(),
            t,
            dt,
            {{NUMBER_OF_VOLUMES_PER_AXIS}}-2,
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            targetPatch
          );

          repositories::{{SOLVER_INSTANCE}}.setMaximumEigenvalue( maxEigenvalue );
        }
  )
  #ifdef UseSmartMPI
  , smartmpi::Task(_optimisticTaskId)
  #endif
{
  logDebug( "{{CLASSNAME}}(...)", "spawn optimistic task for " << marker.toString() << " with t=" << (t+dt) << ", dt=" << predictedTimeStepSize );
}



void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeTaskOutcomeIntoPatch(
  int                    taskNumber,
  double* __restrict__   patch
) {
  std::pair<int, double*> optimisticTaskOutcome = ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndReturnResult(taskNumber);

  #if Dimensions==2
  assertionEquals( optimisticTaskOutcome.first, {{NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_2D}} );
  #else
  assertionEquals( optimisticTaskOutcome.first, {{NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_3D}} );
  #endif

  ::exahype2::fv::insertPatch(
    optimisticTaskOutcome.second,
    patch,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    1
  );

  tarch::freeMemory( optimisticTaskOutcome.second, tarch::MemoryLocation::Heap );
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyKernelToCellBoundary(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double* __restrict__                        reconstructedPatch,
  double* __restrict__                        patchData
) {
  logDebug( "applyKernelToCellBoundary(...)", "update boundary of cell " << marker.toString() << " with t=" << t << ", dt=" << dt );

  {% if PREPROCESS_RECONSTRUCTED_PATCH!="" %}
  assertionMsg(false, "optimistic time stepping does not support pre- and postprocessing" );
  {% endif %}

  auto maskOutInnerVolumes = [&](const tarch::la::Vector<Dimensions, int>& i) -> bool {
    return tarch::la::oneEquals( i, 0 ) or tarch::la::oneEquals( i, {{NUMBER_OF_VOLUMES_PER_AXIS}}-1 );
  };

  ::exahype2::fv::copyPatch(
    reconstructedPatch,
    patchData,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    1,
    maskOutInnerVolumes
  );

  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS(
    [&](
      const double* __restrict__                   QL,
      const double* __restrict__                   QR,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       dx,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double                                       FL[],
      double                                       FR[]
    ) -> void {
    ::exahype2::fv::splitRusanov1d(
      [] (
       const double * __restrict__ Q,
       const tarch::la::Vector<Dimensions,double>&  faceCentre,
       const tarch::la::Vector<Dimensions,double>&  volumeH,
       double                                       t,
       double                                       dt,
       int                                          normal,
       double * __restrict__                        F
      ) -> void {
        {% if FLUX_IMPLEMENTATION=="<none>" %}
        for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) F[i] = 0.0;
        {% else %}
        repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
        {% endif %}
      },
      [] (
        const double* __restrict__                   Q,
        const double * __restrict__                  deltaQ,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       BgradQ[]
      ) -> void {
        {% if NCP_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, normal, BgradQ );
        {% endif %}
      },
      [] (
        const double* __restrict__                   Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) -> double {
        return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
      },
      QL, QR, x, dx, t, dt, normal,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      FL,FR,
      {% if FLUX_IMPLEMENTATION=="<none>" %}
      true,
      {% else %}
      false,
      {% endif %}
      {% if NCP_IMPLEMENTATION=="<none>" %}
      true
      {% else %}
      false
      {% endif %}
    );
    },
  [&](
    const double * __restrict__                  Q,
    const tarch::la::Vector<Dimensions,double>&  x,
    double                                       dx,
    double                                       t,
    double                                       dt,
    double * __restrict__                        S
  ) -> void {
    repositories::{{SOLVER_INSTANCE}}.sourceTerm(
      Q,
      x, dx, t, dt,
      S
    );
  },
    marker.x(),
    marker.h(),
    t,
    dt,
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    reconstructedPatch,
    patchData,
    maskOutInnerVolumes
  );

  double maxEigenvalue = ::exahype2::fv::maxEigenvalue_AoS(
    [] (
      const double* __restrict__                   Q,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) -> double {
      return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
    },
    marker.x(),
    marker.h(),
    t,
    dt,
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    patchData,
    maskOutInnerVolumes
  );

  repositories::{{SOLVER_INSTANCE}}.setMaximumEigenvalue( maxEigenvalue );
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isSmartMPITask() const {
  #ifdef UseSmartMPI
  return true;
  #else
  return false;
  #endif
}


#ifdef UseSmartMPI
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::runLocally() {
  run();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::moveTask(int rank, int tag, MPI_Comm communicator) {
  ::tarch::mpi::DoubleMessage  tMessage(_t);
  ::tarch::mpi::DoubleMessage  dtMessage(_dt);
  ::tarch::mpi::IntegerMessage taskIdMessage(getTaskId());

  ::peano4::datamanagement::CellMarker::send( _marker, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::send( tMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::send( dtMessage, rank, tag, communicator );
  ::tarch::mpi::IntegerMessage::send( taskIdMessage, rank, tag, communicator );

  MPI_Send( _inputValues, _numberOfInputValues, MPI_DOUBLE, rank, tag, communicator );

  logInfo(
    "moveTask(...)",
    "sent (" << _marker.toString() << "," << tMessage.toString() << "," << dtMessage.toString() << "," << _numberOfInputValues <<
    "," << taskIdMessage.toString() << ") to rank " << rank <<
    " via tag " << tag
  );
}


smartmpi::Task* {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveTask(int rank, int tag, MPI_Comm communicator) {
  peano4::grid::GridTraversalEvent dummyEvent;
  const int NumberOfInputValues =
    #if Dimensions==2
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}};
    #else
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}};
    #endif

  ::tarch::mpi::DoubleMessage tMessage;
  ::tarch::mpi::DoubleMessage dtMessage;
  ::tarch::mpi::IntegerMessage taskIdMessage;
  ::peano4::datamanagement::CellMarker markerMessage(dummyEvent);
  double* inputValues = tarch::allocateMemory( NumberOfInputValues, tarch::MemoryLocation::Heap );

  ::peano4::datamanagement::CellMarker::receive( markerMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::receive( tMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::receive( dtMessage, rank, tag, communicator );
  ::tarch::mpi::IntegerMessage::receive( taskIdMessage, rank, tag, communicator );

  logInfo(
    "receiveTask(...)",
    "received (" << markerMessage.toString() << "," << tMessage.toString() << "," << dtMessage.toString() << "," << taskIdMessage.toString() << ") from rank " << rank <<
    " via tag " << tag << " and will now receive " << NumberOfInputValues << " doubles"
  );

  MPI_Recv( inputValues, NumberOfInputValues, MPI_DOUBLE, rank, tag, communicator,
    MPI_STATUS_IGNORE
  );



  {{CLASSNAME}}* result = new {{CLASSNAME}}(
    markerMessage,
    tMessage.getValue(),
    dtMessage.getValue(),
    inputValues
  );
  result->_remoteTaskId = taskIdMessage.getValue();
  return result;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::runLocallyAndSendTaskOutputToRank(int rank, int tag, MPI_Comm communicator) {
  _outputValues = tarch::allocateMemory( _numberOfResultValues, tarch::MemoryLocation::Heap );

  _functor(_inputValues,_outputValues,_marker,_t,_dt);
  tarch::freeMemory(_inputValues,tarch::MemoryLocation::Heap );

  logInfo(
    "receiveTask(...)",
    "executed remote task on this rank. Will start to send result back"
  );

  ::tarch::mpi::DoubleMessage  tMessage(_t);
  ::tarch::mpi::DoubleMessage  dtMessage(_dt);
  ::tarch::mpi::IntegerMessage taskIdMessage(_remoteTaskId);

  ::peano4::datamanagement::CellMarker::send( _marker, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::send( tMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::send( dtMessage, rank, tag, communicator );
  ::tarch::mpi::IntegerMessage::send( taskIdMessage, rank, tag, communicator );

  MPI_Send( _outputValues, _numberOfResultValues, MPI_DOUBLE, rank, tag, communicator );

  logInfo(
    "moveTask(...)",
    "sent (" << _marker.toString() << "," << tMessage.toString() << "," << dtMessage.toString() << "," << _numberOfResultValues <<
    "," << taskIdMessage.toString() << ") to rank " << rank <<
    " via tag " << tag
  );

  tarch::freeMemory(_outputValues,tarch::MemoryLocation::Heap );
}


smartmpi::Task* {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveOutcome(int rank, int tag, MPI_Comm communicator) {
  logInfo( "receiveOutcome(...)", "rank=" << rank << ", tag=" << tag );
  peano4::grid::GridTraversalEvent dummyEvent;
  const int NumberOfResultValues =
    #if Dimensions==2
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}};
    #else
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}};
    #endif

  ::tarch::mpi::DoubleMessage tMessage;
  ::tarch::mpi::DoubleMessage dtMessage;
  ::tarch::mpi::IntegerMessage taskIdMessage;
  ::peano4::datamanagement::CellMarker markerMessage(dummyEvent);
  double* outputValues = tarch::allocateMemory( NumberOfResultValues, tarch::MemoryLocation::Heap );

  ::peano4::datamanagement::CellMarker::receive( markerMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::receive( tMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::receive( dtMessage, rank, tag, communicator );
  ::tarch::mpi::IntegerMessage::receive( taskIdMessage, rank, tag, communicator );

  logInfo(
    "receiveOutcome(...)",
    "received (" << markerMessage.toString() << "," << tMessage.toString() << "," << dtMessage.toString() << "," << taskIdMessage.toString() << ") from rank " << rank <<
    " via tag " << tag << " and will now receive " << NumberOfResultValues << " doubles"
  );

  MPI_Recv( outputValues, NumberOfResultValues, MPI_DOUBLE, rank, tag, communicator,
    MPI_STATUS_IGNORE
  );

  logInfo(
    "receiveOutcome(...)",
    "bookmark outcome of task " << taskIdMessage.getValue()
  );

  ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(taskIdMessage.getValue(),NumberOfResultValues,outputValues);

  return nullptr;
}
#endif
