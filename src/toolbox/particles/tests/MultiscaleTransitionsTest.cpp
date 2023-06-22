#include "MultiscaleTransitionsTest.h"
#include "TestParticle.h"

#include "toolbox/particles/MultiscaleTransitions.h"


tarch::logging::Log toolbox::particles::tests::MultiscaleTransitionsTest::_log( "toolbox::particles::tests::MultiscaleTransitionsTest" );


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",off)
#endif


toolbox::particles::tests::MultiscaleTransitionsTest::MultiscaleTransitionsTest():
  TestCase("toolbox::particles::tests::MultiscaleTransitionsTest") {
}


void toolbox::particles::tests::MultiscaleTransitionsTest::testSievePredicate() {
  #if Dimensions==2
  TestParticle particle( {0.721487,0.624479}, 0.0001 );
  peano4::datamanagement::VertexMarker marker;

  marker._cellCentre = {0.685185,0.648148};
  marker._h          = {0.037037,0.037037};
  marker._isHanging  = 0;
  marker._select     = 1;


  tarch::la::Vector<Dimensions, double> cornerVertex1 = {
    marker._cellCentre(0) + 0.5 * marker._h(0),
    marker._cellCentre(1) - 0.5 * marker._h(1)
  };
//  (debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232) into
// ([0.685185,0.648148],[0.037037,0.037037],hanging=0000,select=1)

  validateWithParams6(
    toolbox::particles::sieveParticle(particle,marker),
    marker.toString(),
    particle._x,
    cornerVertex1,
    marker._h / 2.0,
    tarch::la::abs(marker._cellCentre-particle._x),
    tarch::la::abs(cornerVertex1-particle._x)
  );

/*

  cannot confirm (yet) that particle is local on tree 1: particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232), cell=(x=[0.833333,0.833333],h=[0.333333,0.333333],has-been-refined=1,will-be-refined=1,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[2,2],has-been-enclave=0,will-be-enclave=0)
  cannot confirm (yet) that particle is local on tree 1: particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232), cell=(x=[0.722222,0.722222],h=[0.111111,0.111111],has-been-refined=1,will-be-refined=1,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[0,0],has-been-enclave=0,will-be-enclave=0)
  cannot confirm (yet) that particle is local on tree 1: particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232), cell=(x=[0.611111,0.722222],h=[0.111111,0.111111],has-been-refined=1,will-be-refined=1,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[2,0],has-been-enclave=0,will-be-enclave=0)
  cannot confirm (yet) that particle is local on tree 0: particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232), cell=(x=[0.685185,0.648148],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[0,2],has-been-enclave=0,will-be-enclave=0)
  cannot confirm (yet) that particle is local on tree 0: particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232), cell=(x=[0.722222,0.648148],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[1,2],has-been-enclave=0,will-be-enclave=0)
  cannot confirm (yet) that particle is local on tree 1: particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232), cell=(x=[0.611111,0.611111],h=[0.111111,0.111111],has-been-refined=1,will-be-refined=1,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[2,2],has-been-enclave=0,will-be-enclave=0)
  cannot confirm (yet) that particle is local on tree 0: particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232), cell=(x=[0.685185,0.611111],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[0,1],has-been-enclave=0,will-be-enclave=0)

*/
  #endif
}


void toolbox::particles::tests::MultiscaleTransitionsTest::run() {
  testMethod( testSievePredicate );
}


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif
