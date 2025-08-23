// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <algorithm>
#include <vector>

#include "stacks.h"
#include "STDVectorStack.h"
#include "tarch/Assertions.h"
#include "peano4/grid/GridVertex.h"
#include "peano4/grid/TraversalObserver.h"
#include "peano4/parallel/Node.h"
#include "tarch/logging/Log.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Task.h"

namespace peano4 {
  namespace stacks {
    template <class T>
    class STDVectorStackOverObjectsWithSmartPointers;
  } // namespace stacks
} // namespace peano4

/**
 * Stack over objects, where some of the properties per object are realised through smart pointers
 *
 * This class is similar to the other smart pointer class, but it differs in
 * two ways:
 *
 * 1. It does not assume that there's only a smart pointer. Instead, it
 *    assumes that the manage objects on the stack all are modelled as C++
 *    classes and some of the vectors are realised through
 *    tarch::la::SmartPointerVector.
 * 2. This particular class assumes that each smart pointer's memory region
 *    is known at compile time, i.e. it does not support dynamic size vectors.
 *
 *
 * @param T DaStGen object that hosts a pointer which is backed up by a smart
 *   pointer, i.e. deleted as soon as noone needs it anymore. T has to provide
 *   a clone() operation but it also has to introduce a flattened variant of
 *   its own that we can use for MPI data exchange and copying.
 */
template <class T>
class peano4::stacks::STDVectorStackOverObjectsWithSmartPointers {
private:
  /**
   * Logging device.
   */
  static tarch::logging::Log _log;

  /**
   * This is the attribute holding all the temporary stacks.
   */
  std::vector<T>                 _data;

  using FlattenedContainer = std::vector<typename T::FlattenedType>;

  /**
   * We cannot send out _data, as each element in _data hosts at least one
   * smart pointer.
   */
  FlattenedContainer _flattenedData;

  IOMode _ioMode;
  int    _ioRank;
  int    _ioTag;

#ifdef Parallel
  MPI_Request* _ioMPIRequest;
#endif

public:
  STDVectorStackOverObjectsWithSmartPointers();

  STDVectorStackOverObjectsWithSmartPointers(const STDVectorStackOverObjectsWithSmartPointers<T>&);

  /**
   * One is allowed to copy a stack but it has to be empty.
   */
  STDVectorStackOverObjectsWithSmartPointers<T>& operator=(const STDVectorStackOverObjectsWithSmartPointers<T>& stack) {
    assertion(stack._data.empty());
    assertion(_data.empty());
  }

  /**
   * Clone data into the current object on which clone() is called.
   *
   * This operation overwrites the baseclass version. If we called the
   * plain base class version, we would end up with a shallow copy.
   * However, we want a semi-shallow copy: We replicate the meta data,
   * and then we call the clone() of the actual vector over pointers.
   * It is up to this routine if we craete a shallow copy or not.
   *
   * Read the class documentation why I think that the clone() on the
   * subvectors should create a deep copy. It is the section on
   * intra-node parallelisation.
   *
   * This operation has to use the default constructor of T to resize,
   * as we want the destination data structure to be able to host the
   * target data. So it has to issue all the news if required.
   */
  void clone(const STDVectorStackOverObjectsWithSmartPointers<T>& data);

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
    friend class peano4::stacks::STDVectorStackOverObjectsWithSmartPointers<T>;

    std::vector<T> _data;

    /**
     * Constructor
     */
    PopBlockStackView(int numberOfElements, peano4::stacks::STDVectorStackOverObjectsWithSmartPointers<T>* parentStack) {
      const int remainingElements = parentStack->size() - numberOfElements;

      _data.insert(
        _data.end(),
        std::make_move_iterator(parentStack->_data.begin() + remainingElements),
        std::make_move_iterator(parentStack->_data.end())
      );
      parentStack->_data.erase(parentStack->_data.begin() + remainingElements, parentStack->_data.end());
    }

  public:
    int size() const { return _data.size(); }

    const T& get(int index) const {
      assertion2(index >= 0, index, size());
      assertion2(index < size(), index, size());
      return _data[index];
    }

    T& get(int index) {
      assertion2(index >= 0, index, size());
      assertion2(index < size(), index, size());
      return _data[index];
    }

    std::string toString() const {
      std::ostringstream msg;
      msg << "(size=" << size() << ")";
      return msg.str();
    }
  };

  class PushBlockStackView {
  protected:
    friend class peano4::stacks::STDVectorStackOverObjectsWithSmartPointers<T>;

    peano4::stacks::STDVectorStackOverObjectsWithSmartPointers<T>* _parentStack;

    const int _size;
    const int _baseElement;

    /**
     * Constructor
     *
     * We add a whole set of elements to the container
     */
    PushBlockStackView(int numberOfElements, peano4::stacks::STDVectorStackOverObjectsWithSmartPointers<T>* parentStack):
      _parentStack(parentStack),
      _size(numberOfElements),
      _baseElement(parentStack->_data.size()) {}

  public:
    int size() const { return _size; }

    /**
     * Set an entry
     *
     * But in Peano's context, we only need the move semantics logically,
     * as we only migrate stuff over stacks. Therefore, I tried to play
     * around with move semantics, but this always broke the code. So I
     * thought I'd better stop doing this and use plain old copies here.
     *
     * @return Pointer to element set
     */
    T* set(int index, const T& value) {
      assertion2(index >= 0, index, _size);
      assertion2(index < _size, index, _size);
      _parentStack->_data[_baseElement + index] = value;
      return &(_parentStack->_data[_baseElement + index]);
    }

    const T& get(int index) const {
      assertion2(index >= 0, index, _size);
      assertion2(index < _size, index, _size);
      return _parentStack->_data[_baseElement + index];
    }

    T& get(int index) {
      assertion2(index >= 0, index, _size);
      assertion2(index < _size, index, _size);
      return _parentStack->_data[_baseElement + index];
    }

    std::string toString() const {
      std::ostringstream msg;
      msg << "(size=" << _size << ",baseElement=" << _baseElement << ")";
      return msg.str();
    }
  };

  /**
   * Pops element from a stack.
   *
   * I have to be careful how and why I use std::move here, as we employ
   * smart pointers. I found that a move after the return confuses the
   * reference counting, and eventually leads to memory leaks.
   */
  T pop();

  /**
   * Get top element or shiftth top element. We start to count with
   * 0, i.e. a shift of 0 (default) really returns the top element.
   * A shift of 3 returns the fourth element from the stack
   */
  T& top(int shift = 0);

  /**
   * Get top element or shiftth top element. We start to count with
   * 0, i.e. a shift of 0 (default) really returns the top element.
   * A shift of 3 returns the fourth element from the stack
   */
  const T& top(int shift = 0) const;

  /**
   * Pushes element to a stack.
   */
  void push(const T& element);

  /**
   * This operation grabs numberOfElements from the input stack en block and
   * returns a view to it. Subsequent pops do not affect this block anymore,
   * i.e. the stack is reduced immediately.
   *
   * @param numberOfElements Size of the view
   */
  PopBlockStackView popBlock(int numberOfElements);

  /**
   * Push a block on the output stack
   *
   * @param numberOfElements Size of the view
   */
  PushBlockStackView pushBlock(int numberOfElements);

  int size() const;

  bool empty() const;

  void clear();

  /**
   * Start the actual send process
   *
   * Always pairs up with a tryToFinishSendOrReceive() call.
   *
   * # Gathering the data
   *
   * We know that the data are potentially gathered all over the place on the
   * heap subject to a smart pointer. So we first create a complete copy of all
   * data in a container called _flattenedData. These copies are flattened
   * data, i.e. are stored consecutively in memory.
   *
   * Once we have created this flattened copy, we can delete the original data
   * structure. We won't need it anymore. This is in line with other stacks
   * which clear themselves after they have sent out data. See for example
   * peano4::stacks::STDVectorStackOverSmartPointers<T>::startSend(). This
   * clear() on the original data might not immediately lead to system frees,
   * as other stacks might hold references to the same data on the heap.
   *
   * The flattened data however has to stay there, as we only trigger a
   * non-blocking send, i.e. the data might not have left the system yet.
   * This flattened data are cleaned up within tryToFinishSendOrReceive().
   */
  void startSend(peano4::grid::TraversalObserver::SendReceiveContext context, int rank, int tag, MPI_Comm comm);

  /**
   * Trigger a receive
   *
   * This is the counterpart of startSend() and hence pretty similar.
   * We clear _data and we resize _flattenedData so that latter can take the
   * incoming data. We know that we need that incoming data eventually
   * within _data, but we are not there yet. After that, we issue a
   * non-blocking receive and return.
   *
   * See clone() for a description why we have to use resize() with the
   * standard constructor here.
   *
   * @see startSend()
   */
  void startReceive(
    peano4::grid::TraversalObserver::SendReceiveContext context, int rank, int tag, MPI_Comm comm, int numberOfElements
  );

  /**
   * Finish a send or receive if it is still pending
   *
   * If the stack is not in a sending or receiving state, we return
   * immediately. Otherwise, we issue an MPI_Test to see if MPI tells us
   * if it is done. If so, we can trigger the actual clean-up:
   *
   * - Delete the MPI Request object. We don't need it anymore.
   * - Reset the IO mode to None. We are finished here.
   * - If we have sent data: Clear the flattened data. It has gone out now,
   *   so there's no need for us to hold it anymore.
   * - If we have received data:
   *   - Resize the actual _data array. This is where the incoming data has to
   *     end up eventually.
   *   - Copy the entries from the flattened collection over into _data one by
   *     one. For each entry, we have to unflatten the incoming data.
   *   - Free the flattened data. All is copied over, so we don't need it
   *     anymore.
   *
   */
  bool tryToFinishSendOrReceive();

  /**
   * I need this one to find out whether I'm waiting for data.
   *
   * @return A negative value if we don't send or receive anymore. Otherwise,
   *         we return the rank of the communication partner.
   */
  int sendingOrReceiving() const;

  /**
   * Reversing a stream is something I need extremely rarely. The biggest application
   * is the realisation of joins through
   * peano4::parallel::SpacetreeSet::streamLocalVertexInformationToMasterThroughVerticalStacks(). Here, I need a
   * streamed version of the tree to get the up-to-date data of the mesh in. However, I don't have streams. I have only
   * stacks. So I map the stream idea to a stack.
   */
  void reverse();

  std::string toString() const;
};

#include "STDVectorStackOverObjectsWithSmartPointers.cpph"
