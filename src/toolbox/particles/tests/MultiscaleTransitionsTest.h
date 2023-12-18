// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"

namespace toolbox {
  namespace particles {
    namespace tests {
      class MultiscaleTransitionsTest;
    } // namespace tests
  }   // namespace particles
} // namespace toolbox

class toolbox::particles::tests::MultiscaleTransitionsTest: public tarch::tests::TestCase {
private:
  /**
   * Logging device
   */
  static tarch::logging::Log _log;

  /**
   * Test setup from the Swift test case:
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *       ...::touchVertexFirstTime(...)                       ok, let's sieve:
   * (debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232)
   * into ([0.685185,0.648148],[0.037037,0.037037],hanging=0000,select=1)
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *       touchVertexFirstTime(...) ok, let's sieve:
   * (debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Virtual,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232)
   * into ([0.685185,0.648148],[0.037037,0.037037],hanging=0000,select=1) on tree 0 enterCell(...)            cannot
   * confirm (yet) that particle is local on tree 0:
   * particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Virtual,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232),
   * cell=(x=[0.685185,0.648148],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[0,2],has-been-enclave=0,will-be-enclave=0)
   *       enterCell(...)            cannot confirm (yet) that particle is local on tree 0:
   * particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Virtual,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232),
   * cell=(x=[0.722222,0.648148],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[1,2],has-been-enclave=0,will-be-enclave=0)
   *       enterCell(...)            cannot confirm (yet) that particle is local on tree 0:
   * particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232),
   * cell=(x=[0.685185,0.611111],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[0,1],has-been-enclave=0,will-be-enclave=0)
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   */
  void testSievePredicate();

  /**
   * Test the instructions indicatring whether to lift a particle
   *
   * ## Case
   *
   * Within the Swift project, I got the following output:
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *  82231112769  00:01:22     rank:0       core:7       info         benchmarks::swift2::noh::observers::StepHydroPartDrift_sweep12peano4_toolbox_particles_api_UpdateParticleGridAssociation_LiftDrop1::touchVertexLastTime(...)                 have to lift particle (debugX=[0.555,0.165],debugH=[0,0],x=[0.554725,0.166678],cellH=[0.111111,0.111111],searchRadius=0.0185185,ParallelState=Local,NewParallelState=Local,MoveState=Moved,CellHasUpdatedParticle=1,cfl=0.1,initialTimeStepSize=0.0001,adjustTimeStepSize=0,hydroDimensions=2,etaFactor=1.2348,smlMin=1e-06,smlMax=0.166667,smlTolerance=0.0001,smlMaxIterations=30,alphaAV=0.8,betaAV=3,mass=2.5e-05,v=[-0.16201,0.986788],a=[8.12471e-05,-0.000445081],density=1.00453,pressure=6.71798e-07,smoothingLength=0.00616007,u=1.00315e-06,uDot=1.97937e-06,f=-1.06997e-07,wcount_dh=-55596,rho_dh=-1.3899,wcount=40181.1,hDot=-0.00911606,smoothingLengthIterCount=0,balsara=0.799973,rot_v=8.11773e-05,div_v=-2.94711,v_sig_AV=0.0934877,soundSpeed=0.00105575,v_full=[-0.16201,0.986788],u_full=1.00325e-06,hasNoNeighbours=0,isBoundaryParticle=0,partid=1,dependencyChecksPeanoEventUsedBySwift=touchVertexFirstTime,dependencyChecksAlgorithmStepLastUpdated=FlagBoundaryParticles,dependencyChecksAlgorithmStepUpdates=1,...,dependencyChecksAlgorithmStepMaskOuts=0,...,dependencyChecksInitStepLastUpdated=FlagBoundaryParticles,dependencyChecksInitStepUpdates=1,...,dependencyChecksInitStepMaskOuts=0,...) to next coarser vertex 1. Previously assigned to (cell-centre=[0.611111,0.166667],cell-h=[0.111111,0.111111],hanging=0000,select=0)
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * ## Realisation
   *
   * I depend heavily on the fact that this test is a friend of the vertex
   * marker.
   */
  void testLiftParticleAssociatedWithVertex();
public:
  MultiscaleTransitionsTest();
  virtual void run() override;
};
