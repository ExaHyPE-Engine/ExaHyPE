# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


def number_of_stages(order):
    if order < 1:
        raise ValueError("The Runge-Kutta order is at least one.")
    if order > 4:
        raise ValueError("The Runge-Kutta order is at most four.")
    return order


class ButcherTableau:
    def __init__(self, order):
        self._order = number_of_stages(order)

    def time_step_sizes(self):
        """Returns the left column of the tableau: The fraction of the step every stage advances
        to."""
        return {
            1: [0],
            2: [0, 1 / 2],
            3: [0, 1 / 2, 1],
            4: [0, 1 / 3, 2 / 3, 1],
        }[self._order]

    def final_estimate_weights(self):
        """Returns the bottom row of the tableau: The weight of every stage in the final
        estimate."""
        return {
            1: [1],
            2: [0, 1],
            3: [1 / 6, 2 / 3, 1 / 6],
            4: [1 / 8, 3 / 8, 3 / 8, 1 / 8],
        }[self._order]

    def weight_matrix(self):
        """Returns the matrix of the tableau: The weight of every completed stage in every
        stage."""
        return {
            1: [[0]],
            2: [[0, 0], [0.5, 0]],
            3: [
                [0, 0, 0],
                [0.5, 0, 0],
                [-1, 2, 0],
            ],
            4: [
                [0, 0, 0, 0],
                [1 / 3, 0, 0, 0],
                [-1 / 3, 1, 0, 0],
                [1, -1, 1, 0],
            ],
        }[self._order]
