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
 * I assume that T is in itself a vector over pointers. Usually, I realise it
 * as a subclass of std::vector, i.e. by inheritance, as I need some additional
 * routines and slightly different semantics to std::vector. You could,
 * obviously, also write a whole new class providing all the functions used
 * here.
 *
 * The list of additional routines or routines with special semantics that I
 * need are
 *
 * - clone()
 * - clear() @todo Still valid?
 * - merge()
 * - void setDebugX( const tarch::la::Vector<Dimensions,double>& data );
 * - void setDebugH( const tarch::la::Vector<Dimensions,double>& data );
 * - tarch::la::Vector<Dimensions,double> getDebugX() const;
 * - tarch::la::Vector<Dimensions,double> getDebugH() const;
 *
 * as well as the static routines
 *
 * - static bool send(const peano4::datamanagement::VertexMarker& marker);
 * - static bool receiveAndMerge(const peano4::datamanagement::VertexMarker& marker);
 * - static bool storePersistently(const peano4::datamanagement::VertexMarker& marker);
 * - static bool loadPersistently(const peano4::datamanagement::VertexMarker& marker);
 *
 * The ...Debug... routines are not required in release mode.
 * Finally, the type T has to provide a typedef DoFType which says to which the
 * underlying pointer points to.
 *
 * ## Data ownership
 *
 * The vectors don't actually own any data for this particular stack type.
 * The data resides on the heap and the vectors are mere meta data structures
 * referencing the core data. It is consequently (usually) not copied. We can
 * however happily copy around the meta data on one core or clear some replica or
 * do whatever we want - the actual data stays at the same location on the heap.
 *
 * The stack class works with shallow copies/data ownership.
 *
 * If you clone() a stack, there's no need to clone non-local particles. So the
 * clone is shallow, but it is typically not trivial. You can amend the clone()
 * in your set implementation, as the vector wrapper itself simply calls your
 * container's clone().
 *
 *
 * ## Shared memory parallelisation
 *
 * In principle, shared memory parallelisation would work out of the box. You
 * could use the std::vector copy constructor which automatically copies the
 * pointers to the particles, i.e. all particles reside in the global heap, and
 * multiple subtrees then host pointers within their stacks to these heap
 * objects.
 *
 * This is not the strategy I recommend within Peano, as it does not mirror the
 * design decisions of the normal stacks, where I work with strictly separated
 * (non-overlapping) data domains, and synchronise all data after each grid
 * sweep. It also naturally makes the code vulnerable to race conditions. Yet,
 * you can still work with such a shallow data model.
 *
 * Before we discuss different realisation variants, we recognise that a vertex
 * at a domain boundary pipes its data onto different stacks: the local stack and
 * one stack per communication partner. These communication partner stacks hold
 * shallow copies. So far, nothing is copied really from Peano's point of view.
 *
 * We further more recommend that user codes delete all remote data before they
 * trigger any communication: They should simply throw away what they've got
 * from somewhere else. Particles, e.g., reside on the tree that holds the cell
 * in which the particle is contained. The particle is sent out per grid sweep
 * to all neighbours. So they can throw away stuff - an updated version will be
 * sent in later anyway.
 *
 *
 * ### Shallow copies, i.e. shared objects
 *
 * When you work with shallow copies, all stack entries are literally pointers
 * to the heap, and multiple trees per rank all point to the same heap objects.
 * It is the user code responsibility to ensure that no data races arise when
 * we access the heap objects.
 *
 * To realise this behaviour, let clone() simply copy over the pointers (so act
 * the same way as the std::vector copy constructor), and delegate clear() to
 * the standard std::vector::clear(). That means, the clear erases the pointers,
 * but not the objects the pointers are pointing to.
 *
 * ### Deep copies
 *
 * The clear() operation remains a shallow clear, i.e. it only eliminates the
 * pointers but does not delete any actual data. We expect the actual grid
 * traversal to throw away remote content after the grid sweep. See the
 * comments above.
 *
 * clone() is, in principle, also relatively simple: Ensure that _debugX and
 * _debugH are cloned, i.e. copied over. Otherwise, all of Peano's assertions
 * will fail. After that, we invoke the set's clone operation. This should now
 * be a deep copy. Again, we might be overambitious with creating new local
 * copies of data which are actually not local at all. We don't care. Such
 * data will be thrown away later.
 *
 *
 * ### General remarks
 *
 * Ensure that the underlying static routines T::send(),
 * T::receiveAndMerge(), T::storePersistently() and T::loadPersistently() all
 * return true. Otherwise, no data exchange will be triggered by Peano's trees.
 *
 *
 * ## MPI parallelisation
 *
 * As MPI introduces a distribute memory paradigm, MPI is intrinsically tied
 * to a deep copy data ownership policy.
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
 * From a data ownership point of view, the startReceive() routine plays the
 * role of clone(), i.e. it creates new local copies. These will, from thereon,
 * be merged into the local data via shallow copies, i.e. by moving around
 * pointers to the deep copies.
 *
 * startSend() also copies data explicitly into its local buffer. We have to
 * delete those local copies in tryToFinishSendOrReceive(). Other than that,
 * nothing is to be done. The outgoing stack had pointers to objects on the
 * heap. We may assume that these objects still are used, and thus can simply
 * delete our own pointers without touching the stuff they point to.
 *
 * @see ParticleSet.template.h for an example of the required signature.
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
     * Clone data into the current object on which clone() is called.
     *
     * Semi-shallow copy. We first create exactly the same number of stack
     * elements as data. After that, we invoke T's clone() per stack entry.
     * This is typically a shallow copy, though you might decide that not
     * all entries are to be copied.
     */
    void  clone( const STDVectorOverVectorOfPointersStack<T>&  data ) {
      assertionEquals1( Base::_currentElement, 0, Base::toString() );

      Base::_data.clear();
      Base::_currentElement = data._currentElement;
      Base::_data.resize(Base::_currentElement);

      logDebug( "clone()", "copy over stack of size " << data.size() );

      for (int stackNumber = 0; stackNumber<Base::_currentElement; stackNumber++) {
        Base::_data[stackNumber].clone( data._data[stackNumber] );
      }
    }


    /**
     * As we only have meta data, i.e. as our vectors just point to memory locations
     * on the heap, we simply call clear on each individual substack, i.e. forget the
     * actual pointers, and then we reset _currentElement such that we start to befill
     * our local stack from scratch again.
     */
    void clear() {
      logDebug( "clear()", "clear " << Base::_currentElement << " vectors on current stack" );
      for (auto& p: Base::_data) {
        p.clear();
      }
      Base::_currentElement = 0;
    }


    /**
     * See class documentation.
     */
    void startSend(int rank, int tag, MPI_Comm comm) {
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
      MPI_Isend( _metaDataSizeBuffer, Base::_currentElement, MPI_INT, Base::_ioRank, Base::_ioTag, comm, _metaDataSizeMPIRequest);
      logDebug( "startSend(int,int)", "sent out size meta data of size " << Base::_currentElement );

      #if PeanoDebug>=1
      _metaDataDebugMPIRequest = new MPI_Request;
      MPI_Isend( _metaDataDebugBuffer, Base::_currentElement*2*Dimensions, MPI_DOUBLE, Base::_ioRank, Base::_ioTag, comm, _metaDataDebugMPIRequest);
      logDebug( "startSend(int,int)", "sent out debug meta data of size " << Base::_currentElement*2*Dimensions );
      #endif

      if (flattenedDataSize>0) {
        _deepCopyDataBuffer = new typename T::DoFType[ flattenedDataSize ];
        prepareDataToSendOut();
        logDebug( "startSend(int,int)", "created deep copy of all " << entry << " entries in the vector of pointers" );

        Base::_ioMPIRequest = new MPI_Request;
        MPI_Isend( _deepCopyDataBuffer, flattenedDataSize, T::DoFType::Datatype, Base::_ioRank, Base::_ioTag, comm, Base::_ioMPIRequest);
      }
      else {
        logDebug( "startSend(int,int)", "there's no actual user data to send out" );
      }
      #endif
    }


    /**
     * See class documentation.
     */
    void startReceive(int rank, int tag, MPI_Comm comm, int numberOfElements) {
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

      MPI_Recv( _metaDataSizeBuffer, numberOfElements, MPI_INT, Base::_ioRank, Base::_ioTag, comm, MPI_STATUS_IGNORE );
      logDebug( "startReceive(int,int,int)", "received all meta data from rank " << rank << " on tag " << tag << " with " << numberOfElements << " entries");

      #if PeanoDebug>=1
      _metaDataDebugMPIRequest = new MPI_Request;
      MPI_Irecv( _metaDataDebugBuffer, numberOfElements*2*Dimensions, MPI_DOUBLE, Base::_ioRank, Base::_ioTag, comm, _metaDataDebugMPIRequest);
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
        MPI_Irecv( _deepCopyDataBuffer, flattenedDataSize, T::DoFType::Datatype, Base::_ioRank, Base::_ioTag, comm, Base::_ioMPIRequest);
        logDebug( "startReceive(int,int,int)", "trigger receive of " << flattenedDataSize << " entries of user data in total" );
      }
      else {
        logDebug( "startReceive(int,int,int)", "there's no actual user data to received" );
      }
      #endif
    }


    /**
     * See class documentation.
     */
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
