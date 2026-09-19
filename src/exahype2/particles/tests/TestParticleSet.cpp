// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestParticleSet.h"

#include "peano4/particles/assignmentchecks/TracingAPI.h"
#include "tarch/Assertions.h"

std::vector<exahype2::particles::tests::TestParticle*>
  exahype2::particles::tests::TestParticleSet::_particlesToBeSieved{};
std::vector<exahype2::particles::tests::TestParticle*>
                             exahype2::particles::tests::TestParticleSet::_particlesLiftedIntoSieveSet{};
std::array<int, 5>           exahype2::particles::tests::TestParticleSet::_recordedLiftDropStatistics{};
std::array<int, 3>           exahype2::particles::tests::TestParticleSet::_recordedParticleStateStatistics{};
std::set<std::array<int, 2>> exahype2::particles::tests::TestParticleSet::_recordedLocalParticleNumbers{};
int                          exahype2::particles::tests::TestParticleSet::_numberOfDeletedParticles{0};

tarch::la::Vector<DIMENSIONS, double> exahype2::particles::tests::TestParticle::getX() const { return _x; }

void exahype2::particles::tests::TestParticle::setX(const tarch::la::Vector<DIMENSIONS, double>& value) { _x = value; }

tarch::la::Vector<DIMENSIONS, double> exahype2::particles::tests::TestParticle::getCellH() const { return _cellH; }

void exahype2::particles::tests::TestParticle::setCellH(const tarch::la::Vector<DIMENSIONS, double>& value) {
  _cellH = value;
}

double exahype2::particles::tests::TestParticle::getSearchRadius() const { return _searchRadius; }

void exahype2::particles::tests::TestParticle::setSearchRadius(const double value) { _searchRadius = value; }

int exahype2::particles::tests::TestParticle::getPartid() const { return _partid; }

void exahype2::particles::tests::TestParticle::setPartid(const int value) { _partid = value; }

exahype2::particles::tests::TestParticle::ParallelState exahype2::particles::tests::TestParticle::
  getParallelState() const {
  return _parallelState;
}

void exahype2::particles::tests::TestParticle::setParallelState(const ParallelState value) { _parallelState = value; }

std::bitset<peano4::CellsPerVertex> exahype2::particles::tests::TestParticle::getContainedInAdjacentCell() const {
  return _containedInAdjacentCell;
}

void exahype2::particles::tests::TestParticle::setContainedInAdjacentCell(
  const std::bitset<peano4::CellsPerVertex>& value
) {
  _containedInAdjacentCell = value;
}

int exahype2::particles::tests::TestParticle::getNumber(const int index) const { return _number[index]; }

void exahype2::particles::tests::TestParticle::setNumber(const int index, const int value) { _number[index] = value; }

const std::array<double, 1>& exahype2::particles::tests::TestParticle::getData() const { return _data; }

void exahype2::particles::tests::TestParticle::setData(const int index, const double value) { _data[index] = value; }

#if PEANO_DEBUG
// The particle factory records the position and the mesh width a particle starts with; no test
// reads them back, so both setters drop their value.
void exahype2::particles::tests::TestParticle::setDebugX(
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& value
) {}

void exahype2::particles::tests::TestParticle::setDebugH([[maybe_unused]] const double value) {}
#endif

std::string exahype2::particles::tests::TestParticle::toString() const {
  std::ostringstream out;
  out << "(x=" << _x << ",number=" << _number[0] << "," << _number[1] << ")";
  return out.str();
}

bool exahype2::particles::tests::TestParticleSet::hasParticlesToBeSievedIntoVertices() {
  return not _particlesToBeSieved.empty();
}

/** Hands out every particle a test added to the sieve set and empties it. */
std::list<exahype2::particles::tests::TestParticle*> exahype2::particles::tests::TestParticleSet::
  getParticlesToBeSievedIntoVertex([[maybe_unused]] const peano4::dataview::VertexMarker& marker) {
  std::list<TestParticle*> result(_particlesToBeSieved.begin(), _particlesToBeSieved.end());
  _particlesToBeSieved.clear();
  return result;
}

void exahype2::particles::tests::TestParticleSet::updateLiftDropStatistics(
  const int numberOfLifts,
  const int numberOfDrops,
  const int numberOfLiftsIntoSieveSet,
  const int numberOfDropsFromSieveSet,
  const int numberOfDropsIntoHorizontalTreeDecomposition
) {
  _recordedLiftDropStatistics = {
    numberOfLifts,
    numberOfDrops,
    numberOfLiftsIntoSieveSet,
    numberOfDropsFromSieveSet,
    numberOfDropsIntoHorizontalTreeDecomposition
  };
}

void exahype2::particles::tests::TestParticleSet::updateNumberOfLocalAndExpiredParticles(
  const int                           numberOfRemainingLocalParticles,
  const int                           numberOfExpiredHaloParticles,
  const int                           numberOfParticlesThatHaveLeftTheirDomain,
  const std::set<std::array<int, 2>>& remainingLocalParticleNumbers
) {
  _recordedParticleStateStatistics = {
    numberOfRemainingLocalParticles,
    numberOfExpiredHaloParticles,
    numberOfParticlesThatHaveLeftTheirDomain
  };
  _recordedLocalParticleNumbers = remainingLocalParticleNumbers;
}

/**
 * Deletes the particles the sieve set holds and empties the recorded statistics, so one test
 * neither reads nor frees the data of another.
 */
void exahype2::particles::tests::TestParticleSet::clearTestState() {
  for (auto* const particle : _particlesToBeSieved) {
    delete particle;
  }
  _particlesToBeSieved.clear();
  for (auto* const particle : _particlesLiftedIntoSieveSet) {
    delete particle;
  }
  _particlesLiftedIntoSieveSet.clear();
  _recordedLiftDropStatistics.fill(0);
  _recordedParticleStateStatistics.fill(0);
  _recordedLocalParticleNumbers.clear();
  _numberOfDeletedParticles = 0;
}

void exahype2::particles::tests::TestParticleSet::addParticleToBeSieved(TestParticle* const particle) {
  assertion(particle != nullptr);
  _particlesToBeSieved.push_back(particle);
}

const std::vector<exahype2::particles::tests::TestParticle*>& exahype2::particles::tests::TestParticleSet::
  getParticlesLiftedIntoSieveSet() {
  return _particlesLiftedIntoSieveSet;
}

const std::array<int, 5>& exahype2::particles::tests::TestParticleSet::getRecordedLiftDropStatistics() {
  return _recordedLiftDropStatistics;
}

const std::array<int, 3>& exahype2::particles::tests::TestParticleSet::getRecordedParticleStateStatistics() {
  return _recordedParticleStateStatistics;
}

const std::set<std::array<int, 2>>& exahype2::particles::tests::TestParticleSet::getRecordedLocalParticleNumbers() {
  return _recordedLocalParticleNumbers;
}

int exahype2::particles::tests::TestParticleSet::getNumberOfDeletedParticles() { return _numberOfDeletedParticles; }

void exahype2::particles::tests::TestParticleSet::mergeWithParticle(
  TestParticle* const                                    particle,
  [[maybe_unused]] const peano4::dataview::VertexMarker& marker,
  [[maybe_unused]] const int                             treeId
) {
  assertion(particle != nullptr);
  _container.push_back(particle);
}

void exahype2::particles::tests::TestParticleSet::particlesCanNotBeLiftedLocally() {
  for (auto* const particle : _container) {
    _particlesLiftedIntoSieveSet.push_back(particle);
  }
  _container.clear();
}

exahype2::particles::tests::TestParticleSet::iterator exahype2::particles::tests::TestParticleSet::
  particleCanNotBeLiftedLocally(const iterator particle) {
  _particlesLiftedIntoSieveSet.push_back(*particle);
  return _container.erase(particle);
}

void exahype2::particles::tests::TestParticleSet::addParticle(TestParticle* const particle) {
  assertion(particle != nullptr);
  _container.push_back(particle);
}

exahype2::particles::tests::TestParticleSet::iterator exahype2::particles::tests::TestParticleSet::deleteParticle(
  const iterator particle
) {
  assertion(*particle != nullptr);
  delete *particle;
  _numberOfDeletedParticles++;
  return _container.erase(particle);
}

void exahype2::particles::tests::TestParticleSet::grabParticles(TestParticleSet& sourceParticleSet) {
  assertion(this != &sourceParticleSet);
  _container.insert(_container.end(), sourceParticleSet.begin(), sourceParticleSet.end());
  sourceParticleSet._container.clear();
}

exahype2::particles::tests::TestParticleSet::iterator exahype2::particles::tests::TestParticleSet::grabParticle(
  const iterator   particle,
  TestParticleSet& sourceParticleSet
) {
  assertion(this != &sourceParticleSet);
  _container.push_back(*particle);
  return sourceParticleSet._container.erase(particle);
}

void exahype2::particles::tests::TestParticleSet::deleteParticles() {
  for (auto* const particle : _container) {
    delete particle;
  }
  _container.clear();
}

exahype2::particles::tests::TestParticleSet::iterator exahype2::particles::tests::TestParticleSet::begin() {
  return _container.begin();
}

exahype2::particles::tests::TestParticleSet::iterator exahype2::particles::tests::TestParticleSet::end() {
  return _container.end();
}

exahype2::particles::tests::TestParticleSet::const_iterator exahype2::particles::tests::TestParticleSet::begin() const {
  return _container.begin();
}

exahype2::particles::tests::TestParticleSet::const_iterator exahype2::particles::tests::TestParticleSet::end() const {
  return _container.end();
}

int exahype2::particles::tests::TestParticleSet::size() const { return static_cast<int>(_container.size()); }

bool exahype2::particles::tests::TestParticleSet::isValid() const {
  return std::ranges::all_of(_container, [](const auto* const particle) { return particle != nullptr; });
}

std::string exahype2::particles::tests::TestParticleSet::toString() const {
  std::ostringstream out;
  out << "(size=" << _container.size() << ")";
  return out.str();
}

exahype2::particles::tests::TestVertices::TestVertices(TestParticleSet* const vertices) :
  _vertices(vertices) {
  assertion(vertices != nullptr);
}

exahype2::particles::tests::TestParticleSet& exahype2::particles::tests::TestVertices::operator()(
  const int vertex
) const {
  assertion(_vertices != nullptr);
  assertion(vertex >= 0, vertex);
  assertion(vertex < peano4::VerticesPerCell, vertex);
  return _vertices[vertex];
}

/**
 * Opens the assignment history of the particle at the vertex at vertexX, so a template that
 * detaches the particle from that vertex finds the assignment an application would have
 * recorded. The routine does nothing outside a build with assertions.
 */
void exahype2::particles::tests::recordInitialParticleAssignment(
  const TestParticle&                          particle,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  const int                                    treeId
) {
  peano4::particles::assignmentchecks::assignParticleToVertex(
    "TestParticle",
    particle.getX(),
    particle.getPartid(),
    particle.getParallelState() == TestParticle::ParallelState::Local,
    vertexX,
    vertexH,
    treeId,
    "exahype2::particles::tests::recordInitialParticleAssignment",
    vertexX,
    vertexH,
    true
  );
}
