// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestHelpers.h"

#include "peano4/particles/assignmentchecks/Database.h"
#include "peano4/particles/assignmentchecks/TracingAPI.h"
#include "peano4/particles/assignmentchecks/tests/TestParticle.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/la/Vector.h"

#if PEANO_DEBUG
namespace {
  /** Width of a vertex divided by the width of the same vertex one level finer. */
  constexpr double MeshWidthRatio = peano4::sfc::ActiveCurve::SubdivisionFactor;

  /**
   * Returns the index of the vertex of size vertexH that covers position x. The
   * first vertex sits at the origin, and vertexH is half the width of the
   * square around a vertex.
   */
  int findVertexIndex(const double x, const double vertexH) {
    return static_cast<int>(std::floor((x / (2.0 * vertexH)) + 0.5));
  }

  /**
   * Returns the position of the vertex of size vertexH that covers x. The first
   * vertex sits at the origin.
   */
  tarch::la::Vector<DIMENSIONS, double> findVertexX(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH
  ) {
    tarch::la::Vector<DIMENSIONS, double> vertexX;

    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      const int vertexIndex = findVertexIndex(x(dimension), vertexH(dimension));
      vertexX(dimension)    = 2 * vertexIndex * vertexH(dimension);
    }

    return vertexX;
  }

  /**
   * Returns true if particleX lies outside the square of two times vertexH
   * around the vertex at vertexX and depth is above zero.
   */
  bool liftParticle(
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH,
    const int                                    depth
  ) {
    if (depth == 0) {
      return false;
    }

    const tarch::la::Vector<DIMENSIONS, double> distance = vertexX - particleX;

    if (tarch::la::oneSmaller(distance, -1. * vertexH)) {
      return true;
    }
    if (tarch::la::oneGreater(distance, vertexH)) {
      return true;
    }
    return false;
  }

  /**
   * Returns true if particleX lies inside the square of the next finer vertex
   * and depth stays below maxVertexDepth.
   */
  bool dropParticle(
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH,
    const int                                    depth,
    const int                                    maxVertexDepth
  ) {
    if (depth == maxVertexDepth) {
      return false;
    }

    const tarch::la::Vector<DIMENSIONS, double> newVertexH = vertexH / MeshWidthRatio;
    const tarch::la::Vector<DIMENSIONS, double> vertexX    = findVertexX(particleX, newVertexH);

    const tarch::la::Vector<DIMENSIONS, double> distance = vertexX - particleX;

    return tarch::la::allSmaller(distance, newVertexH) and tarch::la::allGreater(distance, -1. * newVertexH);
  }

  /**
   * Returns a virtual copy of original that is moved by boundary along every
   * axis whose flag is true. The caller deletes the copy.
   */
  peano4::particles::assignmentchecks::tests::TestParticle* mirrorParticleAlongPeriodicBoundary(
    peano4::particles::assignmentchecks::tests::TestParticle* original,
    const bool                                                mirrorX,
    const bool                                                mirrorY,
    const bool                                                mirrorZ,
    const tarch::la::Vector<DIMENSIONS, double>&              boundary
  ) {
    tarch::la::Vector<DIMENSIONS, double> virtualPartX = original->getX();

    if (mirrorX) {
      virtualPartX(0) += boundary(0);
    }
#if DIMENSIONS > 1
    if (mirrorY) {
      virtualPartX(1) += boundary(1);
    }
#endif
#if DIMENSIONS > 2
    if (mirrorZ) {
      virtualPartX(2) += boundary(2);
    }
#endif

    auto* mirrored = new peano4::particles::assignmentchecks::tests::TestParticle(
      virtualPartX,
      original->getPartid(),
      false // virtual particle
    );
    mirrored->setVertexH(original->getVertexH());
    mirrored->setDepth(original->getDepth());

    return mirrored;
  }
} // namespace
#endif

peano4::particles::assignmentchecks::tests::TestHelpers::TestHelpers() :
  TestCase("peano4::particles::assignmentchecks::tests::TestHelpers") {}

/**
 * Checks that two ParticleIdentifier objects of the same particle compare equal
 * and that the comparison orders them by their particle number.
 */
void peano4::particles::assignmentchecks::tests::TestHelpers::testTruthTableSearchAndIdKeys() {
  tarch::la::Vector<DIMENSIONS, double> particleAX;
  tarch::la::Vector<DIMENSIONS, double> particleBX;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> vertexH;
  const int                             treeId = 1;

  for (int i = 0; i < DIMENSIONS; i++) {
    particleAX(i) = 1.;
    particleBX(i) = 2.;
    vertexX(i)    = 1.;
    vertexH(i)    = 1.;
  }

  const peano4::particles::assignmentchecks::ParticleIdentifier firstA =
    peano4::particles::assignmentchecks::ParticleIdentifier("DummyParticle", particleAX, vertexX, vertexH, 1, treeId);
  const peano4::particles::assignmentchecks::ParticleIdentifier firstB =
    peano4::particles::assignmentchecks::ParticleIdentifier("DummyParticle", particleBX, vertexX, vertexH, 2, treeId);

  logDebug(
    "firstA < firstB:" << (firstA < firstB) << "(1) firstA==firstB:" << (firstA == firstB) << "(0) firstA==firstA:"
                       << (firstA == firstA) << "(1) firstB==firstB:" << (firstB == firstB) << "(1)"
  );

  assertion(firstA < firstB);
  assertion(not(firstA == firstB));
  assertion(not(firstB == firstA));
  assertion(not(firstB < firstA));

  const peano4::particles::assignmentchecks::ParticleIdentifier secondA =
    peano4::particles::assignmentchecks::ParticleIdentifier("DummyParticle", particleAX, vertexX, vertexH, 1, treeId);
  const peano4::particles::assignmentchecks::ParticleIdentifier secondB =
    peano4::particles::assignmentchecks::ParticleIdentifier("DummyParticle", particleBX, vertexX, vertexH, 2, treeId);

  logDebug(
    "secondA < secondB:" << (secondA < secondB) << "(1) secondA==secondB:" << (secondA == secondB)
                         << "(0) secondA==secondA:" << (secondA == secondA)
                         << "(1) secondB==secondB:" << (secondB == secondB) << "(1)"
  );

  assertion(secondA < secondB);
  assertion(not(secondA == secondB));
  assertion(not(secondB == secondA));
  assertion(not(secondB < secondA));

  assertion(not(firstA < secondA));
  assertion(not(secondA < firstA));
  assertion(secondA == firstA);
  assertion(firstA == secondA);

  assertion(firstA < secondB);
  assertion(not(secondB < firstA));
  assertion(not(secondB == firstA));
  assertion(not(firstA == secondB));

  assertion(not(firstB < secondA));
  assertion(secondA < firstB);
  assertion(not(secondA == firstB));
  assertion(not(firstB == secondA));

  assertion(not(firstB < secondB));
  assertion(not(secondB < firstB));
  assertion(firstB == secondB);
  assertion(secondB == firstB);
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testAddingSweepsToDatabase() {
  auto& eventDatabase = peano4::particles::assignmentchecks::Database::getInstance();
  eventDatabase.reset();
  peano4::particles::assignmentchecks::checkDatabaseIsEmpty();

  std::vector<std::string> meshSweepNames = {"initial", "alpha", "beta", "gamma", "delta"};

  auto sweep = meshSweepNames.begin();
  // Skip first, which gets automatically added to the database.
  sweep++;

  while (sweep != meshSweepNames.end()) {
    eventDatabase.startMeshSweep(*sweep);
    sweep++;
  }

  [[maybe_unused]] const auto numberOfMeshSweeps = eventDatabase.getMeshSweepData().size();
  assertion(
    numberOfMeshSweeps == meshSweepNames.size(),
    "The database holds a different number of mesh sweeps.",
    numberOfMeshSweeps,
    meshSweepNames.size(),
    eventDatabase.toString()
  );

  assertion(
    (eventDatabase.getCurrentMeshSweepIndex() + 1) == meshSweepNames.size(),
    "The current mesh sweep of the database is not the last one of the test.",
    eventDatabase.getMeshSweepData().size(),
    meshSweepNames.size(),
    eventDatabase.toString()
  );

  auto databaseSweep = eventDatabase.getMeshSweepData().begin();
  sweep              = meshSweepNames.begin();

  while (sweep != meshSweepNames.end()) {
    assertion(
      *sweep == databaseSweep->getName(),
      "The database holds another name for this mesh sweep.",
      *sweep,
      databaseSweep->getName()
    );
    sweep++;
    databaseSweep++;
  }

  eventDatabase.reset();
  peano4::particles::assignmentchecks::checkDatabaseIsEmpty();
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testAddingParticleEvents() {
  auto& eventDatabase = peano4::particles::assignmentchecks::Database::getInstance();
  eventDatabase.reset();
  peano4::particles::assignmentchecks::checkDatabaseIsEmpty();
  // The limit stays above the number of events that the loop below adds.
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(100);

  const int                             treeId  = 1;
  const bool                            isLocal = true;
  tarch::la::Vector<DIMENSIONS, double> particleX;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> vertexH;

  std::string trace;

  std::vector<std::string> meshSweepNames = {"alpha", "beta", "gamma", "delta"};

  const int numberOfParticles = 10;
  auto      sweep             = meshSweepNames.begin();
  while (sweep != meshSweepNames.end()) {
    peano4::particles::assignmentchecks::startMeshSweep(*sweep);

    for (int p = 1; p < numberOfParticles + 1; p++) {
      const int particleId = p;

      for (int i = 0; i < DIMENSIONS; i++) {
        particleX(i) = p;
        vertexX(i)   = p;
        vertexH(i)   = p;
      }

      const peano4::particles::assignmentchecks::ParticleIdentifier identifier =
        peano4::particles::assignmentchecks::ParticleIdentifier(
          "DummyParticle",
          particleX,
          vertexX,
          vertexH,
          particleId,
          treeId
        );

      trace                                                  = "Assign/sweep:" + *sweep;
      peano4::particles::assignmentchecks::Event assignEvent = peano4::particles::assignmentchecks::Event(
        peano4::particles::assignmentchecks::Event::Type::AssignToVertex,
        isLocal,
        vertexX,
        particleX,
        vertexH,
        treeId,
        trace
      );

      eventDatabase.addEvent(identifier, assignEvent);

      trace                                                = "Move/sweep:" + *sweep;
      peano4::particles::assignmentchecks::Event moveEvent = peano4::particles::assignmentchecks::Event(
        peano4::particles::assignmentchecks::Event::Type::MoveWhileAssociatedToVertex,
        vertexX,
        particleX,
        vertexH,
        treeId,
        trace
      );
      eventDatabase.addEvent(identifier, moveEvent);

      trace                                                 = "Sieve/sweep:" + *sweep;
      peano4::particles::assignmentchecks::Event sieveEvent = peano4::particles::assignmentchecks::Event(
        peano4::particles::assignmentchecks::Event::Type::AssignToSieveSet,
        isLocal,
        treeId,
        trace
      );
      eventDatabase.addEvent(identifier, sieveEvent);

      trace = "NotFound/sweep:" + *sweep;
      peano4::particles::assignmentchecks::Event invalidEvent =
        peano4::particles::assignmentchecks::Event(peano4::particles::assignmentchecks::Event::Type::NotFound);
      eventDatabase.addEvent(identifier, invalidEvent);
    }

    sweep++;
  }

  assertion(
    eventDatabase.getNumberOfTracedParticles() == static_cast<int>(numberOfParticles),
    "The database holds a different number of particles.",
    eventDatabase.getNumberOfTracedParticles(),
    numberOfParticles
  );

  for (int p = 1; p < numberOfParticles + 1; p++) {
    const int particleId = p;

    for (int i = 0; i < DIMENSIONS; i++) {
      particleX(i) = p;
      vertexX(i)   = p;
      vertexH(i)   = p;
    }

    const peano4::particles::assignmentchecks::ParticleIdentifier identifier =
      peano4::particles::assignmentchecks::ParticleIdentifier(
        "DummyParticle",
        particleX,
        vertexX,
        vertexH,
        particleId,
        treeId
      );

    [[maybe_unused]] const size_t numberOfEntries = eventDatabase.getTotalParticleEntries(identifier);
    // The loop adds four events per sweep.
    assertion(
      numberOfEntries == (meshSweepNames.size() * 4),
      "The database holds a different number of events for the particle.",
      p,
      numberOfEntries,
      meshSweepNames.size() * 4
    );

    logDebug(eventDatabase.particleHistory(identifier));
  }

  eventDatabase.reset();
  peano4::particles::assignmentchecks::checkDatabaseIsEmpty();
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testRemovingParticleEvents() {
  auto& eventDatabase = peano4::particles::assignmentchecks::Database::getInstance();
  eventDatabase.reset();
  peano4::particles::assignmentchecks::checkDatabaseIsEmpty();
  // The limit stays above the number of events that the loop below adds.
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(20);

  tarch::la::Vector<DIMENSIONS, double> particleX;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> vertexH;

  std::string trace;

  std::vector<std::string> meshSweepNames = {"alpha", "beta", "gamma", "delta", "foo", "bar", "baz", "boo"};

  const int numberOfParticles = 4;
  auto      sweep             = meshSweepNames.begin();

  while (sweep != meshSweepNames.end()) {
    peano4::particles::assignmentchecks::startMeshSweep(*sweep);

    // The loop builds one tree per particle. Every tree holds every particle
    // and owns one of them.
    for (int treeId = 0; treeId < numberOfParticles; treeId++) {
      for (int p = 1; p < numberOfParticles + 1; p++) {
        const bool isLocal = (treeId == p - 1);

        const int particleId = p;

        for (int i = 0; i < DIMENSIONS; i++) {
          particleX(i) = p;
          vertexX(i)   = p;
          vertexH(i)   = p;
        }

        const peano4::particles::assignmentchecks::ParticleIdentifier identifier =
          peano4::particles::assignmentchecks::ParticleIdentifier(
            "DummyParticle",
            particleX,
            vertexX,
            vertexH,
            particleId,
            treeId
          );

        trace                                                  = "Assign/sweep:" + *sweep;
        peano4::particles::assignmentchecks::Event assignEvent = peano4::particles::assignmentchecks::Event(
          peano4::particles::assignmentchecks::Event::Type::AssignToVertex,
          isLocal,
          vertexX,
          particleX,
          vertexH,
          treeId,
          trace
        );

        eventDatabase.addEvent(identifier, assignEvent);

        trace                                                = "Move/sweep:" + *sweep;
        peano4::particles::assignmentchecks::Event moveEvent = peano4::particles::assignmentchecks::Event(
          peano4::particles::assignmentchecks::Event::Type::MoveWhileAssociatedToVertex,
          vertexX,
          particleX,
          vertexH,
          treeId,
          trace
        );
        eventDatabase.addEvent(identifier, moveEvent);

        trace                                                 = "Sieve/sweep:" + *sweep;
        peano4::particles::assignmentchecks::Event sieveEvent = peano4::particles::assignmentchecks::Event(
          peano4::particles::assignmentchecks::Event::Type::AssignToSieveSet,
          isLocal,
          treeId,
          trace
        );
        eventDatabase.addEvent(identifier, sieveEvent);
      }
    }

    sweep++;
  }

  assertion(
    eventDatabase.getNumberOfTracedParticles() == numberOfParticles * numberOfParticles,
    "The database holds a different number of particles.",
    eventDatabase.getNumberOfTracedParticles(),
    numberOfParticles * numberOfParticles
  );

  eventDatabase.reset();
  peano4::particles::assignmentchecks::checkDatabaseIsEmpty();
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testParticleMoveEvents() {
  // The first limit keeps every event, the second one deletes events during
  // the run.
  testAddingParticleMovingEvents(100, 1000);
  testAddingParticleMovingEvents(100, 16);
}

/**
 * Adds numberOfSweeps move events of one particle to the database, which keeps
 * numberOfEventsToKeep events per particle, so a smaller number than
 * numberOfSweeps deletes events during the run.
 */
void peano4::particles::assignmentchecks::tests::TestHelpers::testAddingParticleMovingEvents(
  int numberOfSweeps,
  int numberOfEventsToKeep
) {
  auto& eventDatabase = peano4::particles::assignmentchecks::Database::getInstance();
  eventDatabase.reset();
  peano4::particles::assignmentchecks::checkDatabaseIsEmpty();
  // The limit decides whether the database deletes events during the run.
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(numberOfEventsToKeep);

  const int    particleId = 1;
  const int    treeId     = 1;
  const double dt         = 1.;

  tarch::la::Vector<DIMENSIONS, double> particleX;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> vertexH;

  // Displacement of the particle per step.
  tarch::la::Vector<DIMENSIONS, double> dx;

  for (int i = 0; i < DIMENSIONS; i++) {
    particleX(i) = 1.;
    vertexX(i)   = 1.;
    vertexH(i)   = 1.;
    dx(i)        = 0.05;
  }

  for (int sweep = 0; sweep < numberOfSweeps; sweep++) {
    const std::string sweepName = "Sweep" + std::to_string(sweep);
    peano4::particles::assignmentchecks::startMeshSweep(sweepName);

    for (int i = 0; i < DIMENSIONS; i++) {
      particleX(i) += dx(i) * dt;
    }

    const peano4::particles::assignmentchecks::ParticleIdentifier identifier =
      peano4::particles::assignmentchecks::ParticleIdentifier(
        "DummyParticle",
        particleX,
        vertexX,
        vertexH,
        particleId,
        treeId
      );

    const std::string                          trace     = "Move/sweep:" + std::to_string(sweep);
    peano4::particles::assignmentchecks::Event moveEvent = peano4::particles::assignmentchecks::Event(
      peano4::particles::assignmentchecks::Event::Type::MoveWhileAssociatedToVertex,
      vertexX,
      particleX,
      vertexH,
      treeId,
      trace
    );
    eventDatabase.addEvent(identifier, moveEvent);
  }

  assertion(
    eventDatabase.getNumberOfTracedParticles() == 1,
    "The database holds a different number of particles than the one this test adds.",
    eventDatabase.getNumberOfTracedParticles()
  );

  const peano4::particles::assignmentchecks::ParticleIdentifier identifier =
    peano4::particles::assignmentchecks::ParticleIdentifier(
      "DummyParticle",
      particleX,
      vertexX,
      vertexH,
      particleId,
      treeId
    );

  [[maybe_unused]] const int numberOfEntries = eventDatabase.getTotalParticleEntries(identifier);
  // The database replaces the past trajectory by its last event, so one of the
  // kept events carries that substitute.
  assertion(
    numberOfEntries == (numberOfSweeps % (numberOfEventsToKeep - 1)),
    "The database holds a different number of events for the particle.",
    numberOfEntries,
    numberOfSweeps,
    numberOfEventsToKeep,
    numberOfSweeps % (numberOfEventsToKeep - 1),
    eventDatabase.particleHistory(identifier)
  );

  logDebug(eventDatabase.particleHistory(identifier));

  eventDatabase.reset();
  peano4::particles::assignmentchecks::checkDatabaseIsEmpty();
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testParticleWalkSameTreeLevel() {
#if PEANO_DEBUG

  auto& eventDatabase = peano4::particles::assignmentchecks::Database::getInstance();
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(100);

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  const std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> localPartX;
  // Displacement of the particle per step.
  tarch::la::Vector<DIMENSIONS, double> dx;
  // Every vertex of this test has the same size.
  tarch::la::Vector<DIMENSIONS, double> vertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> prevVertexX;

  const double defaultVertexH = 3.;
  const double dt             = 1.;

  for (int i = 0; i < DIMENSIONS; i++) {
    localPartX(i) = 51.5;
    dx(i)         = -1.;
    vertexH(i)    = defaultVertexH;
  }

  const int particleId = 1;
  const int treeId     = 1;

  vertexX         = findVertexX(localPartX, vertexH);
  int vertexIndex = findVertexIndex(localPartX(0), vertexH(0));

  peano4::particles::assignmentchecks::tests::TestParticle localPart =
    peano4::particles::assignmentchecks::tests::TestParticle(localPartX, particleId, true);
  const std::vector<peano4::particles::assignmentchecks::tests::TestParticle*> assignedParticles = {&localPart};

  peano4::particles::assignmentchecks::startMeshSweep("InitUnitTest");
  peano4::particles::assignmentchecks::assignParticleToVertex(
    particleName,
    localPartX,
    particleId,
    true, // this particle is always local
    vertexX,
    vertexH,
    treeId,
    "initialAssign",
    vertexX,
    vertexH,
    false, // not a new particle
    true   // particle walks from vertex to vertex on same level
  );

  // The database keeps 16 sweeps and two of them are added already, so a loop
  // beyond 14 sweeps deletes entries before the end.
  const size_t maxSweeps = 14;
  for (size_t sweep = 0; sweep < maxSweeps; sweep++) {
    peano4::particles::assignmentchecks::startMeshSweep("MySweep" + std::to_string(sweep));

    auto oldParticlePositions = peano4::particles::assignmentchecks::recordParticlePositions(assignedParticles);

    double dtThisStep = dt;
    if (sweep > 8 and sweep < 12) {
      dtThisStep *= 1e-2;
    }

    localPartX += dtThisStep * dx;
    localPart.setX(localPartX);

    peano4::particles::assignmentchecks::traceParticleMovements(
      assignedParticles,
      oldParticlePositions,
      vertexX,
      vertexH,
      treeId
    );

    logDebug("Moving particle to " << localPartX);

    const int prevVertexIndex = vertexIndex;
    vertexIndex               = findVertexIndex(localPartX(0), vertexH(0));
    for (int i = 0; i < DIMENSIONS; i++) {
      prevVertexX(i) = vertexX(i);
    }
    vertexX = findVertexX(localPartX, vertexH);

    logDebug("Checking vertex");

    if (vertexIndex != prevVertexIndex) {
      logDebug("Detaching vertex");
      peano4::particles::assignmentchecks::detachParticleFromVertex(
        particleName,
        localPart.getX(),
        localPart.getPartid(),
        true, // this particle is always local
        prevVertexX,
        vertexH,
        treeId,
        "vertexChangeDetach"
      );

      logDebug("Attaching vertex");
      peano4::particles::assignmentchecks::assignParticleToVertex(
        particleName,
        localPart.getX(),
        localPart.getPartid(),
        true, // this particle is always local
        vertexX,
        vertexH,
        treeId,
        "vertexChangeAssign",
        prevVertexX,
        vertexH,
        false, // not a new particle
        true   // particle walks from vertex to vertex on same level
      );
    }
  }

  logDebug(eventDatabase.toString());

  assertion(
    eventDatabase.getNumberOfTracedParticles() == 1,
    "The database holds a different number of particles than the one this test adds.",
    eventDatabase.getNumberOfTracedParticles()
  );

  // The database registers one mesh sweep of its own at construction.
  assertion(
    eventDatabase.getCurrentMeshSweepIndex() == maxSweeps + 1,
    "The database counts a different number of mesh sweeps.",
    eventDatabase.getCurrentMeshSweepIndex(),
    maxSweeps
  );

  const peano4::particles::assignmentchecks::ParticleIdentifier identifier =
    peano4::particles::assignmentchecks::ParticleIdentifier(
      "TestParticle",
      localPartX,
      vertexX,
      vertexH,
      particleId,
      treeId
    );

  // The count of 8 follows from the sweeps and the vertex changes this test runs.
  assertion(
    eventDatabase.getTotalParticleEntries(identifier) == 8,
    "The database holds a different number of events for the particle.",
    eventDatabase.getTotalParticleEntries(identifier),
    eventDatabase.particleHistory(identifier)
  );

  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testParticleLiftDrop() {
#if PEANO_DEBUG

  auto& eventDatabase = peano4::particles::assignmentchecks::Database::getInstance();

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  const std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> localPartX;
  tarch::la::Vector<DIMENSIONS, double> vertexH;
  tarch::la::Vector<DIMENSIONS, double> prevVertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> prevVertexX;

  const double defaultVertexH = 3.;
  int          depth          = 3;
  const int    maxVertexDepth = 5;
  int          walkSteps      = 0;

  for (int i = 0; i < DIMENSIONS; i++) {
    localPartX(i) = 51.5;
    vertexH(i)    = defaultVertexH;
    vertexX(i)    = 27.;
  }

  const int particleId = 1;
  const int treeId     = 1;

  peano4::particles::assignmentchecks::tests::TestParticle localPart =
    peano4::particles::assignmentchecks::tests::TestParticle(localPartX, particleId, true);
  const std::vector<peano4::particles::assignmentchecks::tests::TestParticle*> assignedParticles = {&localPart};

  peano4::particles::assignmentchecks::startMeshSweep("InitUnitTest");
  peano4::particles::assignmentchecks::assignParticleToVertex(
    particleName,
    localPartX,
    particleId,
    true, // this particle is always local
    vertexX,
    vertexH,
    treeId,
    "initialAssign",
    vertexX,
    vertexH
  );

  logDebug("Initially assigning to H=" << vertexH << " X=" << vertexX << " depth=" << depth);

  peano4::particles::assignmentchecks::startMeshSweep("Lifts");

  while (liftParticle(localPartX, vertexX, vertexH, depth)) {
    depth--;
    prevVertexH = vertexH;
    prevVertexX = vertexX;
    vertexH *= MeshWidthRatio;
    vertexX = findVertexX(vertexX, vertexH);
    walkSteps++;

    logDebug("Lifting to H=" << vertexH << " X=" << vertexX << " depth=" << depth);

    peano4::particles::assignmentchecks::detachParticleFromVertex(
      particleName,
      localPart.getX(),
      localPart.getPartid(),
      true, // this particle is always local
      prevVertexX,
      prevVertexH,
      treeId,
      "vertexDetachLift"
    );

    peano4::particles::assignmentchecks::assignParticleToVertex(
      particleName,
      localPart.getX(),
      localPart.getPartid(),
      true, // this particle is always local
      vertexX,
      vertexH,
      treeId,
      "vertexAssignLift",
      prevVertexX,
      prevVertexH
    );
  }

  peano4::particles::assignmentchecks::startMeshSweep("Drops");

  while (dropParticle(localPartX, vertexH, depth, maxVertexDepth)) {
    depth++;
    prevVertexH = vertexH;
    prevVertexX = vertexX;
    vertexH /= MeshWidthRatio;
    vertexX = findVertexX(localPartX, vertexH);
    walkSteps++;

    logDebug("Dropping to H=" << vertexH << " X=" << vertexX << " depth=" << depth);

    peano4::particles::assignmentchecks::detachParticleFromVertex(
      particleName,
      localPart.getX(),
      localPart.getPartid(),
      true, // this particle is always local
      prevVertexX,
      prevVertexH,
      treeId,
      "vertexDetachDrop"
    );

    peano4::particles::assignmentchecks::assignParticleToVertex(
      particleName,
      localPart.getX(),
      localPart.getPartid(),
      true, // this particle is always local
      vertexX,
      vertexH,
      treeId,
      "vertexAssignDrop",
      prevVertexX,
      prevVertexH
    );
  }

  assertion(
    eventDatabase.getNumberOfTracedParticles() == 1,
    "The database holds a different number of particles than the one this test adds.",
    eventDatabase.getNumberOfTracedParticles()
  );

  const peano4::particles::assignmentchecks::ParticleIdentifier identifier =
    peano4::particles::assignmentchecks::ParticleIdentifier(
      "TestParticle",
      localPartX,
      vertexX,
      vertexH,
      particleId,
      treeId
    );

  // The initial assign plus one detach and one assign per lift and per drop.
  assertion(
    eventDatabase.getTotalParticleEntries(identifier) == 1 + 2 * walkSteps,
    "The database holds a different number of events for the particle.",
    eventDatabase.getTotalParticleEntries(identifier),
    eventDatabase.particleHistory(identifier)
  );

  logDebug(eventDatabase.toString());

  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testParticleWalk() {
#if PEANO_DEBUG

  auto& eventDatabase = peano4::particles::assignmentchecks::Database::getInstance();

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  const std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> localPartX;
  // Displacement of the particle per step.
  tarch::la::Vector<DIMENSIONS, double> dx;
  tarch::la::Vector<DIMENSIONS, double> vertexH;
  tarch::la::Vector<DIMENSIONS, double> prevVertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> prevVertexX;

  const double defaultVertexH = 3.;
  const double dt             = 0.01;
  int          depth          = 4;
  const int    maxVertexDepth = 6;

  for (int i = 0; i < DIMENSIONS; i++) {
    localPartX(i) = 51.5;
    dx(i)         = -1.;
    vertexH(i)    = defaultVertexH;
  }

  const int particleId = 1;
  const int treeId     = 1;

  vertexX = findVertexX(localPartX, vertexH);

  peano4::particles::assignmentchecks::tests::TestParticle localPart =
    peano4::particles::assignmentchecks::tests::TestParticle(localPartX, particleId, true);
  const std::vector<peano4::particles::assignmentchecks::tests::TestParticle*> assignedParticles = {&localPart};

  peano4::particles::assignmentchecks::startMeshSweep("InitUnitTest");
  peano4::particles::assignmentchecks::assignParticleToVertex(
    particleName,
    localPartX,
    particleId,
    true, // this particle is always local
    vertexX,
    vertexH,
    treeId,
    "initialAssign",
    vertexX,
    vertexH
  );

  for (int sweep = 0; sweep < 1000; sweep++) {
    peano4::particles::assignmentchecks::startMeshSweep("MySweep" + std::to_string(sweep));

    auto oldParticlePositions = peano4::particles::assignmentchecks::recordParticlePositions(assignedParticles);

    double dtThisStep = dt;
    if (sweep > 9 and sweep < 256) {
      dtThisStep *= 1e-2;
    }

    const double dxThisStep = dtThisStep * dx(0);
    // The checks accept a movement below the size of the vertex.
    assertion(dxThisStep <= vertexH(0));

    localPartX += dxThisStep;
    localPart.setX(localPartX);

    peano4::particles::assignmentchecks::traceParticleMovements(
      assignedParticles,
      oldParticlePositions,
      vertexX,
      vertexH,
      treeId
    );

    logDebug("Moving particle to " << localPartX);

    logDebug("Checking vertex H=" << vertexH << " X=" << vertexX);

    while (liftParticle(localPartX, vertexX, vertexH, depth)) {
      depth--;
      prevVertexH = vertexH;
      prevVertexX = vertexX;
      vertexH *= MeshWidthRatio;
      vertexX = findVertexX(localPartX, vertexH);

      logDebug("Lifting to H=" << vertexH << " X=" << vertexX << " depth=" << depth);

      peano4::particles::assignmentchecks::detachParticleFromVertex(
        particleName,
        localPart.getX(),
        localPart.getPartid(),
        true, // this particle is always local
        prevVertexX,
        prevVertexH,
        treeId,
        "vertexDetachLift"
      );

      peano4::particles::assignmentchecks::assignParticleToVertex(
        particleName,
        localPart.getX(),
        localPart.getPartid(),
        true, // this particle is always local
        vertexX,
        vertexH,
        treeId,
        "vertexAssignLift",
        prevVertexX,
        prevVertexH
      );
    }

    while (dropParticle(localPartX, vertexH, depth, maxVertexDepth)) {
      depth++;
      prevVertexH = vertexH;
      prevVertexX = vertexX;
      vertexH /= MeshWidthRatio;
      vertexX = findVertexX(localPartX, vertexH);

      logDebug("Dropping to H=" << vertexH << " X=" << vertexX << " depth=" << depth);

      peano4::particles::assignmentchecks::detachParticleFromVertex(
        particleName,
        localPart.getX(),
        localPart.getPartid(),
        true, // this particle is always local
        prevVertexX,
        prevVertexH,
        treeId,
        "vertexDetachDrop"
      );

      peano4::particles::assignmentchecks::assignParticleToVertex(
        particleName,
        localPart.getX(),
        localPart.getPartid(),
        true, // this particle is always local
        vertexX,
        vertexH,
        treeId,
        "vertexAssignDrop",
        prevVertexX,
        prevVertexH
      );
    }
  }

  logDebug(eventDatabase.toString());

  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testPeriodicBoundaryConditions() {
#if PEANO_DEBUG

  auto& eventDatabase = peano4::particles::assignmentchecks::Database::getInstance();
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(1000);

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  const std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> initialLocalPartX;
  // Displacement of the particle per step.
  tarch::la::Vector<DIMENSIONS, double> dx;
  // The particle travels diagonally, so one point stands for the boundary plane.
  tarch::la::Vector<DIMENSIONS, double> boundary;
  tarch::la::Vector<DIMENSIONS, double> zeroCorner;

  tarch::la::Vector<DIMENSIONS, double> initialVertexH;
  tarch::la::Vector<DIMENSIONS, double> prevVertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> prevVertexX;

  const double defaultVertexH = 3.;
  const double dt             = 1.;
  const int    depth          = 4;
  const int    maxVertexDepth = 5;

  for (int i = 0; i < DIMENSIONS; i++) {
    initialLocalPartX(i) = 7.5;
    dx(i)                = -0.3;
    initialVertexH(i)    = defaultVertexH;
    boundary(i)          = 100.;
    zeroCorner(i)        = 0.;
  }

  const int particleId = 1;
  const int treeId     = 1;

  auto* localPart = new peano4::particles::assignmentchecks::tests::TestParticle(initialLocalPartX, particleId, true);
  localPart->setVertexH(initialVertexH);
  localPart->setDepth(depth);

  std::vector<peano4::particles::assignmentchecks::tests::TestParticle*> particleSet = {localPart};

  vertexX = findVertexX(localPart->getX(), localPart->getVertexH());

  peano4::particles::assignmentchecks::startMeshSweep("InitUnitTest");
  peano4::particles::assignmentchecks::assignParticleToVertex(
    particleName,
    localPart->getX(),
    localPart->getPartid(),
    localPart->isLocal(),
    vertexX,
    localPart->getVertexH(),
    treeId,
    "initialAssign",
    vertexX,
    localPart->getVertexH()
  );

  for (int sweep = 0; sweep < 50; sweep++) {
    peano4::particles::assignmentchecks::startMeshSweep("MySweep" + std::to_string(sweep));

    auto oldParticlePositions = peano4::particles::assignmentchecks::recordParticlePositions(particleSet);

    for (auto* p : particleSet) {
      // The vertex that holds the particle has to be found before the move.
      vertexX = findVertexX(p->getX(), p->getVertexH());

      tarch::la::Vector<DIMENSIONS, double> partX = p->getX();
      partX += dt * dx;
      p->setX(partX);

      peano4::particles::assignmentchecks::traceParticleMovements(
        particleSet,
        oldParticlePositions,
        vertexX,
        p->getVertexH(),
        treeId
      );

      logDebug("Moving particle to " << partX);

      logDebug("Checking vertex H=" << p->getVertexH() << " X=" << vertexX);

      while (liftParticle(p->getX(), vertexX, p->getVertexH(), p->getDepth())) {
        p->setDepth(p->getDepth() - 1);
        prevVertexH = p->getVertexH();
        prevVertexX = vertexX;
        p->setVertexH(p->getVertexH() * MeshWidthRatio);
        vertexX = findVertexX(p->getX(), p->getVertexH());

        logDebug("Lifting to H=" << p->getVertexH() << " X=" << vertexX << " depth=" << p->getDepth());

        peano4::particles::assignmentchecks::detachParticleFromVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          prevVertexX,
          prevVertexH,
          treeId,
          "vertexDetach::Lift"
        );

        peano4::particles::assignmentchecks::assignParticleToVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          vertexX,
          p->getVertexH(),
          treeId,
          "vertexAssign::Lift",
          prevVertexX,
          prevVertexH
        );
      }

      while (dropParticle(p->getX(), p->getVertexH(), p->getDepth(), maxVertexDepth)) {
        p->setDepth(p->getDepth() + 1);
        prevVertexH = p->getVertexH();
        prevVertexX = vertexX;
        p->setVertexH(p->getVertexH() / MeshWidthRatio);
        vertexX = findVertexX(p->getX(), p->getVertexH());

        logDebug("Dropping to H=" << p->getVertexH() << " X=" << vertexX << " depth=" << p->getDepth());

        peano4::particles::assignmentchecks::detachParticleFromVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          prevVertexX,
          prevVertexH,
          treeId,
          "vertexDetach::Drop"
        );

        peano4::particles::assignmentchecks::assignParticleToVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          vertexX,
          p->getVertexH(),
          treeId,
          "vertexAssign::Drop",
          prevVertexX,
          prevVertexH
        );
      }
    } // Drift loop

    for (auto* p : particleSet) {
      // The particle travels diagonally, so one coordinate decides.
      const int vertexIndex = findVertexIndex(p->getX()[0], p->getVertexH()[0]);
      if (vertexIndex == 0) {
        // The particle sits on the (0, 0, 0) boundary vertex and is replicated
        // across the boundary.
        tarch::la::Vector<DIMENSIONS, double> virtualPartX = p->getX() + boundary;
        auto*                                 replica      = new TestParticle(
          virtualPartX,
          p->getPartid(),
          false // virtual particle
        );
        replica->setVertexH(p->getVertexH());
        replica->setDepth(p->getDepth());
        particleSet.push_back(replica);

        logDebug("Creating virtual particle at " << virtualPartX);

        // The virtual particle sits at the depth of the local particle, so it
        // keeps vertexH and takes a new vertexX.
        vertexX = findVertexX(replica->getX(), replica->getVertexH());

        peano4::particles::assignmentchecks::assignParticleToVertex(
          particleName,
          replica->getX(),
          replica->getPartid(),
          false, // this particle is always virtual
          vertexX,
          replica->getVertexH(),
          treeId, // Say we only have 1 spacetree
          "createdVirtualParticle",
          vertexX,
          replica->getVertexH()
        );
      }
    }

    // The loop above adds at most one virtual particle.
    assertion(particleSet.size() <= 2);

    // A particle that crosses the (0, 0, 0) boundary becomes virtual, and a
    // particle that crosses the far boundary becomes local.
    for (auto* p : particleSet) {
      if (p->isLocal()) {
        if (tarch::la::oneSmaller(p->getX(), zeroCorner)) {
          vertexX = findVertexX(p->getX(), p->getVertexH());

          peano4::particles::assignmentchecks::detachParticleFromVertex(
            particleName,
            p->getX(),
            p->getPartid(),
            true, // here it's local
            vertexX,
            p->getVertexH(),
            treeId,
            "vertexDetach::LocalToVirtual"
          );

          peano4::particles::assignmentchecks::assignParticleToVertex(
            particleName,
            p->getX(),
            p->getPartid(),
            false, // now make it virtual
            vertexX,
            p->getVertexH(),
            treeId,
            "vertexAssign::LocalToVirtual",
            vertexX,
            p->getVertexH()
          );

          p->setIsLocal(false);
        }
      } else {
        if (tarch::la::oneSmaller(p->getX(), boundary)) {
          vertexX = findVertexX(p->getX(), p->getVertexH());
          peano4::particles::assignmentchecks::detachParticleFromVertex(
            particleName,
            p->getX(),
            p->getPartid(),
            false, // here it's virtual
            vertexX,
            p->getVertexH(),
            treeId,
            "vertexDetach::VirtualToLocal"
          );

          peano4::particles::assignmentchecks::assignParticleToVertex(
            particleName,
            p->getX(),
            p->getPartid(),
            true, // now make it local
            vertexX,
            p->getVertexH(),
            treeId,
            "vertexAssign::VirtualToLocal",
            vertexX,
            p->getVertexH()
          );

          p->setIsLocal(true);
        }
      }
    }

    auto p = particleSet.begin();
    while (p != particleSet.end()) {
      if (not(*p)->isLocal()) {
        logDebug("Erasing particle at " << (*p)->getX());

        vertexX = findVertexX((*p)->getX(), (*p)->getVertexH());

        peano4::particles::assignmentchecks::detachParticleFromVertex(
          particleName,
          (*p)->getX(),
          (*p)->getPartid(),
          (*p)->isLocal(),
          vertexX,
          (*p)->getVertexH(),
          treeId,
          "vertexDetachErase"
        );
        peano4::particles::assignmentchecks::eraseParticle(
          particleName,
          (*p)->getX(),
          (*p)->getPartid(),
          (*p)->isLocal(),
          vertexX,
          (*p)->getVertexH(),
          treeId,
          "eraseVirtualParticle"
        );
        delete *p;
        p = particleSet.erase(p);
      } else {
        p++;
      }
    }
  } // mesh sweep loops

  logDebug(eventDatabase.toString());

  for (auto* p : particleSet) {
    delete p;
  }
  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testPeriodicBoundaryConditionsCorner() {
#if PEANO_DEBUG

  auto& eventDatabase = peano4::particles::assignmentchecks::Database::getInstance();
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(1000);

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  const std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> initialLocalPartX;
  // Displacement of the particle per step.
  tarch::la::Vector<DIMENSIONS, double> dx;
  // The particle travels diagonally, so one point stands for the boundary plane.
  tarch::la::Vector<DIMENSIONS, double> boundary;
  tarch::la::Vector<DIMENSIONS, double> zeroCorner;

  tarch::la::Vector<DIMENSIONS, double> initialVertexH;
  tarch::la::Vector<DIMENSIONS, double> prevVertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> prevVertexX;

  const double defaultVertexH = 0.33333333;
  const double dt             = 1.;
  const int    depth          = 1;
  const int    maxVertexDepth = 2;

  for (int i = 0; i < DIMENSIONS; i++) {
    initialLocalPartX(i) = 0.01;
    dx(i)                = 0.000001;
    initialVertexH(i)    = defaultVertexH;
    boundary(i)          = 1.;
    zeroCorner(i)        = 0.;
  }

  const int particleId = 1;
  const int treeId     = 1;

  auto* localPart = new peano4::particles::assignmentchecks::tests::TestParticle(initialLocalPartX, particleId, true);
  localPart->setVertexH(initialVertexH);
  localPart->setDepth(depth);

  std::vector<peano4::particles::assignmentchecks::tests::TestParticle*> particleSet = {localPart};

  vertexX = findVertexX(localPart->getX(), localPart->getVertexH());

  peano4::particles::assignmentchecks::startMeshSweep("InitUnitTest");
  peano4::particles::assignmentchecks::assignParticleToVertex(
    particleName,
    localPart->getX(),
    localPart->getPartid(),
    localPart->isLocal(),
    vertexX,
    localPart->getVertexH(),
    treeId,
    "initialAssign",
    vertexX,
    localPart->getVertexH(),
    true
  );

  for (int sweep = 0; sweep < 50; sweep++) {
    peano4::particles::assignmentchecks::startMeshSweep("MySweep" + std::to_string(sweep));

    auto oldParticlePositions = peano4::particles::assignmentchecks::recordParticlePositions(particleSet);

    for (auto* p : particleSet) {
      // The vertex that holds the particle has to be found before the move.
      vertexX = findVertexX(p->getX(), p->getVertexH());

      tarch::la::Vector<DIMENSIONS, double> partX = p->getX();
      partX += dt * dx;
      p->setX(partX);

      peano4::particles::assignmentchecks::traceParticleMovements(
        particleSet,
        oldParticlePositions,
        vertexX,
        p->getVertexH(),
        treeId
      );

      logDebug("Moving particle to " << partX);

      logDebug("Checking vertex H=" << p->getVertexH() << " X=" << vertexX);

      while (liftParticle(p->getX(), vertexX, p->getVertexH(), p->getDepth())) {
        p->setDepth(p->getDepth() - 1);
        prevVertexH = p->getVertexH();
        prevVertexX = vertexX;
        p->setVertexH(p->getVertexH() * MeshWidthRatio);
        vertexX = findVertexX(p->getX(), p->getVertexH());

        logDebug("Lifting to H=" << p->getVertexH() << " X=" << vertexX << " depth=" << p->getDepth());

        peano4::particles::assignmentchecks::detachParticleFromVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          prevVertexX,
          prevVertexH,
          treeId,
          "vertexDetach::Lift"
        );

        peano4::particles::assignmentchecks::assignParticleToVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          vertexX,
          p->getVertexH(),
          treeId,
          "vertexAssign::Lift",
          prevVertexX,
          prevVertexH
        );
      }

      while (dropParticle(p->getX(), p->getVertexH(), p->getDepth(), maxVertexDepth)) {
        p->setDepth(p->getDepth() + 1);
        prevVertexH = p->getVertexH();
        prevVertexX = vertexX;
        p->setVertexH(p->getVertexH() / MeshWidthRatio);
        vertexX = findVertexX(p->getX(), p->getVertexH());

        logDebug("Dropping to H=" << p->getVertexH() << " X=" << vertexX << " depth=" << p->getDepth());

        peano4::particles::assignmentchecks::detachParticleFromVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          prevVertexX,
          prevVertexH,
          treeId,
          "vertexDetach::Drop"
        );

        peano4::particles::assignmentchecks::assignParticleToVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          vertexX,
          p->getVertexH(),
          treeId,
          "vertexAssign::Drop",
          prevVertexX,
          prevVertexH
        );
      }
    } // Drift loop

    std::vector<peano4::particles::assignmentchecks::tests::TestParticle*> mirroredParticleSet;

    for (auto* p : particleSet) {
      // The particle travels diagonally, so one coordinate decides.
      const int vertexIndex = findVertexIndex(p->getX()[0], p->getVertexH()[0]);
      if (vertexIndex == 0) {
        // The particle sits on the (0, 0, 0) boundary vertex and is replicated
        // across the boundary.
        if (DIMENSIONS > 0) {
          peano4::particles::assignmentchecks::tests::TestParticle* mirror1 =
            mirrorParticleAlongPeriodicBoundary(p, true, false, false, boundary);
          mirroredParticleSet.push_back(mirror1);
          logDebug("Creating virtual particle at " << mirror1->getX());
        }
        if (DIMENSIONS > 1) {
          peano4::particles::assignmentchecks::tests::TestParticle* mirror2 =
            mirrorParticleAlongPeriodicBoundary(p, true, true, false, boundary);
          mirroredParticleSet.push_back(mirror2);
          logDebug("Creating virtual particle at " << mirror2->getX());
          peano4::particles::assignmentchecks::tests::TestParticle* mirror3 =
            mirrorParticleAlongPeriodicBoundary(p, false, true, false, boundary);
          mirroredParticleSet.push_back(mirror3);
          logDebug("Creating virtual particle at " << mirror3->getX());
        }
        if (DIMENSIONS > 2) {
          peano4::particles::assignmentchecks::tests::TestParticle* mirror4 =
            mirrorParticleAlongPeriodicBoundary(p, true, true, true, boundary);
          mirroredParticleSet.push_back(mirror4);
          logDebug("Creating virtual particle at " << mirror4->getX());
          peano4::particles::assignmentchecks::tests::TestParticle* mirror5 =
            mirrorParticleAlongPeriodicBoundary(p, false, true, true, boundary);
          mirroredParticleSet.push_back(mirror5);
          logDebug("Creating virtual particle at " << mirror5->getX());
          peano4::particles::assignmentchecks::tests::TestParticle* mirror6 =
            mirrorParticleAlongPeriodicBoundary(p, false, false, true, boundary);
          mirroredParticleSet.push_back(mirror6);
          logDebug("Creating virtual particle at " << mirror6->getX());
          peano4::particles::assignmentchecks::tests::TestParticle* mirror7 =
            mirrorParticleAlongPeriodicBoundary(p, true, false, true, boundary);
          mirroredParticleSet.push_back(mirror7);
          logDebug("Creating virtual particle at " << mirror7->getX());
        }
      }
    }

    for (auto* p : mirroredParticleSet) {
      // The virtual particle sits at the depth of the local particle, so it
      // keeps vertexH and takes a new vertexX.
      vertexX = findVertexX(p->getX(), p->getVertexH());

      peano4::particles::assignmentchecks::assignParticleToVertex(
        particleName,
        p->getX(),
        p->getPartid(),
        false, // this particle is always virtual
        vertexX,
        p->getVertexH(),
        treeId, // Say we only have 1 spacetree
        "createdVirtualParticle",
        vertexX,
        p->getVertexH()
      );
    }

    particleSet.insert(particleSet.end(), mirroredParticleSet.begin(), mirroredParticleSet.end());
    assertion(particleSet.size() >= 2);

    // No particle of this test crosses a boundary, so no particle changes its
    // parallel state.

    auto p = particleSet.begin();
    while (p != particleSet.end()) {
      if (not(*p)->isLocal()) {
        logDebug("Erasing particle at " << (*p)->getX());

        vertexX = findVertexX((*p)->getX(), (*p)->getVertexH());

        peano4::particles::assignmentchecks::detachParticleFromVertex(
          particleName,
          (*p)->getX(),
          (*p)->getPartid(),
          (*p)->isLocal(),
          vertexX,
          (*p)->getVertexH(),
          treeId,
          "vertexDetachErase"
        );
        peano4::particles::assignmentchecks::eraseParticle(
          particleName,
          (*p)->getX(),
          (*p)->getPartid(),
          (*p)->isLocal(),
          vertexX,
          (*p)->getVertexH(),
          treeId,
          "eraseVirtualParticle"
        );
        delete *p;
        p = particleSet.erase(p);
      } else {
        p++;
      }
    }
  } // mesh sweep loops

  logDebug(eventDatabase.toString());

  for (auto* p : particleSet) {
    delete p;
  }
  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testSieveSet() {
#if PEANO_DEBUG

  auto& eventDatabase = peano4::particles::assignmentchecks::Database::getInstance();
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(1000);

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  const std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> initialLocalPartX;
  // Displacement of the particle per step.
  tarch::la::Vector<DIMENSIONS, double> dx;

  tarch::la::Vector<DIMENSIONS, double> vertexH;
  tarch::la::Vector<DIMENSIONS, double> prevVertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> prevVertexX;

  const double defaultVertexH = 3.;
  const double dt             = 1.;

  for (int i = 0; i < DIMENSIONS; i++) {
    initialLocalPartX(i) = 7.5;
    dx(i)                = 0.3 * defaultVertexH;
    vertexH(i)           = defaultVertexH;
  }

  const int particleId = 1;
  const int treeId     = 1;

  auto* localPart = new peano4::particles::assignmentchecks::tests::TestParticle(initialLocalPartX, particleId, true);
  localPart->setVertexH(vertexH);
  localPart->setDepth(1);

  const std::vector<peano4::particles::assignmentchecks::tests::TestParticle*> particleSet = {localPart};

  vertexX = findVertexX(localPart->getX(), localPart->getVertexH());

  peano4::particles::assignmentchecks::startMeshSweep("InitUnitTest");
  peano4::particles::assignmentchecks::assignParticleToVertex(
    particleName,
    localPart->getX(),
    localPart->getPartid(),
    localPart->isLocal(),
    vertexX,
    localPart->getVertexH(),
    treeId,
    "initialAssign",
    vertexX,
    localPart->getVertexH(),
    true
  );

  const size_t numberOfSweeps = 10;
  for (size_t sweep = 0; sweep < numberOfSweeps; sweep++) {
    peano4::particles::assignmentchecks::startMeshSweep("MySweep" + std::to_string(sweep));

    auto oldParticlePositions = peano4::particles::assignmentchecks::recordParticlePositions(particleSet);

    for (auto* p : particleSet) {
      // The vertex that holds the particle has to be found before the move.
      vertexX = findVertexX(p->getX(), p->getVertexH());

      tarch::la::Vector<DIMENSIONS, double> partX = p->getX();
      partX += dt * dx;
      p->setX(partX);

      peano4::particles::assignmentchecks::traceParticleMovements(
        particleSet,
        oldParticlePositions,
        vertexX,
        p->getVertexH(),
        treeId
      );

      logDebug("Moving particle to " << partX);

      prevVertexH = p->getVertexH();
      prevVertexX = vertexX;

      peano4::particles::assignmentchecks::detachParticleFromVertex(
        particleName,
        p->getX(),
        p->getPartid(),
        p->isLocal(),
        prevVertexX,
        prevVertexH,
        treeId,
        "vertexDetach"
      );

      peano4::particles::assignmentchecks::assignParticleToSieveSet(
        particleName,
        p->getX(),
        p->getPartid(),
        p->isLocal(),
        prevVertexX,
        prevVertexH,
        treeId,
        "assignToSieveSet"
      );
      vertexX = findVertexX(p->getX(), p->getVertexH());

      peano4::particles::assignmentchecks::assignParticleToVertex(
        particleName,
        p->getX(),
        p->getPartid(),
        p->isLocal(),
        vertexX,
        p->getVertexH(),
        treeId,
        "vertexAssign::Sieve",
        prevVertexX,
        prevVertexH
      );
    } // Drift loop
  } // mesh sweep loops

  assertion(
    eventDatabase.getNumberOfTracedParticles() == 1,
    "The database holds a different number of particles than the one this test adds.",
    eventDatabase.getNumberOfTracedParticles()
  );

  peano4::particles::assignmentchecks::ParticleIdentifier identifier =
    peano4::particles::assignmentchecks::ParticleIdentifier(
      "TestParticle",
      localPart->getX(),
      vertexX,
      localPart->getVertexH(),
      localPart->getPartid(),
      treeId
    );

  [[maybe_unused]] const size_t numberOfEntries = eventDatabase.getTotalParticleEntries(identifier);
  // The loop adds four events per sweep and one event comes from the start.
  assertion(
    numberOfEntries == (numberOfSweeps * 4 + 1),
    "The database holds a different number of events for the particle.",
    numberOfEntries,
    numberOfSweeps * 4 + 1,
    eventDatabase.particleHistory(identifier),
    eventDatabase.toString()
  );

  logDebug(eventDatabase.particleHistory(identifier));

  logDebug(eventDatabase.toString());

  for (auto* p : particleSet) {
    delete p;
  }
  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testRepeatedVirtualParticleCleanup() {
#if PEANO_DEBUG

  auto& eventDatabase = peano4::particles::assignmentchecks::Database::getInstance();
  eventDatabase.reset();
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(1000);

  const std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> particleX;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> vertexH;

  for (int i = 0; i < DIMENSIONS; i++) {
    particleX(i) = 1.0;
    vertexX(i)   = 1.0;
    vertexH(i)   = 1.0 / 3.0;
  }

  const int particleId = 1;
  const int treeId     = 3;

  peano4::particles::assignmentchecks::startMeshSweep("DuplicateVirtualParticleCleanup");

  peano4::particles::assignmentchecks::assignParticleToVertex(
    particleName,
    particleX,
    particleId,
    false,
    vertexX,
    vertexH,
    treeId,
    "createdVirtualParticle",
    vertexX,
    vertexH
  );

  peano4::particles::assignmentchecks::detachParticleFromVertex(
    particleName,
    particleX,
    particleId,
    false,
    vertexX,
    vertexH,
    treeId,
    "firstVirtualDetach"
  );
  peano4::particles::assignmentchecks::eraseParticle(
    particleName,
    particleX,
    particleId,
    false,
    vertexX,
    vertexH,
    treeId,
    "firstVirtualErase"
  );

  peano4::particles::assignmentchecks::detachParticleFromVertex(
    particleName,
    particleX,
    particleId,
    false,
    vertexX,
    vertexH,
    treeId,
    "duplicateVirtualDetach"
  );
  peano4::particles::assignmentchecks::eraseParticle(
    particleName,
    particleX,
    particleId,
    false,
    vertexX,
    vertexH,
    treeId,
    "duplicateVirtualErase"
  );

  const peano4::particles::assignmentchecks::ParticleIdentifier identifier =
    peano4::particles::assignmentchecks::ParticleIdentifier(
      particleName,
      particleX,
      vertexX,
      vertexH,
      particleId,
      treeId
    );
  assertion(eventDatabase.getTotalParticleEntries(identifier) == 5, eventDatabase.particleHistory(identifier));

  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::run() {
  testMethod(testTruthTableSearchAndIdKeys);
  testMethod(testAddingSweepsToDatabase);
  testMethod(testAddingParticleEvents);
  testMethod(testRemovingParticleEvents);
  testMethod(testParticleMoveEvents);
  testMethod(testParticleWalkSameTreeLevel);
  testMethod(testParticleLiftDrop);
  testMethod(testParticleWalk);
  testMethod(testPeriodicBoundaryConditions);
  testMethod(testPeriodicBoundaryConditionsCorner);
  testMethod(testSieveSet);
  testMethod(testRepeatedVirtualParticleCleanup);
}
