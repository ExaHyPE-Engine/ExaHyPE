// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "toolbox/loadbalancing/Cascade.h"
#include "toolbox/loadbalancing/SpreadOut.h"
#include "toolbox/loadbalancing/RecursiveBipartition.h"


namespace toolbox {
  namespace loadbalancing {
    namespace cascade {
      typedef toolbox::loadbalancing::Cascade<toolbox::loadbalancing::SpreadOut, toolbox::loadbalancing::RecursiveBipartition>  SpreadOut_RecursiveBipartition;
    }
  }
}


