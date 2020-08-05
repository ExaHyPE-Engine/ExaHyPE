// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_STACKS_STDVECTOR_STACK_H_
#define _PEANO4_STACKS_STDVECTOR_STACK_H_


#include <vector>
#include <algorithm>


#include "peano4/parallel/Node.h"


#include "tarch/logging/Log.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Tasks.h"


#include "peano4/grid/GridVertex.h"


namespace peano4 {
  namespace stacks {
    template <class T>
    class STDVectorStack;

    typedef STDVectorStack< peano4::grid::GridVertex >   GridVertexStack;

    enum class IOMode {
      None,
      MPISend,
      MPIReceive
    };
  }
}


std::string toString( peano4::stacks::IOMode mode );


/**
 * 
 */
template <class T>
class peano4::stacks::STDVectorStack {
  private:
    /**
     * Logging device.
     */
    static tarch::logging::Log _log;

    /**
     * This is the attribute holding all the temporary stacks.
     */
    std::vector< T >   _data;

    int                _currentElement;

  protected:
    IOMode              _ioMode;
    int                 _ioRank;
    int                 _ioTag;
    #ifdef Parallel
    MPI_Request*        _ioMPIRequest;
    #endif

  public:
    /**
     * Constructor.
     *
     * There's not really a need to initialise _ioMode and _ioTag, but I do so
     * to ensure that memory checkers don't yield wrong alarms.
     *
     * @see EventStack::EventStack()
     */
    STDVectorStack():
      _data(),
      _currentElement(0),
      _ioMode(IOMode::None),
      _ioRank(-1),
      _ioTag(-1)
      #ifdef Parallel
      ,
      _ioMPIRequest(nullptr)
      #endif
    {}


    ~STDVectorStack() {
    }


    /**
     * One is allowed to clone/copy a stack, but it has to be empty.
     * Usually only when we cut the domain into pieces.
     */
    STDVectorStack<T>( const STDVectorStack<T>& stack ):
      _data(),
      _currentElement(0),
      _ioMode(IOMode::None)
      #ifdef Parallel
	    ,
      _ioMPIRequest(nullptr)
      #endif
    {
      assertionMsg( stack._currentElement==0, "may not copy non-empty stack" );
      #ifdef Parallel
      assertionMsg( stack._ioMPIRequest==nullptr, "may not copy sending/receiving stack" );
      #endif
    }


    /**
     * One is not allowed to copy a stack but it has to be empty.
     */
    STDVectorStack<T>&  operator=( const STDVectorStack<T>& stack ) {
      assertionEquals( stack._currentElement, 0 );
      assertionEquals( _currentElement, 0 );
      assertion( _data.empty() );
    }


    /**
     * Clone data into the current object on which clone() is called.
     */
    void  clone( const STDVectorStack<T>&  data ) {
      assertionEquals1( _currentElement, 0, toString() );
      _data.clear();
      _currentElement = data._currentElement;
      //_data           = data._data;
      _data.resize(_currentElement);
      for (int n = 0; n<_currentElement; n++) {
        _data[n] = data._data[n];
      }
    }


    /**
     * This class represents a whole block of the tree. You can access all
     * element within in random order, or you can pop/push elements. If we
     * grab a block from the tree, it is logically removed from the main stack.
     */
    class PopBlockStackView {
      protected:
        /**
         * Parent is friend
         */
        friend class peano4::stacks::STDVectorStack<T>;

        const int                           _size;
        const int                           _baseElement;
        peano4::stacks::STDVectorStack<T>*  _stack;

        /**
         * Constructor
         */
        PopBlockStackView(int size, int base, peano4::stacks::STDVectorStack<T>* stack):
          _size(size),
          _baseElement(base),
          _stack(stack) {
        }

      public:
        int size() const {
          return _size;
        }

        T get(int index) {
          assertion2( index>=0, index, _size );
          assertion2( index<_size, index, _size );
          return _stack->_data[_baseElement+index];
        }

        std::string toString() const {
          std::ostringstream msg;
          msg << "(size=" << _size
              << ",baseElement=" << _baseElement
              << ")";
          return msg.str();
        }
    };

    class PushBlockStackView {
      protected:
        /**
         * Parent is friend
         */
        friend class peano4::stacks::STDVectorStack<T>;

        /**
         * Constructor
         */
        PushBlockStackView(int size, int base, peano4::stacks::STDVectorStack<T>* stack):
          _size(size),
		  _baseElement(base),
          _stack(stack) {
        }

        const int                           _size;
        const int                           _baseElement;
        peano4::stacks::STDVectorStack<T>*  _stack;
      public:
        int size() const {
          return _size;
        }

        /**
         * @return Pointer to element set
         */
        T* set(int index, const T& value) {
          assertion2( index>=0, index, _size );
          assertion2( index<_size, index, _size );
          _stack->_data[_baseElement+index] = value;
          return &(_stack->_data[_baseElement+index]);
        }

        T& get(int index) {
          assertion2( index>=0, index, _size );
          assertion2( index<_size, index, _size );
          return _stack->_data[_baseElement+index];
        }

        #if PeanoDebug>=1
        T get(int index) const {
          assertion2( index>=0, index, _size );
          assertion2( index<_size, index, _size );
          return _stack->_data[_baseElement+index];
        }
        #endif

        std::string toString() const {
          std::ostringstream msg;
          msg << "(size=" << _size
              << ",baseElement=" << _baseElement
              << ")";
          return msg.str();
        }
    };

    /**
     * Pops element from a stack.
     */
    T pop() {
      assertion1(_currentElement>0,_currentElement);
      _currentElement--;
      return _data[_currentElement];
    }


    /**
     * Get top element or shiftth top element. We start to count with
     * 0, i.e. a shift of 0 (default) really returns the top element.
     * A shift of 3 returns the fourth element from the stack
     */
    T& top(int shift=0) {
      assertion1(shift>=0,shift);
      assertion2(_currentElement>shift, _currentElement, shift);
      return _data[_currentElement-1-shift];
    }

    /**
     * Pushes element to a stack.
     *
     * _currentElement always points to the next free element on the stack.
     */
    void push( const T& element ) {
      assertion( _currentElement <= static_cast<int>(_data.size()) );
      if (_currentElement >= static_cast<int>(_data.size()) ) {
        assertion(_currentElement - static_cast<int>(_data.size()) <= 1 );
        _data.push_back(element);
      }
      else {
        _data[_currentElement] = element;
      }
      _currentElement++;
    }

    /**
     * This operation grabs numberOfElements from the input stack en block and
     * returns a view to it. Subsequent pops do not affect this block anymore,
     * i.e. the stack is reduced immediately.
     *
     * @return Pointer to block. Your are responsible to delete this view afterwards.
     */
    PopBlockStackView  popBlock(int numberOfElements) {
      _currentElement-=numberOfElements;
      assertion( _currentElement>=0 );

      PopBlockStackView result(numberOfElements, _currentElement, this);
      return result;
    }

    /**
     * Push a block on the output stack
     *
     * Pushing a block on the output stack basically means that we move the
     * stack pointer by numberOfElements entries. A block write stems from a
     * regular subgrid, i.e. the corresponding subgrid remains constant, but
     * it might happen that other grid parts processed before have added new
     * vertices. So, we might have to increase the stack size before we open
     * the push view on the stack. Also, the swapping of the stacks might
     * imply that the current output stack is not big enough - the input stack
     * might be, but we are using two distinguished stack data structures.
     *
     * @param numberOfElements Size of the view
     */
    PushBlockStackView  pushBlock(int numberOfElements) {
      PushBlockStackView result(numberOfElements, _currentElement, this);

      _currentElement+=numberOfElements;

      if (static_cast<int>(_data.size())<_currentElement) {
        _data.resize(_currentElement);
      }

      return result;
    }

    int size() const {
      return _currentElement;
    }

    bool empty() const {
      return _currentElement==0;
    }

    void clear() {
      _currentElement = 0;
    }

    /**
     * Always pairs up with a finish... call. When we trigger the send, the
     * code still might insert additional elements, i.e. starting does not
     * mean all the data that is to be sent out is already in the container.
     */
    void startSend(int rank, int tag) {
      #ifdef Parallel
      assertion( _ioMode==IOMode::None );
      assertion( _ioMPIRequest==nullptr );
      _ioMode = IOMode::MPISend;
      _ioTag  = tag;
      _ioRank = rank;

      logDebug( "startSend(int,int,bool)", toString());

      _ioMPIRequest = new MPI_Request;
      int result = MPI_Isend( _data.data(), _currentElement, T::Datatype, _ioRank, _ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), _ioMPIRequest);
      if  (result!=MPI_SUCCESS) {
        logError( "startSend(int,int,bool)", "was not able to send to node " << rank << " on tag " << tag
          << ": " << tarch::mpi::MPIReturnValueToString(result)
        );
      }
      #endif
    }

    /**
     * @see startSend()
     */
    void startReceive(int rank, int tag, int numberOfElements) {
      #ifdef Parallel
      assertion3( _ioMode==IOMode::None, rank, tag, numberOfElements );
      assertion3( numberOfElements>0, rank, tag, numberOfElements );

      _ioMode = IOMode::MPIReceive;
      _ioTag  = tag;
      _ioRank = rank;

      _data.resize(numberOfElements);
      _currentElement = numberOfElements;
      assertionEquals( _data.size(), numberOfElements );

      assertion( _ioMPIRequest == nullptr );
      _ioMPIRequest = new MPI_Request;

      int result = MPI_Irecv( _data.data(), _data.size(), T::Datatype, _ioRank, _ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), _ioMPIRequest);
      if  (result!=MPI_SUCCESS) {
        logError( "startReceive(int,int,int)", "was not able to receive " << numberOfElements << " values from node " << rank << " on tag " << tag
           << ": " << tarch::mpi::MPIReturnValueToString(result)
        );
      }

      logDebug( "startReceive()", toString() );
      #endif
    }


    /**
     * If this routine is invoked on a stack which is neither sending or
     * receiving, then it degenerates to nop. If it is a receiving stack,
     * then the routine waits until all of the data is literally in. If we
     * invoke it on a sending stack, the code checks that the send is
     * complete, i.e. the data is either received or at least somewhere in
     * the network. After that, it releases the underlying stack, i.e.
     * clears it. Once this operation has terminated, the stack's state will
     * be IOMode::None.
     */
    bool tryToFinishSendOrReceive() {
      #ifdef Parallel
      logTraceInWith4Arguments( "tryToFinishSendOrReceive()", ::toString(_ioMode), size(), _ioRank,_ioTag );
      bool result = true;
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
      logTraceOutWith1Argument( "tryToFinishSendOrReceive()", result );
      return result;
      #else
      return true;
      #endif
    }

    /**
     * Reversing a stream is something I need extremely rarely. The biggest application
     * is the realisation of joins through peano4::parallel::SpacetreeSet::streamLocalVertexInformationToMasterThroughVerticalStacks().
     * Here, I need a streamed version of the tree to get the up-to-date data of the mesh
     * in. However, I don't have streams. I have only stacks. So I map the stream idea to
     * a stack.
     */
    void reverse() {
      std::reverse(std::begin(_data), std::end(_data));
    }

    std::string toString() const {
      std::ostringstream msg;
      msg << "(size=" << size()
          << ",current-element=" << _currentElement
          << ",io-mode=" << ::toString( _ioMode )
          << ",rank=" << _ioRank
          << ",tag=" << _ioTag
          << ")";
      return msg.str();
    }
};


template <class T>
tarch::logging::Log peano4::stacks::STDVectorStack<T>::_log( "peano4::stacks::STDVectorStack<T>" );


template <>
void peano4::stacks::STDVectorStack<double>::startSend(int rank, int tag);


template <>
void peano4::stacks::STDVectorStack<double>::startReceive(int rank, int tag, int numberOfElements);


#endif
