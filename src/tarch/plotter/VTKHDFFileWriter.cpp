// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKHDFFileWriter.h"

#ifdef PEANO_WITH_HDF5

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"

#include <hdf5.h>

namespace fs = std::filesystem;

namespace {
  // Which .vtkhdf paths have already been created in this run. The first write
  // to a path truncates and lays out the skeleton; every later write reopens and
  // appends. Static (per process), guarded for the multithreaded plot step.
  std::set<std::string>& createdPaths() {
    static std::set<std::string> paths;
    return paths;
  }
  std::mutex& createdPathsMutex() {
    static std::mutex mutex;
    return mutex;
  }
} // namespace

tarch::plotter::VTKHDFFileWriter::VTKHDFFileWriter(const std::string& outputDirectory, int numberOfSubdomains) :
  FileWriter(outputDirectory, numberOfSubdomains) {}

bool        tarch::plotter::VTKHDFFileWriter::isOpen() const { return true; }
std::string tarch::plotter::VTKHDFFileWriter::getFileExtension() const { return ".vtkhdf"; }

bool tarch::plotter::VTKHDFFileWriter::writeToFile(const std::string& filename, double time) {
  createOutputDirectory();
  const std::string pathStr = (fs::path(_outputDirectory) / (filename + ".vtkhdf")).string();

  bool firstWrite;
  {
    std::lock_guard<std::mutex> lock(createdPathsMutex());
    firstWrite = createdPaths().insert(pathStr).second;
  }

  const auto localData = getSubdomainBuffersSequenceForWriting();
  // Empty ranks are dropped from the writing communicator below, so the schema is taken
  // directly from the local reference; a collective broadcast of it would deadlock here,
  // since the excluded ranks would not take part in it.
  const FieldSchema schema = fieldSchemaFromReference(localData.reference);

  // Every rank merges its local subdomains into one contiguous grid; all ranks'
  // grids are then concatenated into a single VTKHDF part per step (no composite
  // dataset, matching the HDF5/XDMF writer). One collective write per dataset, so
  // a heterogeneous rank distribution cannot mismatch collective call counts.
  unsigned long long localPoints = 0, localCells = 0, localConn = 0;
  for (const SubdomainBuffers* s : localData.payloads) {
    localPoints += s->pointsXYZ.size() / 3;
    localCells += s->types.size();
    localConn += s->connectivity.size();
  }

  unsigned long long offPoints = 0, offCells = 0, offConn = 0;
  unsigned long long totPoints = localPoints, totCells = localCells, totConn = localConn;

#if defined(PEANO_WITH_MPI)
  auto&    rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm comm          = rank.getCommunicator();
  bool     useParallelIO = rank.getNumberOfRanks() > 1;
  bool     isMaster      = rank.isGlobalMaster();

  // Restrict the collective HDF5 I/O to only the ranks that hold data. An empty rank
  // joining a collective write to a chunked + deflated (filtered) dataset with a
  // zero-sized selection deadlocks HDF5's parallel filtered-collective chunk
  // redistribution on some MPI-IO stacks (MPICH/ROMIO on a parallel filesystem). Dropping
  // the empty ranks keeps every participant's collective sequence identical, so the
  // compressed parallel write stays well-defined.
  MPI_Comm ioComm = MPI_COMM_NULL;
  if (useParallelIO) {
    const bool hasData    = (localData.reference != nullptr);
    int        localHas   = hasData ? 1 : 0;
    int        anyHasData = 0;
    tarch::mpi::allreduce(&localHas, &anyHasData, 1, MPI_INT, MPI_MAX, comm);

    if (anyHasData == 0) {
      // Every rank is empty: the master alone writes a (serial) valid empty file.
      if (not rank.isGlobalMaster()) {
        return true;
      }
      useParallelIO = false;
    } else {
      MPI_Comm_split(comm, hasData ? 0 : MPI_UNDEFINED, rank.getRank(), &ioComm);
      if (not hasData) {
        return true; // nothing to contribute; the data-holding ranks write the file
      }
      comm       = ioComm;
      int ioRank = 0;
      MPI_Comm_rank(ioComm, &ioRank);
      isMaster = (ioRank == 0);
    }
  }

  if (useParallelIO) {
    unsigned long long local[3]  = {localPoints, localCells, localConn};
    unsigned long long prefix[3] = {0, 0, 0};
    unsigned long long totals[3] = {0, 0, 0};
    tarch::mpi::scan(local, prefix, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    offPoints = prefix[0] - local[0];
    offCells  = prefix[1] - local[1];
    offConn   = prefix[2] - local[2];
    tarch::mpi::allreduce(local, totals, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    totPoints = totals[0];
    totCells  = totals[1];
    totConn   = totals[2];
  }
#else
  bool useParallelIO = false;
  bool isMaster      = true;
#endif

  hid_t fapl = H5P_DEFAULT;
  hid_t dxpl = H5P_DEFAULT;
#if defined(PEANO_WITH_MPI)
  if (useParallelIO) {
    fapl             = H5Pcreate(H5P_FILE_ACCESS);
    MPI_Info   info  = MPI_INFO_NULL;
    const auto hints = mpiIoHints();
    if (not hints.empty()) {
      MPI_Info_create(&info);
      for (const auto& hint : hints) {
        MPI_Info_set(info, hint.first.c_str(), hint.second.c_str());
      }
    }
    H5Pset_fapl_mpio(fapl, comm, info);
    if (info != MPI_INFO_NULL) {
      MPI_Info_free(&info);
    }
#if H5_VERSION_GE(1, 10, 0)
    H5Pset_all_coll_metadata_ops(fapl, 1);
    H5Pset_coll_metadata_write(fapl, 1);
#endif
    dxpl = H5Pcreate(H5P_DATASET_XFER);
    // The bulk datasets are deflate-filtered, and parallel HDF5 requires
    // collective transfer for filtered datasets. Compression is worth it here:
    // the single growing file is I/O-volume bound, so the smaller bytes win.
    H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE);
  }
#endif

  hid_t fileId = firstWrite ? H5Fcreate(pathStr.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl)
                            : H5Fopen(pathStr.c_str(), H5F_ACC_RDWR, fapl);
  if (fapl != H5P_DEFAULT) {
    H5Pclose(fapl);
  }
#if defined(PEANO_WITH_MPI)
  // H5Pset_fapl_mpio duplicated the communicator into the file, so the local handle is
  // no longer needed (the file keeps its own copy for the collective writes and close).
  if (ioComm != MPI_COMM_NULL) {
    MPI_Comm_free(&ioComm);
  }
#endif
  if (fileId < 0) {
    if (dxpl != H5P_DEFAULT) {
      H5Pclose(dxpl);
    }
    throw std::runtime_error("Failed to open VTKHDF file: " + pathStr);
  }
  hid_t lcpl = H5Pcreate(H5P_LINK_CREATE);
  H5Pset_create_intermediate_group(lcpl, 1);

  struct HandleCloser {
    hid_t fileId, lcpl, dxpl;
    ~HandleCloser() {
      H5Pclose(lcpl);
      if (dxpl != H5P_DEFAULT) {
        H5Pclose(dxpl);
      }
      H5Fclose(fileId);
    }
  } handleCloser{fileId, lcpl, dxpl};

  // Open each dataset once per traversal and reuse the handle across the
  // read-extent, extend and write calls below (closed together at scope exit,
  // also on throw). This removes most of the per-traversal collective metadata
  // opens that dominate the single growing file's MPI cost. Declared after
  // handleCloser so the datasets close before the file.
  std::map<std::string, hid_t> openDatasets;
  struct DatasetCloser {
    std::map<std::string, hid_t>& datasets;
    ~DatasetCloser() {
      for (auto& entry : datasets) {
        if (entry.second >= 0) {
          H5Dclose(entry.second);
        }
      }
    }
  } datasetCloser{openDatasets};
  auto dset = [&](const std::string& name) -> hid_t {
    auto it = openDatasets.find(name);
    if (it != openDatasets.end()) {
      return it->second;
    }
    hid_t handle = H5Dopen(fileId, name.c_str(), H5P_DEFAULT);
    openDatasets.emplace(name, handle);
    return handle;
  };

  auto createExtendible = [&](const std::string& name, int ndims, hsize_t ncomp, hid_t type, bool compress) {
    hsize_t dims[2] = {0, ncomp};
    hsize_t maxd[2] = {H5S_UNLIMITED, ncomp};
    // Bulk geometry/data datasets get a large chunk + deflate, which shrinks the
    // file several-fold and cuts the I/O that dominates plot time; the tiny
    // one-row-per-step index datasets stay small and uncompressed. Deflate stays
    // readable - HDF5/ParaView decompress transparently - and is safe in parallel
    // because every write here is collective.
    // 65536-row chunks measured fastest across rank counts: smaller raises filter
    // overhead, larger raises cross-rank redistribution. Index datasets stay tiny.
    hsize_t chunk[2] = {static_cast<hsize_t>(compress ? 65536 : 1024), ncomp};
    hid_t   sp       = H5Screate_simple(ndims, dims, maxd);
    hid_t   dcpl     = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(dcpl, ndims, chunk);
    if (compress) {
      // Skip the pre-fill only on the compressed bulk datasets: every element is written
      // by the collective append (the rank slabs tile each grown chunk). The uncompressed
      // index/step datasets are written by the master alone, where skipping fill defers
      // chunk allocation to an independent write and corrupts them under parallel HDF5, so
      // they keep the default fill.
      H5Pset_fill_time(dcpl, H5D_FILL_TIME_NEVER);
      // Shuffle before deflate improves the ratio of the homogeneous data; reversed on read.
      H5Pset_shuffle(dcpl);
      H5Pset_deflate(dcpl, 1);
    }
    hid_t ds = H5Dcreate(fileId, name.c_str(), type, sp, lcpl, dcpl, H5P_DEFAULT);
    H5Pclose(dcpl);
    H5Sclose(sp);
    if (ds < 0) {
      throw std::runtime_error("VTKHDF: failed to create dataset " + name);
    }
    H5Dclose(ds);
  };
  auto rowsOf = [&](const std::string& name) -> hsize_t {
    hid_t   sp      = H5Dget_space(dset(name));
    hsize_t dims[2] = {0, 0};
    H5Sget_simple_extent_dims(sp, dims, nullptr);
    H5Sclose(sp);
    return dims[0];
  };
  // Extend a dataset to (cur + globalAdd) rows and write this rank's slab at the
  // exclusive-prefix offset within the new block. With collective IO an empty
  // rank issues a balanced empty write so the collective does not deadlock.
  auto appendSlab = [&](
                      const std::string& name,
                      int                ndims,
                      hsize_t            ncomp,
                      hid_t              type,
                      hsize_t            cur,
                      hsize_t            globalAdd,
                      hsize_t            localRows,
                      hsize_t            localOff,
                      const void*        data
                    ) {
    hid_t   ds      = dset(name);
    hsize_t newd[2] = {cur + globalAdd, ncomp};
    H5Dset_extent(ds, newd);
    hid_t fs = H5Dget_space(ds);
    if (localRows > 0) {
      hsize_t start[2] = {cur + localOff, 0};
      hsize_t count[2] = {localRows, ncomp};
      H5Sselect_hyperslab(fs, H5S_SELECT_SET, start, nullptr, count, nullptr);
      hid_t ms = H5Screate_simple(ndims, count, nullptr);
      if (H5Dwrite(ds, type, ms, fs, dxpl, data) < 0) {
        H5Sclose(ms);
        H5Sclose(fs);
        throw std::runtime_error("VTKHDF hyperslab write failed: " + name);
      }
      H5Sclose(ms);
    } else if (dxpl != H5P_DEFAULT) {
      // Collective filtered I/O: an empty rank must still join the write with an
      // empty selection so all ranks issue the collective the same number of times.
      H5Sselect_none(fs);
      H5Dwrite(ds, type, H5S_ALL, fs, dxpl, nullptr);
    }
    H5Sclose(fs);
  };
  // Append one row to a 1D index dataset. The caller passes the row (= step), so we skip
  // an H5Dget_space per dataset. Extent grows collectively; only the master writes.
  auto appendStepRow = [&](const std::string& name, hid_t type, const void* value, hsize_t row) {
    hid_t   ds      = dset(name);
    hsize_t newd[1] = {row + 1};
    H5Dset_extent(ds, newd);
    hid_t fs = H5Dget_space(ds);
    // Collective write: the master writes the single value, the other ranks join with an
    // empty selection. A master-only write to a shared dataset gives wrong data under
    // parallel HDF5 on a real filesystem, so every rank participates (as for the bulk data).
    if (isMaster) {
      hsize_t start[1] = {row};
      hsize_t count[1] = {1};
      H5Sselect_hyperslab(fs, H5S_SELECT_SET, start, nullptr, count, nullptr);
      hid_t ms = H5Screate_simple(1, count, nullptr);
      H5Dwrite(ds, type, ms, fs, dxpl, value);
      H5Sclose(ms);
    } else if (dxpl != H5P_DEFAULT) {
      H5Sselect_none(fs);
      H5Dwrite(ds, type, H5S_ALL, fs, dxpl, nullptr);
    }
    H5Sclose(fs);
  };
  // Write the trailing Offsets sentinel. Offsets is unfiltered, so the master writes it
  // independently; the extent was already grown collectively by the preceding appendSlab.
  auto writeI64At = [&](const std::string& name, hsize_t position, long long value) {
    hid_t ds = dset(name);
    hid_t fs = H5Dget_space(ds);
    // Collective: master writes the sentinel, the other ranks join with an empty selection.
    if (isMaster) {
      hsize_t start[1] = {position};
      hsize_t count[1] = {1};
      H5Sselect_hyperslab(fs, H5S_SELECT_SET, start, nullptr, count, nullptr);
      hid_t ms = H5Screate_simple(1, count, nullptr);
      H5Dwrite(ds, H5T_NATIVE_LLONG, ms, fs, dxpl, &value);
      H5Sclose(ms);
    } else if (dxpl != H5P_DEFAULT) {
      H5Sselect_none(fs);
      H5Dwrite(ds, H5T_NATIVE_LLONG, H5S_ALL, fs, dxpl, nullptr);
    }
    H5Sclose(fs);
  };

  if (firstWrite) {
    hid_t grp = H5Gcreate(fileId, "/VTKHDF", lcpl, H5P_DEFAULT, H5P_DEFAULT);
    {
      hsize_t   d[1]   = {2};
      long long ver[2] = {2, 5};
      hid_t     sp     = H5Screate_simple(1, d, nullptr);
      hid_t     at     = H5Acreate(grp, "Version", H5T_STD_I64LE, sp, H5P_DEFAULT, H5P_DEFAULT);
      H5Awrite(at, H5T_NATIVE_LLONG, ver);
      H5Aclose(at);
      H5Sclose(sp);
    }
    {
      const char* type = "UnstructuredGrid";
      hid_t       st   = H5Tcopy(H5T_C_S1);
      H5Tset_size(st, std::strlen(type));
      hid_t sp = H5Screate(H5S_SCALAR);
      hid_t at = H5Acreate(grp, "Type", st, sp, H5P_DEFAULT, H5P_DEFAULT);
      H5Awrite(at, st, type);
      H5Aclose(at);
      H5Sclose(sp);
      H5Tclose(st);
    }
    H5Gclose(grp);

    createExtendible("/VTKHDF/Points", 2, 3, H5T_IEEE_F32LE, true);
    createExtendible("/VTKHDF/Connectivity", 1, 1, H5T_STD_I64LE, true);
    createExtendible("/VTKHDF/Offsets", 1, 1, H5T_STD_I64LE, true);
    createExtendible("/VTKHDF/Types", 1, 1, H5T_STD_U8LE, true);
    createExtendible("/VTKHDF/NumberOfPoints", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/NumberOfCells", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/NumberOfConnectivityIds", 1, 1, H5T_STD_I64LE, false);
    // Vectors are stored with 3 components and tensors with 9 so ParaView reads
    // them back as real vectors / tensors; see FileWriter::classifyField.
    for (const auto& f : schema.pointFields) {
      createExtendible(
        "/VTKHDF/PointData/" + f.name,
        2,
        classifyField(f.components).writtenComponents,
        H5T_IEEE_F32LE,
        true
      );
    }
    for (const auto& f : schema.cellFields) {
      createExtendible(
        "/VTKHDF/CellData/" + f.name,
        2,
        classifyField(f.components).writtenComponents,
        H5T_IEEE_F32LE,
        true
      );
    }

    H5Gclose(H5Gcreate(fileId, "/VTKHDF/Steps", lcpl, H5P_DEFAULT, H5P_DEFAULT));
    createExtendible("/VTKHDF/Steps/Values", 1, 1, H5T_IEEE_F32LE, false);
    createExtendible("/VTKHDF/Steps/PartOffsets", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/Steps/PointOffsets", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/Steps/CellOffsets", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/Steps/ConnectivityIdOffsets", 1, 1, H5T_STD_I64LE, false);
    for (const auto& f : schema.pointFields) {
      createExtendible("/VTKHDF/Steps/PointDataOffsets/" + f.name, 1, 1, H5T_STD_I64LE, false);
    }
    for (const auto& f : schema.cellFields) {
      createExtendible("/VTKHDF/Steps/CellDataOffsets/" + f.name, 1, 1, H5T_STD_I64LE, false);
    }
    hid_t     stg  = H5Gopen(fileId, "/VTKHDF/Steps", H5P_DEFAULT);
    hid_t     sp   = H5Screate(H5S_SCALAR);
    long long zero = 0;
    hid_t     at   = H5Acreate(stg, "NSteps", H5T_STD_I64LE, sp, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(at, H5T_NATIVE_LLONG, &zero);
    H5Aclose(at);
    H5Sclose(sp);
    H5Gclose(stg);
  }

  // Current extents before this step (cumulative over all prior steps) - the offsets
  // the VTKHDF reader expects in /VTKHDF/Steps.
  const long long curPoints  = static_cast<long long>(rowsOf("/VTKHDF/Points"));
  const long long curConn    = static_cast<long long>(rowsOf("/VTKHDF/Connectivity"));
  const long long curCells   = static_cast<long long>(rowsOf("/VTKHDF/Types"));
  const hsize_t   curOffsets = rowsOf("/VTKHDF/Offsets");
  const long long curStep    = static_cast<long long>(rowsOf("/VTKHDF/Steps/Values"));

  // Merge this rank's subdomains into one contiguous grid for the step. Points,
  // connectivity and offsets are rebased to step-global indices so that the whole
  // step is a single part (NumberOf*[step] = step totals, one entry per step).
  std::vector<float>   pointsBuf;
  std::vector<int64_t> connBuf, beforeOffsets;
  std::vector<uint8_t> typesBuf;
  pointsBuf.reserve(localPoints * 3);
  connBuf.reserve(localConn);
  beforeOffsets.reserve(localCells);
  typesBuf.reserve(localCells);
  unsigned long long pointCursor = 0;
  unsigned long long connCursor  = 0;
  for (const SubdomainBuffers* s : localData.payloads) {
    const int64_t pointBase = static_cast<int64_t>(offPoints + pointCursor);
    pointsBuf.insert(pointsBuf.end(), s->pointsXYZ.begin(), s->pointsXYZ.end());
    for (auto c : s->connectivity) {
      connBuf.push_back(static_cast<int64_t>(c) + pointBase);
    }
    int32_t prevEnd = 0;
    for (std::size_t i = 0; i < s->types.size(); i++) {
      beforeOffsets.push_back(static_cast<int64_t>(offConn + connCursor) + prevEnd);
      prevEnd = s->offsets[i];
    }
    typesBuf.insert(typesBuf.end(), s->types.begin(), s->types.end());
    pointCursor += s->pointsXYZ.size() / 3;
    connCursor += s->connectivity.size();
  }

  appendSlab("/VTKHDF/Points", 2, 3, H5T_NATIVE_FLOAT, curPoints, totPoints, localPoints, offPoints, pointsBuf.data());
  appendSlab("/VTKHDF/Connectivity", 1, 1, H5T_NATIVE_LLONG, curConn, totConn, localConn, offConn, connBuf.data());
  appendSlab("/VTKHDF/Types", 1, 1, H5T_NATIVE_UINT8, curCells, totCells, localCells, offCells, typesBuf.data());
  // Offsets per step: one before-offset per cell + a trailing sentinel (= step
  // connectivity total), so the step block has totCells + 1 entries.
  appendSlab(
    "/VTKHDF/Offsets",
    1,
    1,
    H5T_NATIVE_LLONG,
    curOffsets,
    totCells + 1,
    localCells,
    offCells,
    beforeOffsets.data()
  );
  writeI64At("/VTKHDF/Offsets", curOffsets + totCells, static_cast<long long>(totConn));

  // Every subdomain shares the schema's field order (validateSubdomainsForWriting),
  // so index by position instead of searching by name.
  for (std::size_t fieldIndex = 0; fieldIndex < schema.pointFields.size(); ++fieldIndex) {
    const auto&        f       = schema.pointFields[fieldIndex];
    const int          written = classifyField(f.components).writtenComponents;
    std::vector<float> buf;
    buf.reserve(localPoints * f.components);
    for (const SubdomainBuffers* s : localData.payloads) {
      const auto& values = s->pointFields[fieldIndex].values;
      buf.insert(buf.end(), values.begin(), values.end());
    }
    // Skip the copy for scalars; only vectors/tensors are widened.
    if (written != f.components) {
      buf = expandFieldComponents(buf, f.components, written);
    }
    const std::string ds = "/VTKHDF/PointData/" + f.name;
    // Point fields share the Points extent (curPoints), so no per-dataset query.
    appendSlab(
      ds,
      2,
      written,
      H5T_NATIVE_FLOAT,
      static_cast<hsize_t>(curPoints),
      totPoints,
      localPoints,
      offPoints,
      buf.data()
    );
    appendStepRow(
      "/VTKHDF/Steps/PointDataOffsets/" + f.name,
      H5T_NATIVE_LLONG,
      &curPoints,
      static_cast<hsize_t>(curStep)
    );
  }
  for (std::size_t fieldIndex = 0; fieldIndex < schema.cellFields.size(); ++fieldIndex) {
    const auto&        f       = schema.cellFields[fieldIndex];
    const int          written = classifyField(f.components).writtenComponents;
    std::vector<float> buf;
    buf.reserve(localCells * f.components);
    for (const SubdomainBuffers* s : localData.payloads) {
      const auto& values = s->cellFields[fieldIndex].values;
      buf.insert(buf.end(), values.begin(), values.end());
    }
    if (written != f.components) {
      buf = expandFieldComponents(buf, f.components, written);
    }
    const std::string ds = "/VTKHDF/CellData/" + f.name;
    // Cell fields share the Types extent (curCells).
    appendSlab(
      ds,
      2,
      written,
      H5T_NATIVE_FLOAT,
      static_cast<hsize_t>(curCells),
      totCells,
      localCells,
      offCells,
      buf.data()
    );
    appendStepRow(
      "/VTKHDF/Steps/CellDataOffsets/" + f.name,
      H5T_NATIVE_LLONG,
      &curCells,
      static_cast<hsize_t>(curStep)
    );
  }

  const long long totPointsSigned = static_cast<long long>(totPoints);
  const long long totCellsSigned  = static_cast<long long>(totCells);
  const long long totConnSigned   = static_cast<long long>(totConn);
  const float     timeF           = static_cast<float>(time);
  const hsize_t   stepRow         = static_cast<hsize_t>(curStep);
  appendStepRow("/VTKHDF/NumberOfPoints", H5T_NATIVE_LLONG, &totPointsSigned, stepRow);
  appendStepRow("/VTKHDF/NumberOfCells", H5T_NATIVE_LLONG, &totCellsSigned, stepRow);
  appendStepRow("/VTKHDF/NumberOfConnectivityIds", H5T_NATIVE_LLONG, &totConnSigned, stepRow);
  appendStepRow("/VTKHDF/Steps/Values", H5T_NATIVE_FLOAT, &timeF, stepRow);
  appendStepRow("/VTKHDF/Steps/PartOffsets", H5T_NATIVE_LLONG, &curStep, stepRow);
  appendStepRow("/VTKHDF/Steps/PointOffsets", H5T_NATIVE_LLONG, &curPoints, stepRow);
  appendStepRow("/VTKHDF/Steps/CellOffsets", H5T_NATIVE_LLONG, &curCells, stepRow);
  appendStepRow("/VTKHDF/Steps/ConnectivityIdOffsets", H5T_NATIVE_LLONG, &curConn, stepRow);

  hid_t     stg = H5Gopen(fileId, "/VTKHDF/Steps", H5P_DEFAULT);
  hid_t     at  = H5Aopen(stg, "NSteps", H5P_DEFAULT);
  long long ns  = curStep + 1;
  H5Awrite(at, H5T_NATIVE_LLONG, &ns);
  H5Aclose(at);
  H5Gclose(stg);

  return true;
}

#endif
