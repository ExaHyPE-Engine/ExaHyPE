// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_PARALLEL_SPACETREE_SET_H_
#define _PEANO4_PARALLEL_SPACETREE_SET_H_


#include "tarch/logging/Log.h"
#include "tarch/services/Service.h"
#include "peano4/maps/maps.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "Tasks.h"
#include "TreeManagementMessage.h"


#include <list>
#include <map>
#include <set>


namespace peano4 {
  namespace grid {
    class Spacetree;
    struct GridStatistics;
    class TraversalObserver;
  }

  namespace parallel {
    class SpacetreeSet;
  }
}


/**
 * The spacetree set has to be a singleton, as it is reponsible to accept
 * requests for new trees from remote ranks. So there is only one set.
 *
 * @author Tobias Weinzierl
 */
class peano4::parallel::SpacetreeSet: public tarch::services::Service {
  private:
    friend class peano4::grid::Spacetree;

    static SpacetreeSet  _singleton;

    std::vector<peano4::parallel::TreeManagementMessage>   _unansweredMessages;

    /**
     * @see orderedBarrier()
     */
    std::map< std::string, bool >                          _hasPassedOrderedBarrier;

    /**
     * Each task triggers the traversal of one specific spacetree. After
     * that, we might directly trigger the data exchanges. Yet, this is not a
     * good idea as other tasks might linger in the background not have sent
     * the data out yet. So we don't to anything here.
     */
    class TraverseTask: public tarch::multicore::Task {
      private:
        peano4::grid::Spacetree&          _spacetree;
        SpacetreeSet&                     _spacetreeSet;
        peano4::grid::TraversalObserver&  _observer;
        const bool                        _invertTreeTraversalDirectionBeforeWeStart;
      public:
        TraverseTask( peano4::grid::Spacetree&  tree, SpacetreeSet& set, peano4::grid::TraversalObserver&  observer, bool invertTreeTraversalDirectionBeforeWeStart );

  	    /**
         * I create the copy of the observer, run the traversal on my local
         * tree _spacetree and finally destroy the local observer copy.
         */
        bool run() override;
	      void prefetch() override;
    };

  public:
    /**
     * This is the kind of routine that does boundary data exchange, i.e.
     * send outs in iteration n and receives (see counterpart routine), but
     * the received data is used only in traversal n+1. The whole exchange
     * thus asynchronous (logically).
     *
     * Typically, this routine is called by user observers to for their
     * user-defined data. We however also use it within the grid to trigger
     * the actual vertex data exchange. As these routines all are invoked
     * via tasks, this routine is typically invoked by multiple threads
     * concurrently.
     *
     * @param symmetricDataCardinality If two ranks A and B are adjacent to
     *   each other, and if A send n entries to B, then B received exactly
     *   n entries in turn.
     *
     * @see finishAllOutstandingSendsAndReceives();
     */
    template <class Container>
    static void exchangeAllHorizontalDataExchangeStacks( Container& stackContainer, int spacetreeId, bool symmetricDataCardinality );

    template <class Container>
    static void exchangeAllPeriodicBoundaryDataStacks( Container& stackContainer, int spacetreeId );

    /**
     * <h2> Prepare new children after fork </h2>
     *
     * When we prepare new children, we have to map output stack on output stack, as both
     * trees are not yet inverted at the time we invoke this routine. That is, the master's
     * output stack is mapped on to the new child's outpu stack. The master is inverted
     * automatically later on (that's the last thing we do throughout the traversal). For
     * the new child, we have to invert manually however. This is done in
     * exchangeDataBetweenExistingAndNewTreesAndRerunNewTrees().
     *
     * @param childrenIds Depending on the context, this is either the children or
     *   the new children that are just about to be kicked off
     */
    template <class Container>
    static void exchangeAllVerticalDataExchangeStacks(
      Container& stackContainer,
      int spacetreeId, int parentId
    );

    template <class Container>
    static void deleteAllStacks(
      Container& stackContainer,
      int spacetreeId
    );

    /**
     * This routine finishes all the sends and receives that are still active,
     * i.e. it searched for pending MPI requests and waits for them to finish.
     * After this is ensured, the routine runs over all stacks and ensures that
     * all temporary data is released. The last step is important, as we otherwise
     * quickly run out of memory - we replicate all data structures whenever we fork
     * and as C++ vectors don't release memory, this memory would be lost without
     * a manual freeing.
     */
    template <class Container>
    static void finishAllOutstandingSendsAndReceives( Container& stackContainer, int spacetreeId );

    /**
     * Copies (streams) data from the master to the worker.
     *
     * These are complete copies of the data set as we know that both trees afterwards
     * will coarsen their mesh and thus free resources.
     *
     * Invoked by user code from streamDataFromSplittingTreeToNewTree() and by
     * spacetree set through SpacetreeSet::streamDataFromSplittingTreesToNewTrees().
     * SpacetreeSet runs over the set of trees with the label EmptyRun, i.e. those
     * that haven't done any iteration yet. It then invokes this routine (indirectly)
     * on the master. That is, even if you are on a rank where the master does not exist,
     * the code will temporarily create an observer for the master and then ask this
     * observer to trigger the data exchange.
     *
     * We have to trigger the routine multiple times, to ensure we catch both the
     * on-rank and the between-rank case. As a consequence, we may only stream if the
     * destination stack is still empty.
     *
     * <h2> On-rank realisation </h2>
     *
     * If source and destination rank are the same, a tree splits up into two trees
     * both residing on the same MPI rank. We therefore simply copy the stream. As
     * this routine is invoked on the master, it is the master that creates the
     * stream on the worker and befills it.
     *
     * <h2> Data exchange between different ranks </h2>
     *
     * If the master rank is the local guy, then we have to trigger a send. Otherwise,
     * we trigger a receive. The message exchange consists of two phases. An integer
     * message first is exchanged. It carries the number of messages. After that, I
     * send out the actual data.
     *
     * We don't have to finish any sends, i.e. wait for Isends or Irecvs. SpacetreeSet
     * will call finishAllOutstandingSendsAndReceives() later on.
     *
     * The routine is idempotent on a single rank, i.e. you can call it multiple times.
     * Only the first one will copy, all the others will become nop. It is not idempotent
     * in a parallel sense. It has to be idempotent, as I indeed have to call it twice
     * in a distributed memory environment: I have to call it on the receiver side and
     * on the sender side.
     */
    template <class Container>
    static void streamDataFromSplittingTreeToNewTree( Container& stackContainer, int master, int worker );

    template <class Container>
    static void streamDataFromJoiningTreeToMasterTree( Container& stackContainer, int master, int worker );

  private:
    /**
     * Logging device.
     */
    static tarch::logging::Log _log;

    /**
     * Semaphore to protect container holding all the local trees.
     */
    static tarch::multicore::BooleanSemaphore                      _semaphore;

    enum class SpacetreeSetState {
      Waiting,
      TraverseTreesAndExchangeData
    };

    static std::string toString( SpacetreeSetState state );

    /**
     * I use this tag to identify messages send from one tree to another rank.
     * All answers go through an answer tag. To identify the right one, please
     * use getAnswerTag(). The tag should be const, but I set it in init() and
     * therefore I had to remove the const - even though its logically
     * not possible to change it.
     */
    int     _requestMessageTag;

    /**
     * Never use this tag directly. It is the first tag of a series fo answer
     * tags. To find the right one for a particular application context, use
     * getAnswerTag().
     */
    int     _answerMessageTag;

    /**
     * These are the local spacetrees.
     */
    std::list< peano4::grid::Spacetree >  _spacetrees;

    /**
     * The state identifies what the set is doing right now. The flag is for example
     * used by receiveDanglingMessages() as we may not create new trees or change
     * tree states while we are running through them or realise their data exchange.
     */
    SpacetreeSetState                     _state;

    /**
     * I create/clone one observer per local tree. This is an
     * on-the-fly process. At the end of the set traversal, we delete all of
     * clones. Originally, I did this delete right after the creation and the
     * subsequent traversal. However, we need the observer clone once more for
     * the data exchange. To avoid reclones, I store all clones in this map and
     * then delete them en bloc.
     *
     * @see deleteClonedObservers
     */
    std::map< int, peano4::grid::TraversalObserver* >    _clonedObserver;

    peano4::grid::Spacetree& getSpacetree(int id);

    const peano4::grid::Spacetree& getSpacetree(int id) const;

    /**
     * @return tag that one should use to answer one particular spacetree
     */
    int getAnswerTag( int targetSpacetreeId ) const;

    /**
     * <h2> Multithreading </h2>
     *
     * I originally intended to make this routine use tasks. However, the data
     * transfer routines do modify the underlying map/stack structures of Peano,
     * as new containers are created, data is moved around, and so forth. So the
     * data exchange per se is not thread-safe. As we do not/can not use threads,
     * we have to be careful with the order. I originally had a loop over the
     * trees that triggered per tree the data exchange and then waited for MPI.
     * Obviously, this does not work with rendezvous protocol. We need two loops.
     * The first one triggers all the MPI sends/receives and it also realises the
     * local data exchange. The second one waits for all MPI operations to
     * terminate.
     */
    void exchangeHorizontalDataBetweenTrees(peano4::grid::TraversalObserver&  observer);
    void exchangeVerticalDataBetweenTrees(peano4::grid::TraversalObserver&  observer);

    /**
     * I do this after a join/after I've removed an empty tree. Have to call it
     * explicitly, as a join does not delete/throw away the data. It simply hands
     * on data ownership.
     */
    void deleteAllStacks( peano4::grid::TraversalObserver&  observer, int spacetreeId );

    /**
     * Copy the data from a splitting tree onto its new workers
     *
     * When we split a tree, we realise this split in two grid sweeps where the second
     * sweep breaks up the traversal into three logical substeps. In the first sweep, the
     * splitting master tells everybody around that it will split. No split is done though.
     * In the second sweep, the master still takes all data that's meant to be for the new
     * worker, and it sends out boundary data where it shares a boundary with the worker.
     *
     * After that, it copies its tree data over to the new worker. This is what
     * this routine is for. Copying means that it is there in the right order, but it
     * also means that the new worker hasn't had time to send out its boundary data in
     * return for the stuff it will receive in the next iteration. Therefore, the new
     * worker will do two additional grid traversal after this copying has terminated.
     *
     * Streaming/copying here means two different things: We treat user and grid (vertex)
     * data differently.
     *
     *
     * <h2> Vertex grid data </h2>
     *
     * The copying of the whole tree data is literally a copying of the output stack of
     * the master. It happens after the master has finished its joining traversal.
     * peano4::grid::Spacetree::traverse() does invert the traversal direction in an
     * epilogue automatically. Therefore, by the time we hit this routine, we have to
     * copy over the input stack.
     *
     * We always have to invoke the data exchange for both the master and the worker, i.e.
     * we call it twice. This way, we invoke the data exchange on the destination rank
     * (MPI receive) and on the source rank (MPI send). For a single-node split, the
     * second invocation degenerates to nop automatically. See streamDataFromSplittingTreeToNewTree()
     * which implements a simple emptyness check.
     *
     * <h2> User data </h2>
     *
     * While it makes sense to copy the whole vertex data, i.e. the whole tree structure,
     * to the new worker, this approach is not good for the new worker. Coyping the
     * whole tree means the worker knows all the stuff about connectivity et al and then
     * successively can coarsen those mesh parts that it si not interested in. When I
     * had a first prototype up that also copied all user data, I alway ran out of
     * memory. Therefore, user data is copied only for local elements. We may assume
     * that the code has already copied all outgoing data into the right stack. We "just"
     * have to copy this very stack over.
     *
     * See the routine peano4::grid::Spacetree::sendUserData() for an explanation how
     * user data is effectively streamed out.
     *
     *
     * @see streamDataFromSplittingTreeToNewTree()
     */
    void streamDataFromSplittingTreesToNewTrees(peano4::grid::TraversalObserver&  observer);

    /**
     * This operation should be called pretty close towards the end of a traversal.
     * I recommend to invoke it after cleanUpTrees(). It creates new trees and thus
     * might invoke remote method calls on other ranks. It is thus important that
     * all the data exchange locally is done - otherwise we might ask another rank
     * to create a tree for us, while the other rank still waits for boundary data
     * from our side.
     *
     * @see exchangeDataBetweenTrees() for details.
     */
    void createNewTrees();

    /**
     * @see _clonedObserver
     */
    void deleteClonedObservers();

    /**
     * Adds a new spacetree to the set. The responsibility goes over to the
     * set. The operation clones the original spacetree handed in into a new
     * spacetree with the id newTreeId.
     *
     * <h2> Local node </h2>
     *
     * If the new tree will be a local tree, we simply add a new tree object.
     * The Spacetree's constructor takes care of all the "cloning". Clone here
     * means a setup. We traverse this tree afterwards separately to stream all
     * data in.
     *
     * <h2> Distributed memory <h2>
     *
     * In a distributed memory environment, we have to break up the creation
     * into a set of sends forth and back through TreeManagementMessages. This
     * routine is the one that sends out the requests.
     *
     * <h2> Multithreading </h2>
     *
     * I may not globally lock this routine, as I otherwise would block the
     * createObserverCloneIfRequired(). So I literally "only" protect the
     * actual push back in the vector.
     *
     * <h2> Call sequence </h2>
     *
     * The operation is used by createNewTrees() .
     */
    void addSpacetree( int masterId, int newTreeId );

    /**
     * Whenever we join two partitions, we have to stream data from the worker
     * to the master. In principle, I could omit this, as I realise a ``only
     * degenerated trees may join'' policy. However, it can happen that a tree
     * joins while other trees split or have split or join as well. While we
     * run through the joining phase, these other ranks have sent their updated
     * info to the worker that is about to join. The master that will hold the
     * data in the future is not aware (yet) of any topology changes. Therefore,
     * it is important that the worker still merges all incoming information
     * into its local tree and then forwards it (streams it) to its master. The
     * master in turn has to traverse later (in a secondary tree traversal) and
     * take this updated topological information into account. This is a stream
     * operation. Therefore, we not only have to exchange stacks, we also have
     * to revert them to transform the stack into a stream.
     *
     * All of this painful stuff is done for the vertex data only. For the user
     * data, we can rely on our traditional vertical data exchange mechanisms.
     *
     * @see peano4::stacks::STDVectorStack::revert()
     */
    void streamLocalVertexInformationToMasterThroughVerticalStacks(
      int spacetreeId, int parentId,
      const std::set<int>& joiningIds
    );

    /**
     * <h2> Merge process </h2>
     *
     * We make the worker first of all decide whether to merge or not. A
     * worker has the control/knowledge whether to join or not. When a rank
     * wants to join, we first of all run the workers. Then, all data is the
     * join buffers. After that, we sweep through the masters to take up the
     * data. So we may not merge both the worker and the master into their
     * workers at the same time. If we did so, we'd not be able to run all
     * the merging trees in parallel.
     */
    void cleanUpTrees(peano4::grid::TraversalObserver&  observer);

    /**
     * I need this routine for technical reasons: Prior to the sweep of trees,
     * I have to identify all of those trees which wanna merge with their
     * workers. This is an analysis I have to do before I actually traverse
     * any worker. Because of this traversal, more trees might denote their
     * workers as joining, so if I query the joining field after the traversal,
     * I intermix newly joining and old joining ranks.
     */
    std::set<int>                 getLocalTreesMergingWithWorkers() const;

    /**
     * Quick lookup whether an observer clone for this tree id does already exist.
     * If not, we create one quickly.
     *
     * \section Multithreading
     *
     * This operation uses a lock on the semaphore to ensure that no two threads
     * insert an observer into the global table at the same time.
     */
    void createObserverCloneIfRequired(peano4::grid::TraversalObserver& observer, int treeId);

    SpacetreeSet();
    SpacetreeSet(const SpacetreeSet& ) = delete;
    SpacetreeSet& operator=(const SpacetreeSet& ) = delete;

  public:
    /**
     * As the setis a singleton and a service, it has to deregister itself. Otherwise, the overall
     * service landscape still might try to call receiveDanglingMessages().
     */
    ~SpacetreeSet();

    static SpacetreeSet& getInstance();


    /**
     * Run through the set of unanswered questions and, well, answer them.
     *
     * The following messages are received/handled:
     *
     * - Action::RequestNewRemoteTree (invoked by split())
     * - Action::CreateNewRemoteTree (invoked by addSpacetree(int,int))
     * - Action::RemoveChildTreeFromBooksAsChildBecameEmpty (invoked by cleanUpTrees())
     *
     * split() is something any rank can trigger at any time. Most default load
     * balancers call it throughout the end of the grid sweep. addSpacetree() is
     * called by the set through createNewTrees() just before all observers are
     * deleted and the traversal terminates. cleanUpTrees() just arises before
     * that one.
     *
     * No all of these messages can be answered at any point of the local grid
     * sweeps. That is, we may never add a tree to the local tree collection while
     * we are right in the middle of traversals on this rank, e.g.
     * We may not insert or
     * remove trees from the global data structure while we are still traversing
     * some local trees or run some data exchange. I originally tried to protect
     * the whole code by a _state==SpacetreeSetState::Waiting check, i.e. to make
     * the set react if and only if we are not doing anything anyway. That did
     * not work properly, as tree requests for example are to be handled
     * immediately. So what I do now is that I pipe all requests into a vector.
     * Then, I run through the vector and, depending on the set state, do answer
     * messages or leave them in the queue for the time being.
     *
     * This approach leads to errors whenever a message send-out is followed by
     * a second message that provides further details. The first message might be
     * buffered locally, and, as we can't answer the first one immediately, the
     * second message (of another datatype) will be interpreted as yet another
     * request. So that means that every single message exchange with the set
     * always has to be follow a send-acknowledge pattern.
     *
     * <h2> Action::RequestNewRemoteTree </h2>
     *
     * Can be answered any time, as it literally just books a new number but
     * nothing more happens at this point. The acknowledgement message carries
     * the new tree's number.
     *
     * <h2> Action::CreateNewRemoteTree </h2>
     *
     * This message triggers the insertation of a new tree into the local set of
     * spacetrees. We thus may handle it if and only if we are the end of a
     * traversal. The message logically consists of two parts: The master of a
     * new tree (which has previously used Action::RequestNewRemoteTree to get a
     * new tree's number) sends out the create message, waits for the
     * acknowledgement and then sends out the tree state, to the rank hosting the
     * new tree can actually create the data structure. This last step is followed
     * by an acknowledge message which carries no particular information. That
     * is, this message exchange belongs to the one-way information flow, but we
     * have one acknowledgement message to say "go ahead", and one message to
     * say "ok, we got it".
     *
     * <h2> Action::RemoveChildTreeFromBooksAsChildBecameEmpty </h2>
     *
     * This is a simple one though we have again to ensure that it is handled if
     * and only if we have finishes the local traversals and thus can safely
     * manipulate the local spacetree.
     *
     * <h2> Data consistency </h2>
     *
     * In this routine, we have to be very careful with the data consistency. While
     * we are running through the set of unanswered messages, new ones might drop
     * in. However, C++ is not very happy if a vector is added further elements
     * while we are traversing it (in theory, it might happen that it is expanded
     * and thus copied). So what I do is something different: I first run through
     * the vector and take those out that I know that I can handle. Then I handle
     * these guys and return - well-aware that meanwhile further messages might
     * have dropped in. Which is something I don't care, as I rely on the calling
     * code just to invoke answer again if it is important for the code's progress.
     *
     *
     * @todo replyToUnansweredMessages() sollte der Name sein
     */
    void answerQuestions();

    /**
     * We poll the tree management messages.
     *
     * Messages are not answered directly. Instead, we buffer them in
     * _unansweredMessages and hand over to replyToUnansweredMessages().
     */
    void receiveDanglingMessages() override;

    /**
     * @see Spacetree::Spacetree()
     * @see shutdown()
     */
    void init(
      const tarch::la::Vector<Dimensions,double>&  offset,
      const tarch::la::Vector<Dimensions,double>&  width,
      const std::bitset<Dimensions>&               periodicBC = 0
    );

    void shutdown() override;

    /**
     * Invoke traverse on all spacetrees in parallel.
     *
     * <h2> Sequence </h2>
     *
     * It is important that I don't answer to spacetree request messages while I am
     * running through the spacetree set. Some of these request messages add further
     * elements to the set. However, the set should remain invariant while I run
     * through it. So I introduced the _state. See also _unansweredMessages.
     *
     * So when I'm done with the local traversal, I have to ensure that all other sets
     * have got their tree modifications through. That is: If another rank wants to
     * insert a tree, that has to happen in the right traversal. As the other rank
     * will wait for an acknowledgement of the addition, there is no risk that this
     * rank has already proceeded wrongly into the next grid sweep. However, the target
     * rank has to answer the request in the right one and may not proceed to early.
     *
     * So I introduce a barrier. The barrier has to do two things: On the one hand, it
     * has to receive dangling messages. On the other hand, it should answer messages
     * that it has not answered before. In principle, this is indirectly done through
     * receiveDanglingMessages(). However, I played around with running the dangling
     * thing if and only if iprobe tells me that there are new messages. So to be on
     * the safe side, I rather invoke the answer routines manually here.
     *
     * There are two options where to place the barrier: We could add it to the end of
     * traverse() or right after we've received the startup message. There are two different
     * kinds of messages: messages that can be answered straightaway (give me  a new
     * rank) or others which can be answered only in-between iterations. If we add
     * a barrier right at the begin of a traversal, then we are fine, as the typical
     * split (asking for further ranks) arises in-between traversals.
     */
    void traverse(peano4::grid::TraversalObserver& observer);

    /**
     * Return statistics object for primary spacetree.
     */
    peano4::grid::GridStatistics  getGridStatistics() const;

    peano4::grid::GridStatistics  getGridStatistics(int treeId) const;

    /**
     * Split a local tree.
     *
     * If the target tree shall be stored on the local node, then you pass
     * <pre>
      peano4::parallel::Node::getInstance().getRank(treeId)
       </pre>
     * as last argument. Splits on the local node allow Peano to exploit more
     * cores on the node.
     *
     * <h2> Split process </h2>
     *
     * @image html SpacetreeSet_split.png
     *
     * If you split a tree, the original tree, i.e. the source of the data,
     * makes the new tree run through two states: split triggered and splitting.
     *
     * In a usual run, Peano's trees do exchange data in a way similar to the
     * Jacobi smoothing: Each tree traverses its domain and sends out all
     * boundary data. I call this horizontal communication as it is data exchange
     * between non-overlapping domains which can be arranged on one grid level.
     * In the next iteration, the data from neighbours has arrived and can be
     * merged in. We obtain a criss-cross communication pattern (blue steps)
     * where an iteration feeds into the neighbour's iteration n+1.
     *
     * While a new id is booked by one tree if a split is triggered, this new tree
     * does not yet physically exist. The original (source) tree keeps the
     * complete ownership of all data, i.e. it does all refines and coarses and
     * also creates all events. However, it already enters the new, yet to be
     * created, tree's indices into all adjacency lists with are consequently
     * sent out to the other ranks. This is the yellow step above. After the grid
     * sweep, we mark the new tree id as splitting. At this point, all neighbours
     * continue to send to the original tree, as they are not yet aware of any
     * domain partition updates.
     *
     * While an id is associated with splitting (orange), we know that all the other
     * trees around start to send data to the new tree that is yet to be
     * created: they receive the information about the updated adjacency and can
     * send out their stuff to the new tree. Which does not exist yet (grey).
     *
     * So the splitting rank (orange) traverses its domain and invokes all
     * events. It has the semantic ownership. It merges in all boundary data from
     * neighbours, but is does not send out the boundary data (anymore).
     * After the splitting tree traversal has terminated, we establish
     * the new tree by duplicating all the local data. This includes the
     * user data, too.
     *
     * The new tree (light green) subsequently is traversed once as empty
     * tree. That is, all data travel over the stacks, but no events are invoked
     * and no communication is happening. The purpose of this empty traversal is
     * to get all data inot the right order.
     *
     * Once the empty traversal has terminated, we run over the new tree
     * again. This time, we label it as new (dark green). While we still do not
     * invoke any event, we now do send out data. This is the sole purpose of the
     * new traversal.
     *
     * It is not guaranteed that the splits are all successful. See the routine
     * peano4::grid::Spacetree::isCellSplitCandidate() which identifies cells
     * that can be split. Effectively, you might call split and not get any
     * tree decomposition at all.
     *
     * @see peano4::grid::Spacetree::split() This is the routine to tell a
     *   particular spacetree to trigger a split in the next grid sweep.
     *
     *
     * @param treeId  Id of tree that should split, i.e. give away cells to a
     *   new tree
     * @param cells   Number of cells to deploy to a new tree. This count refers
     *   to the status-quo, i.e. if dynamic adaptivity makes an unrefined node
     *   of the tree unfold in the next step, the new @f$ 3^d @f$ children do
     *   count only with one towards this quota.
     * @param targetRank  Rank which should host the new tree.
     */
    bool split(int treeId, int cells, int targetRank);

    /**
     * Codes hold one spacetree set per rank. With this routine, you can find
     * out whether a local set contains a particular id.
     */
    bool isLocalSpacetree(int treeId) const;

    /**
     * @return Set of ids of local spacetrees
     */
    std::set<int> getLocalSpacetrees() const;

    /**
     * Try to order the thread execution to some degree.
     *
     * This routine is used by some plotters and other routines. It basically works
     * as follows:
     *
     * - The first thread that hits the barrier in a set traversal synchronises with
     *   all other ranks. All other local threads have to wait meanwhile.
     * - After that, the other ranks are allowed to pass one by one.
     */
    void orderedBarrier( const std::string& identifier );
};


#include "SpacetreeSet.cpph"


#endif
