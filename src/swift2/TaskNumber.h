// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <functional>
#include <set>
#include <string>
#include <vector>

#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "tarch/multicore/Tasks.h"


namespace swift2 {
  /**
   * Task Number
   *
   * A task number in SWIFT is an integer tuple:
   *
   * - The first index is the vertex or cell index that we use.
   * - The second index identifies the event used.
   *
   * We construct our task graph over these tuples and then flatten them
   * down into a normal integer just before we spawn the tasks into the
   * Peano tarch::multicore.
   */
  struct TaskNumber {
    public:
      enum class TaskAssociation {
        TouchVertexFirstTime = 0,
        TouchCellFirstTime   = 1,
        TouchVertexLastTime  = 2,
        NumberOfEntries      = 3
      };

    /**
     * Construct new task number
     *
     * @param number0 Typically the "real" task counter, i.e. a unique number
     *   from a very vast range which distinguishes different task types.
     *   Pass in ::tarch::multicore::NoOutDependencies if you want to actually
     *   construct the object NoOutDependencies. In this case, the other
     *   arguments are ignored.
     * @param number1 Typically the type of mesh entity to which the task is
     *   tied to.
     * @param number2 Typically some kind of species or task type counter.
     */
    TaskNumber(int number0, TaskAssociation number1);

    /**
     * Defines the max indices for each entry in TaskNumber.
     */
    static const TaskNumber Max;
    static const TaskNumber NoOutDependencies;

    std::string toString() const;

    tarch::multicore::TaskNumber flatten() const;

    /**
     * Total order on object. Required to store task numbers in sets.
     */
    bool operator<(const TaskNumber& rhs) const;
    bool equals(const TaskNumber& rhs) const;

  private:
    int             _taskCounter;
    TaskAssociation _taskAssociation;
  };

  std::string toString(const std::set<TaskNumber>& numbers);

  /**
   * Alias around method flatten(). I introduced this one so that both the
   * conversation of a single number and a whole set can be written with the
   * same syntax.
   */
  int           flatten(const TaskNumber& numbers);
  std::set<int> flatten(const std::set<TaskNumber>& numbers);

  /**
   * Get numbers of parent vertices
   *
   * We assume that each parent vertex has a routine getNumber(). This routine
   * returns the set of numbers of all parent cells. A vertex can have up to
   * @f$ 2^d @f$ parent vertices. If the vertex coincides spatially with a
   * vertex on the next coarser grid (cmp. peano4::datamanagement::VertexMarker::coincidesWithCoarseGridVertex()),
   * then the result will only contain one entry. This means that the current
   * vertex is at the corner of a @f$ 3^d @f$ patch within the spacetree. If
   * the vertex is placed along the edges of a patch, one or two parents are
   * returned (unknowns are not stored redundantly).
   */
  template <typename Vertex>
  std::set<::swift2::TaskNumber> getVertexNumbersOfParentVertices(
    const peano4::datamanagement::VertexMarker&      marker,
    peano4::datamanagement::VertexEnumerator<Vertex> coarseGridVertices,
    ::swift2::TaskNumber::TaskAssociation            taskAssociation
  );

  template <typename Vertex>
  std::set<::swift2::TaskNumber> getVertexNumbersOfParentVertices(
    tarch::la::Vector<Dimensions, int>               position,
    peano4::datamanagement::VertexEnumerator<Vertex> coarseGridVertices,
    ::swift2::TaskNumber::TaskAssociation            taskAssociation,
    int                                              dimension
  );

  /**
   * Pending dependencies container
   *
   * When we run through the tree, we sometimes cannot submit dependencies
   * directly. When we are on a fine level, we might, for example, have
   * dependencies from the fine mesh to a touchLastTime event on the next
   * coarser level. This one is not yet submitted. However, once we will
   * get there, i.e. back to the coarser level, the fine grid info will not
   * be at hand anymore. Therefore, we have to memorise these dependencies on
   * the fine grid, and then use this memorised info later.
   *
   * Dependencies are stored as from->to.
   */
  using PendingDependencies = std::set<std::pair<::swift2::TaskNumber, ::swift2::TaskNumber>>;

  /**
   * Extract set of dependencies for given task
   *
   * We take the bookmarked dependencies from pendingDependencies that feed
   * into task and return those. Before we do so, we remove them from
   * pendingDependencies.
   */
  std::set<::swift2::TaskNumber> getDependenciesForTask(
    const ::swift2::TaskNumber& task, PendingDependencies& pendingDependencies
  );
} // namespace swift2



bool operator==(const swift2::TaskNumber& lhs, const swift2::TaskNumber& rhs);
bool operator!=(const swift2::TaskNumber& lhs, const swift2::TaskNumber& rhs);


#include "TaskNumber.cpph"
