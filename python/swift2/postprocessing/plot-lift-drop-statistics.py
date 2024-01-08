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
        self.mesh_association_data = {
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
        self.grid_data = {
          ("local cells", "refined"):  [],
          ("remote cells", "refined"): [],
          ("local cells", "unrefined"):  [],
          ("remote cells", "unrefined"): []
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

    def _getTupleFromLine( self, line, key, verbose ):
        result = (0.0, 0.0)
        try:
            rhs = line.split( key[0] + "=(" )[1].split(")")[0]
            result = (
              float( rhs.split("/")[0] ), 
              float( rhs.split("/")[1] )
            ) 
        except:
            if verbose:
                print( "ERROR for _getTupleFromLine in line {} for key {}".format(line, key) )
        return result
    

    def entry_holds_interesting_data(self, 
                                     key, 
                                     verbose
                                     ):
        """!
        
        Return true if entry for key is not empty and not all values are exactly the same
        
        """
        if verbose:
            print( "search for entry for key {}".format(key) )
        if not key in self.mesh_association_data.keys():
            if verbose:
                print( "no key {}".format(key) )
            return False
        if len(self.mesh_association_data[key])==0:
            if verbose:
                print( "no entry for key {}".format(key) )
            return False
        first_mesh_association_data_entry = self.mesh_association_data[key][0]
        for values in self.mesh_association_data[key]:
            if first_mesh_association_data_entry!=values:
                return True
        return False
        
    def parse_line(self, line, verbose):
        if "reduceAndPrintStatistics":
            for key in self.mesh_association_data.keys():
                if key in line:
                  self.mesh_association_data[key].append( self._getValueFromLine(line, key, verbose) )
                  
        if "TimeStep":
            for key in self.grid_data.keys():
                if key[0] in line:
                    self.grid_data[key].append( self._getTupleFromLine(line, key, verbose) )




def parseParticleGridStatistics( filename, verbose ):
    file   = open( filename, "r" )
    result = Dataset()
    for line in file:
        result.parse_line(line, verbose)
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser( description="ExaHyPE 2 - Particle toolbox postprocessing" )
    parser.add_argument("file", help="Input file")
    parser.add_argument("--verbose", "-v", dest="verbose",  action="store_true", help="Verbose output")
    args = parser.parse_args()

    mesh_association_dataset = parseParticleGridStatistics( args.file, args.verbose )

    plt.clf()
    fig = plt.figure()
    gs = fig.add_gridspec(2, hspace=0)
    axs = gs.subplots(sharex=True, sharey=False)    
    for key in mesh_association_dataset.mesh_association_data.keys():
        if mesh_association_dataset.entry_holds_interesting_data(key, args.verbose):
            axs[0].plot(mesh_association_dataset.mesh_association_data[key], label=key)
    for key in mesh_association_dataset.grid_data.keys():
        try:
            axs[1].plot(mesh_association_dataset.grid_data[key], label="{}/{}".format(key[0],key[1]))
        except:
            print( "ERROR: Cannot find data for {}".format(key) )

    axs[0].legend()
    axs[1].legend()
    plt.xlabel("Mesh sweeps")
    plt.savefig(args.file + ".pdf")
    plt.savefig(args.file + ".png")


