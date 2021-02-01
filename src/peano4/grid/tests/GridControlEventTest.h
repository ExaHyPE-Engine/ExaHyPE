// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_GRID_TESTS_GRID_CONTROL_EVENT_TEST_H_
#define _PEANO4_GRID_TESTS_GRID_CONTROL_EVENT_TEST_H_

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"


namespace peano4 {
  namespace grid {
    namespace tests {
      class GridControlEventTest;
    }
  }
}


/**
 * The GridControlEvent is a generated type, so we don't really test
 * this type. We test the routines around it which are held in grid.h.
 */
class peano4::grid::tests::GridControlEventTest: public tarch::tests::TestCase {
  private:
    /**
     * Logging device
     */
    static tarch::logging::Log _log;

    /**
     * From a debugging session:
     *
     <pre>

 69568402     00:00:00     rank:0       core:0       info         peano4::grid::consolidate(...)                                (refinementControl=Refine,offset=[-0.0333333,-0.0333333],width=[0.4,0.4],h=[0.111111,0.111111])
 69600708     00:00:00     rank:0       core:0       info         peano4::grid::consolidate(...)                                (refinementControl=Refine,offset=[0.3,-0.0333333],width=[0.4,0.4],h=[0.111111,0.111111])
 69630098     00:00:00     rank:0       core:0       info         peano4::grid::consolidate(...)                                (refinementControl=Refine,offset=[0.633333,-0.0333333],width=[0.4,0.4],h=[0.111111,0.111111])
 69658414     00:00:00     rank:0       core:0       info         peano4::grid::consolidate(...)                                (refinementControl=Refine,offset=[0.633333,0.3],width=[0.4,0.4],h=[0.111111,0.111111])
 69739708     00:00:00     rank:0       core:0       info         peano4::grid::consolidate(...)                                (refinementControl=Refine,offset=[0.3,0.3],width=[0.4,0.4],h=[0.111111,0.111111])
 69769194     00:00:00     rank:0       core:0       info         peano4::grid::consolidate(...)                                (refinementControl=Refine,offset=[-0.0333333,0.3],width=[0.4,0.4],h=[0.111111,0.111111])
 69797019     00:00:00     rank:0       core:0       info         peano4::grid::consolidate(...)                                (refinementControl=Refine,offset=[-0.0333333,0.633333],width=[0.4,0.4],h=[0.111111,0.111111])
 69824642     00:00:00     rank:0       core:0       info         peano4::grid::consolidate(...)                                (refinementControl=Refine,offset=[0.3,0.633333],width=[0.4,0.4],h=[0.111111,0.111111])
 69852630     00:00:00     rank:0       core:0       info         peano4::grid::consolidate(...)                                (refinementControl=Refine,offset=[0.633333,0.633333],width=[0.4,0.4],h=[0.111111,0.111111])
 69894892     00:00:00     rank:0       core:0       info         peano4::grid::consolidate(...)                                1: (refinementControl=Refine,offset=[0.633333,0.3],width=[0.4,0.733333],h=[0.111111,0.111111])

     </pre>
     */
    void testMerge1();


    /**
     *
     * From another debugging session:
     *
     * <pre>

 enter with 5
 - in: (refinementControl=Refine,offset=[0.316667,0.316667],width=[0.366667,0.366667],h=[0.111111,0.111111])
 - in: (refinementControl=Refine,offset=[0.65,0.316667],width=[0.366667,0.366667],h=[0.111111,0.111111])
 - in: (refinementControl=Refine,offset=[0.65,-0.0166667],width=[0.366667,0.366667],h=[0.111111,0.111111])
 - in: (refinementControl=Refine,offset=[0.316667,-0.0166667],width=[0.366667,0.366667],h=[0.111111,0.111111])
 - in: (refinementControl=Refine,offset=[-0.0166667,-0.0166667],width=[0.366667,0.366667],h=[0.111111,0.111111])


 8800440139   00:00:08     rank:0       core:0       info         peano4::grid::merge(...)                                       enter with 3
 8800449454   00:00:08     rank:0       core:0       info         peano4::grid::merge(...)                                       - in: (refinementControl=Refine,offset=[-0.0166667,-0.0166667],width=[0.7,0.366667],h=[0.111111,0.111111])
 8800457618   00:00:08     rank:0       core:0       info         peano4::grid::merge(...)                                       - in: (refinementControl=Refine,offset=[0.65,-0.0166667],width=[0.366667,0.7],h=[0.111111,0.111111])
 8800465519   00:00:08     rank:0       core:0       info         peano4::grid::merge(...)                                       - in: (refinementControl=Refine,offset=[0.316667,0.316667],width=[0.366667,0.366667],h=[0.111111,0.111111])

 8800471873   00:00:08     rank:0       core:0       info         peano4::grid::merge(...)                                      have to handle 3 refine/coarsen commands
 - (refinementControl=Refine,offset=[0.316667,0.316667],width=[0.366667,0.366667],h=[0.111111,0.111111])
 - (refinementControl=Refine,offset=[0.65,-0.0166667],width=[0.366667,0.7],h=[0.111111,0.111111])
 - (refinementControl=Refine,offset=[-0.0166667,-0.0166667],width=[0.7,0.366667],h=[0.111111,0.111111])

       </pre>
     *
     */
    void testMerge2();
  public:
    GridControlEventTest();
    void run() override;
};


#endif
