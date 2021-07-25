// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_STACKS_STDVECTOR_POINTER_STACK_H_
#define _PEANO4_STACKS_STDVECTOR_POINTER_STACK_H_


#include <vector>
#include <algorithm>


#include "stacks.h"
#include "STDVectorStack.h"


#include "peano4/parallel/Node.h"
#include "peano4/grid/GridVertex.h"


#include "tarch/logging/Log.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/multicore/Tasks.h"




namespace peano4 {
  namespace stacks {
    template <class T>
    class STDVectorOverVectorOfPointersStack;
  }
}


/**
 * I assume that T is in itself a vector over pointers.
 */
template <class T>
class peano4::stacks::STDVectorOverVectorOfPointersStack: public peano4::stacks::STDVectorStack<T> {
  private:
    /**
     * Logging device.
     */
    static tarch::logging::Log _log;

    typedef peano4::stacks::STDVectorStack<T>   Base;

    typename T::value_type*                     _deepCopyBuffer;
    std::vector<int>                            _deepCopySizePerEntry;

    void clearDeepCopyBuffer() {
      if (_deepCopyBuffer!=nullptr) {
        delete[] _deepCopyBuffer;
      }
    }
  public:
    STDVectorOverVectorOfPointersStack():
      _deepCopyBuffer(nullptr) {
    }


    virtual ~STDVectorOverVectorOfPointersStack() {
      clearDeepCopyBuffer();
    }


    /**
     * I send out meta data blocking.
     */
    void startSend(int rank, int tag) {
      #ifdef Parallel
      assertion( Base::_ioMode==IOMode::None );
      assertion( Base::_ioMPIRequest==nullptr );
      Base::_ioMode = IOMode::MPISend;
      Base::_ioTag  = tag;
      Base::_ioRank = rank;

      logDebug( "startSend(int,int)", toString());

      _deepCopySizePerEntry.clear();
      int flattenedDataSize = 0;
      for (int i=0; i<Base::_currentElement; i++) {
        int sizeOfThisStackEntry = Base::_data[i].size();
        _deepCopySizePerEntry.push_back( sizeOfThisStackEntry );
        flattenedDataSize += sizeOfThisStackEntry;
      }
      logInfo( "startSend(int,int)", "assembled meta data encoding entries per stack entry. Total number of entries is " << flattenedDataSize );

      _deepCopyBuffer = new typename T::value_type[ flattenedDataSize ];
      int entry = 0;
      for (int i=0; i<Base::_currentElement; i++) {
        for (int j=0; j<Base::_data[i].size(); j++) {
          _deepCopyBuffer[entry] = Base::_data[i].data()[j];
          entry++;
        }
      }
      logInfo( "startSend(int,int)", "created deep copy of all " << entry << " entries in the vector of pointers" );
      assertionEquals( flattenedDataSize, entry );

      int result = 0;
      result = MPI_Send( _deepCopySizePerEntry.data(), Base::_currentElement, MPI_INT, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator() );
      if  (result!=MPI_SUCCESS) {
        logError( "startSend(int,int)", "was not able to send to node " << rank << " on tag " << tag
           << ": " << tarch::mpi::MPIReturnValueToString(result)
        );
      }
      logInfo( "startSend(int,int)", "sent out meta data in blocking mode with " << Base::_currentElement << " entries" );

      if (entry>0) {
        Base::_ioMPIRequest = new MPI_Request;
        result = MPI_Isend( _deepCopyBuffer, entry, T::Datatype, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), Base::_ioMPIRequest);
        if  (result!=MPI_SUCCESS) {
          logError( "startSend(int,int)", "was not able to send to node " << rank << " on tag " << tag
           << ": " << tarch::mpi::MPIReturnValueToString(result)
          );
        }
      }
      else {
        Base::_ioMPIRequest = nullptr;
      }
      #endif
    }


    /**
     * @see startSend()
     */
    void startReceive(int rank, int tag, int numberOfElements) {
      #ifdef Parallel
      assertion3( Base::_ioMode==IOMode::None, rank, tag, numberOfElements );
      assertion3( numberOfElements>0, rank, tag, numberOfElements );

      Base::_ioMode = IOMode::MPIReceive;
      Base::_ioTag  = tag;
      Base::_ioRank = rank;

      int result = 0;

      _deepCopySizePerEntry.resize(numberOfElements);
      Base::_data.resize(numberOfElements);
      Base::_currentElement = numberOfElements;
      assertionEquals( Base::_data.size(), numberOfElements );

      result = MPI_Recv( _deepCopySizePerEntry.data(), numberOfElements, MPI_INT, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), MPI_STATUS_IGNORE );
      if  (result!=MPI_SUCCESS) {
        logError( "startReceive(int,int,int)", "was not able to receive " << numberOfElements << " values from node " << rank << " on tag " << tag
           << ": " << tarch::mpi::MPIReturnValueToString(result)
        );
      }
      // @todo Debug
      logInfo( "startReceive(int,int,int)", "received all meta data from rank " << rank << " on tag " << tag << " with " << numberOfElements << " entries");

      int flattenedDataSize = 0;
      for (int i=0; i<numberOfElements; i++) {
        flattenedDataSize += _deepCopySizePerEntry.at(i);
        Base::_data[i].clear();
      }
      // @todo Debug
      logInfo( "startReceive(int,int,int)", "start to receive " << flattenedDataSize << " in total. Have cleared receive data" );

      assertion( Base::_ioMPIRequest == nullptr );

      if (flattenedDataSize>0) {
        Base::_ioMPIRequest = new MPI_Request;
        _deepCopyBuffer = new typename T::value_type[ flattenedDataSize ];
        int result = MPI_Irecv( _deepCopyBuffer, flattenedDataSize, T::Datatype, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), Base::_ioMPIRequest);
        if  (result!=MPI_SUCCESS) {
          logError( "startReceive(int,int,int)", "was not able to receive " << numberOfElements << " values from node " << rank << " on tag " << tag
             << ": " << tarch::mpi::MPIReturnValueToString(result)
          );
        }
      }
      else {
        Base::_ioMPIRequest = nullptr;
      }
      #endif
    }


    bool tryToFinishSendOrReceive() {
      #ifdef Parallel
      logTraceInWith4Arguments( "tryToFinishSendOrReceive()", ::toString(Base::_ioMode), Base::size(), Base::_ioRank, Base::_ioTag );

      bool result = true;
/*
      if ( _ioMode==IOMode::MPISend or _ioMode==IOMode::MPIReceive ) {
        assertion( _ioMPIRequest!=nullptr );

        int          flag = 0;
        MPI_Test( _ioMPIRequest, &flag, MPI_STATUS_IGNORE );
        if (flag) {
          result = true;
          logDebug( "tryToFinishSendOrReceive()", "send/receive complete, free MPI request: " << toString() );
          delete _ioMPIRequest;
          _ioMPIRequest = nullptr;
          if (_ioMode==IOMode::MPISend ) {
            clear();
          }
          _ioMode = IOMode::None;
        }
        else {
          result = false;
        }
      }
      clearDeepCopyBuffer();
*/

      logTraceOutWith1Argument( "tryToFinishSendOrReceive()", result );
      return result;
      #else
      return true;
      #endif
    }
};


template <class T>
tarch::logging::Log peano4::stacks::STDVectorOverVectorOfPointersStack<T>::_log( "peano4::stacks::STDVectorOverVectorOfPointersStack<T>" );


#endif
