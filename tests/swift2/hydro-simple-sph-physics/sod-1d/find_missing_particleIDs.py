#!/usr/bin/env python3

from peano4.toolbox.particles.postprocessing.ParticleVTUReader import ParticleVTUReader

import numpy as np
import sys
import os

if len(sys.argv)<=1:
    print( "Usage: python3 {} vtu-file-name.vtu".format(__file__) )
    print( "       python3 {} vtu-file-name-1.vtu vtu-file-name-2.vtu vtu-file-name-3.vtu ...".format(__file__) )
    print( "       python3 {} *.vtu".format(__file__) )
    exit(1)


for vtufile in sys.argv:
    if vtufile.endswith(".vtu"):
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
