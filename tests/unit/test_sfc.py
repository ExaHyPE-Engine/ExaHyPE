import os
import sys
import pytest

# Ensure the loadbalancing API is importable when running pytest from repo root
sys.path.insert(0, os.path.abspath("src/toolbox/loadbalancing/api"))

from Strategies.SFC import (
    Cartesian_to_Peano_2d,
    Peano_to_Cartesian_2d,
    Cartesian_to_Peano_3d,
    Peano_to_Cartesian_3d,
)


# ---------------------------------------------------------------------------
# 2D: docstring example
# ---------------------------------------------------------------------------

def test_cartesian_to_peano_2d_docstring_example():
    """Docstring example: (4, 2) with size=9 must map to 10."""
    assert Cartesian_to_Peano_2d((4, 2), 9) == 10


def test_peano_to_cartesian_2d_docstring_example():
    """Inverse of docstring example: index 10 with size=9 must map to (4, 2)."""
    assert Peano_to_Cartesian_2d(10, 9) == (4, 2)


# ---------------------------------------------------------------------------
# 2D: base case (size=1)
# ---------------------------------------------------------------------------

def test_cartesian_to_peano_2d_size1():
    assert Cartesian_to_Peano_2d((0, 0), 1) == 0


def test_peano_to_cartesian_2d_size1():
    assert Peano_to_Cartesian_2d(0, 1) == (0, 0)


# ---------------------------------------------------------------------------
# 2D: bijection and round-trip for size=3 and size=9
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("size", [3, 9])
def test_cartesian_to_peano_2d_bijection(size):
    """Every cell maps to a unique index in [0, size²)."""
    seen = set()
    for x in range(size):
        for y in range(size):
            idx = Cartesian_to_Peano_2d((x, y), size)
            assert 0 <= idx < size * size, f"Index {idx} out of range for ({x},{y})"
            assert idx not in seen, f"Duplicate index {idx} for ({x},{y})"
            seen.add(idx)
    assert len(seen) == size * size


@pytest.mark.parametrize("size", [3, 9])
def test_peano_to_cartesian_2d_round_trip(size):
    """Forward then inverse returns the original coordinates for all cells."""
    for x in range(size):
        for y in range(size):
            idx = Cartesian_to_Peano_2d((x, y), size)
            assert Peano_to_Cartesian_2d(idx, size) == (x, y), (
                f"Round-trip failed for ({x},{y}): got {Peano_to_Cartesian_2d(idx, size)}"
            )


# ---------------------------------------------------------------------------
# 3D: base case (size=1)
# ---------------------------------------------------------------------------

def test_cartesian_to_peano_3d_size1():
    assert Cartesian_to_Peano_3d((0, 0, 0), 1) == 0


def test_peano_to_cartesian_3d_size1():
    assert Peano_to_Cartesian_3d(0, 1) == (0, 0, 0)


# ---------------------------------------------------------------------------
# 3D: bijection and round-trip for size=3 (and size=9 marked slow)
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("size", [3])
def test_cartesian_to_peano_3d_bijection(size):
    """Every 3-D cell maps to a unique index in [0, size³)."""
    seen = set()
    for x in range(size):
        for y in range(size):
            for z in range(size):
                idx = Cartesian_to_Peano_3d((x, y, z), size)
                assert 0 <= idx < size ** 3, f"Index {idx} out of range for ({x},{y},{z})"
                assert idx not in seen, f"Duplicate index {idx} for ({x},{y},{z})"
                seen.add(idx)
    assert len(seen) == size ** 3


@pytest.mark.parametrize("size", [3])
def test_peano_to_cartesian_3d_round_trip(size):
    """Forward then inverse returns the original coordinates for all 3-D cells."""
    for x in range(size):
        for y in range(size):
            for z in range(size):
                idx = Cartesian_to_Peano_3d((x, y, z), size)
                assert Peano_to_Cartesian_3d(idx, size) == (x, y, z), (
                    f"Round-trip failed for ({x},{y},{z}): "
                    f"got {Peano_to_Cartesian_3d(idx, size)}"
                )


@pytest.mark.slow
def test_cartesian_to_peano_3d_bijection_size9():
    """Full bijection check for size=9 (729 cells); marked slow."""
    size = 9
    seen = set()
    for x in range(size):
        for y in range(size):
            for z in range(size):
                idx = Cartesian_to_Peano_3d((x, y, z), size)
                assert 0 <= idx < size ** 3
                assert idx not in seen, f"Duplicate index {idx} for ({x},{y},{z})"
                seen.add(idx)
    assert len(seen) == size ** 3


@pytest.mark.slow
def test_peano_to_cartesian_3d_round_trip_size9():
    """Round-trip check for size=9 (729 cells); marked slow."""
    size = 9
    for x in range(size):
        for y in range(size):
            for z in range(size):
                idx = Cartesian_to_Peano_3d((x, y, z), size)
                assert Peano_to_Cartesian_3d(idx, size) == (x, y, z)
