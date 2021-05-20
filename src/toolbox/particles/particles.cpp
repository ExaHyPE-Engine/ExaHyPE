#include "particles.h"
#include "peano4/peano.h"


void toolbox::particles::addBibTeXEntry() {
  static bool added = false;
  if (not added) {
    added = true;
    peano4::addBibTeXEntry(
"## Particle handling ##                                  \n"
"                                                                            \n"
"The particle handling is based upon the idea of particles in dual trees (PIDT)"
"as published in the paper below:                                              "
"                                                                            \n"
"       @article{Weinzierl:2016:PIDT, \n"
"         title = {Two particle-in-grid realisations on spacetrees}, \n"
"         journal = {Parallel Computing}, \n"
"         volume = {52}, \n"
"         pages = {42-64}, \n"
"         year = {2016}, \n"
"         issn = {0167-8191}, \n"
"         doi = {https://doi.org/10.1016/j.parco.2015.12.007}, \n"
"         url = {https://www.sciencedirect.com/science/article/pii/S0167819115001635}, \n"
"         author = {T. Weinzierl and B. Verleye and P. Henri and D. Roose}, \n"
"         keywords = {Particle-in-cell, Spacetree, Particle sorting, AMR, Lagrangian–Eulerian methods, Communication-avoiding}, \n"
"         abstract = {The present paper studies two particle management strategies for dynamically adaptive Cartesian grids at hands of a particle-in-cell code. One holds the particles within the grid cells, the other within the grid vertices. The fundamental challenge for the algorithmic strategies results from the fact that particles may run through the grid without velocity constraints. To facilitate this, we rely on multiscale grid representations. They allow us to lift and drop particles between different spatial resolutions. We call this cell-based strategy particle in tree (PIT). Our second approach assigns particles to vertices describing a dual grid (PIDT) and augments the lifts and drops with multiscale linked cells. Our experiments validate the two schemes at hands of an electrostatic particle-in-cell code by retrieving the dispersion relation of Langmuir waves in a thermal plasma. They reveal that different particle and grid characteristics favour different realisations. The possibility that particles can tunnel through an arbitrary number of grid cells implies that most data is exchanged between neighbouring ranks, while very few data is transferred non-locally. This constraints the scalability as the code potentially has to realise global communication. We show that the merger of an analysed tree grammar with PIDT allows us to predict particle movements among several levels and to skip parts of this global communication a priori. It is capable to outperform several established implementations based upon trees and/or space-filling curves.} \n"
"       } \n"
"\n\n\n\n" );
  }
}
