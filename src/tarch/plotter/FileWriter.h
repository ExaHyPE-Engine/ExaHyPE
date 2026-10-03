// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace tarch::plotter {
  class FileWriter;
} // namespace tarch::plotter

class tarch::plotter::FileWriter {
public:
  /**
   * A plot point's coordinates quantised to the deduplication tolerance.
   */
  using VertexKey = std::tuple<std::int64_t, std::int64_t, std::int64_t>;

  struct DataField {
    std::string         name{};
    int                 components = 1;
    std::vector<double> values{};
  };

  struct VertexHash {
    std::size_t operator()(const VertexKey& key) const;
  };

  /**
   * The geometry and the fields of one piece. offsets stores the end of every cell in
   * connectivity.
   */
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
   * The field names and component counts of a collective write, with empty values. Every rank
   * creates the same datasets, a rank without local data included.
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
    // The number of written components is 3 for a vector, 9 for a tensor and the source count
    // otherwise. A 2D vector gets a zero z.
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
    INLINE_METHOD void writeTuple(int index, const T* values, int numberOfValues);

  private:
    std::vector<DataField>& _targetFields;
    int                     _fieldIndex{-1};
    int                     _components{1};
  };

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

    void plotVertexData(int index, double value);
    void plotVertexData(int index, const tarch::la::Vector<2, double>& value);
    void plotVertexData(int index, const tarch::la::Vector<3, double>& value);
    void plotVertexData(int index, const double* values, int numberOfValues);

    template <class T>
    INLINE_METHOD void plotVertexData(int index, T value);
    template <class T>
    INLINE_METHOD void plotVertexData(int index, const tarch::la::Vector<2, T>& value);
    template <class T>
    INLINE_METHOD void plotVertexData(int index, const tarch::la::Vector<3, T>& value);
    template <class T>
    INLINE_METHOD void plotVertexData(int index, const T* values, int numberOfValues);

  private:
    DataPlotter _impl;
  };

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
    int plotTriangle(const int* const vertexIndices);

  private:
    SubdomainBuffers& _buffers;
    std::int32_t      _currentOffset;

    int pushCell(const std::int32_t* const vertexIndices, const int numberOfVertices, const std::uint8_t vtkType);

    template <int N>
    int plotStandardCell(const int* const vertexIndices, const std::uint8_t vtkType);
  };

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

    void plotCellData(int index, double value);
    void plotCellData(int index, const tarch::la::Vector<2, double>& value);
    void plotCellData(int index, const tarch::la::Vector<3, double>& value);
    void plotCellData(int index, const double* values, int numberOfValues);

    template <class T>
    INLINE_METHOD void plotCellData(int index, T value);
    template <class T>
    INLINE_METHOD void plotCellData(int index, const tarch::la::Vector<2, T>& value);
    template <class T>
    INLINE_METHOD void plotCellData(int index, const tarch::la::Vector<3, T>& value);
    template <class T>
    INLINE_METHOD void plotCellData(int index, const T* values, int numberOfValues);

  private:
    DataPlotter _impl;
  };

  explicit FileWriter(const std::string& outputDirectory);
  FileWriter(const FileWriter&)            = delete;
  FileWriter(FileWriter&&)                 = delete;
  FileWriter& operator=(const FileWriter&) = delete;
  FileWriter& operator=(FileWriter&&)      = delete;
  virtual ~FileWriter()                    = default;

  int acquirePiece(int orderKey);

  SubdomainBuffers& getPieceBuffers(int subdomainId);

  std::unique_ptr<VertexPlotter> createVertexPlotter(
    int           subdomainId,
    VertexMerging vertexMerging = VertexMerging::Merged
  );

  std::unique_ptr<VertexDataPlotter> createVertexDataPlotter(
    const int          subdomainId,
    const std::string& identifier,
    const int          recordsPerVertex
  );

  std::unique_ptr<CellPlotter> createCellPlotter(const int subdomainId);

  std::unique_ptr<CellDataPlotter> createCellDataPlotter(
    const int          subdomainId,
    const std::string& identifier,
    const int          recordsPerCell
  );

  virtual bool isOpen() const;
  virtual bool writeToFile(const std::string& filename, const double time) = 0;

  virtual std::string getFileExtension() const = 0;

  static void setDoublePrecisionOutput(const bool value);
  static bool getDoublePrecisionOutput();

protected:
  static void validateIdentifier(const std::string& id);
  void createOutputDirectory() const;

  static std::vector<std::pair<std::string, std::string>> getMPIFileHints();
  SubdomainBuffersSequence getSubdomainBuffersSequenceForWriting() const;

  static FieldSchema getFieldSchema(const SubdomainBuffers* const localReference);

  static FieldLayout classifyField(const int sourceComponents);

  static std::vector<double> expandFieldComponents(
    const std::vector<double>& values,
    const int                  sourceComponents,
    const int                  writtenComponents
  );

  static std::vector<float> narrowToFloat(const std::vector<double>& values);

  static bool hasSchemaOrGeometry(const SubdomainBuffers& buffers);
  static bool hasPayload(const SubdomainBuffers& buffers);
  const SubdomainBuffers* findReferenceSubdomain() const;
  void validateSubdomainsForWriting(const SubdomainBuffers& reference) const;

  std::string _outputDirectory;
  bool        _written = false;

  /**
   * One buffer set per acquired piece. A deque never relocates its entries
   * when it grows, so the references the plotters bind stay valid while
   * further pieces are claimed.
   */
  std::deque<SubdomainBuffers> _subdomains{};

private:
  /**
   * Order keys of the pieces, parallel to the buffer deque.
   */
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
