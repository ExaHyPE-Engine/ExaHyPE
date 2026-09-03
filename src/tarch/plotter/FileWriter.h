// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace tarch::plotter {
  class FileWriter;
} // namespace tarch::plotter

class tarch::plotter::FileWriter {
public:
  explicit FileWriter(const std::string& outputDirectory);
  FileWriter(const FileWriter&)            = delete;
  FileWriter(FileWriter&&)                 = delete;
  FileWriter& operator=(const FileWriter&) = delete;
  FileWriter& operator=(FileWriter&&)      = delete;
  virtual ~FileWriter()                    = default;

  /**
   * Claims one piece of the output file and returns its id for the create
   * calls. Any number of concurrent plot writers can claim pieces; the file
   * orders them by the caller's key, so the output is deterministic however
   * the writers are scheduled.
   */
  int acquirePiece(int orderKey);

  virtual bool isOpen() const;
  virtual bool writeToFile(const std::string& filename, const double time) = 0;

  virtual std::string getFileExtension() const = 0;

  static void setDoublePrecisionOutput(const bool value);
  static bool getDoublePrecisionOutput();

protected:
  /** A plot point's coordinates quantised to the deduplication tolerance. */
  using VertexKey = std::tuple<std::int64_t, std::int64_t, std::int64_t>;

  struct DataField {
    std::string         name{};
    int                 components = 1;
    std::vector<double> values{};
  };

  struct VertexHash {
    std::size_t operator()(const VertexKey& key) const;
  };

  struct SubdomainBuffers {
    std::vector<double>       pointsXYZ{};
    std::vector<std::int32_t> connectivity{};
    std::vector<std::int32_t> offsets{};
    std::vector<std::uint8_t> types{};

    std::vector<DataField> pointFields{};
    std::vector<DataField> cellFields{};

    std::unordered_map<VertexKey, int, VertexHash> vertexMap;
  };

  struct SubdomainBuffersSequence {
    const SubdomainBuffers*              reference = nullptr;
    std::vector<const SubdomainBuffers*> payloads{};
  };

  /**
   * Field names and component counts for a collective parallel write; the
   * `values` of the DataFields stay empty. Every rank creates and writes the
   * identical set of datasets, a rank without local data included.
   */
  struct FieldSchema {
    std::vector<DataField> pointFields{};
    std::vector<DataField> cellFields{};
  };

  // Role of a data field, derived from its component count: A count of 1 is a
  // scalar, 2 or 3 a vector, 9 a tensor, every other count generic.
  enum class FieldKind { Scalar, Vector, Tensor, Generic };

  struct FieldLayout {
    FieldKind kind;
    // Number of components written. A vector takes 3 components, padded with a
    // zero z in 2D, and a tensor 9; a generic field keeps its source count.
    int writtenComponents;
  };

  class DataPlotter {
  public:
    DataPlotter(std::vector<DataField>& targetFields, const std::string& identifier, const int components);
    DataPlotter(const DataPlotter&)            = delete;
    DataPlotter(DataPlotter&&)                 = delete;
    DataPlotter& operator=(const DataPlotter&) = delete;
    DataPlotter& operator=(DataPlotter&&)      = delete;
    ~DataPlotter()                             = default;

    template <class T>
    void writeTuple(const int index, const T* const values, const int numberOfValues);

  private:
    std::vector<DataField>& _targetFields;
    int                     _fieldIndex{-1};
    int                     _components{1};
  };

public:
  enum class VertexMerging { Merged, Distinct };

  class VertexPlotter {
  public:
    VertexPlotter(FileWriter& plotter, int subdomainId, VertexMerging vertexMerging);
    VertexPlotter(const VertexPlotter&)            = delete;
    VertexPlotter(VertexPlotter&&)                 = delete;
    VertexPlotter& operator=(const VertexPlotter&) = delete;
    VertexPlotter& operator=(VertexPlotter&&)      = delete;
    ~VertexPlotter()                               = default;

    int plotVertex(const tarch::la::Vector<2, double>& position);
    int plotVertex(const tarch::la::Vector<3, double>& position);

  private:
    SubdomainBuffers&   _buffers;
    const VertexMerging _vertexMerging;
  };

  std::unique_ptr<VertexPlotter> createVertexPlotter(
    int           subdomainId,
    VertexMerging vertexMerging = VertexMerging::Merged
  );

  class VertexDataPlotter {
  public:
    VertexDataPlotter(
      FileWriter&        plotter,
      const int          subdomainId,
      const std::string& identifier,
      const int          recordsPerVertex
    );
    VertexDataPlotter(const VertexDataPlotter&)            = delete;
    VertexDataPlotter(VertexDataPlotter&&)                 = delete;
    VertexDataPlotter& operator=(const VertexDataPlotter&) = delete;
    VertexDataPlotter& operator=(VertexDataPlotter&&)      = delete;
    ~VertexDataPlotter()                                   = default;

    void plotVertexData(const int index, const double value);
    void plotVertexData(const int index, const tarch::la::Vector<2, double>& value);
    void plotVertexData(const int index, const tarch::la::Vector<3, double>& value);
    void plotVertexData(const int index, const double* const values, const int numberOfValues);

    template <class T>
    void plotVertexData(const int index, const T value);
    template <class T>
    void plotVertexData(const int index, const tarch::la::Vector<2, T>& value);
    template <class T>
    void plotVertexData(const int index, const tarch::la::Vector<3, T>& value);
    template <class T>
    void plotVertexData(const int index, const T* const values, const int numberOfValues);

  private:
    DataPlotter _impl;
  };

  std::unique_ptr<VertexDataPlotter> createVertexDataPlotter(
    const int          subdomainId,
    const std::string& identifier,
    const int          recordsPerVertex
  );

  class CellPlotter {
  public:
    CellPlotter(FileWriter& plotter, const int subdomainId);
    CellPlotter(const CellPlotter&)            = delete;
    CellPlotter(CellPlotter&&)                 = delete;
    CellPlotter& operator=(const CellPlotter&) = delete;
    CellPlotter& operator=(CellPlotter&&)      = delete;
    ~CellPlotter()                             = default;

    int plotHexahedron(const int* const vertexIndices);
    int plotQuadrangle(const int* const vertexIndices);
    int plotLine(const int* const vertexIndices);
    int plotPoint(const int vertexIndex);
    int plotTriangle(const int* const vertexIndices);

  private:
    SubdomainBuffers& _buffers;
    std::int32_t      _currentOffset;

    int pushCell(const std::int32_t* const vertexIndices, const int numberOfVertices, const std::uint8_t vtkType);

    template <int N>
    int plotStandardCell(const int* const vertexIndices, const std::uint8_t vtkType);
  };

  std::unique_ptr<CellPlotter> createCellPlotter(const int subdomainId);

  class CellDataPlotter {
  public:
    CellDataPlotter(
      FileWriter&        plotter,
      const int          subdomainId,
      const std::string& identifier,
      const int          recordsPerCell
    );
    CellDataPlotter(const CellDataPlotter&)            = delete;
    CellDataPlotter(CellDataPlotter&&)                 = delete;
    CellDataPlotter& operator=(const CellDataPlotter&) = delete;
    CellDataPlotter& operator=(CellDataPlotter&&)      = delete;
    ~CellDataPlotter()                                 = default;

    void plotCellData(const int index, const double value);
    void plotCellData(const int index, const tarch::la::Vector<2, double>& value);
    void plotCellData(const int index, const tarch::la::Vector<3, double>& value);
    void plotCellData(const int index, const double* const values, const int numberOfValues);

    template <class T>
    void plotCellData(const int index, const T value);
    template <class T>
    void plotCellData(const int index, const tarch::la::Vector<2, T>& value);
    template <class T>
    void plotCellData(const int index, const tarch::la::Vector<3, T>& value);
    template <class T>
    void plotCellData(const int index, const T* const values, const int numberOfValues);

  private:
    DataPlotter _impl;
  };

  std::unique_ptr<CellDataPlotter> createCellDataPlotter(
    const int          subdomainId,
    const std::string& identifier,
    const int          recordsPerCell
  );

  // MPI-IO hints for the parallel writers (collective buffering for the large
  // collective writes).
  static std::vector<std::pair<std::string, std::string>> mpiIoHints();

protected:
  static void validateIdentifier(const std::string& id);
  void        createOutputDirectory() const;

  /**
   * Validates the piece id and returns its buffer set, under the acquisition
   * lock: Concurrent writers may still be claiming pieces, which grows the
   * deque, and even a growing deque's size and element lookup must not be
   * read unguarded. The returned reference stays valid without the lock.
   */
  SubdomainBuffers& pieceBuffers(int subdomainId);

  SubdomainBuffersSequence getSubdomainBuffersSequenceForWriting() const;

  // Field schema from the local reference, no MPI exchange. The parallel writers run
  // their collective I/O on a sub-communicator of the data-holding ranks, so every
  // participant has a non-null reference with the identical field list.
  static FieldSchema fieldSchemaFromReference(const SubdomainBuffers* const localReference);

  // Classify a field by its source component count into the written width and
  // role every writer uses (see FieldLayout).
  static FieldLayout classifyField(const int sourceComponents);

  // Re-lay a component-major field buffer (values stored as
  // [tuple0 comp0..compN, tuple1 ...]) from sourceComponents to writtenComponents
  // per tuple, padding the extra trailing components with zero. Returns the input
  // unchanged when the widths already match.
  static std::vector<double> expandFieldComponents(
    const std::vector<double>& values,
    const int                  sourceComponents,
    const int                  writtenComponents
  );

  // Narrow a double buffer to float for single-precision output.
  static std::vector<float> narrowToFloat(const std::vector<double>& values);

  static bool             hasSchemaOrGeometry(const SubdomainBuffers& buffers);
  static bool             hasPayload(const SubdomainBuffers& buffers);
  const SubdomainBuffers* findReferenceSubdomain() const;
  void                    validateSubdomainsForWriting(const SubdomainBuffers& reference) const;

  std::string _outputDirectory;
  bool        _written = false;

  /**
   * One buffer set per acquired piece. A deque never relocates its entries
   * when it grows, so the references the plotters bind stay valid while
   * further pieces are claimed.
   */
  std::deque<SubdomainBuffers> _subdomains{};

private:
  /** Order keys of the pieces, parallel to the buffer deque. */
  std::vector<int>        _pieceOrderKeys{};
  std::unordered_set<int> _claimedPieceOrderKeys{};

  /**
   * Serialises piece acquisition and the piece lookups of the plotter
   * constructors. Each plot writer claims one piece per snapshot before it
   * plots, so the lock sits outside every per-patch path.
   */
  std::mutex _pieceAcquisitionMutex{};

  static bool _doublePrecisionOutput;
};

#include "tarch/plotter/CellDataPlotter.cpph"
#include "tarch/plotter/FileWriter.cpph"
#include "tarch/plotter/VertexDataPlotter.cpph"
