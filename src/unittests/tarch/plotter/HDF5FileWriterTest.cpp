// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/plotter/HDF5FileWriter.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/CellTypes.h"
#include "tarch/plotter/Mesh.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/tarch/plotter/Meshes.h"

#ifdef PEANO_WITH_HDF5

#include <hdf5.h>

namespace {
  void barrier() { tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator()); }

  /**
   * The path, the existence and the content of a file as the global master read it back after the
   * collective write. Every other rank keeps the path alone.
   */
  struct ReadBackFile final {
    std::string path{};
    bool        exists{false};
    std::string content{};
  };

  std::string getPath(const std::string& name) { return (tarch::testing::getWritableTestDirectory() / name).string(); }

  ReadBackFile readOnGlobalMaster(const std::string& name) {
    ReadBackFile file{.path = getPath(name)};
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      file.exists = std::filesystem::exists(file.path);
      const std::ifstream stream(file.path);
      std::stringstream   buffer;
      buffer << stream.rdbuf();
      file.content = buffer.str();
    }
    return file;
  }

  /**
   * Removes the files of names on the global master once every rank finished its write.
   */
  void removeOnGlobalMaster(const std::vector<std::string>& names) {
    barrier();
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      for (const std::string& name : names) {
        std::filesystem::remove(getPath(name));
      }
    }
    barrier();
  }

  /**
   * Writes mesh as the snapshot at the time 0 and returns the .xmf file of the snapshot. The function
   * removes every file of the series.
   */
  ReadBackFile writeAndReadBack(const tarch::plotter::Mesh& mesh, const std::string& filename) {
    tarch::plotter::HDF5FileWriter writer;
    writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.0);
    barrier();
    const ReadBackFile xmfFile = readOnGlobalMaster(filename + "-0_000000.xmf");
    removeOnGlobalMaster({filename + "-0_000000.xmf", filename + "-0_000000.h5", filename + ".xmf"});
    return xmfFile;
  }

  void validateXMFContent(const ReadBackFile& xmfFile, const std::string& expectedSubstring) {
    VALIDATE(xmfFile.exists, xmfFile.path);
    VALIDATE(xmfFile.content.find(expectedSubstring) != std::string::npos, xmfFile.path, expectedSubstring);
  }

  /**
   * Checks that the writer pads the vector field (1.5, -1.5) with a zero third component.
   */
  void validatePaddedVectorField(const hsize_t components, const std::vector<float>& values) {
    VALIDATE_EQUALS(components, 3);
    VALIDATE(values[0] == 1.5F and values[1] == -1.5F and values[2] == 0.0F, values[0], values[1], values[2]);
  }
} // namespace

UNIT_TEST("A unit square writes a mixed topology with X_Y_Z geometry whose coordinates lie in the HDF5 file") {
  GIVEN("A mesh of one unit square on every rank") {
    const std::string    filename = "HDF5Test2DQuadrangle";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 0.0, 1);

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master reads the .xmf file") {
      const ReadBackFile xmfFile = writeAndReadBack(mesh, filename);

      THEN("The .xmf file declares a mixed topology and X_Y_Z geometry with the coordinates in the HDF5 file") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          validateXMFContent(xmfFile, "TopologyType=\"Mixed\"");
          validateXMFContent(xmfFile, "GeometryType=\"X_Y_Z\"");
          validateXMFContent(xmfFile, filename + "-0_000000.h5:/geometry/x");
        }
      }
    }
  }
}

UNIT_TEST("Two particles write their scalar point field as an attribute of the nodes") {
  GIVEN("Two points with the scalar point field Mass on every rank") {
    const std::string    filename = "HDF5Test2DParticles";
    tarch::plotter::Mesh mesh;
    mesh.pointsXYZ = {0.5, 0.5, 0.0, 0.6, 0.6, 0.0};
    mesh.pointFields.push_back({"Mass", 1, {10.0, 20.0}});

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master reads the .xmf file") {
      const ReadBackFile xmfFile = writeAndReadBack(mesh, filename);

      THEN("The .xmf file declares Mass a scalar attribute of the nodes") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          validateXMFContent(xmfFile, "Attribute Name=\"Mass\" AttributeType=\"Scalar\"");
          validateXMFContent(xmfFile, "Center=\"Node\"");
        }
      }
    }
  }
}

UNIT_TEST("A unit cube writes its scalar cell field as an attribute of the cells") {
  GIVEN("A mesh of one unit cube with the scalar cell field Density on every rank") {
    const std::string    filename = "HDF5Test3DCellData";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitCube(mesh, 0.0);
    mesh.cellFields.push_back({"Density", 1, {1.234}});

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master reads the .xmf file") {
      const ReadBackFile xmfFile = writeAndReadBack(mesh, filename);

      THEN("The .xmf file declares Density a scalar attribute of the cells") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          validateXMFContent(xmfFile, "Attribute Name=\"Density\" AttributeType=\"Scalar\"");
          validateXMFContent(xmfFile, "Center=\"Cell\"");
        }
      }
    }
  }
}

UNIT_TEST("Fields of one, three and four components write the attribute types Scalar, Vector and Matrix") {
  GIVEN("One point with point fields of one, three and four components on every rank") {
    const std::string    filename = "HDF5TestTypes";
    tarch::plotter::Mesh mesh;
    mesh.pointsXYZ = {0.0, 0.0, 0.0};
    mesh.pointFields.push_back({"ScalarField", 1, {1.0}});
    mesh.pointFields.push_back({"VectorField", 3, {1.0, 2.0, 3.0}});
    mesh.pointFields.push_back({"TensorField", 4, {1.0, 0.0, 0.0, 1.0}});

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master reads the .xmf file") {
      const ReadBackFile xmfFile = writeAndReadBack(mesh, filename);

      THEN("The .xmf file declares the three fields Scalar, Vector and Matrix") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          validateXMFContent(xmfFile, "Name=\"ScalarField\" AttributeType=\"Scalar\"");
          validateXMFContent(xmfFile, "Name=\"VectorField\" AttributeType=\"Vector\"");
          validateXMFContent(xmfFile, "Name=\"TensorField\" AttributeType=\"Matrix\"");
        }
      }
    }
  }
}

UNIT_TEST("The writer pads a field of two components to a Vector of three and labels nine components a Tensor") {
  GIVEN("A unit square with the cell field V2 of two components and the cell field T9 of nine on every rank") {
    const std::string    filename = "HDF5TestVectorTensor";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 0.0, 1);
    mesh.cellFields.push_back({"V2", 2, {1.5, -1.5}});
    mesh.cellFields.push_back({"T9", 9, {1, 2, 3, 4, 5, 6, 7, 8, 9}});

    WHEN("The writer writes the mesh and the global master reads the dataset of V2 and the .xmf file back") {
      tarch::plotter::HDF5FileWriter writer;
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.0);
      barrier();
      hsize_t            dimensions[2] = {0, 0};
      std::vector<float> vectorValues;
      if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
        const hid_t file      = H5Fopen(getPath(filename + "-0_000000.h5").c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        const hid_t dataset   = H5Dopen(file, "/cell_data/V2", H5P_DEFAULT);
        const hid_t dataSpace = H5Dget_space(dataset);
        H5Sget_simple_extent_dims(dataSpace, dimensions, nullptr);
        vectorValues.resize(dimensions[0] * dimensions[1]);
        H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, vectorValues.data());
        H5Sclose(dataSpace);
        H5Dclose(dataset);
        H5Fclose(file);
      }
      const ReadBackFile xmfFile = readOnGlobalMaster(filename + "-0_000000.xmf");
      removeOnGlobalMaster({filename + "-0_000000.xmf", filename + "-0_000000.h5", filename + ".xmf"});

      THEN("The .xmf file labels the vector and the tensor and the vector dataset contains the row (1.5, -1.5, 0)") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          validateXMFContent(xmfFile, "Attribute Name=\"V2\" AttributeType=\"Vector\"");
          validateXMFContent(xmfFile, "Attribute Name=\"T9\" AttributeType=\"Tensor\"");
          validatePaddedVectorField(dimensions[1], vectorValues);
        }
      }
    }
  }
}

UNIT_TEST("The mixed topology writes the node count for a line and the type alone for a quadrangle") {
  GIVEN("A line and a unit square with a cell field Marker on every rank, apart from the cells of the other ranks") {
    const std::string    filename = "HDF5TestMixedCellTypes";
    const double         x        = 10.0 * tarch::mpi::Rank::getInstance().getRank();
    tarch::plotter::Mesh mesh;
    mesh.pointsXYZ    = {x, 0.0, 0.0, x + 1.0, 0.0, 0.0};
    mesh.connectivity = {0, 1};
    mesh.offsets      = {2};
    mesh.types        = {static_cast<std::uint8_t>(tarch::plotter::CellType::Line)};
    tarch::plotter::unittests::appendUnitSquares(mesh, x + 2.0, 1);
    mesh.cellFields.push_back({"Marker", 1, {1.0, 2.0}});

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master reads the .xmf file") {
      const ReadBackFile xmfFile = writeAndReadBack(mesh, filename);
      const int          ranks   = tarch::mpi::Rank::getInstance().getNumberOfRanks();

      THEN("The grid contains two cells, six points and nine topology entries per rank and the field Marker") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          validateXMFContent(xmfFile, "NumberOfElements=\"" + std::to_string(2 * ranks) + "\"");
          validateXMFContent(xmfFile, "Dimensions=\"" + std::to_string(6 * ranks) + "\"");
          validateXMFContent(xmfFile, "Dimensions=\"" + std::to_string(9 * ranks) + "\"");
          validateXMFContent(xmfFile, "Attribute Name=\"Marker\"");
        }
      }
    }
  }
}

UNIT_TEST("The collective write completes when only the global master writes cells") {
  GIVEN("A unit square with the cell field Marker on the global master and the empty field Marker elsewhere") {
    const std::string    filename = "HDF5TestHeterogeneous";
    tarch::plotter::Mesh mesh;
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      tarch::plotter::unittests::appendUnitSquares(mesh, 0.0, 1);
      mesh.cellFields.push_back({"Marker", 1, {1.0}});
    } else {
      mesh.cellFields.push_back({"Marker", 1, {}});
    }

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master reads the .xmf file") {
      const ReadBackFile xmfFile = writeAndReadBack(mesh, filename);

      THEN("The grid contains the one cell of the global master and the field Marker") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          validateXMFContent(xmfFile, "NumberOfElements=\"1\"");
          validateXMFContent(xmfFile, "Attribute Name=\"Marker\"");
        }
      }
    }
  }
}

UNIT_TEST("The grid of a collective write over ranks with different cell counts contains the cells of all ranks") {
  GIVEN("Three unit squares on the global master and two on every other rank, apart from each other") {
    const std::string    filename = "HDF5TestUneven";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(
      mesh,
      100.0 * tarch::mpi::Rank::getInstance().getRank(),
      tarch::mpi::Rank::getInstance().isGlobalMaster() ? 3 : 2
    );

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master reads the .xmf file") {
      const ReadBackFile xmfFile = writeAndReadBack(mesh, filename);
      const int          cells   = 3 + 2 * (tarch::mpi::Rank::getInstance().getNumberOfRanks() - 1);

      THEN("The grid contains the cells of every rank") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          validateXMFContent(xmfFile, "NumberOfElements=\"" + std::to_string(cells) + "\"");
        }
      }
    }
  }
}

UNIT_TEST("A mesh without points, cells and fields writes a valid empty grid") {
  GIVEN("An empty mesh on every rank") {
    const std::string          filename = "HDF5TestEmpty";
    const tarch::plotter::Mesh mesh;

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master reads the .xmf file") {
      const ReadBackFile xmfFile = writeAndReadBack(mesh, filename);

      THEN("The grid contains zero cells") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          validateXMFContent(xmfFile, "NumberOfElements=\"0\"");
        }
      }
    }
  }
}

UNIT_TEST("The .xmf file of the series embeds the grid of every snapshot file that one writer writes once") {
  GIVEN("A mesh of one unit square on every rank") {
    const std::string    filename = "HDF5TestSeries";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 0.0, 1);

    WHEN("One writer writes the mesh at the times 0, 0.5 and 0.5 and the global master reads the series back") {
      tarch::plotter::HDF5FileWriter writer;
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.0);
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.5);
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.5);
      barrier();
      const ReadBackFile seriesFile = readOnGlobalMaster(filename + ".xmf");
      removeOnGlobalMaster(
        {filename + "-0_000000.xmf",
         filename + "-0_000000.h5",
         filename + "-0_500000.xmf",
         filename + "-0_500000.h5",
         filename + ".xmf"}
      );

      THEN("The series lists the time 0, the time 0.5 once and the topology of the second snapshot") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          const std::string secondSnapshotTime = "<Time Value=\"0.5\"/>";
          validateXMFContent(seriesFile, "<Time Value=\"0\"/>");
          validateXMFContent(seriesFile, secondSnapshotTime);
          validateXMFContent(seriesFile, filename + "-0_500000.h5:/topology");
          VALIDATE(
            seriesFile.content.find(secondSnapshotTime) == seriesFile.content.rfind(secondSnapshotTime),
            seriesFile.path
          );
        }
      }
    }
  }
}

#endif
