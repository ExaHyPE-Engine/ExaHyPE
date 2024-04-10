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
   *  91564839883  00:01:31     rank:0       core:2       info         benchmarks::swift2::noh::observers::StepHydroPartDrift_sweep12peano4_toolbox_particles_api_UpdateParticleGridAssociation_LiftDrop1::touchVertexLastTime(...)                 have to lift particle (debugX=[0.555,0.165],debugH=[0,0],x=[0.554725,0.166678],cellH=[0.111111,0.111111],searchRadius=0.0185185,ParallelState=Local,NewParallelState=Local,MoveState=Moved,CellHasUpdatedParticle=1,cfl=0.1,initialTimeStepSize=0.0001,adjustTimeStepSize=0,hydroDimensions=2,etaFactor=1.2348,smlMin=1e-06,smlMax=0.166667,smlTolerance=0.0001,smlMaxIterations=30,alphaAV=0.8,betaAV=3,mass=2.5e-05,v=[-0.16201,0.986788],a=[8.12471e-05,-0.000445081],density=1.00453,pressure=6.71798e-07,smoothingLength=0.00616007,u=1.00315e-06,uDot=1.97937e-06,f=-1.06997e-07,wcount_dh=-55596,rho_dh=-1.3899,wcount=40181.1,hDot=-0.00911606,smoothingLengthIterCount=0,balsara=0.799973,rot_v=8.11773e-05,div_v=-2.94711,v_sig_AV=0.0934877,soundSpeed=0.00105575,v_full=[-0.16201,0.986788],u_full=1.00325e-06,hasNoNeighbours=0,isBoundaryParticle=0,partid=1,dependencyChecksPeanoEventUsedBySwift=touchVertexFirstTime,dependencyChecksAlgorithmStepLastUpdated=FlagBoundaryParticles,dependencyChecksAlgorithmStepUpdates=1,...,dependencyChecksAlgorithmStepMaskOuts=0,...,dependencyChecksInitStepLastUpdated=FlagBoundaryParticles,dependencyChecksInitStepUpdates=1,...,dependencyChecksInitStepMaskOuts=0,...) to next coarser vertex 1. Previously assigned to (cell-centre=[0.611111,0.166667],cell-h=[0.111111,0.111111],local=1111,hanging=0000,select=0,has-been-refined=1111,will-be-refined=1111,is-parent-vertex-local=1111,is-parent-cell-local=1)
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * That one is just a tiny little bit too much away from its closest vertex
   * (x-marker._cellCentre=[-0.056386,1.1e-05]) and therefore should be
   * assigned to parent 0.
   *
   * In the subsequent iteration, it should be dropped again. In a version of
   * the code dated 18/12/23, I was not able to see this drop however. I check
   * the drop behaviour in two steps: I first ensure that the particle is
   * not(!) dropped into the very same vertex again, and then I check that it
   * is indeed dropped into the neighbour vertex.
   *
   * ## Realisation
   *
   * I depend heavily on the fact that this test is a friend of the vertex
   * marker.
   */
  void testLiftDropOfParticleAssociatedWithVertex01();


  /*
   * Another test case
   *
   * We start with this lift operation:
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  77929644137  00:01:17     rank:0       core:1       info         benchmarks::swift2::noh::observers::StepHydroPartDrift_sweep12peano4_toolbox_particles_api_UpdateParticleGridAssociation_LiftDrop1::touchVertexLastTime(...)                   have to lift particle (debugX=[0.505,0.165],debugH=[0,0],x=[0.504
 975,0.1667],cellH=[0.037037,0.037037],searchRadius=0.0185185,ParallelState=Local,NewParallelState=Local,MoveState=Moved,CellHasUpdatedParticle=1,cfl=0.1,initialTimeStepSize=0.0001,adjustTimeStepSize=0,hydroDimensions=2,etaFactor=1.2348,smlMin=1e-06,smlMax=0.166667,smlTolerance=0.0001,smlMaxIterations=30,a
 lphaAV=0.8,betaAV=3,mass=2.5e-05,v=[-0.0149237,0.999888],a=[7.92017e-06,-0.000467427],density=1.00459,pressure=6.71868e-07,smoothingLength=0.00615988,u=1.0032e-06,uDot=2.00584e-06,f=-1.07003e-07,wcount_dh=-55604.5,rho_dh=-1.39011,wcount=40183.6,hDot=-0.00923729,smoothingLengthIterCount=0,balsara=0.799993,
 rot_v=8.2102e-06,div_v=-2.9864,v_sig_AV=0.0916537,soundSpeed=0.00105578,v_full=[-0.0149237,0.999888],u_full=1.0033e-06,hasNoNeighbours=0,isBoundaryParticle=0,partid=1,dependencyChecksPeanoEventUsedBySwift=touchVertexFirstTime,dependencyChecksAlgorithmStepLastUpdated=FlagBoundaryParticles,dependencyChecksA
 lgorithmStepUpdates=1,...,dependencyChecksAlgorithmStepMaskOuts=0,...,dependencyChecksInitStepLastUpdated=FlagBoundaryParticles,dependencyChecksInitStepUpdates=1,...,dependencyChecksInitStepMaskOuts=0,...) to next coarser vertex 1. Previously assigned to (cell-centre=[0.537037,0.12963],cell-h=[0.037037,0.
 037037],local=1111,hanging=0000,select=2,has-been-refined=0000,will-be-refined=0000,is-parent-vertex-local=1111,is-parent-cell-local=1,rel-pos-within-father=[2,0])
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * Then we lift again:
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 77935688595  00:01:17     rank:0       core:1       info         benchmarks::swift2::noh::observers::StepHydroPartDrift_sweep12peano4_toolbox_particles_api_UpdateParticleGridAssociation_LiftDrop1::touchVertexLastTime(...)                 have to lift particle (debugX=[0.505,0.165],debugH=[0,0],x=[0.50497
5,0.1667],cellH=[0.111111,0.111111],searchRadius=0.0185185,ParallelState=Local,NewParallelState=Local,MoveState=Moved,CellHasUpdatedParticle=1,cfl=0.1,initialTimeStepSize=0.0001,adjustTimeStepSize=0,hydroDimensions=2,etaFactor=1.2348,smlMin=1e-06,smlMax=0.166667,smlTolerance=0.0001,smlMaxIterations=30,alp
haAV=0.8,betaAV=3,mass=2.5e-05,v=[-0.0149237,0.999888],a=[7.92017e-06,-0.000467427],density=1.00459,pressure=6.71868e-07,smoothingLength=0.00615988,u=1.0032e-06,uDot=2.00584e-06,f=-1.07003e-07,wcount_dh=-55604.5,rho_dh=-1.39011,wcount=40183.6,hDot=-0.00923729,smoothingLengthIterCount=0,balsara=0.799993,ro
t_v=8.2102e-06,div_v=-2.9864,v_sig_AV=0.0916537,soundSpeed=0.00105578,v_full=[-0.0149237,0.999888],u_full=1.0033e-06,hasNoNeighbours=0,isBoundaryParticle=0,partid=1,dependencyChecksPeanoEventUsedBySwift=touchVertexFirstTime,dependencyChecksAlgorithmStepLastUpdated=FlagBoundaryParticles,dependencyChecksAlg
orithmStepUpdates=1,...,dependencyChecksAlgorithmStepMaskOuts=0,...,dependencyChecksInitStepLastUpdated=FlagBoundaryParticles,dependencyChecksInitStepUpdates=1,...,dependencyChecksInitStepMaskOuts=0,...) to next coarser vertex 1. Previously assigned to (cell-centre=[0.611111,0.166667],cell-h=[0.111111,0.1
11111],local=1111,hanging=0000,select=0,has-been-refined=1111,will-be-refined=1111,is-parent-vertex-local=1111,is-parent-cell-local=1,rel-pos-within-father=[2,1])
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * This is wrong, as we should have assigned to the vertex 3 in the first
   * step and actually uncovers a potential bug.
   *
   */
  void testLiftDropOfParticleAssociatedWithVertex02();


  /**
   * Another test from the Swift runs where seven particles disappeared
   *
   * This is the dump of the seven missing particles. We only focus on the
   * first one from hereon:
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- (HydroPart,[0.0555708,0.322228]): ->(moved-while-associated-to-vertex,[0.05,0.32]->x_new,tree=0,trace=substitute-for-whole-trajectory)->(detach-from-vertex,local=1,x=[0,0.333333],h=[0.111111,0.111111],tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)->(assign-to-sieve-set,local=1,tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)
- (HydroPart,[0.0556128,0.33212]): ->(moved-while-associated-to-vertex,[0.05,0.33]->x_new,tree=0,trace=substitute-for-whole-trajectory)->(detach-from-vertex,local=1,x=[0,0.333333],h=[0.111111,0.111111],tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)->(assign-to-sieve-set,local=1,tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)
- (HydroPart,[0.944203,0.618454]): ->(moved-while-associated-to-vertex,[0.95,0.62]->x_new,tree=0,trace=substitute-for-whole-trajectory)->(detach-from-vertex,local=1,x=[1,0.666667],h=[0.111111,0.111111],tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)->(assign-to-sieve-set,local=1,tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)
- (HydroPart,[0.944236,0.628335]): ->(moved-while-associated-to-vertex,[0.95,0.63]->x_new,tree=0,trace=substitute-for-whole-trajectory)->(detach-from-vertex,local=1,x=[1,0.666667],h=[0.111111,0.111111],tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)->(assign-to-sieve-set,local=1,tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)
- (HydroPart,[0.944271,0.638218]): ->(moved-while-associated-to-vertex,[0.95,0.64]->x_new,tree=0,trace=substitute-for-whole-trajectory)->(detach-from-vertex,local=1,x=[1,0.666667],h=[0.111111,0.111111],tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)->(assign-to-sieve-set,local=1,tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)
- (HydroPart,[0.944308,0.648103]): ->(moved-while-associated-to-vertex,[0.95,0.65]->x_new,tree=0,trace=substitute-for-whole-trajectory)->(detach-from-vertex,local=1,x=[1,0.666667],h=[0.111111,0.111111],tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)->(assign-to-sieve-set,local=1,tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)
- (HydroPart,[0.944347,0.65799]): ->(moved-while-associated-to-vertex,[0.95,0.66]->x_new,tree=0,trace=substitute-for-whole-trajectory)->(detach-from-vertex,local=1,x=[1,0.666667],h=[0.111111,0.111111],tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)->(assign-to-sieve-set,local=1,tree=0,trace=UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles)
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * This is due to lifts without the corresponding drops. We can see
   * the following line for the first particle:
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 482512398740 00:08:02     rank:0       core:7       info         benchmarks::swift2::noh::vertexdata::HydroPartSet::mergeWithParticle(...)                           inflying particle=(debugX=[0.05,0.32],debugH=[0,0],x=[0.0555708,0.322228],cellH=[0.111111,0.111111],searchRadius=0.0555556,ParallelState=Virtual,MoveState=Moved,CellHasUpdatedParticle=1,mass=0.0001,v=[0.928474,0.37139],a=[-0.000587972,-0.000240799],density=1.01022,pressure=6.76278e-07,smoothingLength=0.0122854,u=1.00416e-06,uDot=1.39669e-06,v_full=[0.928473,0.37139],u_full=1.00416e-06,wcount=10102.2,wcount_dh=-6995.6,f=-4.27191e-07,hDot=-0.0128069,rho_dh=-0.69956,smoothingLengthIterCount=0,hasNoNeighbours=0,isBoundaryParticle=0,partid=1,cfl=0.1,initialTimeStepSize=0.001,adjustTimeStepSize=0,hydroDimensions=2,etaFactor=1.2348,smlMin=1e-06,smlMax=0.166667,smlTolerance=0.0001,smlMaxIterations=30,alphaAV=0.8,betaAV=3,balsara=0.799889,rot_v=0.000278591,div_v=-2.07604,v_sig_AV=0.140208,soundSpeed=0.00105628,dependencyChecksPeanoEventUsedBySwift=touchVertexFirstTime,dependencyChecksAlgorithmStepLastUpdated=FlagBoundaryParticles,dependencyChecksAlgorithmStepUpdates=1,...,dependencyChecksAlgorithmStepMaskOuts=0,...,dependencyChecksInitStepLastUpdated=FlagBoundaryParticles,dependencyChecksInitStepUpdates=1,...,dependencyChecksInitStepMaskOuts=0,...) at vertex (cell-centre=[0.0555556,0.388889],cell-h=[0.111111,0.111111],local=1111,hanging=0000,select=1,has-been-refined=0000,will-be-refined=0000,is-parent-vertex-local=1111,is-parent-cell-local=1,rel-pos-within-father=[0,0],is-adjacent-cell-local=110110000,x(selected)=[0.111111,0.333333]) is new and will be remote on tree  1 (marker=(cell-centre=[0.0555556,0.388889],cell-h=[0.111111,0.111111],local=1111,hanging=0000,select=1,has-been-refined=0000,will-be-refined=0000,is-parent-vertex-local=1111,is-parent-cell-local=1,rel-pos-within-father=[0,0],is-adjacent-cell-local=110110000,x(selected)=[0.111111,0.333333]))
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * so the particle has been local on tree 0 and remote on tree 1, before it
   * started to move. Then it moves and is lifted. We obtain only one drop
   * after that. That drop yields a remote particle on tree 1, i.e. it seems
   * that it is really only dropped on tree 1. And on this tree 1, it is then
   * erased, as it is not local. What we'd like to see is that it is
   *
   * - dropped on both tree 0 and tree 1, with one of them yielding a local
   *   particle; or
   * - dropped on either of the trees where it yields a local particle.
   *
   * @image html MultiscaleTransitionsTest_testLiftDropOfParticleAssociatedWithVertex03.png
   *
   * After studying the setup, it became clear that the sorting per se was
   * correct: The problem here is that the particle should never have ended
   * up on the fine level anyway. If it is there, it should be lifted and
   * counted there, which it is not apparently. So the lift/drop behaves
   * correctly, but the constellation is ill-posed in the first place. If
   * we remove the search radius (as in the implementation of the test), the
   * code shows the correct behaviour.
   *
   * Once we have assessed the original setup, we therefore return to the
   * large search radius of 0.0555556
   *
   */
  void testLiftDropOfParticleAssociatedWithVertex03();


  /**
   * Another lift/drop test
   *
   * I found the following entry in the database after a particle had disappeared:
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- (HydroPart,[0.5,0.5]): ->(detach-from-vertex,local=1,x=[0.333333,0.333333],h=[0.333333,0.333333],tree=0,trace=UpdateParticleGridAssociation_BucketSort::__Template_LiftParticles)->(assign-to-sieve-set,local=1,tree=0,trace=UpdateParticleGridAssociation_BucketSort::__Template_LiftParticles) (file:assignmentchecks/TracingAPI.cpp,line:836)
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * This particle resides exactly in the centre of a cell and seems to be
   * lifted, which contradicts our working assumption that vertices are
   * careful with releasing particles and not aggressive when it comes to
   * grabbing them.
   *
   * The lift stems from this place:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 496656108    00:00:00     rank:0       core:1       info         benchmarks::swift2::noh::observers::StepHydroPartDrift_sweep12peano4_toolbox_particles_api_UpdateParticleGridAssociation_BucketSort0::touchVertexLastTime(...) lift particle (,x=[0.5,0.5],cellH=[0.333333,0.333333],searchRadius=0.15,ParallelState=Local,MoveState=Moved,CellHasUpdatedParticle=1,mass=0.01,v=[-3.51825e-06,3.5199e-06],a=[1.29168e-05,-9.76758e-05],density=1.01558,pressure=6.80968e-07,smoothingLength=0.12252,u=1.00578e-06,uDot=1.15969e-05,v_full=[-3.5176e-06,3.51502e-06],u_full=1.00578e-06,wcount=101.558,wcount_dh=-14.06,f=0,hDot=-1.06525,rho_dh=-0.1406,smoothingLengthIterCount=0,hasNoNeighbours=0,isBoundaryParticle=0,partid=1,cfl=0.1,initialTimeStepSize=0.0001,adjustTimeStepSize=0,hydroDimensions=2,etaFactor=1.2348,smlMin=1e-06,smlMax=0.166667,smlTolerance=0.0001,smlMaxIterations=30,alphaAV=0.8,betaAV=3,balsara=0.8,rot_v=4.39144e-07,div_v=-17.2853,v_sig_AV=3.00064,soundSpeed=0.00105714) globally. Previously assigned to (cell-centre=[0.166667,0.5],cell-h=[0.333333,0.333333],local=1011,hanging=0000,select=1,has-been-refined=0000,will-be-refined=0000,is-parent-vertex-local=1111,is-parent-cell-local=0,rel-pos-within-father=[0,1],is-adjacent-cell-local=000100110,x(selected)=[0.333333,0.333333])
 500006642    00:00:00     rank:0       core:2       info         benchmarks::swift2::noh::repositories::GlobalState::finishIntermediateStep() sort statistics: #lifts=0, #drops=0, #lifts-into-sieve-set=1, #drops-from-sieve-set=0, #drops-into-horizontal-tree-decomposition=0, #reassignments=0

   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * Funny enough, this particular bug did arise only for two threads, but
   * passed with eight. Digging into the output revealed that the inflying
   * particle seems to not to be local on either rank:
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 1616501080   00:00:01     rank:0       core:7       info         benchmarks::swift2::noh::observers::StepHydroPartDrift_sweep12peano4_toolbox_particles_api_UpdateParticleGridAssociation_BucketSort0::touchVertexLastTime(...) lift particle (,x=[0.5,0.5],cellH=[0.333333,0.333333],searchRadius=0.15,ParallelState=Local,MoveState=Moved,CellHasUpdatedParticle=1,mass=0.01,v=[-3.51825e-06,3.5199e-06],a=[1.29168e-05,-9.76758e-05],density=1.01558,pressure=6.80968e-07,smoothingLength=0.12252,u=1.00578e-06,uDot=1.15969e-05,v_full=[-3.5176e-06,3.51502e-06],u_full=1.00578e-06,wcount=101.558,wcount_dh=-14.06,f=0,hDot=-1.06525,rho_dh=-0.1406,smoothingLengthIterCount=0,hasNoNeighbours=0,isBoundaryParticle=0,partid=1,cfl=0.1,initialTimeStepSize=0.0001,adjustTimeStepSize=0,hydroDimensions=2,etaFactor=1.2348,smlMin=1e-06,smlMax=0.166667,smlTolerance=0.0001,smlMaxIterations=30,alphaAV=0.8,betaAV=3,balsara=0.8,rot_v=4.39144e-07,div_v=-17.2853,v_sig_AV=3.00064,soundSpeed=0.00105714) globally from tree 0. Previously assigned to (cell-centre=[0.166667,0.5],cell-h=[0.333333,0.333333],local=1011,hanging=0000,select=1,has-been-refined=0000,will-be-refined=0000,is-parent-vertex-local=1111,is-parent-cell-local=0,rel-pos-within-father=[0,1],is-adjacent-cell-local=000100110,x(selected)=[0.333333,0.333333])
 1620846099   00:00:01     rank:0       core:6       info         benchmarks::swift2::noh::observers::DummyStepHydroPartPredictHydro_sweep22peano4_toolbox_particles_api_UpdateParticleGridAssociation_BucketSort0::touchVertexFirstTime() now we drop particle (,x=[0.5,0.5],cellH=[0.333333,0.333333],searchRadius=0.15,ParallelState=Local,MoveState=Moved,CellHasUpdatedParticle=1,mass=0.01,v=[-3.51825e-06,3.5199e-06],a=[1.29168e-05,-9.76758e-05],density=1.01558,pressure=6.80968e-07,smoothingLength=0.12252,u=1.00578e-06,uDot=1.15969e-05,v_full=[-3.5176e-06,3.51502e-06],u_full=1.00578e-06,wcount=101.558,wcount_dh=-14.06,f=0,hDot=-1.06525,rho_dh=-0.1406,smoothingLengthIterCount=0,hasNoNeighbours=0,isBoundaryParticle=0,partid=1,cfl=0.1,initialTimeStepSize=0.0001,adjustTimeStepSize=0,hydroDimensions=2,etaFactor=1.2348,smlMin=1e-06,smlMax=0.166667,smlTolerance=0.0001,smlMaxIterations=30,alphaAV=0.8,betaAV=3,balsara=0.8,rot_v=4.39144e-07,div_v=-17.2853,v_sig_AV=3.00064,soundSpeed=0.00105714)
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   *
   */
  void testLiftDropOfParticleAssociatedWithVertex04();

public:
  MultiscaleTransitionsTest();
  virtual void run() override;
};
