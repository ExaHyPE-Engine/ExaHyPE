// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "toolbox/loadbalancing/strategies/cascade/Cascade.h"
#include "toolbox/loadbalancing/strategies/SpreadOutOnceGridStagnates.h"
#include "toolbox/loadbalancing/strategies/SplitOversizedTree.h"


namespace toolbox {
  namespace loadbalancing {
    namespace strategies {
      namespace cascade {
        typedef toolbox::loadbalancing::strategies::cascade::Cascade<toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates, toolbox::loadbalancing::strategies::SplitOversizedTree>  SpreadOutOnceGridStagnates_SplitOversizedTree;
      }
    }
  }
}


