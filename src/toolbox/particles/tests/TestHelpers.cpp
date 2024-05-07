#include "TestHelpers.h"

#include "peano4/datamanagement/VertexMarker.h"

#include "toolbox/particles/particles.h"


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", off)
#endif

toolbox::particles::tests::TestHelpers::TestHelpers():
  TestCase("toolbox::particles::tests::TestHelpers") {}


void toolbox::particles::tests::TestHelpers::testParticleAssignedToVertexWillBeLocal() {
  #if Dimensions==2
  peano4::datamanagement::VertexMarker markerAFromYellowRank;
  peano4::datamanagement::VertexMarker markerBFromYellowRank;
  peano4::datamanagement::VertexMarker markerCFromYellowRank;
  peano4::datamanagement::VertexMarker markerDFromYellowRank;
  peano4::datamanagement::VertexMarker markerAFromGreyRank;
  peano4::datamanagement::VertexMarker markerBFromGreyRank;
  peano4::datamanagement::VertexMarker markerCFromGreyRank;
  peano4::datamanagement::VertexMarker markerDFromGreyRank;

  double meshSize = 1.0 / 3.0 / 3.0;

  // assigned to vertex above

  markerAFromYellowRank._cellCentre = {meshSize * 4.5, meshSize * 5.5};
  markerAFromYellowRank._h          = {meshSize, meshSize};
  markerAFromYellowRank._isLocal    = 0b0010;
  markerAFromYellowRank._isHanging  = 0;
  markerAFromYellowRank._select     = 1;
  markerAFromYellowRank._hasBeenRefined = 0;
  markerAFromYellowRank._willBeRefined  = 0;
  markerAFromYellowRank._isAdjacentCellLocal = 0b111000100;

  markerBFromYellowRank._cellCentre = {meshSize * 5.5, meshSize * 5.5};
  markerBFromYellowRank._h          = {meshSize, meshSize};
  markerBFromYellowRank._isLocal    = 0b0011;
  markerBFromYellowRank._isHanging  = 0;
  markerBFromYellowRank._select     = 0;
  markerBFromYellowRank._hasBeenRefined = 0;
  markerBFromYellowRank._willBeRefined  = 0;
  markerBFromYellowRank._isAdjacentCellLocal = 0b111000010;

  markerCFromYellowRank._cellCentre = {meshSize * 4.5, meshSize * 4.5};
  markerCFromYellowRank._h          = {meshSize, meshSize};
  markerCFromYellowRank._isLocal    = 0b1011;
  markerCFromYellowRank._isHanging  = 0;
  markerCFromYellowRank._select     = 3;
  markerCFromYellowRank._hasBeenRefined = 0;
  markerCFromYellowRank._willBeRefined  = 0;
  markerCFromYellowRank._isAdjacentCellLocal = 0b000100111;

  markerDFromYellowRank._cellCentre = {meshSize * 5.5, meshSize * 4.5};
  markerDFromYellowRank._h          = {meshSize, meshSize};
  markerDFromYellowRank._isLocal    = 0b1111;
  markerDFromYellowRank._isHanging  = 0;
  markerDFromYellowRank._select     = 2;
  markerDFromYellowRank._hasBeenRefined = 0;
  markerDFromYellowRank._willBeRefined  = 0;
  markerDFromYellowRank._isAdjacentCellLocal = 0b000010011;

  markerAFromGreyRank = markerAFromYellowRank;
  markerAFromGreyRank._isAdjacentCellLocal.flip();
  markerAFromGreyRank._isLocal = 0b1111;
  markerBFromGreyRank = markerBFromYellowRank;
  markerBFromGreyRank._isAdjacentCellLocal.flip();
  markerBFromGreyRank._isLocal = 0b1111;
  markerCFromGreyRank = markerCFromYellowRank;
  markerCFromGreyRank._isAdjacentCellLocal.flip();
  markerCFromGreyRank._isLocal = 0b1111;
  markerDFromGreyRank = markerDFromYellowRank;
  markerDFromGreyRank._isAdjacentCellLocal.flip();
  markerDFromGreyRank._isLocal = 0b1101;

  tarch::la::Vector<Dimensions, double> x1 = {0.545015,0.5};
  tarch::la::Vector<Dimensions, double> x2 = {0.554996,0.5};

  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerAFromYellowRank ), x1, markerAFromYellowRank.toString(), markerAFromYellowRank.x() );
  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerAFromYellowRank ), x2, markerAFromYellowRank.toString(), markerAFromYellowRank.x() );

  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerBFromYellowRank ), x1, markerBFromYellowRank.toString(), markerBFromYellowRank.x() );
  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerBFromYellowRank ), x2, markerBFromYellowRank.toString(), markerBFromYellowRank.x() );

  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerCFromYellowRank ), x1, markerCFromYellowRank.toString(), markerCFromYellowRank.x() );
  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerCFromYellowRank ), x2, markerCFromYellowRank.toString(), markerCFromYellowRank.x() );

  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerDFromYellowRank ), x1, markerDFromYellowRank.toString(), markerDFromYellowRank.x() );
  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerDFromYellowRank ), x2, markerDFromYellowRank.toString(), markerDFromYellowRank.x() );


  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerAFromGreyRank ), x1, markerAFromGreyRank.toString(), markerAFromGreyRank.x() );
  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerAFromGreyRank ), x2, markerAFromGreyRank.toString(), markerAFromGreyRank.x() );

  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerBFromGreyRank ), x1, markerBFromGreyRank.toString(), markerBFromGreyRank.x() );
  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerBFromGreyRank ), x2, markerBFromGreyRank.toString(), markerBFromGreyRank.x() );

  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerCFromGreyRank ), x1, markerCFromGreyRank.toString(), markerCFromGreyRank.x() );
  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerCFromGreyRank ), x2, markerCFromGreyRank.toString(), markerCFromGreyRank.x() );

  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerDFromGreyRank ), x1, markerDFromGreyRank.toString(), markerDFromGreyRank.x() );
  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerDFromGreyRank ), x2, markerDFromGreyRank.toString(), markerDFromGreyRank.x() );

  markerCFromYellowRank._select = 1;
  markerCFromGreyRank._select   = 1;
  markerDFromYellowRank._select = 0;
  markerDFromGreyRank._select   = 0;

  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerCFromYellowRank ), x1, markerCFromYellowRank.toString(), markerCFromYellowRank.x() );
  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerCFromYellowRank ), x2, markerCFromYellowRank.toString(), markerCFromYellowRank.x() );

  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerDFromYellowRank ), x1, markerDFromYellowRank.toString(), markerDFromYellowRank.x() );
  validateWithParams3( not toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerDFromYellowRank ), x2, markerDFromYellowRank.toString(), markerDFromYellowRank.x() );

  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerCFromGreyRank ), x1, markerCFromGreyRank.toString(), markerCFromGreyRank.x() );
  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerCFromGreyRank ), x2, markerCFromGreyRank.toString(), markerCFromGreyRank.x() );

  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x1, markerDFromGreyRank ), x1, markerDFromGreyRank.toString(), markerDFromGreyRank.x() );
  validateWithParams3( toolbox::particles::particleAssignedToVertexWillBeLocal( x2, markerDFromGreyRank ), x2, markerDFromGreyRank.toString(), markerDFromGreyRank.x() );
  #endif
}


void toolbox::particles::tests::TestHelpers::run() {
  testMethod(testParticleAssignedToVertexWillBeLocal);
}

#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", on)
#endif
