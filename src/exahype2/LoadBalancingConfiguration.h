// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_LOAD_BALANCING_CONFIGURATION_H_
#define _EXAHYPE2_LOAD_BALANCING_CONFIGURATION_H_


#include "toolbox/loadbalancing/RecursiveSubdivision.h"

namespace exahype2 {
  class LoadBalancingConfiguration;
}


/**
 * ExaHyPE 2-specific load balancing configuration
 *
 * We do not impose any constraint while we build up the mesh. Later on, we allow
 * users (via the constructor or Python interface, respectively) to restrict
 * both the size of the trees and the tree count.
 */
class exahype2::LoadBalancingConfiguration: public toolbox::loadbalancing::RecursiveSubdivision::Configuration {
  private:
    const double   _loadBalancingQuality;
    const int      _minSizeOfTree;
    const int      _maxNumberOfTrees;
    const int      _maxSizeOfDeployedRemoteMPITree;
  public:
    LoadBalancingConfiguration( double loadBalancingQuality=0.9, int minSizeOfTree = 0, int maxNumberOfTrees=std::numeric_limits<int>::max(), int maxSizeOfDeployedRemoteMPITree = std::numeric_limits<int>::max() );

    virtual ~LoadBalancingConfiguration() = default;

    bool makeSplitDependOnMemory(Phase phase) override;

    int getMaxLocalTreesPerRank(Phase phase) override;

    double getWorstCaseBalancingRatio(Phase phase) override;

    int getMinTreeSize(Phase phase) override;

    int getMaxTreeSize(Phase phsae) override;
};


#endif
