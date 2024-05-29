#include "TestHelpers.h"
#include "TestParticle.h"

// #include "toolbox/particles/particles.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"
#include "toolbox/particles/assignmentchecks/Utils.h"


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", off)
#endif

toolbox::particles::assignmentchecks::tests::TestHelpers::TestHelpers():
  TestCase("toolbox::particles::assignmentchecks::tests::TestHelpers") {}


void toolbox::particles::assignmentchecks::tests::TestHelpers::testParticleWalkSameTreeLevel() {

#if PeanoDebug > 0
  const bool verbose = true;

  namespace ac = toolbox::particles::assignmentchecks;

  if (verbose) std::cout << ">>>>>>>>>>>> RUNNING testParticleWalkSameTreeLevel() in " << Dimensions << "D" << std::endl;

  ac::internal::Database& eventDatabase = ac::getDatabase();

  // IMPORTANT: Needs to be the same as the particle class name you use!
  // In traceParticleMovements(), this will be derived from the class name.
  std::string particleName = "TestParticle";

  // coordinates of particles
  tarch::la::Vector<Dimensions, double> localPartX, virtPartX;
  // particle displacement each step
  tarch::la::Vector<Dimensions, double> dx;
  // coords of the boundary. Should in principle be a plane,
  // but a point does the trick here.
  tarch::la::Vector<Dimensions, double> boundary;
  // Assume we have vertices of a single size.
  tarch::la::Vector<Dimensions, double> vertexH;
  tarch::la::Vector<Dimensions, double> vertexX;
  tarch::la::Vector<Dimensions, double> prevVertexX;

  // Initialise values
  double vertexH_default = 3.;
  double dt = 1.;

  for (int i = 0; i < Dimensions; i++){
    localPartX(i) = 21.;
    virtPartX(i) = 0.;
    dx(i) = -1.;
    boundary(i) = 100.;
    vertexH(i) = vertexH_default;
  }

  int particleID = 1;
  int spacetreeId = 1;

  // generate (minimal) particle
  ac::tests::TestParticle localPart = ac::tests::TestParticle(localPartX, particleID);
  // pretend to have a ParticleContainer
  std::vector<ac::tests::TestParticle*> assignedParticles = {&localPart};

  // Find initial pseudo-"vertex" particle is assigned to
  int vertexInd = static_cast<int>(std::floor(localPartX(0) / vertexH(0)));
  for (int i = 0; i < Dimensions; i++){
    vertexX(i) = (vertexInd + 0.5) * vertexH(i);
  }

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
    if (verbose)
      std::cout << ">>>>>>>>>>>> Moving particle to " << localPartX << std::endl;
    ac::traceParticleMovements(assignedParticles, oldParticlePositions, vertexH, spacetreeId);

    // pretend a particle can walk from vertex to vertex.
    // find current vertex particle is assigned to.
    vertexInd = static_cast<int>(std::floor(localPartX(0) / vertexH(0)));
    for (int i = 0; i < Dimensions; i++){
      prevVertexX(i) = vertexX(i);
      vertexX(i) = (vertexInd + 0.5) * vertexH(i);
    }

    if (verbose) std::cout << ">>>>>>>>>>>> Checking vertex" << std::endl;

    if (vertexInd != prevVertexInd){
      // Particle has changed vertex. Take note of that.
      if (verbose) std::cout << ">>>>>>>>>>>> Detaching vertex" << std::endl;
      ac::detachParticleFromVertex(
          particleName,
          localPart.getX(),
          localPart.getPartid(),
          true, // this particle is always local
          prevVertexX, // prevVertexX,
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


void toolbox::particles::assignmentchecks::tests::TestHelpers::testPeriodicBoundaryConditions() {}


void toolbox::particles::assignmentchecks::tests::TestHelpers::run() {
  testMethod(testParticleWalkSameTreeLevel);
  testMethod(testPeriodicBoundaryConditions);
}

#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", on)
#endif
