#include "TestHelpers.h"
#include "TestParticle.h"

#include "toolbox/particles/assignmentchecks/TracingAPI.h"
#include "toolbox/particles/assignmentchecks/Utils.h"


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", off)
#endif

toolbox::particles::assignmentchecks::tests::TestHelpers::TestHelpers():
  TestCase("toolbox::particles::assignmentchecks::tests::TestHelpers") {}


bool toolbox::particles::assignmentchecks::tests::internal::liftParticle(
    const tarch::la::Vector<Dimensions, double> particleX,
    const tarch::la::Vector<Dimensions, double> vertexX,
    const tarch::la::Vector<Dimensions, double> vertexH,
    const int depth
    ){

    if (depth == 0) return false;

    tarch::la::Vector<Dimensions, double> dv = vertexX - particleX;

    if (tarch::la::oneSmaller(dv, -1. * vertexH)) return true;
    if (tarch::la::oneGreater(dv, vertexH)) return true;
    return false;
}


bool toolbox::particles::assignmentchecks::tests::internal::dropParticle(
    const tarch::la::Vector<Dimensions, double> particleX,
    const tarch::la::Vector<Dimensions, double> vertexH,
    const int depth
    ){

  if (depth == maxVertexDepth) return false;

  tarch::la::Vector<Dimensions, double> vHnew = vertexH / 3.;
  tarch::la::Vector<Dimensions, double> vertexX = findVertexX(particleX, vHnew);

  tarch::la::Vector<Dimensions, double> dv = vertexX - particleX;

  if (tarch::la::allSmaller(dv, vHnew) and tarch::la::allGreater(dv, -1. * vHnew))
    return true;
  return false;
}


int toolbox::particles::assignmentchecks::tests::internal::findVertexInd(double x, double vertexH){

  int vertexInd = static_cast<int>(std::floor( (x / vertexH) + 0.5));

  return vertexInd;
}



/**
 * Find position of vertex with size `vertexH` covering position `x`
 * assuming they all start at (0, 0, 0).
 */
tarch::la::Vector<Dimensions, double> toolbox::particles::assignmentchecks::tests::internal::findVertexX(
    const tarch::la::Vector<Dimensions, double> x,
    const tarch::la::Vector<Dimensions, double> vertexH
  ){

  tarch::la::Vector<Dimensions, double> vertexX;

  for (int i = 0; i < Dimensions; i++){
    int vertexInd = internal::findVertexInd(x(i), vertexH(i));
    vertexX(i) = vertexInd * vertexH(i);
  }

  return vertexX;
}


void toolbox::particles::assignmentchecks::tests::TestHelpers::testParticleWalkSameTreeLevel() {

#if PeanoDebug > 0
  const bool verbose = false;

  namespace ac = toolbox::particles::assignmentchecks;

  if (verbose) std::cout << ">>>>>>>>>>>> RUNNING testParticleWalkSameTreeLevel() in " << Dimensions << "D" << std::endl;

  ac::internal::Database& eventDatabase = ac::getDatabase();

  // IMPORTANT: Needs to be the same as the particle class name you use!
  // In traceParticleMovements(), this will be derived from the class name.
  std::string particleName = "TestParticle";

  // coordinates of particles
  tarch::la::Vector<Dimensions, double> localPartX;
  // particle displacement each step
  tarch::la::Vector<Dimensions, double> dx;
  // Assume we have vertices of a single size.
  tarch::la::Vector<Dimensions, double> vertexH;
  tarch::la::Vector<Dimensions, double> vertexX;
  tarch::la::Vector<Dimensions, double> prevVertexX;

  // Initialise values
  double vertexH_default = 3.;
  double dt = 1.;

  for (int i = 0; i < Dimensions; i++){
    localPartX(i) = 51.5;
    dx(i) = -1.;
    vertexH(i) = vertexH_default;
  }

  int particleID = 1;
  int spacetreeId = 1;

  // generate (minimal) particle
  ac::tests::TestParticle localPart = ac::tests::TestParticle(localPartX, particleID);
  // pretend to have a ParticleContainer
  std::vector<ac::tests::TestParticle*> assignedParticles = {&localPart};

  // Find initial pseudo-"vertex" particle is assigned to
  vertexX = internal::findVertexX(localPartX, vertexH);
  // Keep track of vertex index for identifiaction
  int vertexInd = internal::findVertexInd(localPartX(0), vertexH(0));

  // First assignments
  ac::startMeshSweep( "InitUnitTest" );
  ac::assignParticleToVertex(
      particleName,
      localPartX,
      particleID,
      true, // this particle is always local
      vertexX,
      vertexH,
      spacetreeId,
      "initialAssign",
      false, // not a new particle
      true   // particle walks from vertex to vertex on same level
  );


  // Do some mesh sweeps.
  // NOTE: The database only keeps up to 16 sweeps in memory by default.
  // After that, entries will be purged. One sweep is added by default ('init').
  // We've already added another sweep ('InitUnitTest'). So if you go beyond 14,
  // it'll start deleting stuff before the end.
  for (int sweep = 0; sweep < 14; sweep++){

    ac::startMeshSweep( "MySweep" + std::to_string(sweep) );

    // record old particle positions.
    auto oldParticlePositions = ac::recordParticlePositions(assignedParticles);
    int prevVertexInd = vertexInd;

    // insert tiny movement steps, make sure movement isn't recorded.
    double dt_use = dt;
    if (sweep > 8 and sweep < 12) {
      dt_use *= ac::internal::ParticleIdentifier::Precision * 1e-2;
    }

    // update particle position.
    localPartX += dt_use * dx;
    localPart.setX(localPartX);

    // Trace the movement.
    ac::traceParticleMovements(assignedParticles, oldParticlePositions, vertexH, spacetreeId);
    if (verbose)
      std::cout << ">>>>>>>>>>>> Moving particle to " << localPartX << std::endl;

    // pretend a particle can walk from vertex to vertex.
    // find current vertex particle is assigned to.
    vertexInd = internal::findVertexInd(localPartX(0), vertexH(0));
    for (int i = 0; i < Dimensions; i++){
      prevVertexX(i) = vertexX(i);
    }
    vertexX = internal::findVertexX(localPartX, vertexH);

    if (verbose) std::cout << ">>>>>>>>>>>> Checking vertex" << std::endl;

    if (vertexInd != prevVertexInd){
      // Particle has changed vertex. Take note of that.
      if (verbose) std::cout << ">>>>>>>>>>>> Detaching vertex" << std::endl;
      ac::detachParticleFromVertex(
          particleName,
          localPart.getX(),
          localPart.getPartid(),
          true, // this particle is always local
          prevVertexX,
          vertexH,
          spacetreeId,
          "vertexChangeDetach"
      );

      if (verbose) std::cout << ">>>>>>>>>>>> Attaching vertex" << std::endl;
      ac::assignParticleToVertex(
          particleName,
          localPart.getX(),
          localPart.getPartid(),
          true, // this particle is always local
          vertexX,
          vertexH,
          spacetreeId,
          "vertexChangeAssign",
          false, // not a new particle
          true   // particle walks from vertex to vertex on same level
      );
    }
  }

  if (verbose) {
    std::cout << ">>>>>>>>>>>> DATA DUMP END " << std::endl;
    std::cout << eventDatabase.toString() << std::endl;
  }
#endif
}




void toolbox::particles::assignmentchecks::tests::TestHelpers::testParticleWalk() {

#if PeanoDebug > 0
  const bool verbose = true;

  namespace ac = toolbox::particles::assignmentchecks;

  if (verbose) std::cout << ">>>>>>>>>>>> RUNNING testParticleWalk() in " << Dimensions << "D" << std::endl;

  ac::internal::Database& eventDatabase = ac::getDatabase();

  // IMPORTANT: Needs to be the same as the particle class name you use!
  // In traceParticleMovements(), this will be derived from the class name.
  std::string particleName = "TestParticle";

  // coordinates of particles
  tarch::la::Vector<Dimensions, double> localPartX;
  // particle displacement each step
  tarch::la::Vector<Dimensions, double> dx;
  // coords of the boundary. Should in principle be a plane,
  // but a point does the trick here.
  // Assume we have vertices of a single size.
  tarch::la::Vector<Dimensions, double> vertexH, prevVertexH;
  tarch::la::Vector<Dimensions, double> vertexX, prevVertexX;

  // Initialise values
  double vertexH_default = 3.;
  double dt = 1.;
  int depth = 4;

  for (int i = 0; i < Dimensions; i++){
    localPartX(i) = 51.5;
    dx(i) = -1.;
    vertexH(i) = vertexH_default;
  }

  int particleID = 1;
  int spacetreeId = 1;

  // generate (minimal) particle
  ac::tests::TestParticle localPart = ac::tests::TestParticle(localPartX, particleID);
  // pretend to have a ParticleContainer
  std::vector<ac::tests::TestParticle*> assignedParticles = {&localPart};

  // Find initial pseudo-"vertex" particle is assigned to.
  vertexX = internal::findVertexX(localPartX, vertexH);
  // Keep track of vertex index for identifiaction
  int vertexInd = internal::findVertexInd(localPartX(0), vertexH(0));

  // First assignments
  ac::startMeshSweep( "InitUnitTest" );
  ac::assignParticleToVertex(
      particleName,
      localPartX,
      particleID,
      true, // this particle is always local
      vertexX,
      vertexH,
      spacetreeId,
      "initialAssign"
  );


  // Do some mesh sweeps.
  // NOTE: The database only keeps up to 16 sweeps in memory by default.
  // After that, entries will be purged. One sweep is added by default ('init').
  // We've already added another sweep ('InitUnitTest'). So if you go beyond 14,
  // it'll start deleting stuff before the end.
  for (int sweep = 0; sweep < 14; sweep++){

    ac::startMeshSweep( "MySweep" + std::to_string(sweep) );

    // record old particle positions.
    auto oldParticlePositions = ac::recordParticlePositions(assignedParticles);

    // insert tiny movement steps, make sure movement isn't recorded.
    double dt_use = dt;
    if (sweep > 8 and sweep < 12) {
      dt_use *= ac::internal::ParticleIdentifier::Precision * 1e-2;
    }

    // update particle position.
    localPartX += dt_use * dx;
    localPart.setX(localPartX);

    // Trace the movement.
    ac::traceParticleMovements(assignedParticles, oldParticlePositions, vertexH, spacetreeId);
    if (verbose)
      std::cout << ">>>>>>>>>>>> Moving particle to " << localPartX << std::endl;

    if (verbose) std::cout << ">>>>>>>>>>>> Checking vertex H=" << vertexH <<
          " X=" << vertexX << std::endl;

    // Does the particle need lifting from the current vertex?
    while (internal::liftParticle(localPartX, vertexX, vertexH, depth)){
      depth--;
      prevVertexH = vertexH;
      prevVertexX = vertexX;
      vertexH *= 3.;
      vertexX = internal::findVertexX(localPartX, vertexH);

      if (verbose)
        std::cout << ">>>>>>>>>>>>>>> Lifting to H=" << vertexH << " X=" << vertexX << " depth=" << depth << std::endl;

      ac::detachParticleFromVertex(
          particleName,
          localPart.getX(),
          localPart.getPartid(),
          true, // this particle is always local
          prevVertexX,
          prevVertexH,
          spacetreeId,
          "vertexDetachLift"
      );

      ac::assignParticleToVertex(
          particleName,
          localPart.getX(),
          localPart.getPartid(),
          true, // this particle is always local
          vertexX,
          vertexH,
          spacetreeId,
          "vertexAssignLift"
      );
    }

    // Now drop it in the correct vertex.
    while (internal::dropParticle(localPartX, vertexH, depth)){
      depth++;
      prevVertexH = vertexH;
      prevVertexX = vertexX;
      vertexH /= 3.;
      vertexX = internal::findVertexX(localPartX, vertexH);
      if (verbose)
        std::cout << ">>>>>>>>>>>>>>> Dropping to H=" << vertexH << " X=" << vertexX << " depth=" << depth << std::endl;

      ac::detachParticleFromVertex(
          particleName,
          localPart.getX(),
          localPart.getPartid(),
          true, // this particle is always local
          prevVertexX,
          prevVertexH,
          spacetreeId,
          "vertexDetachDrop"
      );

      ac::assignParticleToVertex(
          particleName,
          localPart.getX(),
          localPart.getPartid(),
          true, // this particle is always local
          vertexX,
          vertexH,
          spacetreeId,
          "vertexAssignDrop"
      );
    }
  }

  if (verbose) {
    std::cout << ">>>>>>>>>>>> DATA DUMP END " << std::endl;
    std::cout << eventDatabase.toString() << std::endl;
  }
#endif
}



void toolbox::particles::assignmentchecks::tests::TestHelpers::testPeriodicBoundaryConditions() {}


void toolbox::particles::assignmentchecks::tests::TestHelpers::run() {
  testMethod(testParticleWalkSameTreeLevel);
  testMethod(testParticleWalk);
  testMethod(testPeriodicBoundaryConditions);
}

#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", on)
#endif
