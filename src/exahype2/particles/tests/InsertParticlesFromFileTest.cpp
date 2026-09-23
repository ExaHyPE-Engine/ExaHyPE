// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InsertParticlesFromFileTest.h"

#include "exahype2/particles/InsertParticlesFromFile.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/VertexEnumerator.h"
#include "peano4/particles/FileReader.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

#if defined(__NVCOMPILER)
// The compiler nvc++ diagnoses members that only the debug and assertion builds call as unused.
#pragma diag_push
#pragma diag_suppress declared_but_not_referenced
#endif

namespace {
  constexpr int    DataEntriesPerParticle = 4;
  constexpr int    TreeId                 = 5;
  constexpr double CellWidth              = 0.2;
  constexpr double CellCentre             = 0.5;

  /** Stands in for a generated tracer particle. */
  struct TracerParticle final {
    enum class ParallelState { Local, Virtual };

    tarch::la::Vector<DIMENSIONS, double> x{0.0};
    tarch::la::Vector<DIMENSIONS, double> cellH{0.0};
    double                                searchRadius{0.0};
    int                                   number[2]{0, 0};
    int                                   partId{0};
    ParallelState                         parallelState{ParallelState::Virtual};
    std::bitset<peano4::CellsPerVertex>   containedInAdjacentCell{0};
    std::vector<double>                   data = std::vector<double>(DataEntriesPerParticle, 0.0);

    [[nodiscard]] const tarch::la::Vector<DIMENSIONS, double>& getX() const { return x; }
    void                        setX(const tarch::la::Vector<DIMENSIONS, double>& value) { x = value; }
    void                        setDebugX(const tarch::la::Vector<DIMENSIONS, double>& value) { x = value; }
    void                        setDebugH(const tarch::la::Vector<DIMENSIONS, double>& value) { cellH = value; }
    void                        setCellH(const tarch::la::Vector<DIMENSIONS, double>& value) { cellH = value; }
    [[nodiscard]] double        getSearchRadius() const { return searchRadius; }
    void                        setSearchRadius(const double value) { searchRadius = value; }
    [[nodiscard]] int           getNumber(const int index) const { return number[index]; }
    void                        setNumber(const int index, const int value) { number[index] = value; }
    [[nodiscard]] int           getPartid() const { return partId; }
    void                        setPartid(const int value) { partId = value; }
    [[nodiscard]] ParallelState getParallelState() const { return parallelState; }
    void                        setParallelState(const ParallelState value) { parallelState = value; }
    void                        setContainedInAdjacentCell(const std::bitset<peano4::CellsPerVertex>& value) {
      containedInAdjacentCell = value;
    }
    [[nodiscard]] const std::vector<double>& getData() const { return data; }
    void                                     setData(const int index, const double value) { data[index] = value; }
    [[nodiscard]] std::string                toString() const { return "particle"; }
  };

  /** Stands in for the generated particle container of one vertex. */
  struct ParticleSet final {
    std::vector<TracerParticle*> particles{};

    void                      addParticle(TracerParticle* const particle) { particles.push_back(particle); }
    [[nodiscard]] std::string toString() const { return "particleSet"; }
  };

  using Vertices = std::array<ParticleSet, peano4::VerticesPerCell>;

  peano4::dataview::VertexEnumerator<ParticleSet> enumerateVertices(Vertices& vertices) {
    std::array<int, peano4::VerticesPerCell> slots{};
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      slots[vertexNumber] = vertexNumber;
    }
    return peano4::dataview::VertexEnumerator<ParticleSet>(vertices.data(), slots.data(), 0);
  }

  peano4::dataview::CellMarker createCellMarker() {
    peano4::dataview::CellMarker marker{};
    marker.h      = tarch::la::Vector<DIMENSIONS, double>(CellWidth);
    marker.centre = tarch::la::Vector<DIMENSIONS, double>(CellCentre);
    return marker;
  }

  /** Returns every particle the vertices hold, ordered by the tree-local particle number. */
  std::vector<TracerParticle*> collectParticles(const Vertices& vertices) {
    std::vector<TracerParticle*> result;
    for (const ParticleSet& vertex : vertices) {
      for (TracerParticle* const particle : vertex.particles) {
        result.push_back(particle);
      }
    }
    std::sort(result.begin(), result.end(), [](const TracerParticle* const left, const TracerParticle* const right) {
      return left->getNumber(1) < right->getNumber(1);
    });
    return result;
  }

  void deleteParticles(const Vertices& vertices) {
    for (const ParticleSet& vertex : vertices) {
      for (TracerParticle* const particle : vertex.particles) {
        delete particle;
      }
    }
  }

  /**
   * Writes one position per line into a file the reader parses. Every rank of a parallel test
   * run writes its own file, as the ranks work in one directory.
   */
  std::string writeCoordinateFile(const std::string& name, const std::vector<double>& positions) {
    const std::string rankSuffix = "-rank-" + std::to_string(tarch::mpi::Rank::getInstance().getRank());
    const std::string fileName   = (tarch::tests::getWritableTestDirectory() / (rankSuffix + name)).string();
    std::ofstream     file(fileName, std::ios::trunc);
    for (const double position : positions) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        file << position << (axis + 1 < DIMENSIONS ? " " : "");
      }
      file << std::endl;
    }
    return fileName;
  }
} // namespace

#if defined(__NVCOMPILER)
#pragma diag_pop
#endif

exahype2::particles::tests::InsertParticlesFromFileTest::InsertParticlesFromFileTest() :
  tarch::tests::TestCase("exahype2::particles::tests::InsertParticlesFromFileTest") {}

void exahype2::particles::tests::InsertParticlesFromFileTest::run() {
  testMethod(testFileReaderPositionsInsideTheCellBecomeParticles);
  testMethod(testStoreInitialPositionWritesThePositionIntoTheParticleData);
}

/**
 * The two positions of the file inside the cell become particles with consecutive numbers, while
 * the position outside stays with the reader.
 */
void exahype2::particles::tests::InsertParticlesFromFileTest::testFileReaderPositionsInsideTheCellBecomeParticles() {
  const std::string fileName = writeCoordinateFile("InsertParticlesFromFileTest_positions.dat", {0.45, 0.55, 0.9});

  peano4::particles::FileReader fileReader;
  fileReader.readDatFile(fileName, 1.0);

  Vertices vertices;
  auto     vertexEnumerator         = enumerateVertices(vertices);
  int      particleNumberOnThisTree = 0;

  exahype2::particles::insertParticlesFromFile<TracerParticle, false>(
    createCellMarker(),
    fileReader,
    vertexEnumerator,
    TreeId,
    particleNumberOnThisTree,
    nullptr
  );

  const std::vector<TracerParticle*> particles = collectParticles(vertices);
  validateEquals(static_cast<int>(particles.size()), 2);
  validateEquals(particleNumberOnThisTree, 2);
  validateFalse(fileReader.empty());
  for (int index = 0; index < static_cast<int>(particles.size()); index++) {
    const double expectedPosition = index == 0 ? 0.45 : 0.55;
    validateEqualsWithParams1(particles[index]->getNumber(0), TreeId, index);
    validateEqualsWithParams1(particles[index]->getNumber(1), index, index);
    validateTrue(particles[index]->getParallelState() == TracerParticle::ParallelState::Local);
    validateNumericalEqualsWithParams1(particles[index]->getX()(0), expectedPosition, index);
    validateNumericalEqualsWithParams1(particles[index]->getData()[0], 0.0, index);
  }

  deleteParticles(vertices);
  std::filesystem::remove(fileName);
}

/** The position of a particle reaches the data entries the index array names, one per axis. */
void exahype2::particles::tests::InsertParticlesFromFileTest::
  testStoreInitialPositionWritesThePositionIntoTheParticleData() {
  const std::string fileName = writeCoordinateFile("InsertParticlesFromFileTest_initialposition.dat", {0.45});

  peano4::particles::FileReader fileReader;
  fileReader.readDatFile(fileName, 1.0);

  constexpr int InitialPositionIndex[] = {1, 2, 3};

  Vertices vertices;
  auto     vertexEnumerator         = enumerateVertices(vertices);
  int      particleNumberOnThisTree = 0;

  exahype2::particles::insertParticlesFromFile<TracerParticle, true>(
    createCellMarker(),
    fileReader,
    vertexEnumerator,
    TreeId,
    particleNumberOnThisTree,
    InitialPositionIndex
  );

  const std::vector<TracerParticle*> particles = collectParticles(vertices);
  validateEquals(static_cast<int>(particles.size()), 1);
  validateNumericalEquals(particles[0]->getData()[0], 0.0);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateNumericalEqualsWithParams1(particles[0]->getData()[InitialPositionIndex[axis]], 0.45, axis);
  }

  deleteParticles(vertices);
  std::filesystem::remove(fileName);
}
