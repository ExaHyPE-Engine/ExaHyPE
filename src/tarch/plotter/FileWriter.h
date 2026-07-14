// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace tarch::plotter {
  class FileWriter;
} // namespace tarch::plotter

class tarch::plotter::FileWriter {
public:
  FileWriter(const std::string& outputDirectory, int numberOfSubdomains);
  virtual ~FileWriter() = default;

  virtual bool isOpen() const;
  virtual bool writeToFile(const std::string& filename, double time) = 0;

  virtual std::string getFileExtension() const = 0;

protected:
  struct DataField {
    std::string        name{};
    int                components = 1;
    std::vector<float> values{};
  };

  struct VertexHash {
    std::size_t operator()(const std::tuple<long long, long long, long long>& k) const;
  };

  struct SubdomainBuffers {
    // Geometry
    std::vector<float>        pointsXYZ{};
    std::vector<std::int32_t> connectivity{};
    std::vector<std::int32_t> offsets{};
    std::vector<std::uint8_t> types{};

    // Data
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
    // Number of components actually written. A vector is always written with 3
    // components (a 2D vector is padded with a zero z) and a tensor with 9, so
    // the field is a real vector/tensor in ParaView and the other readers
    // regardless of the source count. Generic fields keep their source count.
    int writtenComponents;
  };

  class DataPlotter {
  public:
    DataPlotter(
      FileWriter&             plotter,
      int                     subdomainId,
      std::vector<DataField>& targetFields,
      const std::string&      identifier,
      int                     components
    );
    ~DataPlotter() = default;

    template <class T>
    void writeTuple(int index, const T* v, int n);

  private:
    FileWriter&             _plotter;
    std::vector<DataField>& _targetFields;
    int                     _fieldIndex;
    int                     _components;
  };

public:
  class VertexPlotter {
  public:
    VertexPlotter(FileWriter& plotter, int subdomainId);
    ~VertexPlotter() = default;

    int plotVertex(const tarch::la::Vector<2, double>& position);
    int plotVertex(const tarch::la::Vector<3, double>& position);

  private:
    FileWriter&       _plotter;
    SubdomainBuffers& _buffers;
  };

  std::unique_ptr<VertexPlotter> createVertexPlotter(int subdomainId);

  class VertexDataPlotter {
  public:
    VertexDataPlotter(FileWriter& plotter, int subdomainId, const std::string& id, int recordsPerVertex);
    ~VertexDataPlotter() = default;

    void plotVertexData(int index, double value);
    void plotVertexData(int index, const tarch::la::Vector<2, double>& value);
    void plotVertexData(int index, const tarch::la::Vector<3, double>& value);
    void plotVertexData(int index, double* values, int numberOfValues);

    template <class T>
    void plotVertexData(int index, T value);
    template <class T>
    void plotVertexData(int index, const tarch::la::Vector<2, T>& value);
    template <class T>
    void plotVertexData(int index, const tarch::la::Vector<3, T>& value);
    template <class T>
    void plotVertexData(int index, T* values, int numberOfValues);

  private:
    DataPlotter _impl;
  };

  std::unique_ptr<VertexDataPlotter>
  createVertexDataPlotter(int subdomainId, const std::string& identifier, int recordsPerVertex);

  class CellPlotter {
  public:
    CellPlotter(FileWriter& plotter, int subdomainId);
    ~CellPlotter() = default;

    int plotHexahedron(int vertexIndex[8]);
    int plotQuadrangle(int vertexIndex[4]);
    int plotLine(int vertexIndex[2]);
    int plotPoint(int vertexIndex);
    int plotTriangle(int vertexIndex[3]);

  private:
    FileWriter&       _plotter;
    SubdomainBuffers& _buffers;
    std::int32_t      _currentOffset = 0;

    int pushCell(const std::int32_t* idx, int n, std::uint8_t vtkType);

    template <int N>
    int plotStandardCell(const int* vertexIndex, std::uint8_t vtkType);
  };

  std::unique_ptr<CellPlotter> createCellPlotter(int subdomainId);

  class CellDataPlotter {
  public:
    CellDataPlotter(FileWriter& plotter, int subdomainId, const std::string& id, int recordsPerCell);
    ~CellDataPlotter() = default;

    void plotCellData(int index, double value);
    void plotCellData(int index, const tarch::la::Vector<2, double>& value);
    void plotCellData(int index, const tarch::la::Vector<3, double>& value);
    void plotCellData(int index, const double* values, int numberOfValues);

    template <class T>
    void plotCellData(int index, T value);
    template <class T>
    void plotCellData(int index, const tarch::la::Vector<2, T>& value);
    template <class T>
    void plotCellData(int index, const tarch::la::Vector<3, T>& value);
    template <class T>
    void plotCellData(int index, const T* values, int numberOfValues);

  private:
    DataPlotter _impl;
  };

  std::unique_ptr<CellDataPlotter>
  createCellDataPlotter(int subdomainId, const std::string& identifier, int recordsPerCell);

protected:
  static void validateIdentifier(const std::string& id);
  void        createOutputDirectory();
  void        checkCanOpenPlotter(int subdomainId) const;

  // MPI-IO hints for the parallel writers (collective buffering for the large
  // collective writes).
  static std::vector<std::pair<std::string, std::string>> mpiIoHints();
  SubdomainBuffersSequence                                getSubdomainBuffersSequenceForWriting() const;

  // Field schema from the local reference, no MPI exchange. The parallel writers run
  // their collective I/O on a sub-communicator of the data-holding ranks, so every
  // participant has a non-null reference with the identical field list.
  static FieldSchema fieldSchemaFromReference(const SubdomainBuffers* localReference);

  // Classify a field by its source component count into the written width and
  // role every writer uses (see FieldLayout).
  static FieldLayout classifyField(int sourceComponents);

  // Re-lay a component-major field buffer (values stored as
  // [tuple0 comp0..compN, tuple1 ...]) from sourceComponents to writtenComponents
  // per tuple, padding the extra trailing components with zero. Returns the input
  // unchanged when the widths already match.
  static std::vector<float>
  expandFieldComponents(const std::vector<float>& values, int sourceComponents, int writtenComponents);

  static bool             hasSchemaOrGeometry(const SubdomainBuffers& buffers);
  static bool             hasPayload(const SubdomainBuffers& buffers);
  const SubdomainBuffers* findReferenceSubdomain() const;
  void                    validateSubdomainsForWriting(const SubdomainBuffers& reference) const;

  std::string _outputDirectory;
  bool        _written = false;

  std::vector<SubdomainBuffers> _subdomains{};
};

#include "CellDataPlotter.cpph"
#include "FileWriter.cpph"
#include "VertexDataPlotter.cpph"
