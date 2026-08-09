from toolbox.loadbalancing.api.strategies.Lexicographic import (
    Cartesian_to_lexicographic_2d,
    Cartesian_to_lexicographic_3d,
)


def test_cartesian_to_lexicographic_2d():
    assert Cartesian_to_lexicographic_2d((1, 2), 3) == 7


def test_cartesian_to_lexicographic_3d():
    assert Cartesian_to_lexicographic_3d((1, 2, 0), 3) == 7
