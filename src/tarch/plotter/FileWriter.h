// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace tarch::plotter {
  class FileWriter;
} // namespace tarch::plotter

class tarch::plotter::FileWriter {
public:
  FileWriter(const std::string& outputDirectory, const int numberOfSubdomains);
  FileWriter(const FileWriter&)            = delete;
  FileWriter(FileWriter&&)                 = delete;
  virtual ~FileWriter()                    = default;
  FileWriter& operator=(const FileWriter&) = delete;
  FileWriter& operator=(FileWriter&&)      = delete;

  virtual bool isOpen() const;
  virtual bool writeToFile(const std::string& filename, const double time) = 0;

  virtual std::string getFileExtension() const = 0;

  static void setDoublePrecisionOutput(const bool value);
  static bool getDoublePrecisionOutput();

protected:
  struct DataField {
    std::string         name{};
    int                 components = 1;
    std::vector<double> values{};
  };

  struct VertexHash {
    std::size_t operator()(const std::tuple<long long, long long, long long>& key) const;
  };

  struct SubdomainBuffers {
    std::vector<double>       pointsXYZ{};
    std::vector<std::int32_t> connectivity{};
    std::vector<std::int32_t> offsets{};
    std::vector<std::uint8_t> types{};

    std::vector<DataField> pointFields{};
    std::vector<DataField> cellFields{};

    std::unordered_map<std::tuple<long long, long long, long long>, int, VertexHash> vertexMap;
  };

  struct SubdomainBuffersSequence {
    const SubdomainBuffers*              reference = nullptr;
    std::vector<const SubdomainBuffers*> payloads{};
  };

  /**
   * Globally-agreed data-field schema for a collective parallel write. Holds
   * the field names and component counts (the `values` of the DataFields stay
   * empty); it exists so that every rank - including ranks that hold no local
   * data - creates and collectively writes the identical set of datasets /
   * variables. Without it, an empty rank derives an empty field list from its
   * (null) local reference and skips the per-field collective calls that the
   * non-empty ranks make, which deadlocks collective HDF5/NetCDF IO.
   */
  struct FieldSchema {
    std::vector<DataField> pointFields{};
    std::vector<DataField> cellFields{};
  };

  // Semantic role of a data field, inferred from its component count. The plotter
  // API only carries a count, so the role follows the usual visualisation
  // convention: 1 = scalar, 2 or 3 = vector, 9 = tensor. Any other count stays a
  // generic multi-component array.
  enum class FieldKind { Scalar, Vector, Tensor, Generic };

  struct FieldLayout {
    FieldKind kind;
    // Number of components written. A vector is always written with 3
    // components (a 2D vector is padded with a zero z) and a tensor with 9, so
    // ParaView and the other readers interpret the field as a vector or tensor
    // regardless of the source count. Generic fields keep their source count.
    int writtenComponents;
  };

  class DataPlotter {
  public:
    DataPlotter(
      FileWriter&             plotter,
      const int               subdomainId,
      std::vector<DataField>& targetFields,
      const std::string&      identifier,
      const int               components
    );
    DataPlotter(const DataPlotter&)            = delete;
    DataPlotter(DataPlotter&&)                 = delete;
    ~DataPlotter()                             = default;
    DataPlotter& operator=(const DataPlotter&) = delete;
    DataPlotter& operator=(DataPlotter&&)      = delete;

    template <class T>
    void writeTuple(const int index, const T* const values, const int numberOfValues);

  private:
    FileWriter&             _plotter;
    std::vector<DataField>& _targetFields;
    int                     _fieldIndex;
    int                     _components;
  };

public:
  enum class VertexSharing { Shared, Distinct };

  class VertexPlotter {
  public:
    VertexPlotter(FileWriter& plotter, const int subdomainId, const VertexSharing vertexSharing);
    VertexPlotter(const VertexPlotter&)            = delete;
    VertexPlotter(VertexPlotter&&)                 = delete;
    ~VertexPlotter()                               = default;
    VertexPlotter& operator=(const VertexPlotter&) = delete;
    VertexPlotter& operator=(VertexPlotter&&)      = delete;

    int plotVertex(const tarch::la::Vector<2, double>& position);
    int plotVertex(const tarch::la::Vector<3, double>& position);

  private:
    FileWriter&         _plotter;
    SubdomainBuffers&   _buffers;
    const VertexSharing _vertexSharing;
  };

  std::unique_ptr<VertexPlotter> createVertexPlotter(
    const int           subdomainId,
    const VertexSharing vertexSharing = VertexSharing::Shared
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
    ~VertexDataPlotter()                                   = default;
    VertexDataPlotter& operator=(const VertexDataPlotter&) = delete;
    VertexDataPlotter& operator=(VertexDataPlotter&&)      = delete;

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
    ~CellPlotter()                             = default;
    CellPlotter& operator=(const CellPlotter&) = delete;
    CellPlotter& operator=(CellPlotter&&)      = delete;

    int plotHexahedron(const int* const vertexIndices);
    int plotQuadrangle(const int* const vertexIndices);
    int plotLine(const int* const vertexIndices);
    int plotPoint(const int vertexIndex);
    int plotTriangle(const int* const vertexIndices);

  private:
    FileWriter&       _plotter;
    SubdomainBuffers& _buffers;
    std::int32_t      _currentOffset = 0;

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
    ~CellDataPlotter()                                 = default;
    CellDataPlotter& operator=(const CellDataPlotter&) = delete;
    CellDataPlotter& operator=(CellDataPlotter&&)      = delete;

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

protected:
  static void validateIdentifier(const std::string& id);
  void        createOutputDirectory() const;
  void        checkCanOpenPlotter(const int subdomainId) const;

  // MPI-IO hints for the parallel writers (collective buffering for the large
  // collective writes).
  static std::vector<std::pair<std::string, std::string>> mpiIoHints();
  SubdomainBuffersSequence                                getSubdomainBuffersSequenceForWriting() const;

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

  std::vector<SubdomainBuffers> _subdomains{};

private:
  static bool _doublePrecisionOutput;
};

#include "CellDataPlotter.cpph"
#include "FileWriter.cpph"
#include "VertexDataPlotter.cpph"
