"""

 A simple Python script to visualise the number of lifts, drops, ... over 
 the simulation runtime.

"""


import os, sys
import argparse
import peano4.toolbox.particles.postprocessing

import matplotlib.pyplot as plt


class Dataset:
    def __init__(self):
        self.data = {
          "lifts":                [],
          "drops":                [],
          "lifts-into-sieve-set": [],
          "drops-from-sieve-set": [],
          "skipped-drops-from-sieve-set": [],
          "reassignments": [],
          "remaining": [],
          "expired":   [],
          "leaving":   [],
          "incoming particles (previously known as halos)":         [],
          "incoming particles (previously unseen)":                 [],
          "redundantly shared particles (local on multiple trees)": [], 
          "added virtual particles (flying into halo)":             [], 
          "replaced virtual particles (halo updates)":              [],
          "dropped virtual particles":                              [],
          "eliminated local spatial duplicates":   [],
          "eliminated virtual spatial duplicates": []
        }
        pass

    def _getValueFromLine( self, line, key, verbose ):
        result = 0.0
        try:
            rhs = line.split( key + "=" )[1]
            if "," in rhs:
                rhs = rhs.split(",")[0]
            result = float( rhs )
        except:
            if verbose:
                print( "ERROR in line {} for key {}".format(line,key) )
        return result
    

    def entry_holds_interesting_data(self, key, verbose):
        """!
        
        Return true if entry for key is not empty and not all values are exactly the same
        
        """
        if verbose:
            print( "search for entry for key {}".format(key) )
        if not key in self.data.keys():
            if verbose:
                print( "no key {}".format(key) )
            return False
        if len(self.data[key])==0:
            if verbose:
                print( "no entry for key {}".format(key) )
            return False
        first_data_entry = self.data[key][0]
        for values in self.data[key]:
            if first_data_entry!=values:
                return True
        return False
        
    def parse_line(self, line, verbose):
        if "reduceAndPrintStatistics":
            for key in self.data.keys():
                if key in line:
                  self.data[key].append( self._getValueFromLine(line, key, verbose) )




def parseParticleGridStatistics( filename, verbose ):
    file   = open( filename, "r" )
    result = Dataset()
    for line in file:
        result.parse_line(line, verbose)
# 820169320    00:00:00     rank:0       core:7       info         toolbox::particles::ParticleSet::reduceAndPrintStatistics()   particle state updates: #remaining=0, #expired=0, #leaving=0
# 820172182    00:00:00     rank:0       core:7       info         toolbox::particles::ParticleSet::reduceAndPrintStatistics()   boundary merge statistics: #incoming particles (previously known as halos)=0, #incoming particles (previously unseen)=0, #redundantly shared particles (local on multiple trees)=0, #added virtual particles (flying into halo)=0, #replaced virtual particles (halo updates)=0: #dropped virtual particles=0
# 820174818    00:00:00     rank:0       core:7       info         toolbox::particles::ParticleSet::reduceAndPrintStatistics()   replica deletion statistics: #eliminated local spatial duplicates=0, #eliminated virtual spatial duplicates=0

    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser( description="ExaHyPE 2 - Particle toolbox postprocessing" )
    parser.add_argument("file", help="Input file")
    parser.add_argument("--verbose", "-v", dest="verbose",  action="store_true", help="Verbose output")
    args = parser.parse_args()

    dataset = parseParticleGridStatistics( args.file, args.verbose )

    plt.clf()
    plt.legend()
    plt.xlabel("Mesh sweeps")
    
    for key in dataset.data.keys():
        if dataset.entry_holds_interesting_data(key, args.verbose):
            plt.plot(dataset.data[key], label=key)

    plt.legend()
    plt.savefig(args.file + ".pdf")
    plt.savefig(args.file + ".png")


