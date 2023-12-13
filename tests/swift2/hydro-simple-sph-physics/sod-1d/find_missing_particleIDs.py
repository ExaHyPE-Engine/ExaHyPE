#!/usr/bin/env python3

# Find missing particles by ID.
# Usage: python3 find_missing_particleIDs.py <vtufile>

from peano4.toolbox.particles.postprocessing.ParticleVTUReader import ParticleVTUReader
import numpy as np
import sys
import os

vtufile = sys.argv[1]
if not os.path.exists(vtufile):
    raise FileNotFoundError("no file {}".format(vtufile))

reader = ParticleVTUReader(vtufile=vtufile, verbose=False)
partData = reader.load()
ids = partData.partid
ids = ids.astype(int)
ids = np.sort(ids)

print("First particle ID is", ids[0])

for i in range(ids.shape[0] - 1):
    n = ids[i + 1]
    this = ids[i]
    while n - this != 1:
        this += 1
        print("Missing particle:", this)
