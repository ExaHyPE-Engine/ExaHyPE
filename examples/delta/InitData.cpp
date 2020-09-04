#include "InitData.h"
#include "CellData.h"

#include "peano4/utils/Loop.h"


#include "tarch/multicore/Lock.h"



#if Dimensions==3
#include "delta/ContactPoint.h"
#include "delta/io/vtk.h"
#include "delta/contactdetection/filter.h"
#include "delta/contactdetection/sphere.h"
#include "delta/math.h"
#include "delta/primitives/Cube.h"
#include "delta/primitives/Fault.h"
#endif


tarch::logging::Log examples::delta::InitData::_log( "examples::delta::InitData" );


#if Dimensions==3
examples::delta::InitData::InitData():
  _primitive( nullptr ) {

  _primitive = new ::delta::primitives::Fault();

/*
  _primitive = new ::delta::primitives::Cube(
    0.5, 0.5, 0.5, // centre
    0.2            // h
  );
*/

  ::delta::io::writeVTK(
    _primitive->getNumberOfTriangles(),
    _primitive->getXCoordinates(),
    _primitive->getYCoordinates(),
    _primitive->getZCoordinates(),
    "geometry.vtk"
  );
}
#else
examples::delta::InitData::InitData() {
}
#endif


examples::delta::InitData::~InitData() {
  #if Dimensions==3
  delete _primitive;
  #endif
}


void examples::delta::InitData::beginTraversal() {
}


void examples::delta::InitData::endTraversal() {
}


void examples::delta::InitData::createCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {
  tarch::la::Vector<Dimensions,double> patchH = (1.0 / CellData::DoFsPerAxis) * h;
  tarch::la::Vector<Dimensions,double> offset = -0.5 * h + 0.5 * patchH;

  const double epsilon = h(0) / CellData::DoFsPerAxis;

  int currentEntry = 0;
  // @todo gleich Parallel
  dfor(i,CellData::DoFsPerAxis) {
    tarch::la::Vector<Dimensions,double> x = center + offset + tarch::la::multiplyComponents(tarch::la::convertScalar<double>(i),patchH);

    #if Dimensions==3
    std::vector<::delta::ContactPoint> contactPoints =
      ::delta::contactdetection::filter(
        ::delta::contactdetection::sphereToTriangle(
          x(0),x(1),x(2),
          0.0, // radius. This one is degenerated
          -1,  // Don't need indices here
          _primitive->getNumberOfTriangles(),
          _primitive->getXCoordinates(),
          _primitive->getYCoordinates(),
          _primitive->getZCoordinates(),
          nullptr, // not interested in indices here
          epsilon // epsilon
        ),
        epsilon
      );

    if (contactPoints.empty()) {
      data.valueX[currentEntry] = 0.0;
      data.valueY[currentEntry] = 0.0;
      data.valueZ[currentEntry] = 0.0;
    }
    else {
      //data.valueX[currentEntry] = contactPoints[0].distance;
      const double distance = contactPoints[0].distance;
      data.valueX[currentEntry] = distance * contactPoints[0].normal[0];
      data.valueY[currentEntry] = distance * contactPoints[0].normal[1];
      data.valueZ[currentEntry] = distance * contactPoints[0].normal[2];
      logDebug( "createCell(...)", "contact for point " << x << ": " << contactPoints[0].toString() );
    }
    #else
     data.valueX[currentEntry] = x(0)-0.5;
     data.valueY[currentEntry] = x(1)-0.5;
     data.valueZ[currentEntry] = 0.0;
    #endif

    currentEntry++;
  }
}


void examples::delta::InitData::destroyCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {}


void examples::delta::InitData::touchCellFirstTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {
}


void examples::delta::InitData::touchCellLastTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {
}

