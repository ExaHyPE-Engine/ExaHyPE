import os
import sys

# Ensure the loadbalancing API is importable when running pytest from repo root
sys.path.insert(0, os.path.abspath("src/toolbox/loadbalancing/api"))

from strategies.Lexicographic import (
    Cartesian_to_lexicographic_2d,
    Cartesian_to_lexicographic_3d,
)


def test_cartesian_to_lexicographic_2d():
    assert Cartesian_to_lexicographic_2d((1, 2), 3) == 7


def test_cartesian_to_lexicographic_3d():
    assert Cartesian_to_lexicographic_3d((1, 2, 0), 3) == 7
