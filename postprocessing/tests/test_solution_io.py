# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from pathlib import Path
import json
import tempfile
import unittest

import h5py
import numpy as np

from postprocessing.solution_io import (
    MeshSnapshot,
    SnapshotRef,
    SolutionSource,
    _cells_from_offsets,
    _candidate_directories,
    _decode_xdmf_topology,
    _discover_index,
    _discover_snapshots,
    _drop_trivial_z,
    _index_in_directory,
    _local_tag,
    _load_vtu_meshio,
    _ordered_field_names,
    _pack_cells,
    _scan_path,
    _squeeze_field,
    _time_from_name,
    _time_sort_key,
    load_snapshot_ref,
)

QUAD_POINTS = [
    [0.0, 0.0, 0.0],
    [1.0, 0.0, 0.0],
    [2.0, 0.0, 0.0],
    [0.0, 1.0, 0.0],
    [1.0, 1.0, 0.0],
    [2.0, 1.0, 0.0],
]
QUAD_CONNECTIVITY = [0, 1, 4, 3, 1, 2, 5, 4]


def write_vtkhdf(path, times):
    """Writes a growing VTKHDF file whose steps repeat the same two-quad mesh."""
    number_of_steps = len(times)
    with h5py.File(path, "w") as handle:
        group = handle.create_group("VTKHDF")
        group.create_dataset("NumberOfPoints", data=[6] * number_of_steps)
        group.create_dataset("NumberOfCells", data=[2] * number_of_steps)
        group.create_dataset("NumberOfConnectivityIds", data=[8] * number_of_steps)
        group.create_dataset("Points", data=np.tile(QUAD_POINTS, (number_of_steps, 1)))
        group.create_dataset("Connectivity", data=QUAD_CONNECTIVITY * number_of_steps)
        group.create_dataset("Offsets", data=[0, 4, 8] * number_of_steps)

        point_data = group.create_group("PointData")
        point_data.create_dataset(
            "pressure",
            data=np.arange(6 * number_of_steps, dtype=float).reshape(-1, 1),
        )
        cell_data = group.create_group("CellData")
        cell_data.create_dataset(
            "velocity",
            data=np.arange(2 * number_of_steps * 2, dtype=float).reshape(-1, 2),
        )

        steps = group.create_group("Steps")
        steps.attrs["NSteps"] = number_of_steps
        steps.create_dataset("Values", data=np.asarray(times, dtype=float))
        steps.create_dataset(
            "PointOffsets", data=[6 * step for step in range(number_of_steps)]
        )
        steps.create_dataset(
            "CellOffsets", data=[2 * step for step in range(number_of_steps)]
        )
        steps.create_dataset(
            "ConnectivityIdOffsets", data=[8 * step for step in range(number_of_steps)]
        )
    return path


def write_bare_hdf5(path):
    """Writes one step in the bare HDF5 layout, whose topology stores element codes."""
    with h5py.File(path, "w") as handle:
        geometry = handle.create_group("geometry")
        points = np.asarray(QUAD_POINTS, dtype=float)
        for axis, name in enumerate(("x", "y", "z")):
            geometry.create_dataset(name, data=points[:, axis])
        handle.create_dataset("topology", data=[5, 0, 1, 4, 3, 5, 1, 2, 5, 4])
        handle.create_group("data").create_dataset(
            "pressure", data=np.arange(6, dtype=float).reshape(-1, 1)
        )
    return path


def make_snapshot():
    return MeshSnapshot(
        name="Solver",
        time=0.5,
        points=np.asarray(QUAD_POINTS, dtype=float)[:, :2],
        cells=np.asarray([[0, 1, 4, 3], [1, 2, 5, 4]]),
        node_data={"pressure": np.zeros(6)},
        cell_data={"velocity": np.zeros((2, 2))},
        field_order=[("pressure", "node"), ("velocity", "cell")],
    )


class HelperTest(unittest.TestCase):
    def test_squeeze_field_turns_a_single_column_into_a_scalar(self):
        self.assertEqual(_squeeze_field(np.zeros((4, 1))).shape, (4,))

    def test_squeeze_field_keeps_a_vector_field(self):
        self.assertEqual(_squeeze_field(np.zeros((4, 3))).shape, (4, 3))

    def test_pack_cells_returns_an_array_for_a_uniform_corner_count(self):
        packed = _pack_cells([np.asarray([0, 1, 2, 3]), np.asarray([1, 2, 3, 4])])
        self.assertIsInstance(packed, np.ndarray)
        self.assertEqual(packed.shape, (2, 4))

    def test_pack_cells_returns_a_list_for_mixed_cells(self):
        packed = _pack_cells([np.asarray([0, 1, 2]), np.asarray([1, 2, 3, 4])])
        self.assertIsInstance(packed, list)
        self.assertEqual(len(packed), 2)

    def test_pack_cells_handles_an_empty_mesh(self):
        self.assertEqual(_pack_cells([]).shape, (0, 0))

    def test_ordered_field_names_lists_node_fields_first(self):
        self.assertEqual(
            _ordered_field_names({"p": None}, {"v": None}),
            [("p", "node"), ("v", "cell")],
        )

    def test_time_from_name_reads_the_encoded_stamp(self):
        self.assertAlmostEqual(_time_from_name(Path("elastic-0_500000.h5")), 0.5)

    def test_time_from_name_is_not_a_number_without_a_stamp(self):
        time = _time_from_name(Path("elastic.h5"))
        self.assertNotEqual(time, time)

    def test_time_sort_key_puts_stamped_files_first(self):
        paths = [Path("a.h5"), Path("a-1_0.h5"), Path("a-0_5.h5")]
        self.assertEqual(
            [path.name for path in sorted(paths, key=_time_sort_key)],
            ["a-0_5.h5", "a-1_0.h5", "a.h5"],
        )

    def test_cells_from_offsets_reshapes_a_uniform_stream(self):
        cells = _cells_from_offsets(
            np.asarray(QUAD_CONNECTIVITY), np.asarray([0, 4, 8]), 2
        )
        np.testing.assert_array_equal(cells, [[0, 1, 4, 3], [1, 2, 5, 4]])

    def test_cells_from_offsets_handles_mixed_widths(self):
        cells = _cells_from_offsets(
            np.asarray([0, 1, 2, 0, 1, 2, 3]), np.asarray([0, 3, 7]), 2
        )
        self.assertIsInstance(cells, list)
        np.testing.assert_array_equal(cells[0], [0, 1, 2])

    def test_cells_from_offsets_handles_an_empty_mesh(self):
        self.assertEqual(
            _cells_from_offsets(np.empty(0), np.asarray([0]), 0).shape, (0, 0)
        )

    def test_drop_trivial_z_flattens_a_planar_mesh(self):
        self.assertEqual(_drop_trivial_z(np.zeros((4, 3))).shape, (4, 2))

    def test_drop_trivial_z_keeps_a_genuine_third_axis(self):
        points = np.zeros((4, 3))
        points[0, 2] = 1.0
        self.assertEqual(_drop_trivial_z(points).shape, (4, 3))

    def test_local_tag_strips_a_namespace(self):
        self.assertEqual(_local_tag("{http://www.w3.org/XML}Grid"), "Grid")

    def test_decode_xdmf_topology_reads_a_uniform_stream(self):
        # Element code 5 is a quadrilateral with four corners.
        cells = _decode_xdmf_topology(np.asarray([5, 0, 1, 4, 3, 5, 1, 2, 5, 4]))
        np.testing.assert_array_equal(cells, [[0, 1, 4, 3], [1, 2, 5, 4]])

    def test_decode_xdmf_topology_reads_a_mixed_stream(self):
        cells = _decode_xdmf_topology(np.asarray([4, 0, 1, 2, 5, 0, 1, 4, 3]))
        self.assertIsInstance(cells, list)
        np.testing.assert_array_equal(cells[0], [0, 1, 2])
        np.testing.assert_array_equal(cells[1], [0, 1, 4, 3])

    def test_decode_xdmf_topology_rejects_an_unknown_element(self):
        with self.assertRaises(ValueError):
            _decode_xdmf_topology(np.asarray([99, 0, 1]))

    def test_decode_xdmf_topology_handles_an_empty_stream(self):
        self.assertEqual(_decode_xdmf_topology(np.empty(0)).shape, (0, 0))


class MeshSnapshotTest(unittest.TestCase):
    def test_variables_follow_the_field_order(self):
        self.assertEqual(make_snapshot().variables, ["pressure", "velocity"])

    def test_num_cells_counts_an_array_mesh(self):
        self.assertEqual(make_snapshot().num_cells, 2)

    def test_num_cells_counts_a_mixed_mesh(self):
        snapshot = make_snapshot()
        snapshot.cells = [np.asarray([0, 1, 2]), np.asarray([1, 2, 3, 4])]
        self.assertEqual(snapshot.num_cells, 2)

    def test_field_reports_where_a_field_lives(self):
        snapshot = make_snapshot()
        self.assertEqual(snapshot.field("pressure")[1], "node")
        self.assertEqual(snapshot.field("velocity")[1], "cell")

    def test_field_rejects_an_unknown_name(self):
        with self.assertRaises(KeyError):
            make_snapshot().field("absent")

    def test_snapshot_ref_defaults_to_an_empty_payload(self):
        self.assertEqual(SnapshotRef("vtkhdf", "a.vtkhdf", 0.0, "a").payload, {})


class VtkhdfTest(unittest.TestCase):
    def setUp(self):
        self._directory = tempfile.TemporaryDirectory()
        self.directory = Path(self._directory.name)
        self.path = write_vtkhdf(self.directory / "Solver.vtkhdf", [0.0, 0.5])

    def tearDown(self):
        self._directory.cleanup()

    def test_scan_lists_one_reference_per_step(self):
        name, refs = _scan_path(self.path)
        self.assertEqual(name, "Solver")
        self.assertEqual([ref.time for ref in refs], [0.0, 0.5])
        self.assertEqual([ref.payload["step"] for ref in refs], [0, 1])

    def test_load_returns_the_mesh_of_the_requested_step(self):
        _, refs = _scan_path(self.path)
        snapshot = load_snapshot_ref(refs[1])
        self.assertEqual(snapshot.points.shape, (6, 2))
        np.testing.assert_array_equal(snapshot.cells, [[0, 1, 4, 3], [1, 2, 5, 4]])
        self.assertEqual(snapshot.time, 0.5)

    def test_load_slices_the_fields_of_the_requested_step(self):
        _, refs = _scan_path(self.path)
        np.testing.assert_allclose(
            load_snapshot_ref(refs[1]).node_data["pressure"], np.arange(6, 12)
        )
        self.assertEqual(load_snapshot_ref(refs[1]).cell_data["velocity"].shape, (2, 2))

    def test_load_keeps_the_field_order(self):
        _, refs = _scan_path(self.path)
        self.assertEqual(
            load_snapshot_ref(refs[0]).field_order,
            [("pressure", "node"), ("velocity", "cell")],
        )

    def test_scan_path_rejects_an_unsupported_suffix(self):
        with self.assertRaises(ValueError):
            _scan_path(self.directory / "Solver.txt")


class BareHdf5Test(unittest.TestCase):
    def setUp(self):
        self._directory = tempfile.TemporaryDirectory()
        self.directory = Path(self._directory.name)
        self.path = write_bare_hdf5(self.directory / "Solver-0_500000.h5")

    def tearDown(self):
        self._directory.cleanup()

    def test_scan_reads_the_time_from_the_file_name(self):
        _, refs = _scan_path(self.path)
        self.assertEqual(len(refs), 1)
        self.assertAlmostEqual(refs[0].time, 0.5)

    def test_load_assembles_the_mesh_from_its_three_coordinate_arrays(self):
        _, refs = _scan_path(self.path)
        snapshot = load_snapshot_ref(refs[0])
        self.assertEqual(snapshot.points.shape, (6, 2))
        np.testing.assert_array_equal(snapshot.cells, [[0, 1, 4, 3], [1, 2, 5, 4]])
        self.assertEqual(snapshot.node_data["pressure"].shape, (6,))


class WriterFormatsTest(unittest.TestCase):
    def setUp(self):
        self._directory = tempfile.TemporaryDirectory()
        self.directory = Path(self._directory.name)

    def tearDown(self):
        self._directory.cleanup()

    def test_vtu_rank_pieces_keep_their_fields_and_skip_empty_ranks(self):
        import meshio

        empty = self.directory / "Solver-rank-0.vtu"
        empty.write_text(
            '<VTKFile type="UnstructuredGrid" version="1.0" '
            'byte_order="LittleEndian" header_type="UInt64">'
            '<UnstructuredGrid><Piece NumberOfPoints="0" NumberOfCells="0">'
            "<Points></Points><Cells></Cells></Piece></UnstructuredGrid></VTKFile>"
        )
        meshio.write(
            self.directory / "Solver-rank-1.vtu",
            meshio.Mesh(
                points=np.asarray(QUAD_POINTS),
                cells=[("quad", np.asarray(QUAD_CONNECTIVITY).reshape(2, 4))],
                point_data={"pressure": np.arange(6, dtype=float)},
                cell_data={
                    "Subdomain": [np.ones(2)],
                    "RefinementLevel": [np.full(2, 3)],
                },
            ),
        )
        path = self.directory / "Solver.pvtu"
        path.write_text(
            '<VTKFile type="PUnstructuredGrid"><PUnstructuredGrid>'
            '<Piece Source="Solver-rank-0.vtu"/>'
            '<Piece Source="Solver-rank-1.vtu"/>'
            "</PUnstructuredGrid></VTKFile>"
        )
        snapshot = _load_vtu_meshio(SnapshotRef("vtu", str(path), 0.5, "Solver"), path)
        np.testing.assert_array_equal(
            snapshot.cells, np.asarray(QUAD_CONNECTIVITY).reshape(2, 4)
        )
        np.testing.assert_array_equal(snapshot.points, np.asarray(QUAD_POINTS)[:, :2])
        np.testing.assert_array_equal(snapshot.node_data["pressure"], np.arange(6))
        np.testing.assert_array_equal(snapshot.cell_data["Subdomain"], [1, 1])
        self.assertEqual(
            set(snapshot.variables), {"pressure", "Subdomain", "RefinementLevel"}
        )

    def test_xdmf_index_reads_named_node_and_cell_fields(self):
        data = write_bare_hdf5(self.directory / "Solver-0_500000.h5")
        with h5py.File(data, "a") as handle:
            handle.create_group("cell_data").create_dataset("Subdomain", data=[0, 1])
        path = self.directory / "Solver.xmf"
        path.write_text(
            '<Xdmf><Domain><Grid CollectionType="Temporal">'
            '<Grid><Time Value="0.5"/><Topology TopologyType="Mixed">'
            f"<DataItem>{data.name}:/topology</DataItem></Topology>"
            '<Geometry GeometryType="X_Y_Z">'
            + "".join(
                f"<DataItem>{data.name}:/geometry/{axis}</DataItem>" for axis in "xyz"
            )
            + '</Geometry><Attribute Name="pressure" Center="Node">'
            f"<DataItem>{data.name}:/data/pressure</DataItem></Attribute>"
            '<Attribute Name="Subdomain" Center="Cell">'
            f"<DataItem>{data.name}:/cell_data/Subdomain</DataItem></Attribute>"
            "</Grid></Grid></Domain></Xdmf>"
        )
        source = SolutionSource.open(path)
        snapshot = source.load(0)
        self.assertEqual(source.times, [0.5])
        np.testing.assert_array_equal(
            snapshot.cells, np.asarray(QUAD_CONNECTIVITY).reshape(2, 4)
        )
        np.testing.assert_array_equal(snapshot.node_data["pressure"], np.arange(6))
        np.testing.assert_array_equal(snapshot.cell_data["Subdomain"], [0, 1])

    def test_netcdf_series_reads_components_and_ignores_dimension_scales(self):
        from netCDF4 import Dataset

        data = self.directory / "Solver-0_500000.nc"
        with Dataset(data, "w") as dataset:
            dataset.createDimension("nMesh2d_node", 6)
            dataset.createDimension("nMesh2d_face", 2)
            dataset.createDimension("nMax_face_nodes", 4)
            for dimension in ("nMesh2d_node", "nMesh2d_face"):
                dataset.createVariable(dimension, "i4", (dimension,))
            mesh = dataset.createVariable("mesh2d", "i4")
            mesh.cf_role = "mesh_topology"
            mesh.topology_dimension = 2
            mesh.node_coordinates = "x y"
            mesh.face_node_connectivity = "face_nodes"
            for axis, name in enumerate("xy"):
                dataset.createVariable(name, "f8", ("nMesh2d_node",))[:] = np.asarray(
                    QUAD_POINTS
                )[:, axis]
            cells = dataset.createVariable(
                "face_nodes", "i4", ("nMesh2d_face", "nMax_face_nodes"), fill_value=-1
            )
            cells.start_index = 0
            cells[:] = np.asarray(QUAD_CONNECTIVITY).reshape(2, 4)
            rank = dataset.createVariable("Subdomain", "f8", ("nMesh2d_face",))
            rank.long_name = "Subdomain"
            rank[:] = [0, 1]
            scalar = dataset.createVariable("species_0", "f8", ("nMesh2d_face",))
            scalar.long_name = "species_0"
            scalar[:] = [3, 4]
            for component in range(3):
                velocity = dataset.createVariable(
                    f"velocity_{component}", "f8", ("nMesh2d_node",)
                )
                velocity.long_name = f"velocity (component {component})"
                velocity[:] = np.arange(6) + component
            dataset.createVariable("time", "f8")[...] = 0.5
        path = self.directory / "Solver.nc.series"
        path.write_text(
            json.dumps(
                {
                    "file-series-version": "1.0",
                    "files": [{"name": data.name, "time": 0.5}],
                }
            )
        )
        source = SolutionSource.open(path)
        snapshot = source.load(0)
        self.assertEqual(source.times, [0.5])
        self.assertEqual(
            set(snapshot.variables), {"velocity", "species_0", "Subdomain"}
        )
        np.testing.assert_array_equal(snapshot.cell_data["species_0"], [3, 4])
        np.testing.assert_array_equal(
            snapshot.node_data["velocity"], np.arange(6)[:, None] + np.arange(3)
        )
        np.testing.assert_array_equal(
            snapshot.cells, np.asarray(QUAD_CONNECTIVITY).reshape(2, 4)
        )


class SolutionSourceTest(unittest.TestCase):
    def setUp(self):
        self._directory = tempfile.TemporaryDirectory()
        self.directory = Path(self._directory.name)
        self.path = write_vtkhdf(self.directory / "Solver.vtkhdf", [0.0, 0.5])

    def tearDown(self):
        self._directory.cleanup()

    def test_construction_rejects_an_empty_series(self):
        with self.assertRaises(ValueError):
            SolutionSource("Solver", [])

    def test_open_reads_every_step(self):
        source = SolutionSource.open(self.path)
        self.assertEqual(source.count, 2)
        self.assertEqual(source.times, [0.0, 0.5])
        self.assertEqual(source.format, "VTKHDF")

    def test_open_accepts_a_directory(self):
        self.assertEqual(SolutionSource.open(self.directory).count, 2)

    def test_open_rejects_a_missing_file(self):
        with self.assertRaises(FileNotFoundError):
            SolutionSource.open(self.directory / "absent.vtkhdf")

    def test_snapshot_caches_the_loaded_step(self):
        source = SolutionSource.open(self.path)
        self.assertIs(source.snapshot(0), source.snapshot(0))
        self.assertIsNot(source.snapshot(0), source.load(0))

    def test_variables_come_from_the_first_snapshot(self):
        self.assertEqual(
            SolutionSource.open(self.path).variables, ["pressure", "velocity"]
        )

    def test_describe_names_the_source_and_its_time_span(self):
        description = SolutionSource.open(self.path).describe()
        self.assertIn("VTKHDF", description)
        self.assertIn("2 snapshots", description)
        self.assertIn("0 … 0.5", description)

    def test_discover_finds_an_index_by_solver_name(self):
        source = SolutionSource.discover("Solver", self.directory)
        self.assertEqual(source.count, 2)

    def test_discover_reports_a_solver_without_output(self):
        with self.assertRaises(FileNotFoundError):
            SolutionSource.discover("Absent", self.directory)

    def test_discover_collects_per_step_snapshots(self):
        snapshots = self.directory / "steps"
        snapshots.mkdir()
        write_vtkhdf(snapshots / "Wave-0_000000.vtkhdf", [0.0])
        write_vtkhdf(snapshots / "Wave-1_000000.vtkhdf", [1.0])
        source = SolutionSource.discover("Wave", snapshots)
        self.assertEqual(source.count, 2)
        self.assertEqual(source.times, [0.0, 1.0])

    def test_locate_opens_an_existing_path(self):
        self.assertEqual(SolutionSource.locate(self.path).count, 2)

    def test_locate_discovers_a_solver_name(self):
        self.assertEqual(SolutionSource.locate("Solver", self.directory).count, 2)

    def test_locate_rejects_an_absolute_path_that_does_not_exist(self):
        with self.assertRaises(FileNotFoundError):
            SolutionSource.locate(self.directory / "absent.vtkhdf")

    def test_locate_passes_a_source_through(self):
        source = SolutionSource.open(self.path)
        self.assertIs(SolutionSource.locate(source), source)

    def test_index_in_directory_finds_the_single_index(self):
        self.assertEqual(_index_in_directory(self.directory), self.path)

    def test_index_in_directory_prefers_nothing_when_two_solvers_are_present(self):
        write_vtkhdf(self.directory / "Other.vtkhdf", [0.0])
        with self.assertRaises(ValueError):
            _index_in_directory(self.directory)

    def test_index_in_directory_reports_an_empty_directory(self):
        empty = self.directory / "empty"
        empty.mkdir()
        with self.assertRaises(FileNotFoundError):
            _index_in_directory(empty)

    def test_candidate_directories_include_the_plotter_sub_folders(self):
        patch = self.directory / "patch"
        patch.mkdir()
        candidates = _candidate_directories(self.directory)
        self.assertIn(self.directory, candidates)
        self.assertIn(patch, candidates)

    def test_discover_index_looks_in_a_sub_folder(self):
        patch = self.directory / "patch"
        patch.mkdir()
        moved = write_vtkhdf(patch / "Nested.vtkhdf", [0.0])
        self.assertEqual(_discover_index("Nested", self.directory), moved)

    def test_discover_snapshots_returns_them_in_time_order(self):
        steps = self.directory / "steps"
        steps.mkdir()
        write_vtkhdf(steps / "Wave-1_000000.vtkhdf", [1.0])
        write_vtkhdf(steps / "Wave-0_000000.vtkhdf", [0.0])
        found = _discover_snapshots("Wave", steps)
        self.assertEqual(
            [path.name for path in found],
            ["Wave-0_000000.vtkhdf", "Wave-1_000000.vtkhdf"],
        )

    def test_discover_snapshots_returns_nothing_for_an_unknown_solver(self):
        self.assertEqual(_discover_snapshots("Absent", self.directory), [])


if __name__ == "__main__":
    unittest.main()
