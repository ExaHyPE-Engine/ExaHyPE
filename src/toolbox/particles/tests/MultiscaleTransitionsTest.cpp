#include "MultiscaleTransitionsTest.h"

#include "TestParticle.h"
#include "toolbox/particles/MultiscaleTransitions.h"


tarch::logging::Log toolbox::particles::tests::MultiscaleTransitionsTest::_log(
  "toolbox::particles::tests::MultiscaleTransitionsTest"
);

#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", off)
#endif

toolbox::particles::tests::MultiscaleTransitionsTest::MultiscaleTransitionsTest():
  TestCase("toolbox::particles::tests::MultiscaleTransitionsTest") {}

void toolbox::particles::tests::MultiscaleTransitionsTest::testSievePredicate() {
#if Dimensions == 2
  TestParticle                         particle({0.721487, 0.624479}, 0.0001);
  peano4::datamanagement::VertexMarker marker;

  marker._cellCentre = {0.685185, 0.648148};
  marker._h          = {0.037037, 0.037037};
  marker._isHanging  = 0;
  marker._select     = 1;
  marker._isAdjacentCellLocal.set();

  tarch::la::Vector<Dimensions, double> cornerVertex1 = {
    marker._cellCentre(0) + 0.5 * marker._h(0), marker._cellCentre(1) - 0.5 * marker._h(1)};
  //  (debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232)
  //  into
  // ([0.685185,0.648148],[0.037037,0.037037],hanging=0000,select=1)

  validateWithParams6(
    toolbox::particles::sieveParticle(particle, marker),
    marker.toString(),
    particle._x,
    cornerVertex1,
    marker._h / 2.0,
    tarch::la::abs(marker._cellCentre - particle._x),
    tarch::la::abs(cornerVertex1 - particle._x)
  );

  /*

    cannot confirm (yet) that particle is local on tree 1:
    particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232),
    cell=(x=[0.833333,0.833333],h=[0.333333,0.333333],has-been-refined=1,will-be-refined=1,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[2,2],has-been-enclave=0,will-be-enclave=0)
    cannot confirm (yet) that particle is local on tree 1:
    particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232),
    cell=(x=[0.722222,0.722222],h=[0.111111,0.111111],has-been-refined=1,will-be-refined=1,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[0,0],has-been-enclave=0,will-be-enclave=0)
    cannot confirm (yet) that particle is local on tree 1:
    particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232),
    cell=(x=[0.611111,0.722222],h=[0.111111,0.111111],has-been-refined=1,will-be-refined=1,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[2,0],has-been-enclave=0,will-be-enclave=0)
    cannot confirm (yet) that particle is local on tree 0:
    particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232),
    cell=(x=[0.685185,0.648148],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[0,2],has-been-enclave=0,will-be-enclave=0)
    cannot confirm (yet) that particle is local on tree 0:
    particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232),
    cell=(x=[0.722222,0.648148],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[1,2],has-been-enclave=0,will-be-enclave=0)
    cannot confirm (yet) that particle is local on tree 1:
    particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232),
    cell=(x=[0.611111,0.611111],h=[0.111111,0.111111],has-been-refined=1,will-be-refined=1,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[2,2],has-been-enclave=0,will-be-enclave=0)
    cannot confirm (yet) that particle is local on tree 0:
    particle=(debugX=[0.75,0.5],debugH=[0,0],x=[0.721487,0.624479],ParallelState=Local,searchRadius=0.0001,MoveState=Moved,CellHasUpdatedParticle=0,v=[-0.728231,1.39488],a=[-7.97473,-4.38055],energyKin=1.23859,energyPot=2.3109,energyTot=1.07232),
    cell=(x=[0.685185,0.611111],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,one-vertex-hanging=0,one-vertex-destroyed/created=0,all-vertices-inside-domain=0,no-lb=1,rel-pos=[0,1],has-been-enclave=0,will-be-enclave=0)

  */
#endif
}

void toolbox::particles::tests::MultiscaleTransitionsTest::testLiftDropOfParticleAssociatedWithVertex01() {
  #if Dimensions==2
  const int select = 0;
  peano4::datamanagement::VertexMarker marker;

  const double  meshSize = 0.111111;

  marker._cellCentre = {0.611111, 0.166667};
  marker._h          = {meshSize, meshSize};
  marker._isHanging  = 0;
  marker._hasBeenRefined = 1+2+4+8;
  marker._willBeRefined  = 1+2+4+8;
  marker._isParentVertexLocal = 1+2+4+8;
  marker._isParentCellLocal   = true;
  marker._relativePositionOfCellWithinFatherCell = {0,0};
  marker._isAdjacentCellLocal.set();

  tarch::la::Vector<Dimensions,double> x = {0.554725,0.166678};
  const double                         searchRadius = 0.0185185;
  toolbox::particles::ParticleReassociationInstruction liftInstruction;
  bool                                                 drop;

  // bottom left vertex owns particle
  marker._select     = 0;
  liftInstruction = toolbox::particles::liftParticleAssociatedWithVertex(
    true,                  // ParallelState=Local
    searchRadius,
    x,
    marker
  );
  drop = toolbox::particles::dropParticle(
    searchRadius,
    x,
    marker
  );

/*
  validateWithParams7(
    liftInstruction==0, // that means we lift
    marker.toString(),
    x,
    liftInstruction, drop,
    (x-marker.x()),
    searchRadius,
    tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
*/
  validateWithParams6(
    not drop,           // but we do not drop
    marker.toString(),
    x,
    liftInstruction, drop,
    (x-marker.x()),
    searchRadius
  );

  // top left vertex owns particle
  marker._select     = 2;
  liftInstruction = toolbox::particles::liftParticleAssociatedWithVertex(
    true,                  // ParallelState=Local
    searchRadius,
    x,
    marker
  );
  drop = toolbox::particles::dropParticle(
    searchRadius,
    x,
    marker
  );

  validateWithParams6(
    liftInstruction==ParticleReassociationInstruction_Keep, // keep it
    marker.toString(),
    x,
    liftInstruction, drop,
    (x-marker.x()),
    searchRadius
  );
  validateWithParams6(
    drop,  // but please drop
    marker.toString(),
    x,
    liftInstruction, drop,
    (x-marker.x()),
    searchRadius
  );
  #endif
}

void toolbox::particles::tests::MultiscaleTransitionsTest::run() {
  testMethod(testSievePredicate);
  testMethod(testLiftDropOfParticleAssociatedWithVertex01);
  testMethod(testLiftDropOfParticleAssociatedWithVertex02);
  testMethod(testLiftDropOfParticleAssociatedWithVertex03);
  testMethod(testLiftDropOfParticleAssociatedWithVertex04);
}

void toolbox::particles::tests::MultiscaleTransitionsTest::testLiftDropOfParticleAssociatedWithVertex02() {
  #if Dimensions==2
  const int select = 0;
  peano4::datamanagement::VertexMarker marker;

  const double  meshSize = 0.037037;

  marker._cellCentre = {0.537037,0.12963};
  marker._h          = {meshSize, meshSize};
  marker._isHanging  = 0;
  marker._hasBeenRefined = 0;
  marker._willBeRefined  = 0;
  marker._isParentVertexLocal = 1+2+4+8;
  marker._isParentCellLocal   = true;
  marker._relativePositionOfCellWithinFatherCell = {2,0};
  marker._select     = 2;
  marker._isAdjacentCellLocal.set();

  tarch::la::Vector<Dimensions,double> x = {0.504975,0.1667};
  const double                         searchRadius = 0.0185185;
  toolbox::particles::ParticleReassociationInstruction liftInstruction;
  bool                                                 drop;

  // top left vertex owns particle, but it is even further left up than the
  // owning vertex. So lift.
  liftInstruction = toolbox::particles::liftParticleAssociatedWithVertex(
    true,                  // ParallelState=Local
    searchRadius,
    x,
    marker
  );
  drop = toolbox::particles::dropParticle(
    searchRadius,
    x,
    marker
  );

  validateWithParams7(
    liftInstruction==3, // that means we lift if we don't reassociate within a vertex
    marker.toString(),
    x,
    marker.x(),
    liftInstruction, drop,
    (x-marker.x()),
    searchRadius
  );
  validateWithParams6(
    not drop,           // but we do not drop
    marker.toString(),
    x,
    liftInstruction, drop,
    (x-marker.x()),
    searchRadius
  );
  #endif
}


void toolbox::particles::tests::MultiscaleTransitionsTest::testLiftDropOfParticleAssociatedWithVertex03() {
  #if Dimensions==2
  peano4::datamanagement::VertexMarker marker;

  const double  meshSize = 1.0/3.0/3.0;

  marker._cellCentre = {0.5 * meshSize, 3.5 * meshSize};
  marker._h          = {meshSize, meshSize};
  marker._isLocal    = 0b1111;
  marker._isHanging  = 0b0000;
  marker._hasBeenRefined = 0b0000;
  marker._willBeRefined  = 0b0000;
  marker._isParentVertexLocal = 0b1111;
  marker._isParentCellLocal   = true;
  marker._relativePositionOfCellWithinFatherCell = {0,0};
  marker._isAdjacentCellLocal  = 0b110110000;

  double searchRadius = 0.00555556;
  tarch::la::Vector<Dimensions,double> x = {0.0555708, 0.322228};
  toolbox::particles::ParticleReassociationInstruction liftInstruction;

  // Vertex 0 should trigger a lift globally => we see this in the trace
  marker._select  = 0;
  liftInstruction = liftParticleAssociatedWithVertex(true, searchRadius, x, marker);
  validateEqualsWithParams1(liftInstruction, toolbox::particles::ParticleReassociationInstruction_SieveGlobally, marker.toString());

  // Vertex 1 would have kept particle => consistency check (not observed, but will arise later)
  marker._select  = 1;
  liftInstruction = liftParticleAssociatedWithVertex(true, searchRadius, x, marker);
  validateEqualsWithParams5(
    liftInstruction, toolbox::particles::ParticleReassociationInstruction_Keep,
    internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(x, 0.5, internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    2.0 * searchRadius,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // Vertex 0 should not drop
  marker._select  = 0;
  validateWithParams5(
    not dropParticle(searchRadius,x,marker),
    liftParticleAssociatedWithVertex(true, searchRadius, x, marker),
    internal::fitsIntoLevel(searchRadius, marker),
    internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // Vertex 1 should not drop
  marker._select  = 1;
  validateWithParams5(
    dropParticle(searchRadius,x,marker),
    liftParticleAssociatedWithVertex(true, searchRadius, x, marker),
    internal::fitsIntoLevel(searchRadius, marker),
    internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // And now we study the same thing from the boundary cell below on the other
  // rank
  marker._cellCentre(1) -= meshSize;
  marker._isAdjacentCellLocal.flip();

  // Vertex 2 should not drop
  marker._select  = 2;
  validateWithParams1(not dropParticle(searchRadius,x,marker), marker.toString());

  // Vertex 3 should not drop
  marker._select  = 3;
  validateWithParams1(dropParticle(searchRadius,x,marker), marker.toString());


  // Roll back and see if sieving works
  marker._cellCentre(0) -=   meshSize;
  searchRadius = 0.0555556;

  // Vertex 0 should trigger a lift globally - this time due to the size of the
  // particle and the distance
  marker._select  = 0;
  liftInstruction = liftParticleAssociatedWithVertex(true, searchRadius, x, marker);
  validateEqualsWithParams1(liftInstruction, toolbox::particles::ParticleReassociationInstruction_SieveGlobally, marker.toString());

  // Vertex 1 would not keep particle either as it is too big
  marker._select  = 1;
  liftInstruction = liftParticleAssociatedWithVertex(true, searchRadius, x, marker);
  validateEqualsWithParams5(
    liftInstruction, toolbox::particles::ParticleReassociationInstruction_SieveGlobally,
    internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(x, 0.5, internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    2.0 * searchRadius,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // Next coarser mesh level
  marker._cellCentre     = {1.5*meshSize, 1.5*meshSize};
  marker._select         = 2;
  marker._h              = {3.0*meshSize, 3.0*meshSize};

  validateWithParams5(
    sieveParticle(searchRadius, x, marker),
    internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(x, 0.5, internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    2.0 * searchRadius,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  #endif
}



void toolbox::particles::tests::MultiscaleTransitionsTest::testLiftDropOfParticleAssociatedWithVertex04() {
  #if Dimensions==2
  const int select = 0;
  peano4::datamanagement::VertexMarker marker;

  const double  meshSize = 1.0/3.0;

  marker._cellCentre = {0.166667,0.5};
  marker._h          = {meshSize, meshSize};
  marker._isHanging  = 0;
  marker._hasBeenRefined = 0;
  marker._willBeRefined  = 0;
  marker._isParentVertexLocal = 0b1011;
  marker._isParentCellLocal   = true;
  marker._relativePositionOfCellWithinFatherCell = {2,0};
  marker._select     = 2;
  marker._isAdjacentCellLocal = 0b000100110;

  tarch::la::Vector<Dimensions,double> x = {4.99999996463071805231e-01, 5.00000003548028204570e-01};
  const double                         searchRadius = 0.15;
  toolbox::particles::ParticleReassociationInstruction liftInstruction;
  bool                                                 drop;

  // Vertex 1 triggers a global lift
  marker._select  = 1;
  liftInstruction = liftParticleAssociatedWithVertex(true, searchRadius, x, marker);
  validateEqualsWithParams1(liftInstruction, toolbox::particles::ParticleReassociationInstruction_SieveGlobally, marker.toString());

  validateWithParams5(
    not sieveParticle(searchRadius, x, marker),
    internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(x, 0.5, internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    2.0 * searchRadius,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  marker._select  = 3;
  validateWithParams5(
    sieveParticle(searchRadius, x, marker),
    internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(x, 0.5, internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    2.0 * searchRadius,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // look into cell to the right
  marker._cellCentre(0) += meshSize;
  marker._isAdjacentCellLocal = 0b000110111;

  marker._select  = 0;
  validateWithParams5(
    not sieveParticle(searchRadius, x, marker),
    internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(x, 0.5, internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    2.0 * searchRadius,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  marker._select  = 2;
  validateWithParams5(
    sieveParticle(searchRadius, x, marker),
    internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(x, 0.5, internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    2.0 * searchRadius,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // study diagonal right top cell
  marker._cellCentre(1) += meshSize;
  marker._isAdjacentCellLocal = 0b000000110;

  marker._select  = 0;
  validateWithParams5(
    sieveParticle(searchRadius, x, marker),
    internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(x, 0.5, internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    2.0 * searchRadius,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );


  // Same overall test on other rank: start with left cell
  marker._cellCentre = {0.166667,0.5};
  marker._isAdjacentCellLocal = 0b000100110;
  marker._isAdjacentCellLocal.flip();

  marker._select  = 1;
  validateWithParams5(
    not sieveParticle(searchRadius, x, marker),
    internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(x, 0.5, internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    2.0 * searchRadius,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  marker._select  = 3;
  validateWithParams5(
    sieveParticle(searchRadius, x, marker),
    internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(x, 0.5, internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    2.0 * searchRadius,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // study diagonal right top cell
  marker._cellCentre(0) += meshSize;
  marker._cellCentre(1) += meshSize;
  marker._isAdjacentCellLocal = 0b000000110;
  marker._isAdjacentCellLocal.flip();

  marker._select  = 0;
  validateWithParams5(
    sieveParticle(searchRadius, x, marker),
    internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(x, 0.5, internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    2.0 * searchRadius,
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );
  #endif
}

#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", on)
#endif
