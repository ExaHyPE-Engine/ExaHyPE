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
#include "tarch/multicore/Tasks.h"


namespace peano4 {
  namespace stacks {
    template <class T>
    class STDVectorOverContainerOfPointers;
  } // namespace stacks
} // namespace peano4


/**
 * Stack with vector over pointers aka with varying elements per stack entry
 *
 * I assume that T is in itself a vector over pointers with some additional
 * routines beyond the ones offered by the STL. This class therefore implements a
 * stack as vector over a pimped vector of pointers.
 *
 * The list of additional routines or routines with special semantics that I
 * need from T are
 *
 * - clone() - through this method, it is up to the implementation of T to decide
 *   if you want to work with shallow or deep copies. I strongly recommend deep
 *   copies as per documentation below.
 * - clear() - counterpart of clone(). If you use the standard std::vector's
 *   clear(), you'll end up with a shallow clear, as you simply erase the list
 *   over pointers. Again, up to the implementation to decide what to use, but
 *   some remarks are blow.
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
 * - static LoadStoreComputeFlag loadStoreComputeFlag(const peano4::datamanagement::VertexMarker& marker);
 *
 * The ...Debug... routines are not required in release mode.
 * Finally, the type T has to provide a typedef DoFType which says to which the
 * underlying pointer points to.
 *
 * Views have to be supported by this specialised vector, but I get this for
 * free a I inherit from the other vector class.
 *
 *
 * ## Data ownership (on single tree/subpartition)
 *
 * The vectors don't actually own any data for this particular stack type.
 * The data resides on the heap and the vectors are mere meta data structures
 * referencing the core data.
 *
 * The stack class therefore works with shallow copies by default.
 *
 *
 * ## Intra-node parallelisation
 *
 * The std::vector copy constructor copies the pointers to the particles
 * when we create a new object, while a push on two stacks pushes two
 * pointers to the same stack to the respective containers. Consequently,
 * any particle along the boundary is pushed multiple times: To its actual
 * container, but also to the vectors along the boundary.
 *
 * When we clone a vector along the boundary, it is thus important that this
 * clone manually creates a deep copy. When we merge, merge() takes the
 * deep copy and inserts it into the destination tree (where teh underlying
 * data is marked as ghost and consequently deleted).
 *
 *
 * ## Periodic boundary conditions
 *
 * Same as above. Data is deep copied and then inserted on the other side.
 *
 *
 * ## MPI
 *
 * A vertex or face at a domain boundary pipes its data onto different stacks: the
 * local stack and one stack per communication partner. These communication
 * partner stacks hold shallow copies. So far, nothing is copied really from
 * Peano's point of view. However, the data administered through this outgoing stack
 * not only are a shallow copy, its pointers also point to scattered data.
 *
 * @image html STDVectorOverContainerOfPointers.png
 *
 * The sketch above illustrates this fact:
 *
 * - The actual stack (blue) holds 8 entries. Consequently, also any partner
 *   stack on a neighbouring tree holds that many entries: We always have a
 *   matching number of vertices or faces.
 * - Each entry points to an array (std::vector) in memory. The size of this
 *   array is not known a priori and might be different on a neighbour. These
 *   memory blocks (yellowish) furthermore are scattered in memory and they are
 *   shallow copies, i.e. other stacks might point to them too within the local
 *   spacetree.
 *
 * Taking this memory organisation into account, the send workflow
 * reads as follows:
 *
 * 1. To send out a stack, we first have to deep copy the whole content
 *    into a continuous buffer (green).
 * 2. We send an envelope message out (red). This one is a sequence of 8
 *    entries. Each entry describes how many element in memory have been
 *    copied from the scattered (yellow) data into the continuous (green)
 *    buffer.
 * 3. We send out the envelope first. As the neighbour holds 8 entries on its
 *    stack, too, it knows the size of the envelope.
 * 4. After that, we issue the send of the green data.
 * 5. Eventually, we destroy the deep green copy. Once the sent data has left
 *    the system, there's no need for it anymore.
 *
 * The matching receive workflow is conceptually simple:
 *
 * 1. Get the envelope.
 * 2. We create a continuous buffer for the packed data, and then receive into
 *    this one.
 * 3. Once the receive has terminated, i.e. data has been dumped into our
 *    continuous memory, we "unpack" the data into a vector of
 *    vectors.
 *
 * @see startSend()
 * @see startReceive()
 * @see tryToFinishSendOrReceive()
 *
 *
 * ## Deep copies, data ownership and memory deletes
 *
 * The clear() operation on the subclass is a shallow clear, i.e. it only
 * eliminates the pointers but does not delete any actual data. We expect
 * the actual grid traversal to throw away remote content after the grid
 * sweep. See the comments above.
 *
 *
 *
 * ## General remarks
 *
 * Ensure that the underlying static routines T::send(),
 * T::receiveAndMerge(), T::loadStoreComputeFlag() all
 * return true. Otherwise, no data exchange will be triggered by Peano's trees.
 *
 *
 *
 * @see ParticleSet.template.h for an example of the required signature.
 *
 * @param T Is usually a std::vector over pointers or any class providing a
 *   similar signature.
 *
 * @see peano4::stacks::STDVectorStackOverSmartPointers for a discussion of the
 *  constructor and semantics of ObjectConstruction::NoData.
 */
template <class T>
class peano4::stacks::STDVectorOverContainerOfPointers: public peano4::stacks::STDVectorStack<T> {
private:
  /**
   * Logging device.
   */
  static tarch::logging::Log _log;

  typedef peano4::stacks::STDVectorStack<T> Base;

#ifdef Parallel
  MPI_Request* _metaDataSizeMPIRequest;
  MPI_Request* _metaDataDebugMPIRequest;

  typename T::DoFType* _deepCopyDataBuffer;
  int*                 _metaDataSizeBuffer;
  double*              _metaDataDebugBuffer;

  /**
   * Have to memorize this one for receives.
   */
  peano4::grid::TraversalObserver::SendReceiveContext _context;
  MPI_Comm                                            _communicator;

  /**
   * Take the received data and work them into the current data
   *
   * If we are in debug mode, we receive the meta data and set the meta
   * attributes accordingly.
   *
   * ## Old code versions
   *
   * In previous code generations, I used the size of the received data,
   * which is stored within _metaDataSizeBuffer, and immediately called
   * a resize on the target data structures. While this means that the
   * target data size and the values in _metaDataSizeBuffer hold redundant
   * information, it meant that we avoid frequent reallocations as the
   * target data grows once we actually insert data.
   * The routine solely resized the vectors. It did not create any
   * memory, i.e. the resized vectors stored on the stack held exclusively
   * pointers to garbage once the routine has terminated.
   *
   * In the current, revised version, the particle set does not have to
   * offer a resize() operation. It is not part of @ref toolbox_particles "required signature".
   * @see workInReceivedData() for the next step. Therefore, I had to remove
   * the resize statements of the type
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~
   * const int entries = _metaDataSizeBuffer[i];
   * Base::_data[i].resize(entries);
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * @see workInReceivedData()
   */
  void workInReceivedMetaData();


  /**
   * Take the flattened actual data and unpack it into a proper stack
   *
   * The routine assumes that all entries, i.e. vectors, within the stack
   * aka vector already have been assigned the correct size. This happens
   * through a previous call to workInReceivedMetaData(). In principle, we
   * now run through the flattened input data (which is a mere array) and
   * sort the input one by one into the target stacks.
   *
   * This sorting however requires us to create a deep copy and actually
   * one alloc per received element. The need for a deep copy becomes clear
   * once we recognise that workInReceivedMetaData() has created an array
   * of garbage pointers. They do not point to anything meaningful.
   * Furthermore, tryToFinishSendOrReceive() will eventually free the
   * flattened buffers. So we have deep copy. Furthermore, these deep
   * copies have to happen individual per vector element: After all, the
   * whole exercise of stacks of vectors of pointers is to allow for a
   * lot of flexibility in moving stuff around. So the user might also
   * delete individual entries that a vector points to. If we alloc one
   * huge memory sequence, I don't know whether it would go down well if
   * we deleted only subsegments.
   *
   * ## Implementation remarks
   *
   * I originally used the loop
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   * for (auto p: Base::_data[i]) {
   * }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * but that does not work anymore, as I now hide the particle set
   * implementation. It used to be a list, but now I only expose a very
   * few C++ container routines, and resize and non-const iterators are
   * not among them.
   *
   * @see workInReceivedMetaData()
   * @see tryToFinishSendOrReceive()
   */
  void workInReceivedData();

  /**
   * Prepare receive buffer and trigger non-blocking receive of actual data
   *
   * If you call this routine, you have to ensure that the MPI request
   * _metaDataSizeMPIRequest has terminated successfully. This routine will
   * allocate _deepCopyDataBuffer, so you can check via this flag if the
   * routine has been called before.
   *
   * @see startReceive()
   */
  void triggerNonBlockingDataReceive();

  void prepareDataToSendOut();


  /**
   * @return Total size of all entres
   */
  int prepareMetaDataToSendOut();
#endif

public:
  STDVectorOverContainerOfPointers();


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
   */
  void clone(const STDVectorOverContainerOfPointers<T>& data);


  /**
   * Deep clear
   *
   * Counterpart of clone(). We call clear() per stack entry, which
   * in itself is another vector. So it is up to you to decide if
   * you want to have a shallow or a deep clear().
   */
  void clear();


  /**
   * Start the send process.
   *
   * Please read through the class documentation first.
   *
   * As MPI introduces a distribute memory paradigm, MPI is intrinsically tied
   * to a deep copy data ownership policy.
   *
   * To realise the MPI data exchange, I did originally plan to require the
   * handed in vector datatype to provide some proper MPI datatypes. This did
   * not work and does not help: The data is organised via pointers and those
   * pointers are distributed over the whole memory.
   *
   * Therefore, I introduce the following format:
   *
   * - There's a size meta data buffer which is a sequence of integers. Each
   *   entry stores how big the underlying vector is. I realise the setup
   *   of the meta data in prepareMetaDataToSendOut(). It initialises
   *   _metaDataSizeBuffer properly and returns the size of the output data.
   *   Note that there is still no need to exchange the size of the stack.
   *   The size of two communication partners is always known and the same.
   *   The number of entries per stack entry is not known and has to be
   *   exchanged explicitly.
   * - There's a debug meta data buffer which I befill if and only if we work
   *   with meta data. It is a long sequence of doubles. Per stack entry, I
   *   hold 2*Dimensions data points: Dimensions entries encode the x value of
   *   the debug data, and a further Dimensions entries encode the h value.
   * - The actual data has to be flattened (serialised) too, as I cannot
   *   transfer any pointers. This serialised data is held in yet another
   *   buffer and has to be copied forth and back. Obviously, there's no need
   *   to use this buffer if no data travels in-between MPI boundaries.
   *
   * startSend() ensures that three local buffers are initialised
   * (allocated), befilled and subject to a non-blocking send. It is
   * tryToFinishSendOrReceive() which has to free these data.
   */
  void startSend(peano4::grid::TraversalObserver::SendReceiveContext, int rank, int tag, MPI_Comm comm);


  /**
   * Receive whole stack via MPI
   *
   * We know the number of entries on the stack. They are given by
   * numberOfElements. In line with the class documentation, the receive
   * process is more of less straightforward:
   *
   * 1. Create (resize) the incoming stack. In the introductory example of
   *    the class documentation, it would have 8 entries. Note that we cannot
   *    yet create the real data in the scattered memory, as we don't know
   *    these data yet.
   * 2. Create a receive buffer for the meta data (envelope). Also create an
   *    MPI handle.
   * 3. Allocate data and MPI request object of debug data (if required).
   * 4. Issue non-blocking receive for meta data. This is the envelope, i.e.
   *    the array of integers.
   * 5. Issue a non-blocking receive for the debug data if required.
   *
   * I originally thought I could receive the envelope data blocking. This
   * would have allowed me to set up all receive data immediately and to
   * issue the large non-blocking receive for the actual data. Unfortunately,
   * this will lead to a deadlock: The spacetree set's exchange routine
   * peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks()
   * will first trigger all receives, then all sends, continues with the
   * local data exchange and eventually finalises all send and receives. If
   * there are a lot of receives, a blocking call here could deadlock
   * everything.
   *
   * However, if the receive comes through immediately, then it also makes
   * sense to triggger the non-blocking receive immediately.
   *
   *
   * @see tryToFinishSendOrReceive() for an overview discussion of the receive process.
   * @see workInReceivedData()
   * @see workInReceivedMetaData()
   * @see triggerNonBlockingDataReceive()
   */
  void startReceive(
    peano4::grid::TraversalObserver::SendReceiveContext, int rank, int tag, MPI_Comm comm, int numberOfElements
  );


  /**
   * Finish send or receive if possible and return whether non-blocking data exchange has completed
   *
   * ## Finish a send process
   *
   * To finish a send process, we simply have to delete the three flattened
   * buffers. After that, we call clear() on the stack. This stack had been
   * a shallow copy, i.e. the data resides on a real (persistent) stack for
   * data inside the domain. So a simple clear() does the job.
   *
   * ## Finish a receive process
   *
   * We know that the _metaDataRequest is not a nullptr. This is the only
   * request we know about: the debug data might be a nullptr as we are not
   * in debug mode, and the actual data request might be a nullptr, as there
   * is no data. The completion check thus consists of three steps and
   * encodes some logic:
   *
   * 1. startReceive() will call triggerNonBlockingDataReceive() if the MPI
   *    test says that meta data from the communication partner has arrived
   *    already. Otherwise, it will skip the call. So our first check is if
   *    _metaDataSizeMPIRequest has completed successfully. If this is the
   *    case yet _deepCopyDataBuffer still points to nullptr, we
   *    triggerNonBlockingDataReceive().
   *
   * 2. We next check if _metaDataDebugMPIRequest has completed, though this
   *    one might be a nullptr, as we don't exchange meta data at all.
   *
   * 3. Finally, we check _ioMPIRequest request for completion. Now,
   *    _ioMPIRequest might be nullptr as well, as we haven't received any
   *    meta data yet (see remark on _metaDataSizeMPIRequest and
   *    triggerNonBlockingDataReceive() above).
   *
   * Once we are sure that all three conditions are set (or are actually
   * not required), we can trigger the actual receive process.
   *
   * - workInReceivedMetaData() ensures that both the envelope data and the
   *   debug data are properly set.
   * - Delete meta data and debug data (if used).
   * - Check if any actual data has been received. If so,
   *   - workInReceivedData()
   *   - delete request
   *   - free receive buffer
   *
   *
   * @see startSend()
   * @see startReceive()
   */
  bool tryToFinishSendOrReceive();
};


#include "STDVectorOverContainerOfPointers.cpph"
