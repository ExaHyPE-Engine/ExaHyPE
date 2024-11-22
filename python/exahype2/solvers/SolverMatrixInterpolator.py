import numpy as np
import scipy.sparse as sp
import os

from exahype2.solvers.LagrangeBasis import render_tensor_1

def lineariseIndex(index, max):
    result = 0
    weight = 1
    for d in range(3):
        result += weight * index[d]
        weight *= max
    return result 

class SolverMatrixInterpolator:
    def __init__(self, source_patch_size, destination_patch_size, source_halo_size, destination_halo_size):
        self.source_patch_size = source_patch_size
        self.destination_patch_size = destination_patch_size
        self.source_halo_size = source_halo_size
        self.destination_halo_size = destination_halo_size
    
    def generateData(self, directory_path = "./"):
        self.filepath = directory_path + "matrixdata/Interpolator_" + str(self.source_patch_size) + "_" + str(self.destination_patch_size) + ".h"
        if not os.path.isfile(self.filepath):
            self.generateMatrix()
            self.writeMatrixToFile(directory_path)

    def generateMatrix(self):
        source_dimensions = np.full((3), self.source_patch_size + 2 * self.source_halo_size)
        destination_dimensions = np.full((3), self.destination_patch_size + 2 * self.destination_halo_size)
        matrix = np.zeros((np.prod(destination_dimensions), np.prod(source_dimensions)))

        for i in range(destination_dimensions[0]):
            for j in range(destination_dimensions[1]):
                for k in range(destination_dimensions[2]):
                    destinationVolume = np.array([i, j, k])
                    linearised_destination = lineariseIndex(destinationVolume, self.destination_patch_size + 2 * self.destination_halo_size)

                    corner_source_volume = (np.floor((destinationVolume - np.full((3), self.destination_halo_size)) * self.source_patch_size / self.destination_patch_size + np.full((3), self.source_halo_size)) - np.full((3), 1)).astype(int)
                    np.maximum(corner_source_volume, 0, corner_source_volume)

                    for l in range(corner_source_volume[0], corner_source_volume[0] + 4):     
                        for n in range(corner_source_volume[1], corner_source_volume[1] + 4):  
                            for m in range(corner_source_volume[2], corner_source_volume[2] + 4):     
                                source_volume = np.array([l, n, m])
                                if (np.max(source_volume) >= self.source_patch_size + 2 * self.source_halo_size):
                                    break

                                axisWithSourceOutsidePatch = 0
                                for d in range(3):
                                    if source_volume[d] < self.source_halo_size:
                                        axisWithSourceOutsidePatch += 1
                                    if source_volume[d] >= self.source_patch_size + self.source_halo_size:
                                        axisWithSourceOutsidePatch += 1
                                isFromUnitialisedDiagonal = (axisWithSourceOutsidePatch > 1)

                                normalised_source_position = (source_volume + np.full((3), - self.source_halo_size)) / self.source_patch_size
                                normalised_destination_position = (destinationVolume + np.full((3), - self.destination_halo_size)) / self.destination_patch_size

                                weight = 1.0
                                for d in range(3):
                                    distance = np.abs(normalised_destination_position[d] - normalised_source_position[d])
                                    distance = 1.0 - distance * self.source_patch_size
                                    weight *= np.maximum(distance, 0.0)
                                
                                if isFromUnitialisedDiagonal:
                                    for d in range(3):
                                        source_volume[d] = np.maximum(source_volume[d], self.source_halo_size)
                                        source_volume[d] = np.minimum(source_volume[d], self.source_halo_size + self.source_patch_size - 1)
                                
                                linearised_source = lineariseIndex(source_volume, self.source_patch_size + 2 * self.source_halo_size)
                                matrix[linearised_destination, linearised_source] += weight
        self.csr_matrix = sp.csr_matrix(matrix)

    def writeMatrixToFile(self, directory_path):
        contents = """#pragma once
class InterpolationMatrixData{
public:
"""
        contents += """  static constexpr double Data[] = {};\n""".format(render_tensor_1(tensor=self.csr_matrix.data))
        contents += """  static constexpr int Indices[] = {};\n""".format(render_tensor_1(tensor=self.csr_matrix.indices))
        contents += """  static constexpr int Indptr[] = {};\n""".format(render_tensor_1(tensor=self.csr_matrix.indptr))
        contents += "};"

        if not os.path.isdir(directory_path + "matrixdata/"):
            os.makedirs(directory_path + "matrixdata/")
        file = open(self.filepath, 'w')
        file.write(contents)
        file.close()