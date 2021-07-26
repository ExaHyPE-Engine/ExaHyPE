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
 * Administer vector over pointers
 *
 * I assume that T is in itself a vector over pointers. In the serial version,
 * such a construct works out of the box: It is the vector over pointers (VoP)
 * which travels over the stacks of the spacetree. Once we enter the MPI real,
 * things become a little bit more complicated however: As we have these
 * vectors, we require some serious deep copying.
 *
 * While you can pipe in any vector, I require the vector to have a type
 * DoFType which identifies the type where the pointers point to and is
 * generated via DaStGen, and a couple of additional fields:
 *
 * - tarch::la::Vector<Dimensions,double> _debugX to hold debug info.
 * - tarch::la::Vector<Dimensions,double> _debugH to hold debug info.
 * - static MPI_Datatype   Datatype;
 *
 * The first two entries are required if and only if PeanoDebug holds a value
 * greater or equal to 1. The latter two fields are required if you compile
 * with Parallel.
 *
 * Besides these fields, I expect the following routines for the management of
 * all MPI data:
 *
 * - void setDebugX( const tarch::la::Vector<Dimensions,double>& data );
 * - void setDebugH( const tarch::la::Vector<Dimensions,double>& data );
 * - tarch::la::Vector<Dimensions,double> getDebugX() const;
 * - tarch::la::Vector<Dimensions,double> getDebugH() const;
 * - static void initDatatype();
 * - static void shutdownDatatype();
 *
 * - int explicitlyStoreSize(); Rolls over the size() into an explicit
 *   attribute, which also is covered by the MPI data type. After it
 *   has copied the data, it returns this copy as result.
 * - int getExplicitlyStoredSize() const; Return value of the explicitly
 *   stored integer.
 *
 *
 * @see ParticleSet.template.h for an example of the required signature.
 *
 *
 * <h2> MPI </h2>
 *
 * To realise the MPI data exchange, I have to add two further fields to the stack.
 * One is an additional request (pointer) called _metaDataMPIRequest, the other one
 * is a pointer to an array of type T::value_type. It is called _deepCopyBuffer.
 * I assume that there is a static Datatype field for the administered vectors.
 * If we use this one, it does not exchange actual data, but "only" meta data (if
 * in debug mode) and the integer that stores the size explicitly.
 *
 * The send is a simple send of two nonblocking operations: First, I send out the
 * actual container. This is, I send out one array (vector) which encodes the debug
 * info and the number of entries per stack entry. This send does not encode the
 * pointers, as we cannot send out pointers directly. The request handle of this
 * non-blocking send is stored in _metaDataMPIRequest.
 *
 * To find the actual information of relevance, I run over the all stack entries
 * and call explicitlyStoreSize(). This ensures that each stack entry holds an
 * explicit integer will all information of interest. I also accumulate all of
 * the results of explicitlyStoreSize().
 *
 * If there is data on the stack, i.e. if the accumulated results of
 * explicitlyStoreSize() is bigger than zero, I create a new buffer of DoFType,
 * run once more through all stack entries and copy the data manually over. The
 * result array is stored in _deepCopyBuffer. This second send is not always
 * necessary - we issue it if and only if there's data to exchange. If it is
 * necessary, I dump the result in _ioMPIRequest.
 *
 * The receive also consists of two receives, but they are not of the same type.
 * First, I trigger a blocking receive on the actual stack. In line with the
 * discussion above, this receive sets all debug data, as well as the explicit
 * integer encoding the length per stack entry. I exploit the symmetry of Peano's
 * mesh, i.e. I exploit the fact that I know how many entries (stacks) I'll hold
 * on my stack. This operation is blocking, as I need this meta data to continue.
 * To MPI handle is involved therefore.
 *
 * Next, I run over all stack entres and invoke getExplicitlyStoredSize(). Having
 * the target size explicitly allows me to issue a resize(), and I can also, once
 * again, accumulate all of these results. If the accumulated result is bigger
 * than zero, i.e. if actual data is to be exchanged, I next allocate
 * _deepCopyBuffer and issue a receive. This receive now can be non-blocking.
 *
 * See the documentation of tryToFinishSendOrReceive() for a clean-up and details
 * how the nonblocking receives are mapped onto actual stack content.
 */
template <class T>
class peano4::stacks::STDVectorOverVectorOfPointersStack: public peano4::stacks::STDVectorStack<T> {
  private:
    /**
     * Logging device.
     */
    static tarch::logging::Log _log;

    typedef peano4::stacks::STDVectorStack<T>   Base;

    #ifdef Parallel
    MPI_Request*           _metaDataMPIRequest;
    typename T::DoFType*   _deepCopyBuffer;
    #endif

  public:
    STDVectorOverVectorOfPointersStack():
      _deepCopyBuffer(nullptr) {
    }


    /**
     * See class documentation.
     */
    void startSend(int rank, int tag) {
      #ifdef Parallel
      assertion( Base::_ioMode==IOMode::None );
      assertion( Base::_ioMPIRequest==nullptr );
      Base::_ioMode = IOMode::MPISend;
      Base::_ioTag  = tag;
      Base::_ioRank = rank;

      logDebug( "startSend(int,int)", toString());

      int flattenedDataSize = 0;
      for (int i=0; i<Base::_currentElement; i++) {
        int sizeOfThisStackEntry          = Base::_data[i].explicitlyStoreSize();
        flattenedDataSize                += sizeOfThisStackEntry;

        logInfo(
          "startSend(int,int)",
          "data of entry " << i << ": " << Base::_data.at(i).toString() <<
          " - " << &(Base::_data.at(i)) << "/" << ((long)&(Base::_data.at(i)) - (long)Base::_data.data())
        );
      }
      logInfo( "startSend(int,int)", "assembled meta data encoding entries per stack entry. Total number of entries is " << flattenedDataSize );

      int result = 0;
      _metaDataMPIRequest = new MPI_Request;
      result = MPI_Isend( Base::_data.data(), Base::_currentElement, T::Datatype, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), _metaDataMPIRequest);
      if  (result!=MPI_SUCCESS) {
        logError( "startSend(int,int)", "was not able to send to node " << rank << " on tag " << tag
         << ": " << tarch::mpi::MPIReturnValueToString(result)
        );
      }
      logInfo( "startSend(int,int)", "sent out meta data of size " << Base::_currentElement );

      if (flattenedDataSize>0) {
        _deepCopyBuffer = new typename T::DoFType[ flattenedDataSize ];
        int entry = 0;
        for (int i=0; i<Base::_currentElement; i++) {
          for (int j=0; j<Base::_data[i].size(); j++) {
            _deepCopyBuffer[entry] = *(Base::_data[i].data()[j]);
            entry++;
          }
        }
        logInfo( "startSend(int,int)", "created deep copy of all " << entry << " entries in the vector of pointers" );
        assertionEquals( flattenedDataSize, entry );

        Base::_ioMPIRequest = new MPI_Request;
        result = MPI_Isend( _deepCopyBuffer, flattenedDataSize, T::DoFType::Datatype, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), Base::_ioMPIRequest);
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

      Base::_data.resize(numberOfElements);
      Base::_currentElement = numberOfElements;
      assertionEquals( Base::_data.size(), numberOfElements );

      // @todo Wieder raus?
/*
      for (int i=0; i<numberOfElements; i++) {
        Base::_data[i].clear();
        logInfo(
          "startReceive(int,int,int)",
          "dummy entry " << i << ": " << Base::_data.at(i).toString()
        );
      }
*/

      result = MPI_Recv( &(Base::_data.data()[0]), numberOfElements, T::Datatype, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), MPI_STATUS_IGNORE );
      if  (result!=MPI_SUCCESS) {
        logError( "startReceive(int,int,int)", "was not able to receive " << numberOfElements << " values from node " << rank << " on tag " << tag
           << ": " << tarch::mpi::MPIReturnValueToString(result)
        );
      }
      // @todo Debug
      logInfo( "startReceive(int,int,int)", "received all meta data from rank " << rank << " on tag " << tag << " with " << numberOfElements << " entries");

      int flattenedDataSize = 0;
      assertionEquals( Base::_data.size(), numberOfElements );
      for (int i=0; i<numberOfElements; i++) {
        Base::_data[i]. xxx.resize(sizeOfThisStackEntry);
        logInfo(
          "startReceive(int,int,int)",
          "data of entry " << i << ": " << Base::_data.at(i).toString() <<
          " - " << &(Base::_data.at(i)) << "/" << ((long)&(Base::_data.at(i)) - (long)Base::_data.data())
        );

        int sizeOfThisStackEntry  = Base::_data.at(i).getExplicitlyStoredSize();
        flattenedDataSize        += sizeOfThisStackEntry;
        Base::_data[i].resize(sizeOfThisStackEntry);
      }
      // @todo Debug
      logInfo( "startReceive(int,int,int)", "start to receive " << flattenedDataSize << " in total. Have cleared receive data" );

      assertion( Base::_ioMPIRequest == nullptr );

      if (flattenedDataSize>0) {
        Base::_ioMPIRequest = new MPI_Request;
        _deepCopyBuffer = new typename T::DoFType[ flattenedDataSize ];
        int result = MPI_Irecv( _deepCopyBuffer, flattenedDataSize, T::DoFType::Datatype, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), Base::_ioMPIRequest);
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

      // The receive always consists of a blocking receive and then, maybe, a
      // nonblocking one on _ioMPIRequest.
      if ( Base::_ioMode==IOMode::MPIReceive and Base::_ioMPIRequest!=nullptr ) {
        int          flag = 0;
        MPI_Test( Base::_ioMPIRequest, &flag, MPI_STATUS_IGNORE );
        if (flag) {
          result = true;
          // @todo Debug
          logInfo( "tryToFinishSendOrReceive()", "receive complete, free MPI request on actual data: " << Base::toString() );
          delete Base::_ioMPIRequest;
          Base::_ioMPIRequest = nullptr;
          Base::_ioMode       = IOMode::None;

          assertionMsg(false, "hier muss ich noch vom flat rueberkopieren")

          assertion(_deepCopyBuffer!=nullptr);
          delete[] _deepCopyBuffer;
        }
        else {
          result = false;
        }
      }
      else if ( Base::_ioMode==IOMode::MPIReceive ) {
        // @todo Debug
        logInfo( "tryToFinishSendOrReceive()", "receive complete, but no actual data transferred. No need to free any request therefore: " << Base::toString() );
        Base::_ioMode = IOMode::None;
        result = true;
      }
      // The send always sends out the meta request non-blocking. Then, it might also
      // send out the actual data non-blocking.
      else if ( Base::_ioMode==IOMode::MPISend and Base::_ioMPIRequest!=nullptr ) {
        assertion( _metaDataMPIRequest!=nullptr );

        int          flag = 0;
        MPI_Test( Base::_ioMPIRequest, &flag, MPI_STATUS_IGNORE );
        if (flag) {
          MPI_Wait( _metaDataMPIRequest, MPI_STATUS_IGNORE );
          result = true;

          // @todo Debug
          logInfo( "tryToFinishSendOrReceive()", "send complete, free both data and meta data request: " << Base::toString() );

          delete Base::_ioMPIRequest;
          delete _metaDataMPIRequest;
          Base::_ioMPIRequest = nullptr;
          _metaDataMPIRequest = nullptr;

          Base::clear();
          Base::_ioMode = IOMode::None;

          // @todo Debug
          logInfo( "tryToFinishSendOrReceive()", "delete deep copy" );
          assertion(_deepCopyBuffer!=nullptr);
          delete[] _deepCopyBuffer;
        }
        else {
          result = false;
        }
      }
      // Send without any real data
      else if ( Base::_ioMode==IOMode::MPISend ) {
        assertion( _metaDataMPIRequest!=nullptr );

        int          flag = 0;
        MPI_Test( _metaDataMPIRequest, &flag, MPI_STATUS_IGNORE );
        if (flag) {
          result = true;

          // @todo Debug
          logInfo( "tryToFinishSendOrReceive()", "send complete, but no data transferred. Delete meta data request: " << Base::toString() );

          delete _metaDataMPIRequest;
          _metaDataMPIRequest = nullptr;

          Base::clear();
          Base::_ioMode = IOMode::None;
        }
        else {
          result = false;
        }

        assertion(_deepCopyBuffer==nullptr);
      }

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
