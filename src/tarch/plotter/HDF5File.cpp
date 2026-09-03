// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5File.h"

#ifdef PEANO_WITH_HDF5

#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/FileWriter.h"

#include <hdf5.h>

// The header stores every handle as std::int64_t to keep <hdf5.h> out of it.
static_assert(std::is_same_v<hid_t, std::int64_t>);

namespace {
  hid_t toNativeType(const tarch::plotter::HDF5File::ElementType type) {
    switch (type) {
    case tarch::plotter::HDF5File::ElementType::Float32:
      return H5T_NATIVE_FLOAT;
    case tarch::plotter::HDF5File::ElementType::Float64:
      return H5T_NATIVE_DOUBLE;
    case tarch::plotter::HDF5File::ElementType::Int32:
      return H5T_NATIVE_INT;
    case tarch::plotter::HDF5File::ElementType::Int64:
      return H5T_NATIVE_LLONG;
    }
    tarch::terminateWithError("Unknown HDF5 element type.");
  }
} // namespace

tarch::plotter::HDF5File::Dataset::Dataset(
  HDF5File&                file,
  const std::string&       name,
  const unsigned long long rows,
  const int                columns,
  const ElementType        storageType
) :
  _dataset(-1),
  _transferProperties(file._transferProperties),
  _columns(columns) {
  if (rows == 0) {
    return;
  }

  const int     fileDimensions = columns == 0 ? 1 : 2;
  const hsize_t dimensions[2]  = {rows, static_cast<hsize_t>(columns)};
  const hid_t   fileSpace      = H5Screate_simple(fileDimensions, dimensions, nullptr);
  const hid_t   type           = toNativeType(storageType);

  // Chunk and deflate the dataset; readers decompress transparently. Parallel
  // HDF5 needs collective writes to filtered datasets, which writeRows() are.
  const hid_t creationProperties = H5Pcreate(H5P_DATASET_CREATE);

  // A chunk holds whole rows and targets about one MiB.
  constexpr hsize_t ChunkByteTarget     = 1U << 20;
  const hsize_t     rowElements         = std::max<hsize_t>(1, dimensions[1]);
  const hsize_t     bytesPerRow         = std::max<hsize_t>(1, rowElements * static_cast<hsize_t>(H5Tget_size(type)));
  const hsize_t     targetRows          = std::max<hsize_t>(1, ChunkByteTarget / bytesPerRow);
  const hsize_t     chunkDimensions[2]  = {std::min(static_cast<hsize_t>(rows), targetRows), dimensions[1]};
  H5Pset_chunk(creationProperties, fileDimensions, chunkDimensions);

  // The rank slabs tile the dataset exactly, so skip the default pre-write chunk fill.
  H5Pset_fill_time(creationProperties, H5D_FILL_TIME_NEVER);

  // Byte-shuffle before deflate improves the ratio of the homogeneous data; reversed on read.
  H5Pset_shuffle(creationProperties);
  H5Pset_deflate(creationProperties, 1);

  _dataset = H5Dcreate(
    file._file,
    name.c_str(),
    type,
    fileSpace,
    file._linkCreationProperties,
    creationProperties,
    H5P_DEFAULT
  );
  H5Pclose(creationProperties);
  H5Sclose(fileSpace);
  if (_dataset < 0) {
    tarch::terminateWithError("Failed to create HDF5 dataset '" + name + "'.");
  }
}

tarch::plotter::HDF5File::Dataset::~Dataset() {
  if (_dataset >= 0) {
    H5Dclose(_dataset);
  }
}

void tarch::plotter::HDF5File::Dataset::writeRows(
  const unsigned long long firstRow,
  const unsigned long long rows,
  const void* const        data,
  const ElementType        memoryType
) {
  if (_dataset < 0) {
    return;
  }

  const int   fileDimensions = _columns == 0 ? 1 : 2;
  const hid_t fileSpace      = H5Dget_space(_dataset);
  if (rows > 0) {
    const hsize_t localDimensions[2] = {rows, static_cast<hsize_t>(_columns)};
    const hsize_t offsets[2]         = {firstRow, 0};
    const hid_t   memorySpace        = H5Screate_simple(fileDimensions, localDimensions, nullptr);
    const herr_t  selectStatus =
      H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, offsets, nullptr, localDimensions, nullptr);
    const herr_t writeStatus =
      selectStatus >= 0
        ? H5Dwrite(_dataset, toNativeType(memoryType), memorySpace, fileSpace, _transferProperties, data)
        : -1;
    H5Sclose(memorySpace);
    if (selectStatus < 0 or writeStatus < 0) {
      H5Sclose(fileSpace);
      tarch::terminateWithError("Failed to write an HDF5 dataset hyperslab.");
    }
  } else if (_transferProperties != H5P_DEFAULT) {
    // Empty rank: Join the collective write with an empty selection, so all
    // ranks issue the same number of collective writes.
    H5Sselect_none(fileSpace);
    if (H5Dwrite(_dataset, toNativeType(memoryType), H5S_ALL, fileSpace, _transferProperties, nullptr) < 0) {
      H5Sclose(fileSpace);
      tarch::terminateWithError("Failed to write an empty HDF5 dataset selection.");
    }
  }
  H5Sclose(fileSpace);
}

void tarch::plotter::HDF5File::Dataset::writeRuns(
  const unsigned long long* const firstRows,
  const unsigned long long* const rowCounts,
  const int                       numberOfRuns,
  const void* const               data,
  const ElementType               memoryType
) {
  if (_dataset < 0) {
    return;
  }

  unsigned long long rows = 0;
  for (int run = 0; run < numberOfRuns; run++) {
    rows += rowCounts[run];
  }
  if (rows == 0) {
    writeRows(0, 0, data, memoryType);
    return;
  }

  const int   fileDimensions = _columns == 0 ? 1 : 2;
  const hid_t fileSpace      = H5Dget_space(_dataset);
  herr_t      selectStatus   = 0;
  bool        firstRun       = true;
  for (int run = 0; run < numberOfRuns and selectStatus >= 0; run++) {
    if (rowCounts[run] == 0) {
      continue;
    }
    const hsize_t runDimensions[2] = {rowCounts[run], static_cast<hsize_t>(_columns)};
    const hsize_t offsets[2]       = {firstRows[run], 0};
    selectStatus                   = H5Sselect_hyperslab(
      fileSpace,
      firstRun ? H5S_SELECT_SET : H5S_SELECT_OR,
      offsets,
      nullptr,
      runDimensions,
      nullptr
    );
    firstRun = false;
  }

  const hsize_t localDimensions[2] = {rows, static_cast<hsize_t>(_columns)};
  const hid_t   memorySpace        = H5Screate_simple(fileDimensions, localDimensions, nullptr);
  const herr_t  writeStatus =
    selectStatus >= 0 ? H5Dwrite(_dataset, toNativeType(memoryType), memorySpace, fileSpace, _transferProperties, data)
                      : -1;
  H5Sclose(memorySpace);
  H5Sclose(fileSpace);
  if (selectStatus < 0 or writeStatus < 0) {
    tarch::terminateWithError("Failed to write the union of the HDF5 dataset row runs.");
  }
}

tarch::plotter::HDF5File::HDF5File(
  const std::int64_t file,
  const std::int64_t linkCreationProperties,
  const std::int64_t transferProperties,
  const std::int64_t tableTransferProperties,
  const bool         writesTables
) :
  _file(file),
  _linkCreationProperties(linkCreationProperties),
  _transferProperties(transferProperties),
  _tableTransferProperties(tableTransferProperties),
  _writesTables(writesTables) {}

tarch::plotter::HDF5File::~HDF5File() {
  H5Pclose(_linkCreationProperties);
  if (_transferProperties != H5P_DEFAULT) {
    H5Pclose(_transferProperties);
  }
  if (_tableTransferProperties != H5P_DEFAULT) {
    H5Pclose(_tableTransferProperties);
  }
  H5Fclose(_file);
}

std::unique_ptr<tarch::plotter::HDF5File>
tarch::plotter::HDF5File::openForDataHoldingRanks(const std::string& path, const bool holdsData) {
  const auto& rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm    communicator  = rank.getCommunicator();
  bool        useParallelIO = rank.getNumberOfRanks() > 1;
  bool        writesTables  = rank.isGlobalMaster();

  MPI_Comm fileCommunicator = MPI_COMM_NULL;
  if (useParallelIO) {
    const int localHolds = holdsData ? 1 : 0;
    int       anyHolds   = 0;
    tarch::mpi::allreduce(&localHolds, &anyHolds, 1, MPI_INT, MPI_MAX, communicator);

    if (anyHolds == 0) {
      // Every rank is empty: The master alone creates a serial, valid empty file.
      if (not rank.isGlobalMaster()) {
        return nullptr;
      }
      useParallelIO = false;
    } else {
      MPI_Comm_split(communicator, holdsData ? 0 : MPI_UNDEFINED, rank.getRank(), &fileCommunicator);
      if (not holdsData) {
        return nullptr;
      }
      communicator = fileCommunicator;
      int fileRank = 0;
      MPI_Comm_rank(fileCommunicator, &fileRank);
      writesTables = (fileRank == 0);
    }
  }

  hid_t fileAccessProperties    = H5P_DEFAULT;
  hid_t transferProperties      = H5P_DEFAULT;
  hid_t tableTransferProperties = H5P_DEFAULT;

  if (useParallelIO) {
    fileAccessProperties = H5Pcreate(H5P_FILE_ACCESS);

    MPI_Info   info  = MPI_INFO_NULL;
    const auto hints = tarch::plotter::FileWriter::mpiIoHints();
    if (not hints.empty()) {
      MPI_Info_create(&info);
      for (const auto& hint : hints) {
        MPI_Info_set(info, hint.first.c_str(), hint.second.c_str());
      }
    }
    H5Pset_fapl_mpio(fileAccessProperties, communicator, info);
    if (info != MPI_INFO_NULL) {
      MPI_Info_free(&info);
    }

#if H5_VERSION_GE(1, 10, 0)
    // Collective metadata operations merge the ranks' small metadata writes.
    H5Pset_all_coll_metadata_ops(fileAccessProperties, true);
    H5Pset_coll_metadata_write(fileAccessProperties, true);
#endif

    // Collective transfer: One balanced write per dataset per rank, which lets
    // ROMIO aggregate the per-rank slabs into large file writes.
    transferProperties = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(transferProperties, H5FD_MPIO_COLLECTIVE);

    // One rank writes each table whole, so tables go out independently.
    tableTransferProperties = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(tableTransferProperties, H5FD_MPIO_INDEPENDENT);
  }

  const hid_t file = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fileAccessProperties);

  if (fileAccessProperties != H5P_DEFAULT) {
    H5Pclose(fileAccessProperties);
  }
  // H5Pset_fapl_mpio duplicated the communicator into the file, which keeps
  // its own copy for the collective writes and the close.
  if (fileCommunicator != MPI_COMM_NULL) {
    MPI_Comm_free(&fileCommunicator);
  }

  if (file < 0) {
    if (transferProperties != H5P_DEFAULT) {
      H5Pclose(transferProperties);
    }
    if (tableTransferProperties != H5P_DEFAULT) {
      H5Pclose(tableTransferProperties);
    }
    tarch::terminateWithError("Failed to create '" + path + "'.");
  }

  const hid_t linkCreationProperties = H5Pcreate(H5P_LINK_CREATE);
  H5Pset_create_intermediate_group(linkCreationProperties, 1);

  return std::unique_ptr<HDF5File>(
    new HDF5File(file, linkCreationProperties, transferProperties, tableTransferProperties, writesTables)
  );
}

bool tarch::plotter::HDF5File::writesTables() const { return _writesTables; }

void tarch::plotter::HDF5File::deriveRowRanges(
  const unsigned long long* const localRows,
  unsigned long long* const       firstRows,
  unsigned long long* const       totalRows,
  const int                       count
) const {
  if (_transferProperties == H5P_DEFAULT) {
    for (int i = 0; i < count; i++) {
      firstRows[i] = 0;
      totalRows[i] = localRows[i];
    }
    return;
  }

  // H5Pget_fapl_mpio hands out a duplicate of the file's communicator, which
  // the caller frees.
  const hid_t accessProperties = H5Fget_access_plist(_file);
  MPI_Comm    fileCommunicator = MPI_COMM_NULL;
  H5Pget_fapl_mpio(accessProperties, &fileCommunicator, nullptr);
  H5Pclose(accessProperties);
  // Inclusive prefix sum; subtracting the local count yields the exclusive first row.
  tarch::mpi::scan(localRows, firstRows, count, MPI_UNSIGNED_LONG_LONG, MPI_SUM, fileCommunicator);
  tarch::mpi::allreduce(localRows, totalRows, count, MPI_UNSIGNED_LONG_LONG, MPI_SUM, fileCommunicator);
  MPI_Comm_free(&fileCommunicator);
  for (int i = 0; i < count; i++) {
    firstRows[i] -= localRows[i];
  }
}

void tarch::plotter::HDF5File::writeTable(
  const std::string&       name,
  const unsigned long long rows,
  const int                columns,
  const double* const      data
) {
  writeTable(name, rows, columns, static_cast<const void*>(data), ElementType::Float64);
}

void tarch::plotter::HDF5File::writeTable(
  const std::string&       name,
  const unsigned long long rows,
  const int                columns,
  const std::int32_t* const data
) {
  writeTable(name, rows, columns, static_cast<const void*>(data), ElementType::Int32);
}

void tarch::plotter::HDF5File::writeTable(
  const std::string&       name,
  const unsigned long long rows,
  const int                columns,
  const std::int64_t* const data
) {
  writeTable(name, rows, columns, static_cast<const void*>(data), ElementType::Int64);
}

void tarch::plotter::HDF5File::writeTable(
  const std::string&       name,
  const unsigned long long rows,
  const int                columns,
  const void* const        data,
  const ElementType        storageType
) {
  const int     fileDimensions = columns == 0 ? 1 : 2;
  const hsize_t dimensions[2]  = {rows, static_cast<hsize_t>(columns)};
  const hid_t   type           = toNativeType(storageType);
  const hid_t   fileSpace      = H5Screate_simple(fileDimensions, dimensions, nullptr);
  const hid_t   dataset
    = H5Dcreate(_file, name.c_str(), type, fileSpace, _linkCreationProperties, H5P_DEFAULT, H5P_DEFAULT);
  if (dataset < 0) {
    H5Sclose(fileSpace);
    tarch::terminateWithError("Failed to create HDF5 table '" + name + "'.");
  }
  if (_writesTables and rows > 0) {
    if (H5Dwrite(dataset, type, H5S_ALL, fileSpace, _tableTransferProperties, data) < 0) {
      H5Dclose(dataset);
      H5Sclose(fileSpace);
      tarch::terminateWithError("Failed to write HDF5 table '" + name + "'.");
    }
  }
  H5Dclose(dataset);
  H5Sclose(fileSpace);
}

#endif
