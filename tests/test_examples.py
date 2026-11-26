# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os
import shlex
import subprocess
import sys

from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[1]
LAUNCHER = shlex.split(os.environ.get("PARALLEL_LAUNCH", ""))

SCENARIOS = """
Group             | Folder                      | Generator                                          | Executable     | Peano arguments              | Hilbert arguments            | Error check
FV                | examples/fv                 | breaking-dam.py --dimensions 2                     | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | breaking-dam.py --dimensions 3                     | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | channel-flow.py --dimensions 2                     | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | channel-flow.py --dimensions 3                     | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | gaussian-bell.py --dimensions 2                    | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 | GaussianBell.output/Error.csv 8e-3
FV                | examples/fv                 | gaussian-bell.py --dimensions 3                    | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 | GaussianBell.output/Error.csv 8e-3
FV                | examples/fv                 | gaussian-explosion.py --dimensions 2               | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | gaussian-explosion.py --dimensions 3               | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | point-explosion.py --dimensions 2                  | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | point-explosion.py --dimensions 3                  | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | smooth-convergence.py --dimensions 2               | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | smooth-convergence.py --dimensions 3               | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | taylor-green-vortex.py --dimensions 2              | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | taylor-green-vortex.py --dimensions 3              | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | dry-ring.py --dimensions 2                         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV                | examples/fv                 | isothermal-stratification.py --dimensions 2        | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Tracers        | examples/fv-tracers         | fv-tracers-advection.py --dimensions 2             | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Tracers        | examples/fv-tracers         | fv-tracers-periodic-advection.py --dimensions 2    | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Tracers        | examples/fv-tracers         | fv-tracers-circular-bubble.py --dimensions 2       | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Tracers        | examples/fv-tracers         | fv-tracers-drag-force-validation.py --dimensions 2 | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Tracers        | examples/fv-tracers         | fv-tracers-vertical-plume-rise.py --dimensions 2   | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Tracers        | examples/fv-tracers         | fv-tracers-one-body-problem.py --dimensions 2      | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Tracers        | examples/fv-tracers         | fv-tracers-radial-jet.py --dimensions 2            | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Tracers        | examples/fv-tracers         | fv-tracers-volcanic-plume-rise.py --dimensions 2   | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Static AMR     | examples/fv-static-amr      | fv-static-amr.py --periodic-bc on --dimensions 2   | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Static AMR     | examples/fv-static-amr      | fv-static-amr.py --periodic-bc on --dimensions 3   | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Static AMR     | examples/fv-static-amr      | fv-static-amr.py --periodic-bc off --dimensions 2  | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
FV Static AMR     | examples/fv-static-amr      | fv-static-amr.py --periodic-bc off --dimensions 3  | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/fv-dynamic-amr     | acoustic-point-explosion.py --dimensions 2         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/fv-dynamic-amr     | acoustic-point-explosion.py --dimensions 3         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/fv-dynamic-amr     | freestream.py --dimensions 2                       | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 | FreeStream.output/Error.csv 1e-10
Dynamic AMR       | examples/fv-dynamic-amr     | freestream.py --dimensions 3                       | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 | FreeStream.output/Error.csv 1e-10
Dynamic AMR       | examples/fv-dynamic-amr     | periodic-advection.py --dimensions 2               | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/fv-dynamic-amr     | periodic-advection.py --dimensions 3               | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/fv-dynamic-amr     | radial-advection.py --dimensions 2                 | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/fv-dynamic-amr     | radial-advection.py --dimensions 3                 | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/fv-dynamic-amr     | euler-point-explosion.py --dimensions 2            | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/fv-dynamic-amr     | euler-point-explosion.py --dimensions 3            | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/aderdg-dynamic-amr | acoustic-point-explosion.py --dimensions 2         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/aderdg-dynamic-amr | acoustic-point-explosion.py --dimensions 3         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/aderdg-dynamic-amr | freestream.py --dimensions 2                       | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 | FreeStream.output/Error.csv 1e-10
Dynamic AMR       | examples/aderdg-dynamic-amr | freestream.py --dimensions 3                       | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 | FreeStream.output/Error.csv 1e-10
Dynamic AMR       | examples/aderdg-dynamic-amr | periodic-advection.py --dimensions 2               | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/aderdg-dynamic-amr | periodic-advection.py --dimensions 3               | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/aderdg-dynamic-amr | radial-advection.py --dimensions 2                 | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/aderdg-dynamic-amr | radial-advection.py --dimensions 3                 | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/rkdg-dynamic-amr   | acoustic-point-explosion.py --dimensions 2         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/rkdg-dynamic-amr   | acoustic-point-explosion.py --dimensions 3         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/rkdg-dynamic-amr   | freestream.py --dimensions 2                       | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 | FreeStream.output/Error.csv 1e-10
Dynamic AMR       | examples/rkdg-dynamic-amr   | freestream.py --dimensions 3                       | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 | FreeStream.output/Error.csv 1e-10
Dynamic AMR       | examples/rkdg-dynamic-amr   | periodic-advection.py --dimensions 2               | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/rkdg-dynamic-amr   | periodic-advection.py --dimensions 3               | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/rkdg-dynamic-amr   | radial-advection.py --dimensions 2                 | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
Dynamic AMR       | examples/rkdg-dynamic-amr   | radial-advection.py --dimensions 3                 | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s AdvectionLinear -pr float             | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s AdvectionLinear -pr double            | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s AcousticPlanarWaves -pr float         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s AcousticPlanarWaves -pr double        | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s ElasticPlanarWaves -pr float          | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s ElasticPlanarWaves -pr double         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s ElasticRickerPulse -pr float          | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s ElasticRickerPulse -pr double         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s EulerGaussianBell -pr float           | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s EulerGaussianBell -pr double          | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 | EulerGaussianBell.output/Error_EulerGaussianBell.csv 5e-4
ADERDG            | examples/aderdg             | aderdg.py -s EulerGaussianBell3D -pr float         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s EulerGaussianBell3D -pr double        | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s EulerIsentropicVortex -pr float       | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s EulerIsentropicVortex -pr double      | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s SWERadialDamBreak -pr float           | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s SWERadialDamBreak -pr double          | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s SWERestingLake -pr float              | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s SWERestingLake -pr double             | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s SWESteadyVortex -pr float             | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s SWESteadyVortex -pr double            | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s NavierStokesCosineBubble -pr float    | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s NavierStokesCosineBubble -pr double   | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s NavierStokesTwoBubbles -pr float      | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s NavierStokesTwoBubbles -pr double     | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s NavierStokesTaylorGreen -pr float     | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s NavierStokesTaylorGreen -pr double    | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s NavierStokesABCFlow -pr float         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG            | examples/aderdg             | aderdg.py -s NavierStokesABCFlow -pr double        | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG Static AMR | examples/aderdg-static-amr  | shallow-water-radial-dam-break.py                  | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG Static AMR | examples/aderdg-static-amr  | acoustic-point-explosion.py                        | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG Limiting   | examples/aderdg-limiting    | euler-airfoil.py                                   | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG Limiting   | examples/aderdg-limiting    | euler-corner-blast.py                              | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG Limiting   | examples/aderdg-limiting    | euler-double-rarefaction.py                        | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG Limiting   | examples/aderdg-limiting    | euler-quadrant-riemann.py                          | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG Limiting   | examples/aderdg-limiting    | euler-rayleigh-taylor.py                           | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG GRMHD      | examples/aderdg-grmhd       | aderdg-grmhd.py -s AlfvenWave                      | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG GRMHD      | examples/aderdg-grmhd       | aderdg-grmhd.py -s MichelSphericalAccretion        | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
ADERDG Coupling   | examples/aderdg-coupling    | LOH1.py                                            | ./coupled_LOH1 | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s AdvectionLinear                         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s AcousticPlanarWaves                     | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s ElasticPlanarWaves                      | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s ElasticRickerPulse                      | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s EulerGaussianBell                       | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 | EulerGaussianBell.output/Error_EulerGaussianBell.csv 1.5e-3
RKDG              | examples/rkdg               | rkdg.py -s EulerGaussianBell3D                     | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s EulerIsentropicVortex                   | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s SWERadialDamBreak                       | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s SWERestingLake                          | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s SWESteadyVortex                         | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s NavierStokesCosineBubble                | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s NavierStokesTwoBubbles                  | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s NavierStokesTaylorGreen                 | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG              | examples/rkdg               | rkdg.py -s NavierStokesABCFlow                     | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG Static AMR   | examples/rkdg-static-amr    | shallow-water-radial-dam-break.py                  | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
RKDG Static AMR   | examples/rkdg-static-amr    | acoustic-point-explosion.py                        | ./ExaHyPE      | -nt 10 -ns 2 -lmin 2 -lmax 4 | -nt 10 -ns 2 -lmin 3 -lmax 5 |
"""


def read_table(table):
    """Returns one dictionary per table row, keyed by the header row."""
    lines = table.strip().splitlines()
    header, *rows = [[cell.strip() for cell in line.split("|")] for line in lines]
    return [dict(zip(header, row)) for row in rows]


def commands(scenario, options, generator_arguments):
    """Returns the commands that generate, build, run and check a scenario."""
    generator = shlex.split(scenario["Generator"])
    result = [[sys.executable] + generator + generator_arguments]
    if not options.build_only:
        arguments = shlex.split(scenario[options.curve + " arguments"])
        result.append(LAUNCHER + [scenario["Executable"]] + arguments)
        if scenario["Error check"]:
            check = [sys.executable, "-m", "postprocessing.check_error_norms"]
            result.append(check + shlex.split(scenario["Error check"]))
    return result


if __name__ == "__main__":
    scenarios = read_table(SCENARIOS)
    groups = list(dict.fromkeys(scenario["Group"] for scenario in scenarios))
    parser = argparse.ArgumentParser(
        epilog=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--group", nargs="+", choices=groups, help="Groups to run, all by default"
    )
    parser.add_argument(
        "--build-only", action="store_true", help="Generate and build without running"
    )
    parser.add_argument(
        "--curve",
        choices=["Peano", "Hilbert"],
        default="Peano",
        help="Space-filling curve of the engine build",
    )
    options, generator_arguments = parser.parse_known_args()
    environment = dict(
        os.environ,
        PYTHONPATH=os.pathsep.join([str(REPOSITORY / "api"), str(REPOSITORY)]),
    )
    failures = []
    for scenario in scenarios:
        if options.group and scenario["Group"] not in options.group:
            continue
        title = scenario["Folder"] + ": " + scenario["Generator"]
        print("=== " + title, flush=True)
        try:
            for command in commands(scenario, options, generator_arguments):
                subprocess.run(
                    command,
                    cwd=REPOSITORY / scenario["Folder"],
                    env=environment,
                    check=True,
                )
        except (subprocess.CalledProcessError, OSError) as error:
            print(error, flush=True)
            failures.append(title)
    for title in failures:
        print("Failed: " + title)
    sys.exit(1 if failures else 0)
