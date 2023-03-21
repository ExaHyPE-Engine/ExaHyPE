
# Readme of a Peano 4 project

This experiment did rely on Peano 4 or a toolbox created on top of Peano.   
If you publish results obtained with this source code, we would appreciate  
if you could point to the project homepage                                  
                                                                            
http://www.peano-framework.org/                                             
                                                                            
and cite the appropriate papers from below.


## Peano 4 ##                                  
                                                                            
Peano's core ideas, algorithms and (usage) vision are best discussed in the 
TOMS paper below. The TOMS paper describes the third generation of Peano. 
While it is thus not a perfect description, the core concepts by means of 
data structures and user model is the same as in Peano 4. Please cite this
paper when you use the software:


        @article{Weinzierl:2019:Peano,                    
          author = {Weinzierl, Tobias},                   
          title = {The Peano Software-Parallel, Automaton-Based, Dynamically Adaptive Grid Traversals},                    
          year = {2019},                    
          issue_date = {June 2019},                    
          publisher = {Association for Computing Machinery},                    
          address = {New York, NY, USA},                    
          volume = {45},                    
          number = {2},                    
          issn = {0098-3500},                    
          url = {https://doi.org/10.1145/3319797},                    
          doi = {10.1145/3319797},                    
          abstract = {We discuss the design decisions, design alternatives, and rationale behind the third generation of Peano, a framework for dynamically adaptive Cartesian meshes derived from spacetrees. Peano ties the mesh traversal to the mesh storage and supports only one element-wise traversal order resulting from space-filling curves. The user is not free to choose a traversal order herself. The traversal can exploit regular grid subregions and shared memory as well as distributed memory systems with almost no modifications to a serial application code. We formalize the software design by means of two interacting automata-one automaton for the multiscale grid traversal and one for the application-specific algorithmic steps. This yields a callback-based programming paradigm. We further sketch the supported application types and the two data storage schemes realized before we detail high-performance computing aspects and lessons learned. Special emphasis is put on observations regarding the used programming idioms and algorithmic concepts. This transforms our report from a "one way to implement things" code description into a generic discussion and summary of some alternatives, rationale, and design decisions to be made for any tree-based adaptive mesh refinement software.},
          journal = {ACM Trans. Math. Softw.},                              month = apr,                    
          articleno = {14},                    
          numpages = {41},                    
          keywords = {Software, spacetree, adaptive mesh refinement, parallel multiscale grid traversal
        }


There is an earlier publication about Peano in SISC which discusses the core 
algorithms with the stacks in great detail. This paper can be seen as release 
paper for previous Peano code generations (generation 2).


        @article{10.1137/100799071,
          author = {Weinzierl, Tobias and Mehl, Miriam},
          title = {Peano-A Traversal and Storage Scheme for Octree-Like Adaptive Cartesian Multiscale Grids}, 
          year = {2011}, 
          issue_date = {September 2011}, 
          publisher = {Society for Industrial and Applied Mathematics}, 
          address = {USA}, 
          volume = {33}, 
          number = {5}, 
          issn = {1064-8275}, 
          url = {https://doi.org/10.1137/100799071}, 
          doi = {10.1137/100799071}, 
          abstract = {Almost all approaches to solving partial differential equations (PDEs) are based upon a spatial discretization of the computational domain - a grid. This paper presents an algorithm to generate, store, and traverse a hierarchy of $d$-dimensional Cartesian grids represented by a $(k=3)$-spacetree, a generalization of the well-known octree concept, and it also shows the correctness of the approach. These grids may change their adaptive structure throughout the traversal. The algorithm uses $2d+4$ stacks as data structures for both cells and vertices, and the storage requirements for the pure grid reduce to one bit per vertex for both the complete grid connectivity structure and the multilevel grid relations. Since the traversal algorithm uses only stacks, the algorithm's cache hit rate is continually higher than 99.9 percent, and the runtime per vertex remains almost constant; i.e., it does not depend on the overall number of vertices or the adaptivity pattern. We use the algorithmic approach as the fundamental concept for a mesh management for $d$-dimensional PDEs and for a matrix-free PDE solver represented by a compact discrete $3^d$-point operator. In the latter case, one can implement a Jacobi smoother, a Krylov solver, or a geometric multigrid scheme within the presented traversal scheme which inherits the low memory requirements and the good memory access characteristics directly.}, 
          journal = {SIAM J. Sci. Comput.}, 
          month = oct, 
          pages = {2732--2760}, 
          numpages = {29}, 
          keywords = {octree, cache efficiency, multiscale, space-filling curve, adaptive Cartesian grid, spacetree, partial differential equation} 
        } 

                                                                            
 
    
## Information provided by used packages    
    



### MPI parallelisation

Peano's distributed memory parallelisation is based upon plain space-filling 
curves which are used first to split the cells among the ranks, and, after   
that, to split the cells into chunks and deploy them onto cores (if shared   
memory is used). There is no particularly novel contribution/paper on this   
aspect of the code. The paper that is (algorithmically) most interesting is  
an old EuroPar publication:                                                 


       @InProceedings{Bungartz:2006:Parallelisation,
         author={Bungartz, Hans-Joachim and Mehl, Miriam and Weinzierl, Tobias}, 
         editor={Nagel, Wolfgang E. and Walter, Wolfgang V. and Lehner, Wolfgang}, 
         title={A Parallel Adaptive Cartesian PDE Solver Using Space--Filling Curves},  
         booktitle={Euro-Par 2006 Parallel Processing}, 
         year={2006}, 
         publisher={Springer Berlin Heidelberg}, 
         address={Berlin, Heidelberg}, 
         pages={1064--1074}, 
         abstract={In this paper, we present a parallel multigrid PDE solver working on adaptive hierarchical cartesian grids. The presentation is restricted to the linear elliptic operator of second order, but extensions are possible and have already been realised as prototypes. Within the solver the handling of the vertices and the degrees of freedom associated to them is implemented solely using stacks and iterates of a Peano space--filling curve. Thus, due to the structuredness of the grid, two administrative bits per vertex are sufficient to store both geometry and grid refinement information. The implementation and parallel extension, using a space--filling curve to obtain a load balanced domain decomposition, will be formalised. In view of the fact that we are using a multigrid solver of linear complexity {\$}{\backslash}mathcal{\{}O{\}}(n){\$}, it has to be ensured that communication cost and, hence, the parallel algorithm's overall complexity do not exceed this linear behaviour.}, 
         isbn={978-3-540-37784-9} 
       } 




### OpenMP parallelisation

Peano 4 run uses a wrapper around OpenMP to obtain a high task efficiency. 
The wrapper can be read as user-level threading implemented on top of OpenMP's 
tasking mechanism. It is described in
                                                                            
       @article{Schulz:2021:Tasking,   
         title = {Task inefficiency patterns for a wave equation solver},   
         journal = {IWOMP},   "
         year = {2021},   
         author = {Holger Schulz and Gonzalo Brito Gadeschi and Oleksandr Rudyy and Tobias Weinzierl},
      } 






Peano relies on a mixture of classic domain decomposition and task-based  
parallelism. The domain decomposition provides the baseline performance,  
and the tasking adds the big flexibility and scalability gain on top. The 
key publication discussing the overall idea and algorithmic ingredients is
the SISC paper
                                                                            
       @article{Charrier:2020:Enclave,
         author = {Charrier, Dominic Etienne and Hazelwood, Benjamin and Weinzierl, Tobias},
         title = {Enclave Tasking for DG Methods on Dynamically Adaptive Meshes},
         journal = {SIAM Journal on Scientific Computing},
         volume = {42},
         number = {3},
         pages = {C69-C96},
         year = {2020},
         doi = {10.1137/19M1276194},
         URL = {https://doi.org/10.1137/19M1276194},
         eprint = {https://doi.org/10.1137/19M1276194}
       }




### SWIFT 2 

This code uses the second generation of the SWIFT code controlled through 
its Python API. Under the hood it uses Peano 4. We do not yet have a release 
paper for this second generation of SWIFT yet, and  thus appreciate any 
citation of the original SWIFT paper
                                                                            
       @article{Schaller:2020:SWIFT,   
         title = {t.b.d.}   
       } 



### Data structure modelling

Peano models its data structures via a tool called DaStGen. The actual DaStGen 
version in use is the second generation of DaStGen which is integrated into 
LLVM, e.g. The first generation of DaStGen has been a stand-alone Java tool
which serves as a precompiler. As DaStGen 2 is not published yet, we appreciate 
a citation of the two original DaStGen papers when you discuss the memory needs: 


        @InProceedings{Bungartz:2008:DaStGen,
          author={Bungartz, Hans--Joachim and Eckhardt, Wolfgang and Mehl, Miriam and Weinzierl, Tobias}, "
          editor={Bubak, Marian and van Albada, Geert Dick and Dongarra, Jack and Sloot, Peter M. A.}, 
          title={DaStGen---A Data Structure Generator for Parallel C++ HPC Software}, 
          booktitle={Computational Science -- ICCS 2008}, 
          year=2008,
          publisher={Springer Berlin Heidelberg}, 
          address={Berlin, Heidelberg}, "
          pages={213--222}, 
          isbn={978-3-540-69389-5} 
        }


        @article{Bungartz:2010:DaStGen, 
          title = {A precompiler to reduce the memory footprint of multiscale PDE solvers in C++}, 
          journal = {Future Generation Computer Systems},"
          volume = {26}, 
          number = {1}, 
          pages = {175-182}, 
          year = {2010}, 
          issn = {0167-739X}, 
          doi = {https://doi.org/10.1016/j.future.2009.05.011},
          url = {https://www.sciencedirect.com/science/article/pii/S0167739X09000673},
          author = {H.-J. Bungartz and W. Eckhardt and T. Weinzierl and C. Zenger}, 
          keywords = {Data compaction and compression, Code generation, Multigrid and multilevel methods}, 
          abstract = {A PDE solver's value is increasingly co-determined by its memory footprint, as the increase of computational multicore power overtakes the memory access speed, and as memory restricts the maximum experiment size. Tailoring a code to require less memory is technical challenging, error-prone, and hardware-dependent. Object-oriented code typically consumes much memory, though developers favour such high-level languages offering meaningful models and good maintainability. We augment the language C++ with new keywords branding records to be memory-critical. Our precompiler DaStGen then transforms this augmented specification into plain C++ optimised for low memory requirements. Hereby, it encodes multiple attributes with fixed range within one variable, and it reduces the number of bits per floating point value. The tool also generates one user-defined MPI data type per class and, thus, facilitates the construction of parallel codes with small messages.} 
        } 


### Particle handling 
                                                                            
The particle handling is based upon the idea of particles in dual trees (PIDT)
as published in the paper below:                                              
                                                                            
       @article{Weinzierl:2016:PIDT, 
         title = {Two particle-in-grid realisations on spacetrees}, 
         journal = {Parallel Computing}, 
         volume = {52}, 
         pages = {42-64}, 
         year = {2016}, 
         issn = {0167-8191}, 
         doi = {https://doi.org/10.1016/j.parco.2015.12.007}, 
         url = {https://www.sciencedirect.com/science/article/pii/S0167819115001635}, 
         author = {T. Weinzierl and B. Verleye and P. Henri and D. Roose}, 
         keywords = {Particle-in-cell, Spacetree, Particle sorting, AMR, Lagrangian-Eulerian methods, Communication-avoiding}, 
         abstract = {The present paper studies two particle management strategies for dynamically adaptive Cartesian grids at hands of a particle-in-cell code. One holds the particles within the grid cells, the other within the grid vertices. The fundamental challenge for the algorithmic strategies results from the fact that particles may run through the grid without velocity constraints. To facilitate this, we rely on multiscale grid representations. They allow us to lift and drop particles between different spatial resolutions. We call this cell-based strategy particle in tree (PIT). Our second approach assigns particles to vertices describing a dual grid (PIDT) and augments the lifts and drops with multiscale linked cells. Our experiments validate the two schemes at hands of an electrostatic particle-in-cell code by retrieving the dispersion relation of Langmuir waves in a thermal plasma. They reveal that different particle and grid characteristics favour different realisations. The possibility that particles can tunnel through an arbitrary number of grid cells implies that most data is exchanged between neighbouring ranks, while very few data is transferred non-locally. This constraints the scalability as the code potentially has to realise global communication. We show that the merger of an analysed tree grammar with PIDT allows us to predict particle movements among several levels and to skip parts of this global communication a priori. It is capable to outperform several established implementations based upon trees and/or space-filling curves.}
       }

 
    
## Information provided by data model
    


### Particle hydroPart

There's no additional information available about this particle
data structure.



## Makefile settings

CXX             = mpiicpc
CXXFLAGS        = -fsycl -fma -fomit-frame-pointer -Wno-unknown-attributes -Ofast -march=native -mtune=native --std=c++20 -fiopenmp -fopenmp-targets=spir64 -I/home/tobias/git/otter/include -I/opt/otf2/include -g -I/opt/intel/oneapi/itac/2021.8.0/include -DHYDRO_DIMENSION=2 -DHYDRO_GAMMA_5_3 -DQUARTIC_SPLINE_KERNEL
FC              = gfortran
FCFLAGS         = 
LDFLAGS         = -Ofast -g -fiopenmp -fopenmp-targets=spir64 -L/home/tobias/git/otter/build/lib -L/opt/otf2/lib -fsycl -L/opt/intel/oneapi/itac/2021.8.0/lib -fopenmp -fopenmp-targets=spir64 
GPUOBJS         = 
LIBS            = -ltbb -lVT -ldwarf -lelf -lvtunwind -lm -lpthread -lz -lotter-serial -lotf2
DIM             = 2
CONFIGUREPATH   = ../../../..
EXECUTABLENAME  = noh2D
FORTRAN_MODULES = []
MODE            = RELEASE


## Compiler version

Intel(R) oneAPI DPC++/C++ Compiler 2023.0.0 (2023.0.0.20221201)
Target: x86_64-unknown-linux-gnu
Thread model: posix
InstalledDir: /opt/intel/oneapi/compiler/2023.0.0/linux/bin-llvm
Configuration file: /opt/intel/oneapi/compiler/2023.0.0/linux/bin-llvm/../bin/icpx.cfg



## Summary

File has been generated by    

python3  noh.py -np 200 -et 0.02 -dt 1e-3 -cfl 0.2 -plot 0.0 -cs 0.1

www.peano-framework.org
(C) 2021-  Tobias Weinzierl    
    
