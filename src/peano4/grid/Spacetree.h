// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AutomatonState.h"
#include "GridVertex.h"
#include "GridStatistics.h"
#include "GridControlEvent.h"
#include "GridTraversalEventGenerator.h"

#include "tarch/logging/Log.h"

#include "peano4/stacks/STDVectorStack.h"

#include "peano4/maps/STDStackMap.h"
#include "peano4/maps/HierarchicalStackMap.h"

#include <set>
#include <bitset>

namespace peano4 {
  namespace grid {
    class Spacetree;
    class TraversalObserver;
    struct GridTraversalEvent;

    namespace tests {
      class SpacetreeTest;
    }
  }

  namespace parallel {
    class SpacetreeSet;
  }
}

/**
 * Represents one tree
 */
class peano4::grid::Spacetree {
  public:
    /**
     * Periodic boundary conditions are technically realised as domain
     * decomposition, i.e. the vertices along the periodic boundaries carry
     * rank numbers which host RankOfPeriodicBoundaryCondition. So this is
     * the rank number dedicated to periodic BCs. As we treat the BCs as
     * particular neighbour ranks, all data along the periodic BCs is
     * automatically piped onto specialised streams. This holds for both
     * user and grid data.
     *
     * @see receiveAndMergeUserData()
     * @see sendUserData()
     */
    static constexpr int RankOfPeriodicBoundaryCondition = -2;
    static constexpr int NumberOfStationarySweepsToWaitAtLeastTillJoin = 2;

  private:
    static tarch::logging::Log  _log;

    friend class peano4::parallel::SpacetreeSet;
    friend class peano4::grid::tests::SpacetreeTest;

    /**
     * Can a cell be split (deployed to another rank)
     *
     * Not all cells are a fit and can be deployed to another rank even
     * though the spacetree set wants to split a tree. Please read through
     * @ref peano_domain_decomposition "Peano's domain decomposition" discussion
     * and the two splitting constraints therein to understand why this
     * routine may or may not label a cell as splitable.
     *
     * Cells that disqualify for splitting are
     *
     * - Cells which a adjacent to a periodic boundary conditions. Such cells
     *   (including their adjacent vertices) all are kept on tree 0 so all
     *   periodic boundary data exchange happens on tree 0 only.
     * - Non-local cells (obviously)
     * - Local cells whose master is not local. If we'd move such cells to
     *   another rank, we'd destroy the logical tree topology.
     * - Cells that are adjacent of hanging nodes, while the parent cell is
     *   not splitable.
     */
    bool isCellSplitCandidate(
      GridVertex                         coarseGridVertices[TwoPowerD],
      GridVertex                         fineGridVertices[TwoPowerD]
    ) const;

    int              _id;

    SpacetreeState   _spacetreeState;

    /**
     * The root of a spacetree corresponds to the initial state of the tree
     * traversal automaton. So we reuse the object here. It is basically the
     * bounding box.
     */
    AutomatonState   _root;

    GridStatistics   _statistics;

    /**
     * This is not a static master. There's only a real master-worker relation
     * built into this part of the code when we actually split or join.
     *
     * This field should be const (it never changes), but in the MPI case, I
     * wanna be able to construct a tree first and then to set the master.
     * Actually, I should introduce a special constructor for this.
     */
    int            _masterId;
    std::set<int>  _childrenIds;

    /**
     * Indicate per axis whether we have periodic boundary conditions.
     */
    const std::bitset<Dimensions>      _periodicBC;

    /**
     * A split is identified by a tuple of id and cell count which tells the
     * code how many cells should go to a particular id. The actual split then
     * is done in a second iteration, i.e. we first bookmark all split requests
     * and then we roll over the requests in the subsequent iteration to be
     * actually performed.
     */
    SplitSpecification   _splitTriggered;

    std::set<int>        _splitting;

    GridTraversalEventGenerator _gridTraversalEventGenerator;

    /**
     * I need this post-mortem list to identify which tree structures have to be
     * copied/replicated where.
     */
    std::set<int>        _hasSplit;


    constexpr static int NoJoin = -1;
    /**
     * This should be -1 if no join is triggered. Otherwise it holds the id of
     * the joining rank.
     */
    std::set< int >      _joinTriggered;

    /**
     * If master: Set of workers that should join
     */
    std::set< int >      _joining;

    //typedef peano4::maps::STDStackMap< peano4::stacks::GridVertexStack >   GridVertexStackMap;
    typedef peano4::maps::HierarchicalStackMap< peano4::stacks::GridVertexStack >   GridVertexStackMap;

    static GridVertexStackMap  _vertexStack;

    /**
     * We get these control events when we kick off the traversal and then
     * realise/interpret them.
     */
    std::vector< peano4::grid::GridControlEvent >       _gridControlEvents;

    /**
     * Should only be called for inner cells
     *
     * We run over all the grid refinement/erase events and first check if
     * there are overlaps (refinement) or if the state is completely
     * embedded within the current cell.
     *
     * If there's an overlap with a refinement event, then we refine all
     * adjacent vertices which are not yet refined.
     *
     * If there's an erase event that's relevant, then we erase all
     * adjacent vertices which are refined. To avoid that we have
     * oscillations, I make the cell twice as big along each coordinate
     * axis.
     */
    void evaluateGridControlEvents(
      const AutomatonState& fineGridState,
      GridVertex            coarseGridVertices[TwoPowerD],
      GridVertex            fineGridVertices[TwoPowerD]
    );

    /**
     * setIsAntecessorOfRefinedVertexInCurrentTreeSweep()
     *
     * If a cell is given away to another rank, we have to mark the vertices
     * of its mother cell such that we do not coarsen above it and thus
     * effectively remove our whole splitted subpartition. We do this by two
     * means: First, we set the marker
     * setIsAntecessorOfRefinedVertexInCurrentTreeSweep(). We erase at most
     * one level at a time in Peano. Therefore, this flag effectively vetoes
     * any erase call. This flag ensures that updateVertexAfterLoad() in the
     * next grid sweep will erase the mesh.
     *
     * We do so on both the master and the worker. On the master cell, i.e.
     * the one that forks another cell off, it is clear that we have to mark
     * the vertices, as the vertex information ("hey, I have forked off a finer
     * grid cell and you therefore shall not erase the mesh here") has to
     * propagate immediately. We also set it on the worker, as its vertices
     * will be subject to boundary data exchange. This way, we inform
     * neighbours about the topology, too.
     */
    void markVerticesAroundForkedCell(
      GridVertex            coarseGridVertices[TwoPowerD],
      GridVertex            fineGridVertices[TwoPowerD]
    ) const;


    /**
     * Returns if a vertex is local to adjacent tree.
     *
     * In the definition here, hanging vertices are never adjacent to the local
     * tree. This is different to the definition in the event generator, where
     * hanging nodes are taken into account.
     *
     * @see GridTraversalEventGenerator::isVertexAdjacentToLocalSpacetree() for a
     *   slightly different notion of locality.
     */
    bool isVertexAdjacentToLocalSpacetree(
      GridVertex  vertex,
      bool        splittingIsConsideredLocal,
      bool        joiningIsConsideredLocal
    ) const;


    /**
     * You may exchange data horizontally with rank if and only if
     *
     * - a grid entity is local
     * - this predicate holds for rank
     *
     * @see getNeighbourTrees()
     */
    bool doesRankIndexIdentifyHorizontalDataExchange(int rank, bool calledByReceivingProcess) const;

    /**
     *
     *
     * ## Veto erases
     *
     * The routine markVerticesAroundForkedCell() is used in this routine
     * to ensure that we don't erase the grid over a worker domain. We have to do
     * this both on the master and the worker. The worker invokes the veto
     * coarsening when it comes from a coarse remote tree and runs into local fine
     * grid cell. The master does it the other way round: It sets the vetos if
     * we hit a remote cell out from the local tree. In both cases, the bottom-up
     * analysis of the flag ensures that the information propagates all the way up
     * the grid hierarchies.
     *
     */
    void descend(
      const AutomatonState& state,
      GridVertex            vertices[TwoPowerD],
      TraversalObserver&    observer
    );

    /**
     * Every local refined cell should call this routine. We increment the
     * respective counter. Upon storage, we then refine if the counter
     * equals 2^d. This way we avoid hanging vertices within the domain.
     */
    static void incrementNumberOfAdjacentRefinedLocalCells(GridVertex  vertices[TwoPowerD]);

    /**
     * Takes a state (describing a node in the tree) and returns the
     * @f$ 3^d @f$ states on the next finer level along the Peano SFC. This
     * routine is basically the grammar generation of Bader et al. It relies
     * internally on a recursive call of the other refineState() routine
     * along the spatial dimensions.
     *
     * The array fineGrid is filled along the Peano SFC. So you don't have to
     * traverse the array with a zfor - a simple for is sufficient.
     */
    static void refineState(
      const AutomatonState&              coarseGrid,
      AutomatonState                     fineGrid[ThreePowerD],
      tarch::la::Vector<Dimensions,int>  fineGridPosition = tarch::la::Vector<Dimensions,int>(0),
      int                                axis = Dimensions-1
    );

    /**
     * Wrapper around GridTraversalEventGenerator::isSpacetreeNodeLocal()
     *
     * This is a boolean which solely studies the vertices passed in. The
     * generator is the more powerful routine. It offers, for example, an
     * operation GridTraversalEventGenerator::getTreeOwningSpacetreeNode()
     * to find out who actually owns a cell.
     */
    bool isSpacetreeNodeLocal(
      GridVertex    vertices[TwoPowerD],
      bool          splittingIsConsideredLocal,
      bool          joiningIsConsideredLocal
    ) const;



    bool areAllVerticesRefined(
      GridVertex            vertices[TwoPowerD]
    ) const;

    bool areAllVerticesUnrefined(
      GridVertex            vertices[TwoPowerD]
    ) const;

    /**
     * Could also be called areAllVerticesPersistent() in the Peano
     * terminology.
     */
    bool areAllVerticesNonHanging(
      GridVertex            vertices[TwoPowerD]
    ) const;


    /**
     * Load the vertices of one cell
     *
     * The load process has to be done along the local order of the Peano
     * SFC. For this, I rely on PeanoCurve::getFirstVertexIndex().
     *
     * <h2> Hanging vertices </h2>
     *
     * I originally thought that I might be able to run hanging vertices
     * through the in/out stacks, too. In Peano 3, I had such a feature.
     * Yet, there I had a way more complicated logic. In Peano 4, I simplify
     * the logic. As a consequence, I don't know how many adjacent cells a
     * hanging vertex has and thus create a hanging vertex per adjacent
     * cell.
     */
    void loadVertices(
      const AutomatonState&                        fineGridState,
      GridVertex                                   coarseGridVertices[TwoPowerD],
      GridVertex                                   fineGridVertices[TwoPowerD],
      const tarch::la::Vector<Dimensions,int>&     cellPositionWithin3x3Patch,
      TraversalObserver&                           observer
    );

    void storeVertices(
      const AutomatonState&                        fineGridState,
      GridVertex                                   coarseGridVertices[TwoPowerD],
      GridVertex                                   fineGridVertices[TwoPowerD],
      const tarch::la::Vector<Dimensions,int>&     cellPositionWithin3x3Patch,
      TraversalObserver&                           observer
    );

    /**
     * Little helper. Should likely go into la or utils.
     */
    static tarch::la::Vector<Dimensions,int> convertToIntegerVector( const std::bitset<Dimensions>& in );

    /**
     * This operation has multiple purposes
     *
     * - Merge with neighbour vertices. See
     *   receiveAndMergeGridVertexAtHorizontalBoundary().
     * - Roll over the flags. These guys now are merged already. See
     *   receiveAndMergeGridVertexAtHorizontalBoundary().
     * - Do the refinement flag (state) update.
     * - Erase non-local vertices if they do not carry a veto flag.
     *
     *
     * <h2> Flag update </h2>
     *
     * Implements the standard refinement status transition, i.e. a triggered
     * becomes actually ing. And if a vertex has been refining or erasing in
     * the previous sweep, we finally update it to refined or unrefined.
     *
     * This operation has to be called whenever we load a vertex from the input
     * stream, i.e. we touch it for the very first time. We are not allowed to
     * invoke it after we've created a new vertex.
     *
     * We don't do any statistics here. I've moved all the statistics into
     * updateVertexBeforeStore().
     */
    void updateVertexAfterLoad(
      GridVertex&                               vertex,
      GridVertex                                fineGridVertices[TwoPowerD],
      const tarch::la::Vector<Dimensions,int>&  fineVertexPositionWithinPatch,
      TraversalObserver&                        observer
    );

    /**
     * Some of the entries in the event are modelled as array (for example the
     * set of neighbours of a vertex), though they are logically sets. So I
     * befill the lists and then eventually remove duplicates from the lists
     * before I hand out the event. As a result, the lists hold set data, i.e.
     * each entry only once.
     *
     * Routine could even be static. Nothing changes in the spacetree state.
     * We actually don't even need it.
     */
    void removeDuplicateEntriesFromAdjancyListInEvent(
      GridTraversalEvent&  event
    ) const;




    /**
     *
     *
     * <h2> Implementation </h2>
     *
     * Run through the vertices in the Peano vertex order (modified z depending on
     * curve orientation). This information is already implicitly encoded in the order
     * of the indices within the event. So no fancy arithmetics is required here
     * anymore.
     */
    void receiveAndMergeUserData(const AutomatonState& state, TraversalObserver&  observer, const GridTraversalEvent&  enterCellTraversalEvent, GridVertex  fineGridVertices[TwoPowerD]);

    /**
     * Send user data
     *
     * Send out data along the MPI boundary (horizontal) and stream data to the
     * splitting, new workers (vertical). This routine is called within
     * descend() as epilogue, i.e. after all data have been stored savely away
     * on the output streams.
     *
     *
     * ## Data order on output stacks
     *
     * When we send out data, we send that data from the output stack, i.e. we assume that
     * the user code has already piped its stuff there. As the output data structure is a
     * stack, we have to be careful: If a cell writes N pieces of data to the output stream
     * and we want to copy the first piece of data into an output stream for MPI, then we
     * have to pick the Nth entry from the output stream. With our modified top(), we can
     * access these elements directly. We just have to be careful that top() equals actually
     * a top(0), so picking the Nth element requires us to call top(N-1).
     *
     *
     * ## Masking and selecting transfer data
     *
     * The routine here does not realise the actual data transfer. It delegates
     * the actual streaming to the observer's sendFace(), sendVertex() and
     * sendCell(). As observers are provided by the user - most frequently
     * generated through the Python API - we know which user data we have
     * tied to the mesh and hence can champion the data transfer.
     *
     * This separation of concerns implies that the sendXXX routines also can
     * decide to mask out sent data. However, we have to be careful with this
     * masking: We indeed want to be able to mask out horizontal data transfer
     * and vertical data transfer which corresponds to the domain decomposition
     * data exchange. We do not want to mask anything out that has to do with
     * splits or joins.
     *
     *
     *
     * @todo Was passiert bei dynamic AMR?
     *
     * @see peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree()
     *   for a discussion of the splitting process and the required data flows.
     */
    void sendUserData(const AutomatonState& state, TraversalObserver&  observer, const GridTraversalEvent&  enterCellTraversalEvent, GridVertex  fineGridVertices[TwoPowerD]);


    /**
     * @see splitOrJoinCellBottomUp() For both the usage and a description how we
     *   hijack the routine.
     */
    std::vector<int>  _splittedCells;

    /**
     * Realise the splits and joins
     *
     * This routine alters the vertex adjacency information. That is, it does
     * not yet really split or join. It identifies where to split and join.
     * The adjacency information will be exchanged with (potential) neighbours
     * after this grid traversal and the actual splitting or joining then
     * happens in the next grid sweep. That is, this routine realises the
     * SplitTriggered or JoinTriggered phase. See the discussion in
     * @ref peano_domain_decomposition "Peano's domain decomposition"
     * documentation for a discussion of these subsequent phases.
     *
     * On the split side, we have two types of splits: Splits that are
     * identified bottom-up. These splits are realised carefully and never
     * split off too many cells. The other splits are top-down splits and
     * they are more aggressive, i.e. they might split off too many cells
     * eventually. What type of split you want to have is controlled by a
     * flag within the split specification when you call split().
     *
     *
     * ## Bottom-up splits
     *
     * The split logic is protected by one big check if splits are active.
     * If no splits are triggered, there is no need to check things further.
     * Any further logic is guided by the fact if a spacetree node is refined
     * and if it is a potential split candidate. The latter is determined by
     * evaluating isCellSplitCandidate().
     *
     * Our logic is that we focus first of all on the fine grid cells: We aim
     * to split a certain number of fine grid cells into a new tree. If all
     * children of a refined node are split into the same new tree, and if this
     * (parent) node is a potential split candidate, too, then we move it to
     * the newly created tree as well.
     *
     * We realise an @f$ LR(3^d) @f$ grammar to identify the latter situation.
     * This grammar uses the container (stack) _splittedCells over integers.
     *
     * - Every leaf pushes a marker into this container: _id if it is and
     *   remains local, the new id if the node has been split, and a -1
     *   otherwise.
     * - A refined cell induces a reduce on the container. @f$ 3^d@f$ markers
     *   are removed and analysed: If they all have the same id as the next
     *   split marker to be used, we add it.
     * - A refined cell finally adds its marker again.
     *
     * It is important that I evaluate the reduce analysis even if no splits
     * are do be done anymore. Because it might happen that I have just done
     * exactly 9 splits for example (2d) and thus, the parent of these 9
     * guys should go to the remote node, too. The punchline is that only
     * leaves check if they have to fork. The behaviour of refined octants
     * follows the decisions made for their children before.
     *
     *
     * ## Top-down splits
     *
     * The down splits are discussed in splitCellTopDown(). This is where all
     * the logic sits. The present routine merely impleemnts the decisions
     * made there.
     *
     *
     * ## Rationale
     *
     * I originally realised the splits solely bottom-up, i.e. decided to
     * omit any analysis if the coarse node has already been split. This way, it is
     * very easy to accommodate Peano's @ref peano_domain_decomposition "two fundamental splitting constraints":
     * We simply veto the split-off of any cell with a hanging node. However,
     * such an approach leads to the situation where adaptive mesh refinements
     * severely constrain the algorithm. In the example in the general
     * discussion
     *
     * @image html ../../../documentation/Doxygen/domain-decomposition/regularity-constraint.png
     *
     * we would not split off any of the cells that are adjacent to the
     * AMR boundary. As we never split off a cell unless all of its
     * children are of the same rank, we would never ever split off the
     * green cell. This is bad in a situation where only splitting off the
     * green cell and all of its children would yield a good domain
     * decomposition. So a sole bottom-up approach is not an option.
     *
     * @see splitCellTopDown()
     */
    void splitOrJoinCellBottomUp(
      GridVertex                                vertex[TwoPowerD],
      GridVertex                                fineGridVertices[TwoPowerD]
    );

    /**
     * Split cell in a top down fashion
     *
     * The algorithm logic behind this splitting strategy is very simple: We
     * first find out what the next split-off tree would be.
     *
     * - If a cell's parent is deployed to this tree, deploy the current octant
     *   too. After all, we want to split in a top-down manner.
     * - If an octant's parent is not yet deployed but the cell would be a
     *   split candidate, we trigger the split.
     * - If we deploy an unrefined octant to a new tree, we have to decrease
     *   the internal counter by calling updateSplittingCounter().
     *
     * Unfortunately, the actual splitting is not that simple: We cannot alter
     * the adjacency information of a vertex on-the-fly without running risk to
     * change the vertex ownership. Therefore, we only realise the analysis
     * here and deploy the actual split realisation to splitOrJoinCellBottomUp().
     * Analysis means that we push a marker on the helper stack
     * _splittedCells whenever we want to split. And we update our internal
     * splitting counters. But we do not realise the split itself, i.e. we do
     * not alter adjacency information.
     *
     * @see splitOrJoinCellBottomUp()
     */
    void splitCellTopDown(
      GridVertex                                vertex[TwoPowerD],
      GridVertex                                fineGridVertices[TwoPowerD]
    );

    /**
     * Merge data from worker with master data throughout join
     *
     * ## Join process
     *
     * If we receive a remote cell, then this cell is not refined. In this case
     * the (former) workers holds all the valid adjacency data: We might on a
     * master receive a cell right from the middle of the worker's domain where
     * the master has no clue about any adjacency. So we might think that we can
     * copy over the (former) worker's adjacency data.
     *
     * As we also receive hanging vertices from the worker, we can safely (and
     * pretty naively) copy over the adjacency. THe first non-hanging vertex
     * will bring in the right adjacency information.
     *
     * There is however one tricky special case. Assume that a rank A serves as
     * master to B and C and both of these workers merge into A. We furthermore
     * study one vertex in-between B and C. The merge runs as follows:
     *
     * - Rank B tells rank C that it will merge its cell into A.
     * - Rank C tells rank B that it will merge its cell into A.
     * - Both ranks update their respective local adjacency lists.
     * - Rank B streams its vertex up to rank A.
     *
     * If we now simply copied the adjacency list from B, we'd loose the
     * information that a cell is adjacent to C. B has already updated its list,
     * so it is slightly "ahead" of time.
     *
     * So for a successful merge, it is important that we actually only reset
     * the flags of this very cell. We do so through
     * updateVertexRanksWithinCell(). This logic relies on the fact that we
     * keep adjacency flags of all vertices which are remote yet just one level
     * below the current rank. This check, realised in updateVertexBeforeStore(),
     * ensures we kind of know what we do.
     *
     *
     * <h2> Joins </h2>
     *
     * If we join (parts of) a worker into the local partition, it can happen
     * that the rank receives a vertex that used to be remote before. As a
     * consequence, the local copy of a vertex holds invalid data whereas the
     * incoming data holds up-to-date adjacency information. So if a vertex is
     * remote locally, we take the worker's vertex to deliver accurate adjacency
     * data. The only alteration we make is that we replace the
     * RankOfCellWitchWillBeJoined markers with the rank of the worker.
     * RankOfCellWitchWillBeJoined is used to mark those cells which go to the
     * worker in the join triggered phase, i.e. it is used only on the worker
     * and only to prepare a join.
     *
     *
     * @see updateVertexBeforeStore()
     */
    void mergeCellFromWorkerWithMasterThroughoutJoin(
      GridVertex                                vertex[TwoPowerD],
      GridVertex                                fineGridVertices[TwoPowerD]
    );

    /**
     *
     * <h2> Implementation </h2>
     *
     * We only remove adjacency information for unrefined outside vertices
     * and we are very restrictive what we consider to be outside. Indeed,
     * we consider any vertex still local when we are in a join or fork.
     * If a vertex is really outside but refined, we wait for the erase to
     * pass through before we consider erasing any adjacency info.
     *
     * @see updateVertexAfterLoad Erases refined vertices outside of local
     *                            domain.
     */
    bool shouldEraseAdjacencyInformation(
      const GridVertex&                  vertex,
      GridVertex                         coarseGridVertices[TwoPowerD],
      tarch::la::Vector<Dimensions,int>  fineVertexPositionWithinPatch
    ) const;

    /**
     * <h2> Restriction of veto flags </h2>
     *
     * We make each vertex hold a flag isAntecessorOfRefinedVertex. If it is
     * set, we veto any erasing here. This way, we can ensure that the trees
     * remove at most one level at a time. We illustrate all further
     * explanations with a simple example:
     *
     * @image html Spacetree_updateVertexBeforeStore_restrictIsAntecessorOfRefinedVertex.png
     *
     * Let the red vertices be refined. In a serial code, the tree is not
     * split. You have to glue together the left and right tree. As the middle
     * level's vertex holds the refinement flag, the top level vertex carries
     * the flag isAntecessorOfRefinedVertex. It consequently never is erased.
     * Users first have to remove the finer level.
     *
     * If we split up tree into two threads, this approach fails if the flag
     * is restricted per core in the bottom-up steps. By the end of the sweep,
     * the flag is set on the right tree, but it is not set on the left tree.
     * We consequently might decide to erase on the left tree but veto this on
     * the right tree.
     *
     * To eliminate this behaviour, we split up the flag into a current flag
     * and a flag from the previous solution. This flag is rolled over. If the
     * flag is set, it undoes any erase trigger.
     *
     * <h2> Clean-up of vertices </h2>
     *
     * I originally had the following idea:
     *
     * If a vertex is remote, we manually clear all of its adjacency flags.
     * This is a security thing. It might happen that such a vertex refers to
     * a tree x and that this index x is later re-used (for a new local subtree
     * for example). In this case, a left-over index might suddenly suggest that
     * a totally irrelevant vertex is adjacent to the newish x tree.
     *
     * However, this is wrong in some cases: We have to keep our adjacency.
     * Because if we merge later on, we rely on this adjacency to find out
     * which vertices we have to join.
     *
     * So we have to run for the middle way: We erase all adjacency data but
     * if ad only if a vertex is not adjacent to any kid.
     *
     * <h2> A posteriori refinement </h2>
     *
     * If a vertex is surrounded by @f$ 2^d @f$ refined cells and is not a refined
     * vertex, we have this weird situation that we have a hanging vertex right
     * within a regularly refined subdomain. Topologically, this is allowed, but
     * it makes no sense, introduces strange artefacts in the visualisation and
     * is very difficult to explain. So I keep track of the refined adjacent cells
     * and refine a posteriori if all adjacent cells are refined.
     *
     * @param fineVertexPositionWithinPatch Position of vertex within 3x3 or 3x3x3 patch respectively
     *
     * @see updateVertexAfterLoad()
     * @see mergeCellFromWorkerWithMasterThroughoutJoin() for an explanation why we have to
     *         keep all adjacency information that we actually need later for
     *         merges.
     */
    void updateVertexBeforeStore(
      GridVertex&                               vertex,
      GridVertex                                fineGridVertices[TwoPowerD],
      const tarch::la::Vector<Dimensions,int>&  fineVertexPositionWithinPatch
    );

    /**
     * Determines whether to restrict a vertex to the coarser level or not.
     */
    static bool restrictToCoarseGrid(
      const tarch::la::Vector<Dimensions,int>&  coarseVertexPosition,
      const tarch::la::Vector<Dimensions,int>&  fineVertexPosition
    );

    /**
     * If a cell gets a new id, we have to update its vertices. This routine is
     * only used for splits. I originally thought I might use it for joins as
     * well. Indeed I can. But I can only do this on the master. The worker may
     * not update any cell immediately. If I do this, then the adjacency
     * information of the adjacent vertices is overwritten and I loose this
     * clue that these vertices are adjacent to the local, joining rank.
     *
     * If we split, we have to be careful that we preserve the master-worker
     * topology over the ranks. So this routine may only be called on the master,
     * on a cell that is local, and on a cell whose parent is local, too.
     */
    static void updateVertexRanksWithinCell(
      GridVertex  fineGridVertices[TwoPowerD],
	  int         newId
    );

    /**
     * @todo
     */
    tarch::la::Vector<TwoPowerD,int> getAdjacentRanksForNewVertex(
      GridVertex                                   coarseGridVertices[TwoPowerD],
      const tarch::la::Vector<Dimensions,int>&     vertexPositionWithin3x3Patch
    ) const;

    /**
     * Add a split instruction
     *
     * Add a new split instruction. Peano's documentation
     * @ref peano_domain_decomposition "discusses splits in detail", and also
     * clarifies why we need two different approaches to split (see
     * SplitInstruction).
     *
     * If we add a split, we should commit to one split type: Either all of
     * our splits are top-down or all of them are bottom-up. If we had both
     * types in one grid sweep, we could run into inconsistent grid
     * configurations. At the moment, this is an assumption, i.e. it might
     * work, but I'm just not sure if it really holds.
     *
     * This constrains (only one type of splitting) is not necessary
     * algorithmically. The logic would support a sequence of different
     * split types. However, both splits use the same helper data structures,
     * and they might be messed up with different split types are combined.
     */
    void split(int treeId, const SplitInstruction& instruction);

    /**
     * Get the ids of the surrounding cells of a vertex.
     *
     * This operation studies the vertex only. Please check manually whether
     * your code is in the states SpacetreeState::NewFromSplit or
     * SpacetreeState::EmptyRun. Joining is not taken into account
     * either. So as a summary: I do analyse the vertex data and I do
     * take into account whether subranks are currently joining or
     * triggered to split. But I do ignore the current spacetree's
     * state.
     *
     * The operation returns the empty set if a vertex is not local.
     * It also returns the empty set if a vertex is hanging.
     *
     * @param calledByReceivingProcess The operation relies on
     *   GridTraversalEventGenerator::getAdjacentRanksOfFace() to find the
     *   adjacent faces. This routine in turn uses getBackupOfAdjacentRanks()
     *   if you invoke it on the receiving side and the new adjacent ranks
     *   otherwise.
     *
     */
    std::set<int>  getNeighbourTrees( const GridVertex& vertex, bool calledByReceivingProcess ) const;

    /**
     * Get the ids of the surround ids of a face.
     *
     * We really return only neighbour ids, i.e. no ids of periodic boundary conditions.
     *
     * <h2> Implementation remarks </h2>
     *
     * The domain ids (adjacency lists) along the boundary tell us what the neighbour
     * number is. If a neighbouring rank has triggered a split, we get an updated
     * adjacency list for affected vertices. This list is immediately merged into the
     * the local vertex. The new entries however are only to be taken into account
     * when we send data. For receives, we should stick to the old entries. Therefore,
     * we use the backup of the adjacency list when we receive data, but we use the
     * updated/new list when we send out stuff.
     *
     * If we bump into a new vertex, we should explicitly ignore it when we receive.
     * After all, new vertices cannot yet have triggered incoming data. The counterpart
     * is deleted vertices which should not contribute towards a send command.
     *
     * @return -1  (TraversalObserver::NoData) if there's no neighbour or face is not local.
     */
    int  getNeighbourTrees( GridVertex vertex[TwoPowerD], int faceNumber, bool calledByReceivingProcess ) const;

    bool isFaceAlongPeriodicBoundaryCondition(GridVertex vertex[TwoPowerD], int faceNumber, bool calledByReceivingProcess) const;


    /**
     * This one is to be invoked if and only if a vertex goes to the in/out
     * stacks. The routine should be called just before the vertex goes to
     * the output stack. We call it in updateVertexBeforeStore() here, so
     * this constraints automatically is followed.
     *
     * The routine builds up a set over integers into which it adds all ranks
     * that have to receive a copy. Once this is done, it iterates over the
     * set and sends out data. The two-step scheme becomes necessary, as we
     * have to avoid that vertices are sent around multiple times.
     *
     * As this routine is called from within updateVertexBeforeStore(), we
     * may assume that this is not an empty tree run.
     */
    void sendGridVertex( const GridVertex& vertex );

    /**
     * Manage the data exchange after a vertex is loaded for the first time
     *
     * The operation has three jobs to do:
     * - We backup the adjacency ranks.
     * - Exchange vertices along domain boundary.
     * - Exchange vertices belonging to periodic boundaries.
     *
     * The order of these three steps is important. The first one is a simple
     * copy. The other ones loop over neighbours and call a series of operations.
     * Logically, domain boundaries and periodic boundaries for me are both
     * realised by domain cuts. Therefore, we use the same routines within.
     *
     * <h2> Backup of adjacency data </h2>
     *
     * It is convenient to merge the adjacency flags straightaway after a vertex
     * has been loaded and its boundary counterparts have dropped in. However, once
     * we merge we loose the information about the previous traversal's adjacency.
     * This means, when we construct the neighbour information (who merges with
     * what) for the user data (createEnterCellTraversalEvent()) we lack the
     * information we actually need. Therefore, this routine backups the
     * adjacency information from the vertex.
     *
     * <h2> Boundary data exchange (grid) </h2>
     *
     * For a local vertex, i.e. one adjacent to the current active tree, which
     * is neighbouring another tree, we do receive this tree's data copy and
     * then merge it into the local tree.
     *
     * This approach doesn't work straightforwardly when we split: Lets assume
     * tree 0 is triggered to split into 0 and 1. In a first step, tree 0 enters
     * the state split triggered. It now updates all adjacency lists, so
     * boundary data is already sent out to 1 - even though 1 is not instantiated
     * yet. Once we are done, we copy over (parts of) tree 0 into tree 1, so
     * tree 1 now does exist and is well-prepared to receive the data already
     * dropped by 0. This second step is the actual splitting step. Now, other
     * ranks have still sent in data to 0 which actually should go to 1. We
     * receive those guys, but we have to eliminate explicitly any incoming data
     * we'd expect from 1 as 1 didn't have the chance yet to send it out.
     *
     * The function is called directly
     * after a vertex has been read from the input stream. Please note that
     * an update of the refinement states (e.g. switch from
     * refinement-triggered to refining) happens after the merge. Any update of the
     * refinement state in this operation hence immediately affects the
     * vertex updates in this very iteration.
     *
     * The update of the adjacency information is simple: If a neighbour tells us
     * that it has changed an adjacency entry in one of its own fields, i.e. if
     * it has changed its own entry, we copy this information over. Otherwise, we
     * ignore any updates (there should not be any).
     *
     * <h2> Periodic boundary conditions </h2>
     *
     * Periodic boundary conditions fall back to standard boundary data
     * exchanges through specialised stacks. They are supported on tree 0 only.
     * Different to standard boundaries, we don't have to update any adjacency data
     * here, as all periodic values are always handled on spacetree 0.
     *
     * <h2> Horizontal vs. vertical </h2>
     *
     * Has to happen before receiveAndMergeGridVertexAtVerticalBoundary(). See the
     * receiveAndMergeGridVertexAtVerticalBoundary()'s documentation for an explanation.
     * Some more reasons are given in the guidebook.
     *
     * <h2> Context </h2>
     *
     * The routine is called by updateVertexAfterLoad(). The counterpart of the
     * routine is sendGridVertex(). However, as my sends are literally
     * just memcopies, sends are way simpler than this routine. The operation
     * affects solely the grid's vertices. It does not interfere with any user
     * data. In principle, this follows my pattern that the grid has to be there
     * first and then events are triggered afterwards.
     */
    void receiveAndMergeGridVertexAtHorizontalBoundary( GridVertex& vertex );

    /**
     * This is a merge routine for vertical data exchange. It is important
     * that you merge
     *
     * <h2> Join behaviour </h2>
     *
     * If we are joining in another rank, we might get update adjacency information
     * for this rank. Such adjacency updates are important for the outgoing data,
     * i.e. for the send. Logically, the master and the dying worker run in parallel,
     * that is the updates on the worker belong logically into steps after the actual
     * load.
     *
     * It therefore is important that you invoke this operation afer
     * receiveAndMergeGridVertexAtVerticalBoundary() and that this routine overwrites the
     * adjacentRanks yet not the backup of this field. This way, we are consistent
     * with any horizontal merges on the worker.
     *
     * @see receiveAndMergeGridVertexAtHorizontalBoundary()
     */
    void receiveAndMergeGridVertexAtVerticalBoundary( GridVertex& vertex );

    /**
     * Called by receiveAndMergeGridVertexAtHorizontalBoundary(). Besides tons of assertions,
     * the routine realises the mergers of the refinement flags and the
     * setIsAntecessorOfRefinedVertexInCurrentTreeSweep. Please consult
     * receiveAndMergeGridVertexAtHorizontalBoundary() for a higher level overview.
     */
    void mergeGridVertexRefinementStateAtHorizontalDomainBoundary( GridVertex& vertex, const GridVertex& inVertex, int neighbour );
    void mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary( GridVertex& vertex, const GridVertex& inVertex, int neighbour );

    /**
     * Only used by SpacetreeSet to create children of the original tree.
     *
     * We have to set the stats's stationary counter manually, as clear() does not
     * reset it. We furthermore set it to -2, as we'll need two iterations to set
     * a new remote spacetree up.
     */
    Spacetree(
      int newId,
      int masterId,
      const tarch::la::Vector<Dimensions,double>&  offset,
      const tarch::la::Vector<Dimensions,double>&  width,
      bool  traversalInverted
    );

    /**
     * Join with master. Call this routine only for degenerated trees,
     * i.e. for trees without any refined local cells.
     *
     * We have to be careful with the merges. Often when we fork a tree,
     * this tree first is not refined further. We need a couple of sweeps
     * to see whether a tree is refined further or not.
     */
    void joinWithMaster();
    void joinWithWorker(int id);

    /**
     * Only ranks that have no kids are allowed to join. This is
     * implicitly ensured as we have only degenerated trees here,
     * i.e. we only allow a tree to join if it does not host local
     * spacetree nodes. Furthermore, we require a tree to be
     * stationary, i.e. to linger around like that for a while.
     *
     * @see join()
     */
    bool mayJoinWithMaster() const;

    /**
     * We allow at most one join at a time and not while we split
     */
    bool mayJoinWithWorker() const;

    /**
     * Is the tree in principle allowed to split
     *
     * A tree cannot split if it is brand new or currently involved in some
     * load balancing. Even if a tree is allowed to split, it does not mean
     * that any split might be successful. A tree might for example be
     * stationary yet host so few cells that it cannot split anymore without
     * violating @ref peano_domain_decomposition "Peano's domain decomposition constraints".
     * This per-cell decision is made by peano4::grid::Spacetree::isCellTopDownSplitCandidate()
     * and  peano4::grid::Spacetree::isCellBottomUpSplitCandidate().
     */
    bool maySplit() const;

    /**
     * @return Id of splitting tree or -1 if there's none.
     */
    int getSplittingTree() const;

    /**
     * Reduce splitting counter
     *
     * Every tree in the list of trees for which we have to trigger a split
     * has an integer counter which highlights how many cells still to offload
     * to the newly created tree. Decrement this counter.
     *
     * However, do not remove the entry from the split triggered set. We need
     * all of these indices to know in the next sweep which child trees are now
     * in the state splitting i.e. transition from split triggered into
     * splitting.
     *
     * If we have top-down splits, we might split off two many cells. In this
     * case, ensure that the internal counter does not underrun zero.
     */
    void updateSplittingCounter( int treeId );
  public:
    Spacetree(
      const tarch::la::Vector<Dimensions,double>&  offset,
      const tarch::la::Vector<Dimensions,double>&  width,
      const std::bitset<Dimensions>&               periodicBC = 0
    );

    ~Spacetree();

    /**
     * @see GridTraversalEvent for a discussion of the arising actions. The traversal
     *   actually does not issue these actions. It invokes only an observer and hands
     *   in the GridTraversalEvent. It is then up to the observer to read out the
     *   event and to issue the actions.
     *
     * @param calledFromSpacetreeSet If you use traverse directly, please do
     *     not alter this value
     */
    void traverse(TraversalObserver& observer, bool calledFromSpacetreeSet = false);

    GridStatistics getGridStatistics() const;

    std::string toString() const;

    bool isInvolvedInJoinOrFork() const;
};
