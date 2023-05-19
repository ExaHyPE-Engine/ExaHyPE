// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"


namespace toolbox {
  namespace particles {
    namespace tests {
      class MultiscaleTransitionsTest;
    }
  }
}


class toolbox::particles::tests::MultiscaleTransitionsTest: public tarch::tests::TestCase {
  private:
    /**
     * Logging device
     */
    static tarch::logging::Log _log;

    /**
     * Test setup from the Swift test case:
     *
     *       ...::touchVertexFirstTime(...)                       ok, let's sieve: (debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,cutOffRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232) into ([0.685185,0.648148],[0.037037,0.037037],hanging=0000,select=1)
     *
     * So it is clear that this sieve should go through. This info is fine, and
     * the unit test simply confirms it. In the test case, this event/message
     * did arise twice when I run the code with two trees. Which is, in
     * principle, fine, given that the vertex of interest might be exactly at
     * the domain boundary.
     *
     * I would now expect one cell to say "hey, this particle is local here".
     * As particles are associated to vertices, we might to get up to three
     * "no, not here" messages if a vertex is local. These numbers refer to a
     * 2d setup. However, I got
     *
     *       touchVertexFirstTime(...) ok, let's sieve: (debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Virtual,cutOffRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232) into ([0.685185,0.648148],[0.037037,0.037037],hanging=0000,select=1) on tree 0
     *       enterCell(...)            cannot confirm (yet) that particle is local on tree 0: particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Virtual,cutOffRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232), cell=(x=[0.685185,0.648148],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[0,2],has-been-enclave=0,will-be-enclave=0)
     *       enterCell(...)            cannot confirm (yet) that particle is local on tree 0: particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Virtual,cutOffRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232), cell=(x=[0.722222,0.648148],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[1,2],has-been-enclave=0,will-be-enclave=0)
     *       enterCell(...)            cannot confirm (yet) that particle is local on tree 0: particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,cutOffRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232), cell=(x=[0.685185,0.611111],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[0,1],has-been-enclave=0,will-be-enclave=0)
     *
     */
    void testSievePredicate();
  public:
    MultiscaleTransitionsTest();
    virtual void run() override;
};
