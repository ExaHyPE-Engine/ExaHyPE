// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKSamplerTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/PVDTimeSeriesWriter.h"
#include "tarch/plotter/VTUFileWriter.h"
#include "tarch/reader/VTKSampler.h"

#ifdef PEANO_WITH_HDF5
#include "tarch/plotter/VTKHDFFileWriter.h"
#endif

namespace {
  void barrier() {
#ifdef PEANO_WITH_MPI
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
#endif
  }

  int  thisRank() { return tarch::mpi::Rank::getInstance().getRank(); }
  bool isMaster() { return tarch::mpi::Rank::getInstance().isGlobalMaster(); }

  std::filesystem::path testDirectory() { return tarch::tests::getWritableTestDirectory(); }

  /**
   * Plots one quad into one piece of a plotter, shifted by `shift` in x so the
   * pieces and the ranks do not overlap. Corner k carries the scalar
   * vertexBase+k and the vector (vertexBase+k, -(vertexBase+k)), while the quad
   * itself carries cellValue.
   *
   * Both file writers derive from tarch::plotter::FileWriter, so the two
   * back-ends of the sampler are fed with the very same geometry and data.
   */
  void plotQuad(tarch::plotter::FileWriter& plotter, int piece, double shift, double vertexBase, double cellValue) {
    auto vertexPlotter     = plotter.createVertexPlotter(piece);
    auto cellPlotter       = plotter.createCellPlotter(piece);
    auto vertexDataPlotter = plotter.createVertexDataPlotter(piece, "u", 1);
    auto vectorDataPlotter = plotter.createVertexDataPlotter(piece, "v", 2);
    auto cellDataPlotter   = plotter.createCellDataPlotter(piece, "c", 1);

    int quad[4] = {
      vertexPlotter->plotVertex(tarch::la::Vector<2, double>{shift + 0.0, 0.0}),
      vertexPlotter->plotVertex(tarch::la::Vector<2, double>{shift + 1.0, 0.0}),
      vertexPlotter->plotVertex(tarch::la::Vector<2, double>{shift + 1.0, 1.0}),
      vertexPlotter->plotVertex(tarch::la::Vector<2, double>{shift + 0.0, 1.0})
    };

    for (int corner = 0; corner < 4; corner++) {
      const double value = vertexBase + corner;
      vertexDataPlotter->plotVertexData(quad[corner], value);
      vectorDataPlotter->plotVertexData(quad[corner], tarch::la::Vector<2, double>{value, -value});
    }

    const int cell = cellPlotter->plotQuadrangle(quad);
    cellDataPlotter->plotCellData(cell, cellValue);
  }

  // --------------------------------------------------------------------------
  // VTU fixtures
  // --------------------------------------------------------------------------

  /** Name the time series writer stamps a snapshot's time into. */
  std::string snapshotStem(const std::string& filename, double time) {
    std::ostringstream stream;
    stream << filename << "-" << std::fixed << std::setprecision(6) << time;
    std::string name = stream.str();
    std::replace(name.begin(), name.end(), '.', '_');
    return name;
  }

  /**
   * Writes one quad per rank per step into a .pvd series, i.e. exactly what a
   * run without HDF5 support leaves behind. The values follow the same scheme
   * as the VTKHDF fixture, so both back-ends are validated against the same
   * expectations.
   */
  void writeVTU(const std::string& filename, int numberOfSteps) {
    tarch::plotter::PVDTimeSeriesWriter timeSeries;

    for (int step = 0; step < numberOfSteps; step++) {
      // One writer per step, as in a real run where every PlotSolution
      // traversal creates its own writer.
      tarch::plotter::VTUFileWriter plotter(testDirectory().string());
      plotter.acquirePiece(0);

      plotQuad(plotter, 0, 2.0 * thisRank(), 10.0 * thisRank() + step, 100.0 * thisRank() + step);

      timeSeries.writeSnapshotFile(filename, 0.5 * step, plotter);
    }

    timeSeries.writeTimeSeriesMetaFile(testDirectory().string(), filename);
  }

  /** Removes the whole series. Only the global master may call this. */
  void removeVTU(const std::string& filename, int numberOfSteps) {
    const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

    for (int step = 0; step < numberOfSteps; step++) {
      const std::string stem = snapshotStem(filename, 0.5 * step);
      for (int rank = 0; rank < numberOfRanks; rank++) {
        std::filesystem::remove(testDirectory() / (stem + "-rank-" + std::to_string(rank) + ".vtu"));
      }
      std::filesystem::remove(testDirectory() / (stem + ".pvtu"));
    }

    std::filesystem::remove(testDirectory() / (filename + ".pvd"));
  }

  std::string collectionPath(const std::string& filename) {
    return (testDirectory() / (filename + ".pvd")).string();
  }

  std::string encodeBase64(const std::vector<std::uint8_t>& bytes) {
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    for (std::size_t byte = 0; byte < bytes.size(); byte += 3) {
      const std::uint32_t first  = bytes[byte];
      const std::uint32_t second = (byte + 1 < bytes.size()) ? bytes[byte + 1] : 0u;
      const std::uint32_t third  = (byte + 2 < bytes.size()) ? bytes[byte + 2] : 0u;
      const std::uint32_t triple = (first << 16) | (second << 8) | third;

      result += alphabet[(triple >> 18) & 0x3Fu];
      result += alphabet[(triple >> 12) & 0x3Fu];
      result += (byte + 1 < bytes.size()) ? alphabet[(triple >> 6) & 0x3Fu] : '=';
      result += (byte + 2 < bytes.size()) ? alphabet[triple & 0x3Fu] : '=';
    }
    return result;
  }

  /**
   * One inline binary array: the byte count in the file's header type followed
   * by the values, the whole of it base64 encoded.
   */
  template <class T>
  std::string inlineArray(const std::vector<T>& values) {
    const std::uint64_t     numberOfBytes = values.size() * sizeof(T);
    std::vector<std::uint8_t> bytes(sizeof(std::uint64_t) + numberOfBytes);

    std::memcpy(bytes.data(), &numberOfBytes, sizeof(std::uint64_t));
    if (not values.empty()) {
      std::memcpy(bytes.data() + sizeof(std::uint64_t), values.data(), static_cast<std::size_t>(numberOfBytes));
    }

    return encodeBase64(bytes);
  }

  // --------------------------------------------------------------------------
  // VTKHDF fixtures
  // --------------------------------------------------------------------------
#ifdef PEANO_WITH_HDF5
  /**
   * Writes one quad per rank into a growing .vtkhdf. Every step repeats the
   * geometry and adds the step index to all values, so a test can tell the
   * steps apart.
   */
  void writeVTKHDF(const std::string& filename, int numberOfSteps) {
    for (int step = 0; step < numberOfSteps; step++) {
      tarch::plotter::VTKHDFFileWriter plotter(".", 1);

      plotQuad(plotter, 0, 2.0 * thisRank(), 10.0 * thisRank() + step, 100.0 * thisRank() + step);

      plotter.writeToFile(filename, 0.5 * step);
    }
  }

  void removeVTKHDF(const std::string& filename) {
    const std::string path = filename + ".vtkhdf";
    if (std::filesystem::exists(path)) {
      std::filesystem::remove(path);
    }
  }
#endif
} // namespace

tarch::reader::tests::VTKSamplerTest::VTKSamplerTest():
  TestCase("tarch::reader::tests::VTKSamplerTest") {}

void tarch::reader::tests::VTKSamplerTest::run() {
#ifdef PEANO_WITH_HDF5
  testMethod(testSampleVertexData);
  testMethod(testSampleClosestVertex);
  testMethod(testSelectStep);
  testMethod(testSampleCellData);
  testMethod(testUnknownFieldThrows);
#endif

  testMethod(testSampleVertexDataFromVTU);
  testMethod(testSampleClosestVertexFromVTU);
  testMethod(testSelectStepFromVTU);
  testMethod(testSampleCellDataFromVTU);
  testMethod(testSampleSeveralPiecesFromVTU);
  testMethod(testReadSnapshotFileFromVTU);
  testMethod(testUnknownFieldThrowsInVTU);
  testMethod(testSampleAsciiVTU);
  testMethod(testSampleBase64VTU);
  testMethod(testUnknownFormatThrows);
}

#ifdef PEANO_WITH_HDF5

/**
 * A query that sits exactly on a plotted vertex has to return that vertex'
 * data, for scalars as well as for the components of a vector.
 */
void tarch::reader::tests::VTKSamplerTest::testSampleVertexData() {
  const std::string filename = "VTKSamplerTestVertexData";
  writeVTKHDF(filename, 1);
  barrier();

  if (isMaster()) {
    VTKSampler sampler(filename + ".vtkhdf", {"u", "v"});

    validateEquals(sampler.getNumberOfSteps(), 1);
    validateEquals(sampler.getStep(), 0);
    validate(not sampler.samplesCellData());
    validateNumericalEquals(sampler.getTimeStamp(), 0.0);

    // The rank-0 quad's corners, in the order they have been plotted.
    validateNumericalEquals(sampler.sample("u", 0, 0.0, 0.0), 0.0);
    validateNumericalEquals(sampler.sample("u", 0, 1.0, 0.0), 1.0);
    validateNumericalEquals(sampler.sample("u", 0, 1.0, 1.0), 2.0);
    validateNumericalEquals(sampler.sample("u", 0, 0.0, 1.0), 3.0);

    // A 2d vector is padded to three components by the plotter.
    validateEquals(sampler.getNumberOfComponents("v"), 3);
    const int corner = sampler.getClosestPoint(1.0, 1.0);
    validateNumericalEquals(sampler.getValue("v", 0, corner), 2.0);
    validateNumericalEquals(sampler.getValue("v", 1, corner), -2.0);
    validateNumericalEquals(sampler.getValue("v", 2, corner), 0.0);

    removeVTKHDF(filename);
  }

  barrier();
}

/**
 * Away from the sample points the sampler returns the closest one - it does
 * not interpolate.
 */
void tarch::reader::tests::VTKSamplerTest::testSampleClosestVertex() {
  const std::string filename = "VTKSamplerTestClosestVertex";
  writeVTKHDF(filename, 1);
  barrier();

  if (isMaster()) {
    VTKSampler sampler(filename + ".vtkhdf", {"u"});

    validateNumericalEquals(sampler.sample("u", 0, 0.9, 0.1), 1.0);
    validateNumericalEquals(sampler.sample("u", 0, 0.4, 0.6), 3.0);
    // Outside the point cloud the closest corner still wins.
    validateNumericalEquals(sampler.sample("u", 0, -5.0, -5.0), 0.0);

    removeVTKHDF(filename);
  }

  barrier();
}

/**
 * The default reads the last step of the growing file, and a negative index
 * counts from its end.
 */
void tarch::reader::tests::VTKSamplerTest::testSelectStep() {
  const std::string filename = "VTKSamplerTestSteps";
  writeVTKHDF(filename, 3);
  barrier();

  if (isMaster()) {
    VTKSampler lastStep(filename + ".vtkhdf", {"u"});
    validateEquals(lastStep.getNumberOfSteps(), 3);
    validateEquals(lastStep.getStep(), 2);
    validateNumericalEquals(lastStep.getTimeStamp(), 1.0);
    validateNumericalEquals(lastStep.sample("u", 0, 0.0, 0.0), 2.0);

    VTKSampler firstStep(filename + ".vtkhdf", {"u"}, 0);
    validateEquals(firstStep.getStep(), 0);
    validateNumericalEquals(firstStep.getTimeStamp(), 0.0);
    validateNumericalEquals(firstStep.sample("u", 0, 0.0, 0.0), 0.0);

    VTKSampler lastButOneStep(filename + ".vtkhdf", {"u"}, -2);
    validateEquals(lastButOneStep.getStep(), 1);
    validateNumericalEquals(lastButOneStep.sample("u", 0, 0.0, 0.0), 1.0);

    removeVTKHDF(filename);
  }

  barrier();
}

/**
 * A field that lives in the file's CellData turns the cells into the sample
 * points, represented through their centroids.
 */
void tarch::reader::tests::VTKSamplerTest::testSampleCellData() {
  const std::string filename = "VTKSamplerTestCellData";
  writeVTKHDF(filename, 2);
  barrier();

  if (isMaster()) {
    VTKSampler sampler(filename + ".vtkhdf", {"c"});

    validate(sampler.samplesCellData());
    validateEquals(sampler.getNumberOfPoints(), tarch::mpi::Rank::getInstance().getNumberOfRanks());
    // The rank-0 quad spans [0,1]^2, so its centroid is (0.5,0.5) - and any
    // query inside the quad is closest to it.
    validateNumericalEquals(sampler.sample("c", 0, 0.5, 0.5), 1.0);
    validateNumericalEquals(sampler.sample("c", 0, 0.1, 0.9), 1.0);

    removeVTKHDF(filename);
  }

  barrier();
}

/**
 * Asking for a field the file does not hold is a configuration error and has
 * to surface as such, rather than as zeros in the initial condition.
 */
void tarch::reader::tests::VTKSamplerTest::testUnknownFieldThrows() {
  const std::string filename = "VTKSamplerTestUnknownField";
  writeVTKHDF(filename, 1);
  barrier();

  if (isMaster()) {
    bool hasThrown = false;
    try {
      VTKSampler sampler(filename + ".vtkhdf", {"u", "notAField"});
    } catch (const std::runtime_error&) {
      hasThrown = true;
    }
    validate(hasThrown);

    // So is mixing the two associations within one sampler.
    hasThrown = false;
    try {
      VTKSampler sampler(filename + ".vtkhdf", {"u", "c"});
    } catch (const std::runtime_error&) {
      hasThrown = true;
    }
    validate(hasThrown);

    removeVTKHDF(filename);
  }

  barrier();
}

#endif

/**
 * The VTU counterpart of testSampleVertexData(): the sampler follows the .pvd
 * to the step's .pvtu and from there to the ranks' .vtu files, and returns the
 * data of the vertex a query sits on.
 */
void tarch::reader::tests::VTKSamplerTest::testSampleVertexDataFromVTU() {
  const std::string filename = "VTUSamplerTestVertexData";
  writeVTU(filename, 1);
  barrier();

  if (isMaster()) {
    VTKSampler sampler(collectionPath(filename), {"u", "v"});

    validateEquals(sampler.getNumberOfSteps(), 1);
    validateEquals(sampler.getStep(), 0);
    validate(not sampler.samplesCellData());
    validateNumericalEquals(sampler.getTimeStamp(), 0.0);
    validateEquals(sampler.getNumberOfPoints(), 4 * tarch::mpi::Rank::getInstance().getNumberOfRanks());

    validateNumericalEquals(sampler.sample("u", 0, 0.0, 0.0), 0.0);
    validateNumericalEquals(sampler.sample("u", 0, 1.0, 0.0), 1.0);
    validateNumericalEquals(sampler.sample("u", 0, 1.0, 1.0), 2.0);
    validateNumericalEquals(sampler.sample("u", 0, 0.0, 1.0), 3.0);

    validateEquals(sampler.getNumberOfComponents("v"), 3);
    const int corner = sampler.getClosestPoint(1.0, 1.0);
    validateNumericalEquals(sampler.getValue("v", 0, corner), 2.0);
    validateNumericalEquals(sampler.getValue("v", 1, corner), -2.0);
    validateNumericalEquals(sampler.getValue("v", 2, corner), 0.0);

    removeVTU(filename, 1);
  }

  barrier();
}

void tarch::reader::tests::VTKSamplerTest::testSampleClosestVertexFromVTU() {
  const std::string filename = "VTUSamplerTestClosestVertex";
  writeVTU(filename, 1);
  barrier();

  if (isMaster()) {
    VTKSampler sampler(collectionPath(filename), {"u"});

    validateNumericalEquals(sampler.sample("u", 0, 0.9, 0.1), 1.0);
    validateNumericalEquals(sampler.sample("u", 0, 0.4, 0.6), 3.0);
    validateNumericalEquals(sampler.sample("u", 0, -5.0, -5.0), 0.0);

    removeVTU(filename, 1);
  }

  barrier();
}

/**
 * The steps of a VTU run are the entries of its collection, and the sampler
 * takes their time stamps from there.
 */
void tarch::reader::tests::VTKSamplerTest::testSelectStepFromVTU() {
  const std::string filename = "VTUSamplerTestSteps";
  writeVTU(filename, 3);
  barrier();

  if (isMaster()) {
    VTKSampler lastStep(collectionPath(filename), {"u"});
    validateEquals(lastStep.getNumberOfSteps(), 3);
    validateEquals(lastStep.getStep(), 2);
    validateNumericalEquals(lastStep.getTimeStamp(), 1.0);
    validateNumericalEquals(lastStep.sample("u", 0, 0.0, 0.0), 2.0);

    VTKSampler firstStep(collectionPath(filename), {"u"}, 0);
    validateEquals(firstStep.getStep(), 0);
    validateNumericalEquals(firstStep.getTimeStamp(), 0.0);
    validateNumericalEquals(firstStep.sample("u", 0, 0.0, 0.0), 0.0);

    VTKSampler lastButOneStep(collectionPath(filename), {"u"}, -2);
    validateEquals(lastButOneStep.getStep(), 1);
    validateNumericalEquals(lastButOneStep.getTimeStamp(), 0.5);
    validateNumericalEquals(lastButOneStep.sample("u", 0, 0.0, 0.0), 1.0);

    removeVTU(filename, 3);
  }

  barrier();
}

/**
 * A .vtu stores one offset per cell and no leading zero, so the centroids are
 * the place where a mix-up with the VTKHDF layout would show up.
 */
void tarch::reader::tests::VTKSamplerTest::testSampleCellDataFromVTU() {
  const std::string filename = "VTUSamplerTestCellData";
  writeVTU(filename, 2);
  barrier();

  if (isMaster()) {
    VTKSampler sampler(collectionPath(filename), {"c"});

    validate(sampler.samplesCellData());
    validateEquals(sampler.getNumberOfPoints(), tarch::mpi::Rank::getInstance().getNumberOfRanks());
    validateNumericalEquals(sampler.sample("c", 0, 0.5, 0.5), 1.0);
    validateNumericalEquals(sampler.sample("c", 0, 0.1, 0.9), 1.0);

    removeVTU(filename, 2);
  }

  barrier();
}

/**
 * ExaHyPE writes one piece per subdomain, so a single .vtu already holds dozens
 * of them. Their connectivity is piece-local, i.e. it indexes the piece's own
 * points, while the sampler merges all of them into one point cloud.
 */
void tarch::reader::tests::VTKSamplerTest::testSampleSeveralPiecesFromVTU() {
  const std::string filename = "VTUSamplerTestPieces";

  {
    tarch::plotter::VTUFileWriter plotter(testDirectory().string());
    plotter.acquirePiece(0);
    plotter.acquirePiece(1);

    const double shift = 4.0 * thisRank();
    plotQuad(plotter, 0, shift + 0.0, 100.0 * thisRank() + 0.0, 1.0 + 10.0 * thisRank());
    plotQuad(plotter, 1, shift + 2.0, 100.0 * thisRank() + 20.0, 2.0 + 10.0 * thisRank());

    plotter.writeToFile(filename, 0.0);
  }
  barrier();

  if (isMaster()) {
    const std::string pvtuPath = (testDirectory() / (filename + ".pvtu")).string();

    VTKSampler cells(pvtuPath, {"c"});
    validate(cells.samplesCellData());
    validateEquals(cells.getNumberOfPoints(), 2 * tarch::mpi::Rank::getInstance().getNumberOfRanks());
    // The second piece' quad spans [2,3]x[0,1], and its corners are numbered
    // from zero again - a centroid at (2.5,0.5) proves that the sampler honours
    // that.
    validateNumericalEquals(cells.sample("c", 0, 0.5, 0.5), 1.0);
    validateNumericalEquals(cells.sample("c", 0, 2.5, 0.5), 2.0);

    VTKSampler vertices(pvtuPath, {"u"});
    validateEquals(vertices.getNumberOfPoints(), 8 * tarch::mpi::Rank::getInstance().getNumberOfRanks());
    validateNumericalEquals(vertices.sample("u", 0, 3.0, 1.0), 22.0);

    const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    for (int rank = 0; rank < numberOfRanks; rank++) {
      std::filesystem::remove(testDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu"));
    }
    std::filesystem::remove(testDirectory() / (filename + ".pvtu"));
  }

  barrier();
}

/**
 * A single snapshot can be sampled without its collection. Its time then only
 * lives in the file name, which is where the sampler picks it up.
 */
void tarch::reader::tests::VTKSamplerTest::testReadSnapshotFileFromVTU() {
  const std::string filename = "VTUSamplerTestSnapshot";
  writeVTU(filename, 2);
  barrier();

  if (isMaster()) {
    const std::string snapshot = (testDirectory() / (snapshotStem(filename, 0.5) + ".pvtu")).string();

    VTKSampler sampler(snapshot, {"u"});
    validateEquals(sampler.getNumberOfSteps(), 1);
    validateEquals(sampler.getStep(), 0);
    validateNumericalEquals(sampler.getTimeStamp(), 0.5);
    validateNumericalEquals(sampler.sample("u", 0, 0.0, 0.0), 1.0);

    // The rank's own piece can be read directly, too.
    const std::string piece = (testDirectory() / (snapshotStem(filename, 0.5) + "-rank-0.vtu")).string();

    VTKSampler pieceSampler(piece, {"u"});
    validateNumericalEquals(pieceSampler.getTimeStamp(), 0.5);
    validateEquals(pieceSampler.getNumberOfPoints(), 4);
    validateNumericalEquals(pieceSampler.sample("u", 0, 1.0, 1.0), 3.0);

    removeVTU(filename, 2);
  }

  barrier();
}

void tarch::reader::tests::VTKSamplerTest::testUnknownFieldThrowsInVTU() {
  const std::string filename = "VTUSamplerTestUnknownField";
  writeVTU(filename, 1);
  barrier();

  if (isMaster()) {
    bool hasThrown = false;
    try {
      VTKSampler sampler(collectionPath(filename), {"u", "notAField"});
    } catch (const std::runtime_error&) {
      hasThrown = true;
    }
    validate(hasThrown);

    hasThrown = false;
    try {
      VTKSampler sampler(collectionPath(filename), {"u", "c"});
    } catch (const std::runtime_error&) {
      hasThrown = true;
    }
    validate(hasThrown);

    removeVTU(filename, 1);
  }

  barrier();
}

/**
 * Peano appends its arrays as raw binary, but the format also allows for inline
 * ascii, which a hand-written or converted file may well use.
 */
void tarch::reader::tests::VTKSamplerTest::testSampleAsciiVTU() {
  if (isMaster()) {
    const std::filesystem::path path = testDirectory() / "VTUSamplerTestAscii.vtu";

    {
      std::ofstream out(path);
      out << "<?xml version=\"1.0\"?>\n"
          << "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\">\n"
          << "  <UnstructuredGrid>\n"
          << "    <Piece NumberOfPoints=\"4\" NumberOfCells=\"1\">\n"
          << "      <PointData>\n"
          << "        <DataArray type=\"Float64\" Name=\"u\" format=\"ascii\">0 1 2 3</DataArray>\n"
          << "      </PointData>\n"
          << "      <CellData>\n"
          << "        <DataArray type=\"Float64\" Name=\"c\" format=\"ascii\">7</DataArray>\n"
          << "      </CellData>\n"
          << "      <Points>\n"
          << "        <DataArray type=\"Float64\" Name=\"Points\" NumberOfComponents=\"3\" format=\"ascii\">\n"
          << "          0 0 0 1 0 0 1 1 0 0 1 0\n"
          << "        </DataArray>\n"
          << "      </Points>\n"
          << "      <Cells>\n"
          << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">0 1 2 3</DataArray>\n"
          << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">4</DataArray>\n"
          << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">9</DataArray>\n"
          << "      </Cells>\n"
          << "    </Piece>\n"
          << "  </UnstructuredGrid>\n"
          << "</VTKFile>\n";
    }

    VTKSampler vertices(path.string(), {"u"});
    validateEquals(vertices.getNumberOfPoints(), 4);
    validateNumericalEquals(vertices.getTimeStamp(), 0.0);
    validateNumericalEquals(vertices.sample("u", 0, 1.0, 0.0), 1.0);
    validateNumericalEquals(vertices.sample("u", 0, 0.0, 1.0), 3.0);

    VTKSampler cells(path.string(), {"c"});
    validate(cells.samplesCellData());
    validateEquals(cells.getNumberOfPoints(), 1);
    validateNumericalEquals(cells.sample("c", 0, 0.5, 0.5), 7.0);

    std::filesystem::remove(path);
  }

  barrier();
}

/**
 * The other way of inlining an array is base64, which is what a file exported
 * from ParaView typically uses.
 */
void tarch::reader::tests::VTKSamplerTest::testSampleBase64VTU() {
  if (isMaster()) {
    const std::filesystem::path path = testDirectory() / "VTUSamplerTestBase64.vtu";

    const std::vector<double> points = {
      0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0
    };
    const std::vector<double>       u            = {0.0, 1.0, 2.0, 3.0};
    const std::vector<std::int32_t> connectivity = {0, 1, 2, 3};
    const std::vector<std::int32_t> offsets      = {4};
    const std::vector<std::uint8_t> types        = {9};

    {
      std::ofstream out(path);
      out << "<?xml version=\"1.0\"?>\n"
          << "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\" "
             "header_type=\"UInt64\">\n"
          << "  <UnstructuredGrid>\n"
          << "    <Piece NumberOfPoints=\"4\" NumberOfCells=\"1\">\n"
          << "      <PointData>\n"
          << "        <DataArray type=\"Float64\" Name=\"u\" format=\"binary\">" << inlineArray(u)
          << "</DataArray>\n"
          << "      </PointData>\n"
          << "      <Points>\n"
          << "        <DataArray type=\"Float64\" Name=\"Points\" NumberOfComponents=\"3\" format=\"binary\">"
          << inlineArray(points) << "</DataArray>\n"
          << "      </Points>\n"
          << "      <Cells>\n"
          << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"binary\">" << inlineArray(connectivity)
          << "</DataArray>\n"
          << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"binary\">" << inlineArray(offsets)
          << "</DataArray>\n"
          << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"binary\">" << inlineArray(types)
          << "</DataArray>\n"
          << "      </Cells>\n"
          << "    </Piece>\n"
          << "  </UnstructuredGrid>\n"
          << "</VTKFile>\n";
    }

    VTKSampler sampler(path.string(), {"u"});
    validateEquals(sampler.getNumberOfPoints(), 4);
    validateNumericalEquals(sampler.sample("u", 0, 0.0, 0.0), 0.0);
    validateNumericalEquals(sampler.sample("u", 0, 1.0, 0.0), 1.0);
    validateNumericalEquals(sampler.sample("u", 0, 1.0, 1.0), 2.0);
    validateNumericalEquals(sampler.sample("u", 0, 0.0, 1.0), 3.0);

    std::filesystem::remove(path);
  }

  barrier();
}

/**
 * A path whose extension belongs to neither format is a configuration error,
 * not something to guess about.
 */
void tarch::reader::tests::VTKSamplerTest::testUnknownFormatThrows() {
  bool hasThrown = false;
  try {
    VTKSampler sampler("no-such-file.txt", {"u"});
  } catch (const std::runtime_error&) {
    hasThrown = true;
  }
  validate(hasThrown);
}
