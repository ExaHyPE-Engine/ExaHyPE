// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/logging/Log.h"
#include "tarch/mpi/mpi.h"

#include "peano4/parallel/Node.h"

#include <map>

#include "Configuration.h"
#include "Blacklist.h"
#include "Statistics.h"


namespace toolbox {
  namespace loadbalancing {
    class AbstractLoadBalancing;
  }
}


class toolbox::loadbalancing::AbstractLoadBalancing {
  public:
    /**
     * Constructor
     *
     * Data ownership is handed over to the load balancing, i.e. the balancing
     * has to delete the configuration object unless you pass in a false.
     * Consequently, deleteInConstructor should not hold if you pass in the
     * nullptr.
     *
     * Many implementations of AbstractLoadBalancing change the _state field
     * immediately in their constructor. Please ensure that you invoke
     *
     *       _statistics.notifyOfStateChange(_state);
     *
     * afterwards to ensure that the statistics are correct right from the
     * start.
     */
    AbstractLoadBalancing(
      Configuration* configuration,
      bool deleteInDestructor
    );


    virtual ~AbstractLoadBalancing();


    /**
     * Generic string serialisation. You might want to extend it. Therefore,
     * the routine is labelled as virtual.
     */
    virtual std::string toString() const;


    /**
     * Switch on/off.
     */
    virtual void enable(bool);


    virtual bool hasSplitRecently() const;


    /**
     * Clarifies whether load balancing is, in principle, enabled. Might however
     * mean that the load balancing is currently (temporarily) disabled. In
     * this case, the routine still returns true.
     */
    bool isEnabled(bool globally) const;


    /**
     * Delegate to stats.
     */
    virtual int getGlobalNumberOfTrees() const;


    /**
     * A load balancing can either be stagnating or be switched off for
     * this predicate to hold.
     */
    virtual bool hasStagnated() const;


    virtual void finishSimulation();


    /**
     * Finish the step
     *
     * No matter what you do here, you have to invoke
     *
     *          _statistics.updateGlobalView();
     *          _blacklist.update();
     *
     * in this routine.
     */
    virtual void finishStep() = 0;
  protected:
    static tarch::logging::Log  _log;

    Blacklist   _blacklist;

    Statistics  _statistics;

    Configuration*  _configuration;

    /**
     * Ensure that you invoke
     *
     *        _statistics.notifyOfStateChange(_state)
     *
     * whenever you change the state. Otherwise, the MPI will have an invalid
     * view of the whole world.
     */
    State  _state;

    bool _deleteInDestructor;


    /**
     * Ensure enough memory is left-over.
     */
    bool fitsIntoMemory(State state) const;


    /**
     * Is the balancing between the ranks ok
     *
     * This operation checks if the inter-rank load decomposition does
     * violate the overall load balancing constraints. The global balancing
     * is determined by a cascade of checks:
     *
     * - If we have an inconsistent data view, i.e. if the stats are not valid,
     *   we have to assume that everything is fine.
     * - If we have fewer trees globally than we have ranks, the balancing is
     *   by definition poor.
     * - If we host more trees than allowed, the balancing by definition is
     *   good, since we could not refine further anyway.
     * - Otherwise, we look if the max load per rank and the min load per rank
     *   differ more than the tolerance.
     */
    bool isInterRankBalancingBad() const;


    /**
     * Is the balancing on the rank ok
     *
     * Similar to isInterRankBalancingBad(), this operation runs through a
     * series of checks:
     *
     * - If there are no trees on the rank, then the result is false. By
     *   definition, everything is well-balanced.
     * - If there is only one tree, the balancing is by definition bad, as
     *   we cannot make a balancing statement.
     * - If we host more trees than allowed, the balancing by definition is
     *   good, since we could not refine further anyway.
     * - Otherwise, we compare the minimum and maximum tree.
     */
    bool isIntraRankBalancingBad() const;

   
    bool areRanksUnemployed() const;

};

std::ostream& operator<<(std::ostream& out, const toolbox::loadbalancing::AbstractLoadBalancing& balancing);
