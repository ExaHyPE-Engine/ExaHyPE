# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
class SimTimeStamp:
    def __init__(
        self,
        nanosecond: int,
        local_unrefined_cells_list,
        remote_unrefined_cells_list,
        local_unrefined_cells_total,
        remote_unrefined_cells_total,
    ):
        self.nanosecond = nanosecond
        self.seconds = self.nanosecond / 1000 / 1000 / 1000
        self.local_unrefined_cells_list = local_unrefined_cells_list
        self.remote_unrefined_cells_list = remote_unrefined_cells_list
        self.local_unrefined_cells_total = local_unrefined_cells_total
        self.remote_unrefined_cells_total = remote_unrefined_cells_total

    def __str__(self):
        return str(vars(self))

    # def getPlottableLocalCells(self):
    #    return [(self.seconds,x)  for x in self.local_unrefined_cells_list]

    # def getPlottableRemoteCells(self):
    #    return [(self.seconds,x) for x in self.remote_unrefined_cells_list]

    def getPlottableLocalTotalCells(self):
        return (self.seconds, self.local_unrefined_cells_total)

    def getPlottableRemoteTotalCells(self):
        return (self.seconds, self.remote_unrefined_cells_total)

    def avg_of_local_cells_list(self):
        return sum(self.local_unrefined_cells_list) / len(
            self.local_unrefined_cells_list
        )

    def avg_of_remote_cells_list(self):
        return sum(self.remote_unrefined_cells_list) / len(
            self.remote_unrefined_cells_list
        )

    def maxDeviationOfLocalAvgCells(self):
        """
        returns (AbsoluteDeviation,RelativeDeviation)
        """
        maximum = max(self.local_unrefined_cells_list)
        avg = self.avg_of_local_cells_list()
        return (maximum - avg, maximum / avg)

    # Absolute Deviation loses information of average of current cell --> Leads incorrect numbers.
