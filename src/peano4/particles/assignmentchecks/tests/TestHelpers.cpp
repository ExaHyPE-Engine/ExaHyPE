// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "TestHelpers.h"

#include "peano4/particles/assignmentchecks/Database.h"
#include "peano4/particles/assignmentchecks/TracingAPI.h"
#include "peano4/particles/assignmentchecks/tests/TestParticle.h"
#include "tarch/instrumentation/Debug.h"

peano4::particles::assignmentchecks::tests::TestHelpers::TestHelpers() :
  TestCase("peano4::particles::assignmentchecks::tests::TestHelpers") {}

/**
 * Returns true if particleX lies outside the square of two times vertexH
 * around the vertex at vertexX and depth is above zero.
 */
bool peano4::particles::assignmentchecks::tests::internal::liftParticle(
  const tarch::la::Vector<DIMENSIONS, double> particleX,
  const tarch::la::Vector<DIMENSIONS, double> vertexX,
  const tarch::la::Vector<DIMENSIONS, double> vertexH,
  const int                                   depth
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
bool peano4::particles::assignmentchecks::tests::internal::dropParticle(
  const tarch::la::Vector<DIMENSIONS, double> particleX,
  const tarch::la::Vector<DIMENSIONS, double> vertexH,
  const int                                   depth,
  const int                                   maxVertexDepth
) {
  if (depth == maxVertexDepth) {
    return false;
  }

  const tarch::la::Vector<DIMENSIONS, double> newVertexH = vertexH / 3.;
  const tarch::la::Vector<DIMENSIONS, double> vertexX    = findVertexX(particleX, newVertexH);

  const tarch::la::Vector<DIMENSIONS, double> distance = vertexX - particleX;

  if (tarch::la::allSmaller(distance, newVertexH) and tarch::la::allGreater(distance, -1. * newVertexH)) {
    return true;
  }
  return false;
}

/**
 * Returns the index of the vertex of size vertexH that covers position x. The
 * first vertex sits at the origin.
 */
int peano4::particles::assignmentchecks::tests::internal::findVertexInd(double x, double vertexH) {
  // vertexH is half vertex square size. So multiply by 2 here.
  int vertexInd = static_cast<int>(std::floor((x / (2. * vertexH)) + 0.5));

  return vertexInd;
}

/**
 * Returns the position of the vertex of size vertexH that covers x. The first
 * vertex sits at the origin.
 */
tarch::la::Vector<DIMENSIONS, double> peano4::particles::assignmentchecks::tests::internal::findVertexX(
  const tarch::la::Vector<DIMENSIONS, double> x,
  const tarch::la::Vector<DIMENSIONS, double> vertexH
) {
  tarch::la::Vector<DIMENSIONS, double> vertexX;

  for (int i = 0; i < DIMENSIONS; i++) {
    int vertexInd = internal::findVertexInd(x(i), vertexH(i));
    vertexX(i)    = 2 * vertexInd * vertexH(i);
  }

  return vertexX;
}

/**
 * Returns a virtual copy of original that is moved by boundary along every
 * axis whose flag is true. The caller deletes the copy.
 */
peano4::particles::assignmentchecks::tests::TestParticle* peano4::particles::assignmentchecks::tests::internal::
  mirrorParticleAlongPeriodicBoundary(
    peano4::particles::assignmentchecks::tests::TestParticle* original,
    bool                                                       mirrorX,
    bool                                                       mirrorY,
    bool                                                       mirrorZ,
    tarch::la::Vector<DIMENSIONS, double>                      boundary
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

  TestParticle* mirrored = new TestParticle(
    virtualPartX,
    original->getPartid(),
    false // virtual particle
  );
  mirrored->setVertexH(original->getVertexH());
  mirrored->setDepth(original->getDepth());

  return mirrored;
}

/**
 * Checks that two ParticleIdentifier objects of the same particle compare equal
 * and that the comparison orders them by their particle number.
 */
void peano4::particles::assignmentchecks::tests::TestHelpers::testTruthTableSearchAndIdKeys() {
  namespace ac = ::peano4::particles::assignmentchecks;

  tarch::la::Vector<DIMENSIONS, double> particleAX;
  tarch::la::Vector<DIMENSIONS, double> particleBX;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> vertexH;
  int                                   treeId = 1;

  for (int i = 0; i < DIMENSIONS; i++) {
    particleAX(i) = 1.;
    particleBX(i) = 2.;
    vertexX(i)    = 1.;
    vertexH(i)    = 1.;
  }

  ac::internal::ParticleIdentifier firstA =
    ac::internal::ParticleIdentifier("DummyParticle", particleAX, vertexX, vertexH, 1, treeId);
  ac::internal::ParticleIdentifier firstB =
    ac::internal::ParticleIdentifier("DummyParticle", particleBX, vertexX, vertexH, 2, treeId);

  logDebug(
    "firstA < firstB:" << (firstA < firstB) << "(1) firstA==firstB:" << (firstA == firstB) << "(0) firstA==firstA:"
                       << (firstA == firstA) << "(1) firstB==firstB:" << (firstB == firstB) << "(1)"
  );

  assertion(firstA < firstB);
  assertion(not(firstA == firstB));
  assertion(not(firstB == firstA));
  assertion(not(firstB < firstA));

  ac::internal::ParticleIdentifier secondA =
    ac::internal::ParticleIdentifier("DummyParticle", particleAX, vertexX, vertexH, 1, treeId);
  ac::internal::ParticleIdentifier secondB =
    ac::internal::ParticleIdentifier("DummyParticle", particleBX, vertexX, vertexH, 2, treeId);

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
  namespace ac = ::peano4::particles::assignmentchecks;

  ac::internal::Database& eventDatabase = ac::internal::Database::getInstance();
  eventDatabase.reset();
  ac::checkDatabaseIsEmpty();

  std::vector<std::string> meshSweepNames = {"initial", "alpha", "beta", "gamma", "delta"};

  std::vector<std::string>::iterator sweep = meshSweepNames.begin();
  // Skip first, which gets automatically added to the database.
  sweep++;

  while (sweep != meshSweepNames.end()) {
    eventDatabase.startMeshSweep(*sweep);
    sweep++;
  }

  assertion(
    eventDatabase.getMeshSweepData().size() == meshSweepNames.size(),
    "Wrong number of mesh sweeps in database",
    eventDatabase.getMeshSweepData().size(),
    meshSweepNames.size(),
    eventDatabase.toString()
  );

  assertion(
    (eventDatabase.getCurrentMeshSweepIndex() + 1) == meshSweepNames.size(),
    "Wrong count of mesh sweeps in database",
    eventDatabase.getMeshSweepData().size(),
    meshSweepNames.size(),
    eventDatabase.toString()
  );

  auto databaseSweep = eventDatabase.getMeshSweepData().begin();
  sweep              = meshSweepNames.begin();

  while (sweep != meshSweepNames.end()) {
    assertion(*sweep == databaseSweep->getName(), "Wrong Sweep Name", *sweep, databaseSweep->getName());
    sweep++;
    databaseSweep++;
  }

  eventDatabase.reset();
  ac::checkDatabaseIsEmpty();
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testAddingParticleEvents() {
  namespace ac = ::peano4::particles::assignmentchecks;

  ac::internal::Database& eventDatabase = ac::internal::Database::getInstance();
  eventDatabase.reset();
  ac::checkDatabaseIsEmpty();
  // The limit stays above the number of events that the loop below adds.
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(100);

  int                                   treeId  = 1;
  bool                                  isLocal = true;
  tarch::la::Vector<DIMENSIONS, double> particleX;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> vertexH;

  std::string trace;

  std::vector<std::string> meshSweepNames = {"alpha", "beta", "gamma", "delta"};

  int                                numberOfParticles = 10;
  std::vector<std::string>::iterator sweep             = meshSweepNames.begin();
  while (sweep != meshSweepNames.end()) {
    ac::startMeshSweep(*sweep);

    for (int p = 1; p < numberOfParticles + 1; p++) {
      int particleId = p;

      for (int i = 0; i < DIMENSIONS; i++) {
        particleX(i) = p;
        vertexX(i)   = p;
        vertexH(i)   = p;
      }

      ac::internal::ParticleIdentifier identifier =
        ac::internal::ParticleIdentifier("DummyParticle", particleX, vertexX, vertexH, particleId, treeId);

      trace                           = "Assign/sweep:" + *sweep;
      ac::internal::Event assignEvent = ac::internal::Event(
        ac::internal::Event::Type::AssignToVertex,
        isLocal,
        vertexX,
        particleX,
        vertexH,
        treeId,
        trace
      );

      eventDatabase.addEvent(identifier, assignEvent);

      trace                         = "Move/sweep:" + *sweep;
      ac::internal::Event moveEvent = ac::internal::Event(
        ac::internal::Event::Type::MoveWhileAssociatedToVertex,
        vertexX,
        particleX,
        vertexH,
        treeId,
        trace
      );
      eventDatabase.addEvent(identifier, moveEvent);

      trace = "Sieve/sweep:" + *sweep;
      ac::internal::Event sieveEvent =
        ac::internal::Event(ac::internal::Event::Type::AssignToSieveSet, isLocal, treeId, trace);
      eventDatabase.addEvent(identifier, sieveEvent);

      trace                            = "NotFound/sweep:" + *sweep;
      ac::internal::Event invalidEvent = ac::internal::Event(ac::internal::Event::Type::NotFound);
      eventDatabase.addEvent(identifier, invalidEvent);
    }

    sweep++;
  }

  assertion(
    eventDatabase.getNumberOfTracedParticles() == static_cast<int>(numberOfParticles),
    "Wrong particle count in database",
    eventDatabase.getNumberOfTracedParticles(),
    numberOfParticles
  );

  for (int p = 1; p < numberOfParticles + 1; p++) {
    int particleId = p;

    for (int i = 0; i < DIMENSIONS; i++) {
      particleX(i) = p;
      vertexX(i)   = p;
      vertexH(i)   = p;
    }

    ac::internal::ParticleIdentifier identifier =
      ac::internal::ParticleIdentifier("DummyParticle", particleX, vertexX, vertexH, particleId, treeId);

    [[maybe_unused]] size_t numberOfEntries = eventDatabase.getTotalParticleEntries(identifier);
    // The loop adds four events per sweep.
    assertion(
      numberOfEntries == (meshSweepNames.size() * 4),
      "Wrong number of entries for particle ",
      p,
      numberOfEntries,
      meshSweepNames.size() * 4
    );

    logDebug(eventDatabase.particleHistory(identifier));
  }

  eventDatabase.reset();
  ac::checkDatabaseIsEmpty();
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testRemovingParticleEvents() {
  namespace ac = ::peano4::particles::assignmentchecks;

  ac::internal::Database& eventDatabase = ac::internal::Database::getInstance();
  eventDatabase.reset();
  ac::checkDatabaseIsEmpty();
  // The limit stays above the number of events that the loop below adds.
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(20);

  tarch::la::Vector<DIMENSIONS, double> particleX;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> vertexH;

  std::string trace;

  std::vector<std::string> meshSweepNames = {"alpha", "beta", "gamma", "delta", "foo", "bar", "baz", "boo"};

  int                                numberOfParticles = 4;
  std::vector<std::string>::iterator sweep             = meshSweepNames.begin();

  while (sweep != meshSweepNames.end()) {
    ac::startMeshSweep(*sweep);

    // Make a tree for each particle. Each tree contains
    // all particles, but only 1 particle is local.
    for (int treeId = 0; treeId < numberOfParticles; treeId++) {
      for (int p = 1; p < numberOfParticles + 1; p++) {
        bool isLocal = (treeId == p - 1);

        int particleId = p;

        for (int i = 0; i < DIMENSIONS; i++) {
          particleX(i) = p;
          vertexX(i)   = p;
          vertexH(i)   = p;
        }

        ac::internal::ParticleIdentifier identifier =
          ac::internal::ParticleIdentifier("DummyParticle", particleX, vertexX, vertexH, particleId, treeId);

        trace                           = "Assign/sweep:" + *sweep;
        ac::internal::Event assignEvent = ac::internal::Event(
          ac::internal::Event::Type::AssignToVertex,
          isLocal,
          vertexX,
          particleX,
          vertexH,
          treeId,
          trace
        );

        eventDatabase.addEvent(identifier, assignEvent);

        trace                         = "Move/sweep:" + *sweep;
        ac::internal::Event moveEvent = ac::internal::Event(
          ac::internal::Event::Type::MoveWhileAssociatedToVertex,
          vertexX,
          particleX,
          vertexH,
          treeId,
          trace
        );
        eventDatabase.addEvent(identifier, moveEvent);

        trace = "Sieve/sweep:" + *sweep;
        ac::internal::Event sieveEvent =
          ac::internal::Event(ac::internal::Event::Type::AssignToSieveSet, isLocal, treeId, trace);
        eventDatabase.addEvent(identifier, sieveEvent);
      }
    }

    sweep++;
  }

  assertion(
    eventDatabase.getNumberOfTracedParticles() == numberOfParticles * numberOfParticles,
    "Wrong particle count in database",
    eventDatabase.getNumberOfTracedParticles(),
    numberOfParticles * numberOfParticles
  );

  eventDatabase.reset();
  ac::checkDatabaseIsEmpty();
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testParticleMoveEvents() {
  // Run without cleaning up database on-the-fly
  testAddingParticleMovingEvents(100, 1000);
  // Run with cleaning up database on-the-fly
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
  namespace ac = ::peano4::particles::assignmentchecks;

  ac::internal::Database& eventDatabase = ac::internal::Database::getInstance();
  eventDatabase.reset();
  ac::checkDatabaseIsEmpty();
  // The limit decides whether the database deletes events during the run.
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(numberOfEventsToKeep);

  int    particleId = 1;
  int    treeId     = 1;
  double dt         = 1.;

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
    std::string sweepName = "Sweep" + std::to_string(sweep);
    ac::startMeshSweep(sweepName);

    for (int i = 0; i < DIMENSIONS; i++) {
      particleX(i) += dx(i) * dt;
    }

    ac::internal::ParticleIdentifier identifier =
      ac::internal::ParticleIdentifier("DummyParticle", particleX, vertexX, vertexH, particleId, treeId);

    std::string         trace     = "Move/sweep:" + std::to_string(sweep);
    ac::internal::Event moveEvent = ac::internal::Event(
      ac::internal::Event::Type::MoveWhileAssociatedToVertex,
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
    "Wrong particle count in database, should be 1",
    eventDatabase.getNumberOfTracedParticles()
  );

  ac::internal::ParticleIdentifier identifier =
    ac::internal::ParticleIdentifier("DummyParticle", particleX, vertexX, vertexH, particleId, treeId);

  [[maybe_unused]] int numberOfEntries = eventDatabase.getTotalParticleEntries(identifier);
  // The database replaces the past trajectory by its last event, so one of the
  // kept events carries that substitute.
  assertion(
    numberOfEntries == (numberOfSweeps % (numberOfEventsToKeep - 1)),
    "Wrong number of entries for particle ",
    numberOfEntries,
    numberOfSweeps,
    numberOfEventsToKeep,
    numberOfSweeps % (numberOfEventsToKeep - 1),
    eventDatabase.particleHistory(identifier)
  );

  logDebug(eventDatabase.particleHistory(identifier));

  eventDatabase.reset();
  ac::checkDatabaseIsEmpty();
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testParticleWalkSameTreeLevel() {
#if PEANO_DEBUG

  namespace ac = peano4::particles::assignmentchecks;

  ac::internal::Database& eventDatabase = ac::internal::Database::getInstance();
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(100);

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> localPartX;
  // Displacement of the particle per step.
  tarch::la::Vector<DIMENSIONS, double> dx;
  // Assume we have vertices of a single size.
  tarch::la::Vector<DIMENSIONS, double> vertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> prevVertexX;

  double defaultVertexH = 3.;
  double dt             = 1.;

  for (int i = 0; i < DIMENSIONS; i++) {
    localPartX(i) = 51.5;
    dx(i)         = -1.;
    vertexH(i)    = defaultVertexH;
  }

  int particleId = 1;
  int treeId     = 1;

  vertexX       = internal::findVertexX(localPartX, vertexH);
  int vertexInd = internal::findVertexInd(localPartX(0), vertexH(0));

  ac::tests::TestParticle               localPart         = ac::tests::TestParticle(localPartX, particleId, true);
  std::vector<ac::tests::TestParticle*> assignedParticles = {&localPart};

  ac::startMeshSweep("InitUnitTest");
  ac::assignParticleToVertex(
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
  size_t maxSweeps = 14;
  for (size_t sweep = 0; sweep < maxSweeps; sweep++) {
    ac::startMeshSweep("MySweep" + std::to_string(sweep));

    auto oldParticlePositions = ac::recordParticlePositions(assignedParticles);

    double dtThisStep = dt;
    if (sweep > 8 and sweep < 12) {
      dtThisStep *= 1e-2;
    }

    localPartX += dtThisStep * dx;
    localPart.setX(localPartX);

    ac::traceParticleMovements(assignedParticles, oldParticlePositions, vertexX, vertexH, treeId);

    logDebug("Moving particle to " << localPartX);

    int prevVertexInd = vertexInd;
    vertexInd         = internal::findVertexInd(localPartX(0), vertexH(0));
    for (int i = 0; i < DIMENSIONS; i++) {
      prevVertexX(i) = vertexX(i);
    }
    vertexX = internal::findVertexX(localPartX, vertexH);

    logDebug("Checking vertex");

    if (vertexInd != prevVertexInd) {
      logDebug("Detaching vertex");
      ac::detachParticleFromVertex(
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
      ac::assignParticleToVertex(
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
    "Wrong particle count in database",
    eventDatabase.getNumberOfTracedParticles()
  );

  // The database registers one mesh sweep of its own at construction.
  assertion(
    eventDatabase.getCurrentMeshSweepIndex() == maxSweeps + 1,
    "Wrong mesh sweep count in database",
    eventDatabase.getCurrentMeshSweepIndex(),
    maxSweeps
  );

  ac::internal::ParticleIdentifier identifier =
    ac::internal::ParticleIdentifier("TestParticle", localPartX, vertexX, vertexH, particleId, treeId);

  // 8 is hardcoded here. Modify manually if you change test setup.
  assertion(
    eventDatabase.getTotalParticleEntries(identifier) == 8,
    "Wrong number of snapshots stored for particle",
    eventDatabase.getTotalParticleEntries(identifier),
    eventDatabase.particleHistory(identifier)
  );

  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testParticleLiftDrop() {
#if PEANO_DEBUG

  namespace ac = peano4::particles::assignmentchecks;

  ac::internal::Database& eventDatabase = ac::internal::Database::getInstance();

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> localPartX;
  tarch::la::Vector<DIMENSIONS, double> vertexH, prevVertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX, prevVertexX;

  double defaultVertexH = 3.;
  int    depth          = 3;
  int    maxVertexDepth = 5;

  for (int i = 0; i < DIMENSIONS; i++) {
    localPartX(i) = 51.5;
    vertexH(i)    = defaultVertexH;
    vertexX(i)    = 27.;
  }

  int particleId = 1;
  int treeId     = 1;

  ac::tests::TestParticle               localPart         = ac::tests::TestParticle(localPartX, particleId, true);
  std::vector<ac::tests::TestParticle*> assignedParticles = {&localPart};

  ac::startMeshSweep("InitUnitTest");
  ac::assignParticleToVertex(
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

  ac::startMeshSweep("Lifts");

  while (internal::liftParticle(localPartX, vertexX, vertexH, depth)) {
    depth--;
    prevVertexH = vertexH;
    prevVertexX = vertexX;
    vertexH *= 3.;
    vertexX = internal::findVertexX(vertexX, vertexH);

    logDebug("Lifting to H=" << vertexH << " X=" << vertexX << " depth=" << depth);

    ac::detachParticleFromVertex(
      particleName,
      localPart.getX(),
      localPart.getPartid(),
      true, // this particle is always local
      prevVertexX,
      prevVertexH,
      treeId,
      "vertexDetachLift"
    );

    ac::assignParticleToVertex(
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

  ac::startMeshSweep("Drops");

  while (internal::dropParticle(localPartX, vertexH, depth, maxVertexDepth)) {
    depth++;
    prevVertexH = vertexH;
    prevVertexX = vertexX;
    vertexH /= 3.;
    vertexX = internal::findVertexX(localPartX, vertexH);

    logDebug("Dropping to H=" << vertexH << " X=" << vertexX << " depth=" << depth);

    ac::detachParticleFromVertex(
      particleName,
      localPart.getX(),
      localPart.getPartid(),
      true, // this particle is always local
      prevVertexX,
      prevVertexH,
      treeId,
      "vertexDetachDrop"
    );

    ac::assignParticleToVertex(
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
    "Wrong particle count in database",
    eventDatabase.getNumberOfTracedParticles()
  );

  ac::internal::ParticleIdentifier identifier =
    ac::internal::ParticleIdentifier("TestParticle", localPartX, vertexX, vertexH, particleId, treeId);

  // 13 is hardcoded here. Modify manually if you change test setup.
  assertion(
    eventDatabase.getTotalParticleEntries(identifier) == 13,
    "Wrong number of snapshots stored for particle",
    eventDatabase.getTotalParticleEntries(identifier),
    eventDatabase.particleHistory(identifier)
  );

  logDebug(eventDatabase.toString());

  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testParticleWalk() {
#if PEANO_DEBUG

  namespace ac = peano4::particles::assignmentchecks;

  ac::internal::Database& eventDatabase = ac::internal::Database::getInstance();

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> localPartX;
  // Displacement of the particle per step.
  tarch::la::Vector<DIMENSIONS, double> dx;
  tarch::la::Vector<DIMENSIONS, double> vertexH, prevVertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX, prevVertexX;

  double defaultVertexH = 3.;
  double dt             = 0.01;
  int    depth          = 4;
  int    maxVertexDepth = 6;

  for (int i = 0; i < DIMENSIONS; i++) {
    localPartX(i) = 51.5;
    dx(i)         = -1.;
    vertexH(i)    = defaultVertexH;
  }

  int particleId = 1;
  int treeId     = 1;

  vertexX = internal::findVertexX(localPartX, vertexH);

  ac::tests::TestParticle               localPart         = ac::tests::TestParticle(localPartX, particleId, true);
  std::vector<ac::tests::TestParticle*> assignedParticles = {&localPart};

  ac::startMeshSweep("InitUnitTest");
  ac::assignParticleToVertex(
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
    ac::startMeshSweep("MySweep" + std::to_string(sweep));

    auto oldParticlePositions = ac::recordParticlePositions(assignedParticles);

    double dtThisStep = dt;
    if (sweep > 9 and sweep < 256) {
      dtThisStep *= 1e-2;
    }

    double dxThisStep = dtThisStep * dx(0);
    // The checks accept a movement below the size of the vertex.
    assertion(dxThisStep <= vertexH(0));

    localPartX += dxThisStep;
    localPart.setX(localPartX);

    ac::traceParticleMovements(assignedParticles, oldParticlePositions, vertexX, vertexH, treeId);

    logDebug("Moving particle to " << localPartX);

    logDebug("Checking vertex H=" << vertexH << " X=" << vertexX);

    while (internal::liftParticle(localPartX, vertexX, vertexH, depth)) {
      depth--;
      prevVertexH = vertexH;
      prevVertexX = vertexX;
      vertexH *= 3.;
      vertexX = internal::findVertexX(localPartX, vertexH);

      logDebug("Lifting to H=" << vertexH << " X=" << vertexX << " depth=" << depth);

      ac::detachParticleFromVertex(
        particleName,
        localPart.getX(),
        localPart.getPartid(),
        true, // this particle is always local
        prevVertexX,
        prevVertexH,
        treeId,
        "vertexDetachLift"
      );

      ac::assignParticleToVertex(
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

    while (internal::dropParticle(localPartX, vertexH, depth, maxVertexDepth)) {
      depth++;
      prevVertexH = vertexH;
      prevVertexX = vertexX;
      vertexH /= 3.;
      vertexX = internal::findVertexX(localPartX, vertexH);

      logDebug("Dropping to H=" << vertexH << " X=" << vertexX << " depth=" << depth);

      ac::detachParticleFromVertex(
        particleName,
        localPart.getX(),
        localPart.getPartid(),
        true, // this particle is always local
        prevVertexX,
        prevVertexH,
        treeId,
        "vertexDetachDrop"
      );

      ac::assignParticleToVertex(
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

  namespace ac = peano4::particles::assignmentchecks;

  ac::internal::Database& eventDatabase = ac::internal::Database::getInstance();
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(1000);

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> initialLocalPartX;
  // Displacement of the particle per step.
  tarch::la::Vector<DIMENSIONS, double> dx;
  // The particle travels diagonally, so one point stands for the boundary plane.
  tarch::la::Vector<DIMENSIONS, double> boundary;
  tarch::la::Vector<DIMENSIONS, double> zeroCorner;

  tarch::la::Vector<DIMENSIONS, double> initialVertexH, prevVertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX, prevVertexX;

  double defaultVertexH = 3.;
  double dt             = 1.;
  int    depth          = 4;
  int    maxVertexDepth = 5;

  for (int i = 0; i < DIMENSIONS; i++) {
    initialLocalPartX(i) = 7.5;
    dx(i)                = -0.3;
    initialVertexH(i)    = defaultVertexH;
    boundary(i)          = 100.;
    zeroCorner(i)        = 0.;
  }

  int particleId = 1;
  int treeId     = 1;

  ac::tests::TestParticle* localPart = new ac::tests::TestParticle(initialLocalPartX, particleId, true);
  localPart->setVertexH(initialVertexH);
  localPart->setDepth(depth);

  std::vector<ac::tests::TestParticle*> particleSet = {localPart};

  vertexX = internal::findVertexX(localPart->getX(), localPart->getVertexH());

  ac::startMeshSweep("InitUnitTest");
  ac::assignParticleToVertex(
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
    ac::startMeshSweep("MySweep" + std::to_string(sweep));

    auto oldParticlePositions = ac::recordParticlePositions(particleSet);

    for (auto p : particleSet) {
      // The vertex that holds the particle has to be found before the move.
      vertexX = internal::findVertexX(p->getX(), p->getVertexH());

      tarch::la::Vector<DIMENSIONS, double> partX = p->getX();
      partX += dt * dx;
      p->setX(partX);

      ac::traceParticleMovements(particleSet, oldParticlePositions, vertexX, p->getVertexH(), treeId);

      logDebug("Moving particle to " << partX);

      logDebug("Checking vertex H=" << p->getVertexH() << " X=" << vertexX);

      while (internal::liftParticle(p->getX(), vertexX, p->getVertexH(), p->getDepth())) {
        p->setDepth(p->getDepth() - 1);
        prevVertexH = p->getVertexH();
        prevVertexX = vertexX;
        p->setVertexH(p->getVertexH() * 3.);
        vertexX = internal::findVertexX(p->getX(), p->getVertexH());

        logDebug("Lifting to H=" << p->getVertexH() << " X=" << vertexX << " depth=" << p->getDepth());

        ac::detachParticleFromVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          prevVertexX,
          prevVertexH,
          treeId,
          "vertexDetach::Lift"
        );

        ac::assignParticleToVertex(
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

      while (internal::dropParticle(p->getX(), p->getVertexH(), p->getDepth(), maxVertexDepth)) {
        p->setDepth(p->getDepth() + 1);
        prevVertexH = p->getVertexH();
        prevVertexX = vertexX;
        p->setVertexH(p->getVertexH() / 3.);
        vertexX = internal::findVertexX(p->getX(), p->getVertexH());

        logDebug("Dropping to H=" << p->getVertexH() << " X=" << vertexX << " depth=" << p->getDepth());

        ac::detachParticleFromVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          prevVertexX,
          prevVertexH,
          treeId,
          "vertexDetach::Drop"
        );

        ac::assignParticleToVertex(
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

    for (auto p : particleSet) {
      // The particle travels diagonally, so one coordinate decides.
      int vertexInd = internal::findVertexInd(p->getX()[0], p->getVertexH()[0]);
      if (vertexInd == 0) {
        // The particle sits on the (0, 0, 0) boundary vertex and is replicated
        // across the boundary.
        tarch::la::Vector<DIMENSIONS, double> virtualPartX = p->getX() + boundary;
        TestParticle*                         replica      = new TestParticle(
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
        vertexX = internal::findVertexX(replica->getX(), replica->getVertexH());

        ac::assignParticleToVertex(
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
    for (auto p : particleSet) {
      if (p->isLocal()) {
        if (tarch::la::oneSmaller(p->getX(), zeroCorner)) {
          vertexX = internal::findVertexX(p->getX(), p->getVertexH());

          ac::detachParticleFromVertex(
            particleName,
            p->getX(),
            p->getPartid(),
            true, // here it's local
            vertexX,
            p->getVertexH(),
            treeId,
            "vertexDetach::LocalToVirtual"
          );

          ac::assignParticleToVertex(
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
          vertexX = internal::findVertexX(p->getX(), p->getVertexH());
          ac::detachParticleFromVertex(
            particleName,
            p->getX(),
            p->getPartid(),
            false, // here it's virtual
            vertexX,
            p->getVertexH(),
            treeId,
            "vertexDetach::VirtualToLocal"
          );

          ac::assignParticleToVertex(
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
        logDebug("erasing " << (*p)->getX());

        vertexX = internal::findVertexX((*p)->getX(), (*p)->getVertexH());

        ac::detachParticleFromVertex(
          particleName,
          (*p)->getX(),
          (*p)->getPartid(),
          (*p)->isLocal(),
          vertexX,
          (*p)->getVertexH(),
          treeId,
          "vertexDetachErase"
        );
        ac::eraseParticle(
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

  for (auto p : particleSet) {
    delete p;
  }
  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testPeriodicBoundaryConditionsCorner() {
#if PEANO_DEBUG

  namespace ac = peano4::particles::assignmentchecks;

  ac::internal::Database& eventDatabase = ac::internal::Database::getInstance();
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(1000);

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> initialLocalPartX;
  // Displacement of the particle per step.
  tarch::la::Vector<DIMENSIONS, double> dx;
  // The particle travels diagonally, so one point stands for the boundary plane.
  tarch::la::Vector<DIMENSIONS, double> boundary;
  tarch::la::Vector<DIMENSIONS, double> zeroCorner;

  tarch::la::Vector<DIMENSIONS, double> initialVertexH, prevVertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX, prevVertexX;

  double defaultVertexH = 0.33333333;
  double dt             = 1.;
  int    depth          = 1;
  int    maxVertexDepth = 2;

  for (int i = 0; i < DIMENSIONS; i++) {
    initialLocalPartX(i) = 0.01;
    dx(i)                = 0.000001;
    initialVertexH(i)    = defaultVertexH;
    boundary(i)          = 1.;
    zeroCorner(i)        = 0.;
  }

  int particleId = 1;
  int treeId     = 1;

  ac::tests::TestParticle* localPart = new ac::tests::TestParticle(initialLocalPartX, particleId, true);
  localPart->setVertexH(initialVertexH);
  localPart->setDepth(depth);

  std::vector<ac::tests::TestParticle*> particleSet = {localPart};

  vertexX = internal::findVertexX(localPart->getX(), localPart->getVertexH());

  ac::startMeshSweep("InitUnitTest");
  ac::assignParticleToVertex(
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
    ac::startMeshSweep("MySweep" + std::to_string(sweep));

    auto oldParticlePositions = ac::recordParticlePositions(particleSet);

    for (auto p : particleSet) {
      // The vertex that holds the particle has to be found before the move.
      vertexX = internal::findVertexX(p->getX(), p->getVertexH());

      tarch::la::Vector<DIMENSIONS, double> partX = p->getX();
      partX += dt * dx;
      p->setX(partX);

      ac::traceParticleMovements(particleSet, oldParticlePositions, vertexX, p->getVertexH(), treeId);

      logDebug("Moving particle to " << partX);

      logDebug("Checking vertex H=" << p->getVertexH() << " X=" << vertexX);

      while (internal::liftParticle(p->getX(), vertexX, p->getVertexH(), p->getDepth())) {
        p->setDepth(p->getDepth() - 1);
        prevVertexH = p->getVertexH();
        prevVertexX = vertexX;
        p->setVertexH(p->getVertexH() * 3.);
        vertexX = internal::findVertexX(p->getX(), p->getVertexH());

        logDebug("Lifting to H=" << p->getVertexH() << " X=" << vertexX << " depth=" << p->getDepth());

        ac::detachParticleFromVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          prevVertexX,
          prevVertexH,
          treeId,
          "vertexDetach::Lift"
        );

        ac::assignParticleToVertex(
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

      while (internal::dropParticle(p->getX(), p->getVertexH(), p->getDepth(), maxVertexDepth)) {
        p->setDepth(p->getDepth() + 1);
        prevVertexH = p->getVertexH();
        prevVertexX = vertexX;
        p->setVertexH(p->getVertexH() / 3.);
        vertexX = internal::findVertexX(p->getX(), p->getVertexH());

        logDebug("Dropping to H=" << p->getVertexH() << " X=" << vertexX << " depth=" << p->getDepth());

        ac::detachParticleFromVertex(
          particleName,
          p->getX(),
          p->getPartid(),
          p->isLocal(),
          prevVertexX,
          prevVertexH,
          treeId,
          "vertexDetach::Drop"
        );

        ac::assignParticleToVertex(
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

    std::vector<ac::tests::TestParticle*> mirroredParticleSet;

    for (auto p : particleSet) {
      // The particle travels diagonally, so one coordinate decides.
      int vertexInd = internal::findVertexInd(p->getX()[0], p->getVertexH()[0]);
      if (vertexInd == 0) {
        // The particle sits on the (0, 0, 0) boundary vertex and is replicated
        // across the boundary.
        if (DIMENSIONS > 0) {
          ac::tests::TestParticle* mirror1 =
            internal::mirrorParticleAlongPeriodicBoundary(p, true, false, false, boundary);
          mirroredParticleSet.push_back(mirror1);
          logDebug("Creating virtual particle at " << mirror1->getX(););
        }
        if (DIMENSIONS > 1) {
          ac::tests::TestParticle* mirror2 =
            internal::mirrorParticleAlongPeriodicBoundary(p, true, true, false, boundary);
          mirroredParticleSet.push_back(mirror2);
          logDebug("Creating virtual particle at " << mirror2->getX(););
          ac::tests::TestParticle* mirror3 =
            internal::mirrorParticleAlongPeriodicBoundary(p, false, true, false, boundary);
          mirroredParticleSet.push_back(mirror3);
          logDebug("Creating virtual particle at " << mirror3->getX(););
        }
        if (DIMENSIONS > 2) {
          ac::tests::TestParticle* mirror4 =
            internal::mirrorParticleAlongPeriodicBoundary(p, true, true, true, boundary);
          mirroredParticleSet.push_back(mirror4);
          logDebug("Creating virtual particle at " << mirror4->getX(););
          ac::tests::TestParticle* mirror5 =
            internal::mirrorParticleAlongPeriodicBoundary(p, false, true, true, boundary);
          mirroredParticleSet.push_back(mirror5);
          logDebug("Creating virtual particle at " << mirror5->getX(););
          ac::tests::TestParticle* mirror6 =
            internal::mirrorParticleAlongPeriodicBoundary(p, false, false, true, boundary);
          mirroredParticleSet.push_back(mirror6);
          logDebug("Creating virtual particle at " << mirror6->getX(););
          ac::tests::TestParticle* mirror7 =
            internal::mirrorParticleAlongPeriodicBoundary(p, true, false, true, boundary);
          mirroredParticleSet.push_back(mirror7);
          logDebug("Creating virtual particle at " << mirror7->getX(););
        }
      }
    }

    for (auto p : mirroredParticleSet) {
      // The virtual particle sits at the depth of the local particle, so it
      // keeps vertexH and takes a new vertexX.
      vertexX = internal::findVertexX(p->getX(), p->getVertexH());

      ac::assignParticleToVertex(
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
        logDebug("erasing " << (*p)->getX());

        vertexX = internal::findVertexX((*p)->getX(), (*p)->getVertexH());

        ac::detachParticleFromVertex(
          particleName,
          (*p)->getX(),
          (*p)->getPartid(),
          (*p)->isLocal(),
          vertexX,
          (*p)->getVertexH(),
          treeId,
          "vertexDetachErase"
        );
        ac::eraseParticle(
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

  for (auto p : particleSet) {
    delete p;
  }
  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testSieveSet() {
#if PEANO_DEBUG

  namespace ac = peano4::particles::assignmentchecks;

  ac::internal::Database& eventDatabase = ac::internal::Database::getInstance();
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(1000);

  // The name has to match the class name, as traceParticleMovements() derives it from the type.
  std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> initialLocalPartX;
  // Displacement of the particle per step.
  tarch::la::Vector<DIMENSIONS, double> dx;

  tarch::la::Vector<DIMENSIONS, double> vertexH, prevVertexH;
  tarch::la::Vector<DIMENSIONS, double> vertexX, prevVertexX;

  double defaultVertexH = 3.;
  double dt             = 1.;

  for (int i = 0; i < DIMENSIONS; i++) {
    initialLocalPartX(i) = 7.5;
    dx(i)                = 0.3 * defaultVertexH;
    vertexH(i)           = defaultVertexH;
  }

  int particleId = 1;
  int treeId     = 1;

  ac::tests::TestParticle* localPart = new ac::tests::TestParticle(initialLocalPartX, particleId, true);
  localPart->setVertexH(vertexH);
  localPart->setDepth(1);

  std::vector<ac::tests::TestParticle*> particleSet = {localPart};

  vertexX = internal::findVertexX(localPart->getX(), localPart->getVertexH());

  ac::startMeshSweep("InitUnitTest");
  ac::assignParticleToVertex(
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

  size_t numberOfSweeps = 10;
  for (size_t sweep = 0; sweep < numberOfSweeps; sweep++) {
    ac::startMeshSweep("MySweep" + std::to_string(sweep));

    auto oldParticlePositions = ac::recordParticlePositions(particleSet);

    for (auto p : particleSet) {
      // The vertex that holds the particle has to be found before the move.
      vertexX = internal::findVertexX(p->getX(), p->getVertexH());

      tarch::la::Vector<DIMENSIONS, double> partX = p->getX();
      partX += dt * dx;
      p->setX(partX);

      ac::traceParticleMovements(particleSet, oldParticlePositions, vertexX, p->getVertexH(), treeId);

      logDebug("Moving particle to " << partX);

      prevVertexH = p->getVertexH();
      prevVertexX = vertexX;

      ac::detachParticleFromVertex(
        particleName,
        p->getX(),
        p->getPartid(),
        p->isLocal(),
        prevVertexX,
        prevVertexH,
        treeId,
        "vertexDetach"
      );

      ac::assignParticleToSieveSet(
        particleName,
        p->getX(),
        p->getPartid(),
        p->isLocal(),
        prevVertexX,
        prevVertexH,
        treeId,
        "assignToSieveSet"
      );
      vertexX = internal::findVertexX(p->getX(), p->getVertexH());

      ac::assignParticleToVertex(
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
    "Wrong particle count in database",
    eventDatabase.getNumberOfTracedParticles()
  );

  ac::internal::ParticleIdentifier identifier = ac::internal::ParticleIdentifier(
    "TestParticle",
    localPart->getX(),
    vertexX,
    localPart->getVertexH(),
    localPart->getPartid(),
    treeId
  );

  [[maybe_unused]] size_t numberOfEntries = eventDatabase.getTotalParticleEntries(identifier);
  // The loop adds four events per sweep and one event comes from the start.
  assertion(
    numberOfEntries == (numberOfSweeps * 4 + 1),
    "Wrong number of entries for particle ",
    numberOfEntries,
    numberOfSweeps * 4 + 1,
    eventDatabase.particleHistory(identifier),
    eventDatabase.toString()
  );

  logDebug(eventDatabase.particleHistory(identifier));

  logDebug(eventDatabase.toString());

  for (auto p : particleSet) {
    delete p;
  }
  eventDatabase.reset();

#endif
}

void peano4::particles::assignmentchecks::tests::TestHelpers::testRepeatedVirtualParticleCleanup() {
#if PEANO_DEBUG

  namespace ac = peano4::particles::assignmentchecks;

  ac::internal::Database& eventDatabase = ac::internal::Database::getInstance();
  eventDatabase.reset();
  eventDatabase.setMaxParticleSnapshotsToKeepTrackOf(1000);

  std::string particleName = "TestParticle";

  tarch::la::Vector<DIMENSIONS, double> particleX;
  tarch::la::Vector<DIMENSIONS, double> vertexX;
  tarch::la::Vector<DIMENSIONS, double> vertexH;

  for (int i = 0; i < DIMENSIONS; i++) {
    particleX(i) = 1.0;
    vertexX(i)   = 1.0;
    vertexH(i)   = 1.0 / 3.0;
  }

  int particleId = 1;
  int treeId     = 3;

  ac::startMeshSweep("DuplicateVirtualParticleCleanup");

  ac::assignParticleToVertex(
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

  ac::detachParticleFromVertex(
    particleName,
    particleX,
    particleId,
    false,
    vertexX,
    vertexH,
    treeId,
    "firstVirtualDetach"
  );
  ac::eraseParticle(particleName, particleX, particleId, false, vertexX, vertexH, treeId, "firstVirtualErase");

  ac::detachParticleFromVertex(
    particleName,
    particleX,
    particleId,
    false,
    vertexX,
    vertexH,
    treeId,
    "duplicateVirtualDetach"
  );
  ac::eraseParticle(particleName, particleX, particleId, false, vertexX, vertexH, treeId, "duplicateVirtualErase");

  ac::internal::ParticleIdentifier identifier =
    ac::internal::ParticleIdentifier(particleName, particleX, vertexX, vertexH, particleId, treeId);
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
