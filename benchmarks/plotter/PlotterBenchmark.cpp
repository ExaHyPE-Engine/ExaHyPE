// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// Benchmark for the parallel plotter back-ends (VTU, HDF5/XDMF, VTKHDF, NetCDF).
// Each rank builds several subdomains in parallel (one per OpenMP thread, as in a
// real traversal) and writes them with one collective writeToFile per back-end. Build
// and write are timed separately (max across ranks); the collective write is one thread
// per rank by nature, so threads speed up the build, not the write.
//
// Command-line arguments (all optional):
//   --n <int>           cells per axis per subdomain   (default 24 -> n^DIM cells)
//   --fields <int>      number of scalar cell fields   (default 5)
//   --snapshots <int>   writeToFile calls per back-end (default 3)
//   --subdomains <int>  subdomains per rank            (default: OpenMP thread count)
//   --empty             only rank 0 holds data (heterogeneous stress)
//   --keep              keep the output files (default: delete them after timing)
#include "tarch/Init.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/plotter/VTUFileWriter.h"
#ifdef PEANO_WITH_HDF5
#include "tarch/plotter/HDF5FileWriter.h"
#include "tarch/plotter/VTKHDFFileWriter.h"
#endif
#ifdef PEANO_WITH_NETCDF
#include "tarch/plotter/NetCDFFileWriter.h"
#endif

#if defined(PEANO_WITH_OPENMP)
#include <omp.h>
#endif

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
  int argInt(int argc, char** argv, const std::string& flag, int fallback) {
    for (int i = 1; i + 1 < argc; ++i) {
      if (flag == argv[i]) {
        return std::atoi(argv[i + 1]);
      }
    }
    return fallback;
  }

  bool argFlag(int argc, char** argv, const std::string& flag) {
    for (int i = 1; i < argc; ++i) {
      if (flag == argv[i]) {
        return true;
      }
    }
    return false;
  }

  double seconds(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b) {
    return std::chrono::duration<double>(b - a).count();
  }

  // Remove the benchmark's output files (everything starting with prefix) in the working
  // directory.
  void removeOutput(const std::string& prefix) {
    std::error_code                    ec;
    std::vector<std::filesystem::path> doomed;
    for (const auto& entry : std::filesystem::directory_iterator(".", ec)) {
      if (entry.path().filename().string().rfind(prefix, 0) == 0) {
        doomed.push_back(entry.path());
      }
    }
    for (const auto& path : doomed) {
      std::filesystem::remove(path, ec);
    }
  }

  // Build one subdomain's regular grid and its cell-data fields.
  template <typename Writer>
  void buildSubdomain(Writer& w, int subdomain, int n, int fields, int snapshot) {
    auto                                                                      vp = w.createVertexPlotter(subdomain);
    auto                                                                      cp = w.createCellPlotter(subdomain);
    std::vector<std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter>> fp;
    for (int f = 0; f < fields; ++f) {
      fp.push_back(w.createCellDataPlotter(subdomain, "field" + std::to_string(f), 1));
    }

    const int stride = n + 1;
#if DIMENSIONS == 2
    for (int j = 0; j <= n; ++j) {
      for (int i = 0; i <= n; ++i) {
        vp->plotVertex(tarch::la::Vector<2, double>{double(i) / n, double(j) / n});
      }
    }
    for (int j = 0; j < n; ++j) {
      for (int i = 0; i < n; ++i) {
        int quad[4] = {j * stride + i, j * stride + i + 1, (j + 1) * stride + i + 1, (j + 1) * stride + i};
        int c       = cp->plotQuadrangle(quad);
        for (int f = 0; f < fields; ++f) {
          fp[f]->plotCellData(c, double(snapshot) + 0.001 * c + f);
        }
      }
    }
#else
    auto V = [stride](int i, int j, int k) { return (k * stride + j) * stride + i; };
    for (int k = 0; k <= n; ++k) {
      for (int j = 0; j <= n; ++j) {
        for (int i = 0; i <= n; ++i) {
          vp->plotVertex(tarch::la::Vector<3, double>{double(i) / n, double(j) / n, double(k) / n});
        }
      }
    }
    for (int k = 0; k < n; ++k) {
      for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
          int hex[8] = {
            V(i, j, k),
            V(i + 1, j, k),
            V(i + 1, j + 1, k),
            V(i, j + 1, k),
            V(i, j, k + 1),
            V(i + 1, j, k + 1),
            V(i + 1, j + 1, k + 1),
            V(i, j + 1, k + 1)
          };
          int c = cp->plotHexahedron(hex);
          for (int f = 0; f < fields; ++f) {
            fp[f]->plotCellData(c, double(snapshot) + 0.001 * c + f);
          }
        }
      }
    }
#endif
  }

  template <typename Writer>
  void runWriter(
    const std::string& tag,
    int                n,
    int                fields,
    int                snapshots,
    int                numSub,
    bool               emptyMode,
    bool               sameFile,
    bool               cleanup
  ) {
    auto&      rankObj  = tarch::mpi::Rank::getInstance();
    const bool haveData = not(emptyMode and rankObj.getRank() != 0);

    double sumBuild = 0.0;
    double sumWrite = 0.0;
    double sumTotal = 0.0;
    double maxTotal = 0.0;

    try {
      for (int s = 0; s < snapshots; ++s) {
        Writer w(".", numSub);

#if defined(PEANO_WITH_MPI)
        MPI_Barrier(rankObj.getCommunicator()); // align the start of this output
#endif
        const auto outputStart = std::chrono::steady_clock::now();

        // Build the subdomains in parallel: each thread owns a distinct subdomain (and
        // hence a distinct buffer), so no synchronisation is needed.
        if (haveData) {
#if defined(PEANO_WITH_OPENMP)
#pragma omp parallel for schedule(static)
#endif
          for (int sd = 0; sd < numSub; ++sd) {
            buildSubdomain<Writer>(w, sd, n, fields, s);
          }
        }
        double buildTime = seconds(outputStart, std::chrono::steady_clock::now());

        const std::string fname      = sameFile ? ("bench_" + tag) : ("bench_" + tag + "_" + std::to_string(s));
        const auto        writeStart = std::chrono::steady_clock::now();
        w.writeToFile(fname, double(s));
        const auto outputEnd = std::chrono::steady_clock::now();

        double writeTime = seconds(writeStart, outputEnd);
        double totalTime = seconds(outputStart, outputEnd); // build + write = one plot

#if defined(PEANO_WITH_MPI)
        MPI_Allreduce(MPI_IN_PLACE, &buildTime, 1, MPI_DOUBLE, MPI_MAX, rankObj.getCommunicator());
        MPI_Allreduce(MPI_IN_PLACE, &writeTime, 1, MPI_DOUBLE, MPI_MAX, rankObj.getCommunicator());
        MPI_Allreduce(MPI_IN_PLACE, &totalTime, 1, MPI_DOUBLE, MPI_MAX, rankObj.getCommunicator());
#endif
        sumBuild += buildTime;
        sumWrite += writeTime;
        sumTotal += totalTime;
        maxTotal = (totalTime > maxTotal) ? totalTime : maxTotal;
      }

      if (rankObj.isGlobalMaster()) {
        std::printf(
          "  %-7s : %2d outputs | total %8.4fs (build %7.4fs + write %7.4fs) | max %8.4fs\n",
          tag.c_str(),
          snapshots,
          sumTotal / snapshots,
          sumBuild / snapshots,
          sumWrite / snapshots,
          maxTotal
        );
        std::fflush(stdout);
      }
    } catch (const std::exception& e) {
      if (rankObj.isGlobalMaster()) {
        std::printf("  %-7s : FAILED (%s)\n", tag.c_str(), e.what());
        std::fflush(stdout);
      }
    }

    if (cleanup) {
#if defined(PEANO_WITH_MPI)
      MPI_Barrier(rankObj.getCommunicator()); // all ranks done writing before we delete
#endif
      if (rankObj.isGlobalMaster()) {
        removeOutput("bench_" + tag);
      }
    }
  }
} // namespace

int main(int argc, char** argv) {
  tarch::init(&argc, &argv, {0.0, 0.0, true});

  int threads = 1;
#if defined(PEANO_WITH_OPENMP)
  threads = omp_get_max_threads();
#endif

  const int  n         = argInt(argc, argv, "--n", 24);
  const int  fields    = argInt(argc, argv, "--fields", 5);
  const int  snapshots = argInt(argc, argv, "--snapshots", 3);
  const int  numSub    = argInt(argc, argv, "--subdomains", threads > 0 ? threads : 1);
  const bool emptyMode = argFlag(argc, argv, "--empty");
  const bool cleanup   = not argFlag(argc, argv, "--keep");

  auto& rankObj = tarch::mpi::Rank::getInstance();
  if (rankObj.isGlobalMaster()) {
    unsigned long long cellsPerSubdomain = 1;
    for (int d = 0; d < DIMENSIONS; ++d) {
      cellsPerSubdomain *= static_cast<unsigned long long>(n);
    }
    std::printf(
      "PlotterBenchmark: ranks=%d threads=%d subdomains=%d dim=%d n=%d cells/subdomain=%llu fields=%d snapshots=%d empty=%d\n",
      rankObj.getNumberOfRanks(),
      threads,
      numSub,
      DIMENSIONS,
      n,
      cellsPerSubdomain,
      fields,
      snapshots,
      emptyMode ? 1 : 0
    );
    std::fflush(stdout);
  }

  runWriter<tarch::plotter::VTUFileWriter>("vtu", n, fields, snapshots, numSub, emptyMode, false, cleanup);
#ifdef PEANO_WITH_HDF5
  runWriter<tarch::plotter::HDF5FileWriter>("hdf5", n, fields, snapshots, numSub, emptyMode, false, cleanup);
  runWriter<tarch::plotter::VTKHDFFileWriter>("vtkhdf", n, fields, snapshots, numSub, emptyMode, true, cleanup);
#endif
#ifdef PEANO_WITH_NETCDF
  runWriter<tarch::plotter::NetCDFFileWriter>("netcdf", n, fields, snapshots, numSub, emptyMode, false, cleanup);
#endif

  if (rankObj.isGlobalMaster()) {
    std::printf("PlotterBenchmark: done\n");
    std::fflush(stdout);
  }

  tarch::shutdown();
  return 0;
}
