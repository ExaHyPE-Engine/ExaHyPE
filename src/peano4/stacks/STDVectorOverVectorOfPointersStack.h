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
 * DoFType which identifies the type where the pointers point to and this target
 * data type has to be generated via DaStGen. At least I need a valid Datatype.
 * Furthermore, I expect the following routines to be available in debug mode:
 *
 * - void setDebugX( const tarch::la::Vector<Dimensions,double>& data );
 * - void setDebugH( const tarch::la::Vector<Dimensions,double>& data );
 * - tarch::la::Vector<Dimensions,double> getDebugX() const;
 * - tarch::la::Vector<Dimensions,double> getDebugH() const;
 *
 * These two operations should be available, too, if I work with MPI, and
 * they typically just forward the call to the underlying datatype DoFType:
 *
 * - static void initDatatype();
 * - static void shutdownDatatype();
 *
 * @see ParticleSet.template.h for an example of the required signature.
 *
 *
 * <h2> MPI </h2>
 *
 * To realise the MPI data exchange, I did originally plan to require the
 * handed in vector datatype to provide some proper MPI datatypes. This did
 * not work: If we have inheritance, the byte alignment easily becomes messed
 * up and I never really managed to make everything work. Therefore, I do
 * work now with three different, explicit buffers:
 *
 * - There's a size meta data buffer which is a sequence of integers. Each
 *   entry stores how big the underlying vector is.
 * - There's a debug meta data buffer which I befill if and only if we work
 *   with meta data. It is a long sequence of doubles. Per stack entry, I
 *   hold 2*Dimensions data points: Dimensions entries encode the x value of
 *   the debug data, and a further Dimensions entries encode the h value.
 * - The actual data has to be flattened (serialised) too, as I cannot
 *   transfer any pointers. This serialised data is held in yet another
 *   buffer and has to be copied forth and back. Obviously, there's no need
 *   to use this buffer if no data travels in-between MPI boundaries.
 *
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
    MPI_Request*           _metaDataSizeMPIRequest;
    MPI_Request*           _metaDataDebugMPIRequest;

    typename T::DoFType*   _deepCopyDataBuffer;
    int*                   _metaDataSizeBuffer;
    double*                _metaDataDebugBuffer;

    void workInReceivedMetaData() {
      assertion( _metaDataSizeBuffer!=nullptr );
      #if PeanoDebug>=1
      assertion( _metaDataDebugBuffer!=nullptr );
      #endif
      for (int i=0; i<Base::_currentElement; i++) {
        const int entries = _metaDataSizeBuffer[i];
        Base::_data[i].resize(entries);
        #if PeanoDebug>=1 and Dimensions==2
        tarch::la::Vector<Dimensions,double> x = {_metaDataDebugBuffer[i*2*2+0], _metaDataDebugBuffer[i*2*2+1]};
        tarch::la::Vector<Dimensions,double> h = {_metaDataDebugBuffer[i*2*2+2], _metaDataDebugBuffer[i*2*2+3]};
        Base::_data[i].setDebugX( x );
        Base::_data[i].setDebugH( h );
        #endif
        #if PeanoDebug>=1 and Dimensions==3
        tarch::la::Vector<Dimensions,double> x = {_metaDataDebugBuffer[i*2*3+0], _metaDataDebugBuffer[i*2*3+1], _metaDataDebugBuffer[i*2*3+2]};
        tarch::la::Vector<Dimensions,double> h = {_metaDataDebugBuffer[i*2*3+3], _metaDataDebugBuffer[i*2*3+4], _metaDataDebugBuffer[i*2*3+5]};
        Base::_data[i].setDebugX( x );
        Base::_data[i].setDebugH( h );
        #endif
        logDebug( "workInReceivedMetaData()", "initialised entry " << i << ": " << Base::_data[i].toString() );
      }
    }


    void workInReceivedData() {
      int entry = 0;
      for (int i=0; i<Base::_currentElement; i++) {
        for (int j=0; j<Base::_data[i].size(); j++) {
          *(Base::_data[i].data()[j]) = _deepCopyDataBuffer[entry];
          entry++;
        }
      }
    }


    void prepareDataToSendOut() {
      int entry = 0;
      for (int i=0; i<Base::_currentElement; i++) {
        for (int j=0; j<Base::_data[i].size(); j++) {
          _deepCopyDataBuffer[entry] = *(Base::_data[i].data()[j]);
          entry++;
        }
      }
    }

    /**
     * @return Total size of all entres
     */
    int prepareMetaDataToSendOut() {
      int flattenedDataSize = 0;
      for (int i=0; i<Base::_currentElement; i++) {
        int sizeOfThisStackEntry          = Base::_data[i].size();
        flattenedDataSize                += sizeOfThisStackEntry;
        _metaDataSizeBuffer[i]            = sizeOfThisStackEntry;
        #if PeanoDebug>=1 and Dimensions==2
        _metaDataDebugBuffer[i*2*2+0]       = Base::_data[i].getDebugX()[0];
        _metaDataDebugBuffer[i*2*2+1]       = Base::_data[i].getDebugX()[1];
        _metaDataDebugBuffer[i*2*2+2]       = Base::_data[i].getDebugH()[0];
        _metaDataDebugBuffer[i*2*2+3]       = Base::_data[i].getDebugH()[1];
        #endif
        #if PeanoDebug>=1 and Dimensions==3
        _metaDataDebugBuffer[i*2*3+0]       = Base::_data[i].getDebugX()[0];
        _metaDataDebugBuffer[i*2*3+1]       = Base::_data[i].getDebugX()[1];
        _metaDataDebugBuffer[i*2*3+2]       = Base::_data[i].getDebugX()[2];
        _metaDataDebugBuffer[i*2*3+3]       = Base::_data[i].getDebugH()[0];
        _metaDataDebugBuffer[i*2*3+4]       = Base::_data[i].getDebugH()[1];
        _metaDataDebugBuffer[i*2*3+5]       = Base::_data[i].getDebugH()[2];
        #endif
        logDebug( "prepareMetaDataToSendOut()", "prepare entry " << i << ": " << Base::_data[i].toString() );
      }
      return flattenedDataSize;
    }

    #endif

  public:
    STDVectorOverVectorOfPointersStack() {
      #ifdef Parallel
      _metaDataSizeMPIRequest  = nullptr;
      _metaDataDebugMPIRequest = nullptr;
      Base::_ioMPIRequest      = nullptr;

      _deepCopyDataBuffer  = nullptr;
      _metaDataSizeBuffer  = nullptr;
      _metaDataDebugBuffer = nullptr;
      #endif
    }


    /**
     * See class documentation.
     */
    void startSend(int rank, int tag) {
      #ifdef Parallel
      assertion( Base::_ioMode==IOMode::None );

      Base::_ioMode = IOMode::MPISend;
      Base::_ioTag  = tag;
      Base::_ioRank = rank;

      assertion( _metaDataSizeMPIRequest  == nullptr );
      assertion( _metaDataDebugMPIRequest == nullptr );
      assertion( Base::_ioMPIRequest      == nullptr );

      assertion( _deepCopyDataBuffer  == nullptr );
      assertion( _metaDataSizeBuffer  == nullptr );
      assertion( _metaDataDebugBuffer == nullptr );

      logDebug( "startSend(int,int)", toString());

      _metaDataSizeBuffer  = new int[Base::_currentElement];
      #if PeanoDebug>=1
      _metaDataDebugBuffer = new double[Base::_currentElement * 2 * Dimensions];
      #endif

      int flattenedDataSize = prepareMetaDataToSendOut();
      logDebug( "startSend(int,int)", "assembled and flattened meta data encoding entries per stack entry. Total number of entries is " << flattenedDataSize );

      _metaDataSizeMPIRequest = new MPI_Request;
      MPI_Isend( _metaDataSizeBuffer, Base::_currentElement, MPI_INT, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), _metaDataSizeMPIRequest);
      logDebug( "startSend(int,int)", "sent out size meta data of size " << Base::_currentElement );

      #if PeanoDebug>=1
      _metaDataDebugMPIRequest = new MPI_Request;
      MPI_Isend( _metaDataDebugBuffer, Base::_currentElement*2*Dimensions, MPI_DOUBLE, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), _metaDataDebugMPIRequest);
      logDebug( "startSend(int,int)", "sent out debug meta data of size " << Base::_currentElement*2*Dimensions );
      #endif

      if (flattenedDataSize>0) {
        _deepCopyDataBuffer = new typename T::DoFType[ flattenedDataSize ];
        prepareDataToSendOut();
        logDebug( "startSend(int,int)", "created deep copy of all " << entry << " entries in the vector of pointers" );

        Base::_ioMPIRequest = new MPI_Request;
        MPI_Isend( _deepCopyDataBuffer, flattenedDataSize, T::DoFType::Datatype, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), Base::_ioMPIRequest);
      }
      else {
        logDebug( "startSend(int,int)", "there's no actual user data to send out" );
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

      assertion( _metaDataSizeMPIRequest  == nullptr );
      assertion( _metaDataDebugMPIRequest == nullptr );
      assertion( Base::_ioMPIRequest      == nullptr );

      assertion( _deepCopyDataBuffer  == nullptr );
      assertion( _metaDataSizeBuffer  == nullptr );
      assertion( _metaDataDebugBuffer == nullptr );

      Base::_data.resize(numberOfElements);
      Base::_currentElement = numberOfElements;
      assertionEquals( Base::_data.size(), numberOfElements );

      _metaDataSizeBuffer  = new int[Base::_currentElement];
      #if PeanoDebug>=1
      _metaDataDebugBuffer = new double[Base::_currentElement * 2 * Dimensions];
      #endif

      MPI_Recv( _metaDataSizeBuffer, numberOfElements, MPI_INT, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), MPI_STATUS_IGNORE );
      logDebug( "startReceive(int,int,int)", "received all meta data from rank " << rank << " on tag " << tag << " with " << numberOfElements << " entries");

      #if PeanoDebug>=1
      _metaDataDebugMPIRequest = new MPI_Request;
      MPI_Irecv( _metaDataDebugBuffer, numberOfElements*2*Dimensions, MPI_DOUBLE, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), _metaDataDebugMPIRequest);
      logDebug( "startReceive(int,int,int)", "trigger receive of debug data with cardinality " << numberOfElements*2*Dimensions );
      #endif

      int flattenedDataSize = 0;
      assertionEquals( Base::_data.size(), numberOfElements );
      for (int i=0; i<numberOfElements; i++) {
        flattenedDataSize        += _metaDataSizeBuffer[i];
      }

      if (flattenedDataSize>0) {
        Base::_ioMPIRequest = new MPI_Request;
        _deepCopyDataBuffer = new typename T::DoFType[ flattenedDataSize ];
        MPI_Irecv( _deepCopyDataBuffer, flattenedDataSize, T::DoFType::Datatype, Base::_ioRank, Base::_ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), Base::_ioMPIRequest);
        logDebug( "startReceive(int,int,int)", "trigger receive of " << flattenedDataSize << " entries of user data in total" );
      }
      else {
        logDebug( "startReceive(int,int,int)", "there's no actual user data to received" );
      }
      #endif
    }


    bool tryToFinishSendOrReceive() {
      #ifdef Parallel
      logTraceInWith4Arguments( "tryToFinishSendOrReceive()", ::toString(Base::_ioMode), Base::size(), Base::_ioRank, Base::_ioTag );

      bool result = true;

      if ( Base::_ioMode==IOMode::MPIReceive ) {
        assertion( _metaDataSizeMPIRequest == nullptr );
        assertion( _metaDataSizeBuffer     != nullptr );

        int metaDataDebugFlag = 1;
        if (_metaDataDebugMPIRequest != nullptr )
          MPI_Test( _metaDataDebugMPIRequest, &metaDataDebugFlag, MPI_STATUS_IGNORE );

        int dataFlag = 1;
        if (Base::_ioMPIRequest != nullptr )
          MPI_Test( Base::_ioMPIRequest, &dataFlag, MPI_STATUS_IGNORE );

        if (metaDataDebugFlag and dataFlag) {
          result = true;

          logDebug( "tryToFinishSendOrReceive()", "receive complete" );

          workInReceivedMetaData();

          if (_metaDataDebugMPIRequest!=nullptr) {
            delete    _metaDataDebugMPIRequest;
            delete[]  _metaDataDebugBuffer;
            _metaDataDebugMPIRequest = nullptr;
            _metaDataDebugBuffer     = nullptr;
          }
          if (Base::_ioMPIRequest!=nullptr) {
            workInReceivedData();

            delete    Base::_ioMPIRequest;
            delete[]  _deepCopyDataBuffer;
            Base::_ioMPIRequest = nullptr;
            _deepCopyDataBuffer = nullptr;
          }

          Base::_ioMode = IOMode::None;
        }
        else {
          result = false;
        }
      }
      else if ( Base::_ioMode==IOMode::MPISend ) {
        assertion( _metaDataSizeMPIRequest!=nullptr );

        int metaDataSizeFlag = 1;
        if (_metaDataSizeMPIRequest != nullptr )
          MPI_Test( _metaDataSizeMPIRequest, &metaDataSizeFlag, MPI_STATUS_IGNORE );

        int metaDataDebugFlag = 1;
        if (_metaDataDebugMPIRequest != nullptr )
          MPI_Test( _metaDataDebugMPIRequest, &metaDataDebugFlag, MPI_STATUS_IGNORE );

        int dataFlag = 1;
        if (Base::_ioMPIRequest != nullptr )
          MPI_Test( Base::_ioMPIRequest, &dataFlag, MPI_STATUS_IGNORE );

        if (metaDataSizeFlag and metaDataDebugFlag and dataFlag) {
          result = true;

          logDebug( "tryToFinishSendOrReceive()", "send complete" );

          if (_metaDataSizeMPIRequest!=nullptr) {
            delete    _metaDataSizeMPIRequest;
            delete[]  _metaDataSizeBuffer;
            _metaDataSizeMPIRequest = nullptr;
            _metaDataSizeBuffer     = nullptr;
          }
          if (_metaDataDebugMPIRequest!=nullptr) {
            delete    _metaDataDebugMPIRequest;
            delete[]  _metaDataDebugBuffer;
            _metaDataDebugMPIRequest = nullptr;
            _metaDataDebugBuffer     = nullptr;
          }
          if (Base::_ioMPIRequest!=nullptr) {
            delete    Base::_ioMPIRequest;
            delete[]  _deepCopyDataBuffer;
            Base::_ioMPIRequest = nullptr;
            _deepCopyDataBuffer = nullptr;
          }

          Base::clear();
          Base::_ioMode = IOMode::None;
        }
        else {
          result = false;
        }
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
