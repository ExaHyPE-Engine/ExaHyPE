#!/bin/bash
# Generates, builds and runs every scenario of one test family inside a toolchain image.
# Usage: .github/scripts/run-tests.sh <family> | --list <full|branch> <tests|applications>

set -euo pipefail

BranchTests=("Code Generation" "FV" "ADERDG" "RKDG")
FullTests=(
  "Code Generation" "FV" "FV Tracers" "FV Static AMR" "Dynamic AMR" "ADERDG" "ADERDG Static AMR" "ADERDG Limiting"
  "ADERDG GRMHD" "ADERDG Coupling" "RKDG" "RKDG Static AMR" "Fortran Coupling" "Tutorials"
)
FullApplications=(
  "ExaSkred Tsunami" "ExaSkred Meteotsunami" "ExaSkred Landslide" "ExaSkred Coupled" "ExaSeis" "ExaGRyPE"
)
CpuOnlyFamilies=("Code Generation" "Fortran Coupling" "Tutorials")

if [ "${1:-}" = --list ]; then
  families=()
  if [ "${2:-}" = branch ] && [ "${3:-}" = tests ]; then
    families=("${BranchTests[@]}")
  elif [ "${2:-}" = full ] && [ "${3:-}" = tests ]; then
    families=("${FullTests[@]}")
  elif [ "${2:-}" = full ] && [ "${3:-}" = applications ]; then
    families=("${FullApplications[@]}")
  fi
  selected=()
  for family in "${families[@]}"; do
    if [ "${GPU_BACKEND:-none}" != none ] && [[ " ${CpuOnlyFamilies[*]} " == *" ${family} "* ]]; then
      continue
    fi
    selected+=("\"${family}\"")
  done
  echo "[$(
    IFS=,
    echo "${selected[*]}"
  )]"
  exit 0
fi

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <family> | --list <full|branch> <tests|applications>" >&2
  exit 2
fi

root="${PWD}"
MpiRanks=2
OmpThreads=2
Steps="-nt 300 -ns 2 -lmin ${MIN_GRID_LEVELS} -lmax ${MAX_GRID_LEVELS}"
failures=()

# Runs a command in a folder and records a failure under the title of the scenario.
step() {
  local title="$1" folder="$2" command="$3"
  echo "::group::${title}"
  if ! (cd "${folder}" && bash -eo pipefail -c "${command}"); then
    echo "::error::${title} failed"
    failures+=("${title}")
  fi
  echo "::endgroup::"
}

# Generates the executable of a scenario, runs it and compares its error norms with a tolerance.
# Usage: scenario <folder> <generator arguments> <executable and arguments> [<error file> <tolerance>]
scenario() {
  local folder="$1" generator="$2" launch="$3" errors="${4:-}" tolerance="${5:-}"
  local command="PYTHONPATH=${root}/api python3 ${generator} --build-mode ${BUILD_MODE}"
  if [ "${RUN_SIMULATIONS}" = true ]; then
    command+=" && OMP_NUM_THREADS=${OmpThreads} mpirun --bind-to none -np ${MpiRanks} ${launch}"
    if [ -n "${errors}" ]; then
      command+=" && PYTHONPATH=${root} python3 -m postprocessing.check_error_norms ${errors} ${tolerance}"
    fi
  fi
  step "${folder}: ${generator}" "${folder}" "${command}"
}

AderdgScenarios=(
  AdvectionLinear AcousticPlanarWaves ElasticPlanarWaves ElasticRickerPulse EulerGaussianBell EulerGaussianBell3D
  EulerIsentropicVortex SWERadialDamBreak SWERestingLake SWESteadyVortex NavierStokesCosineBubble
  NavierStokesTwoBubbles NavierStokesTaylorGreen NavierStokesABCFlow
)

case "$1" in
  "Code Generation")
    mkdir -p code-generation
    step "Code Generation" code-generation \
      "PYTHONPATH=${root}/api python3 ${root}/api/exahype2/tests/generate_all_pde_terms.py --build-mode ${BUILD_MODE}"
    ;;
  "FV")
    for test in breaking-dam channel-flow gaussian-bell gaussian-explosion point-explosion smooth-convergence \
      taylor-green-vortex; do
      for dimensions in 2 3; do
        if [ "${test}" = gaussian-bell ]; then
          scenario tests/fv "${test}.py --dimensions ${dimensions}" "./ExaHyPE ${Steps}" GaussianBell.output/Error.csv 8e-3
        else
          scenario tests/fv "${test}.py --dimensions ${dimensions}" "./ExaHyPE ${Steps}"
        fi
      done
    done
    for test in dry-ring isothermal-stratification; do
      scenario tests/fv "${test}.py --dimensions 2" "./ExaHyPE ${Steps}"
    done
    ;;
  "FV Tracers")
    for test in advection periodic-advection circular-bubble drag-force-validation vertical-plume-rise \
      one-body-problem radial-jet volcanic-plume-rise; do
      scenario tests/fv-tracers "fv-tracers-${test}.py --dimensions 2" "./ExaHyPE ${Steps}"
    done
    ;;
  "FV Static AMR")
    for periodic in on off; do
      for dimensions in 2 3; do
        scenario tests/fv-static-amr "fv-static-amr.py --periodic-bc ${periodic} --dimensions ${dimensions}" \
          "./ExaHyPE ${Steps}"
      done
    done
    ;;
  "Dynamic AMR")
    for solver in fv aderdg rkdg; do
      tests=(acoustic-point-explosion freestream periodic-advection radial-advection)
      if [ "${solver}" = fv ]; then
        tests+=(euler-point-explosion)
      fi
      for test in "${tests[@]}"; do
        for dimensions in 2 3; do
          if [ "${test}" = freestream ]; then
            scenario "tests/${solver}-dynamic-amr" "${test}.py --dimensions ${dimensions}" "./ExaHyPE ${Steps}" \
              FreeStream.output/Error.csv 1e-10
          else
            scenario "tests/${solver}-dynamic-amr" "${test}.py --dimensions ${dimensions}" "./ExaHyPE ${Steps}"
          fi
        done
      done
    done
    ;;
  "ADERDG")
    for test in "${AderdgScenarios[@]}"; do
      for precision in float double; do
        if [ "${test}" = EulerGaussianBell ] && [ "${precision}" = double ]; then
          scenario tests/aderdg "aderdg.py -s ${test} -pr ${precision}" "./ExaHyPE ${Steps}" \
            EulerGaussianBell.output/Error_EulerGaussianBell.csv 5e-4
        else
          scenario tests/aderdg "aderdg.py -s ${test} -pr ${precision}" "./ExaHyPE ${Steps}"
        fi
      done
    done
    ;;
  "ADERDG Static AMR")
    for test in shallow-water-radial-dam-break acoustic-point-explosion; do
      scenario tests/aderdg-static-amr "${test}.py" "./ExaHyPE ${Steps}"
    done
    ;;
  "ADERDG Limiting")
    for test in euler-airfoil euler-corner-blast euler-double-rarefaction euler-quadrant-riemann euler-rayleigh-taylor; do
      scenario tests/aderdg-limiting "${test}.py" "./ExaHyPE ${Steps}"
    done
    ;;
  "ADERDG GRMHD")
    for test in AlfvenWave MichelSphericalAccretion; do
      scenario tests/aderdg-grmhd "aderdg-grmhd.py -s ${test}" "./ExaHyPE ${Steps}"
    done
    ;;
  "ADERDG Coupling")
    scenario tests/aderdg-coupling LOH1.py "./coupled_LOH1 ${Steps}"
    ;;
  "RKDG")
    for test in "${AderdgScenarios[@]}"; do
      if [ "${test}" = EulerGaussianBell ]; then
        scenario tests/rkdg "rkdg.py -s ${test}" "./ExaHyPE ${Steps}" \
          EulerGaussianBell.output/Error_EulerGaussianBell.csv 1.5e-3
      else
        scenario tests/rkdg "rkdg.py -s ${test}" "./ExaHyPE ${Steps}"
      fi
    done
    ;;
  "RKDG Static AMR")
    for test in shallow-water-radial-dam-break acoustic-point-explosion; do
      scenario tests/rkdg-static-amr "${test}.py" "./ExaHyPE ${Steps}"
    done
    ;;
  "Fortran Coupling")
    scenario tests/fortran-coupling fortran-coupling.py "./ExaHyPE ${Steps}"
    ;;
  "Tutorials")
    for tutorial in 01_acoustic/AcousticWaves 02_shallow-water/ShallowWater 03_euler/Euler 04_limiting/EulerShock \
      05_elastic/ElasticWaves; do
      folder="tutorials/${tutorial%/*}"
      notebook="${tutorial#*/}.ipynb"
      # Without simulations, the job runs the cells of the notebook up to its build.
      script="${notebook}"
      if [ "${RUN_SIMULATIONS}" != true ]; then
        script=build.ipy
        python3 .github/scripts/extract-build-cells.py "${folder}/${notebook}" "${folder}/${script}"
      fi
      step "${folder}" "${folder}" "PYTHONPATH=${root}/api ipython ${script} -- --build-mode ${BUILD_MODE}"
    done
    ;;
  "ExaSkred Tsunami")
    for test in artificial chile oscillating-lake solitary-wave three-mounds-channel tohoku; do
      scenario "applications/exaskred/tsunami/${test}" "${test}.py" "./ExaSkred-Tsunami ${Steps}"
    done
    ;;
  "ExaSkred Meteotsunami")
    for test in gulf-of-mexico r1; do
      scenario "applications/exaskred/meteotsunami/${test}" "${test}.py" "./ExaSkred-Meteotsunami ${Steps}"
    done
    ;;
  "ExaSkred Landslide")
    scenario applications/exaskred/landslide/tafjord tafjord.py "./ExaSkred-Landslide ${Steps}"
    ;;
  "ExaSkred Coupled")
    for test in impulse-wave tafjord; do
      scenario "applications/exaskred/coupled/${test}" "${test}.py" "./ExaSkred-Coupled ${Steps}"
    done
    ;;
  "ExaSeis")
    scenario applications/exaseis/cartesian cartesian.py "./ExaSeis ${Steps}"
    for application in curvilinear pml; do
      for test in LOH1 Zugspitze; do
        scenario "applications/exaseis/${application}" "${application}.py -s ${test}" "./ExaSeis ${Steps}"
      done
    done
    for application in dynamic-rupture pml-dynamic-rupture; do
      for test in TPV5 TPV6 TPV16 TPV26 TPV28 TPV29 TPV34 Husavik; do
        scenario "applications/exaseis/${application}" "${application}.py -s ${test}" "./ExaSeis ${Steps}"
      done
    done
    ;;
  "ExaGRyPE")
    scenario applications/exagrype/gauge-wave "gauge-wave.py --solver fv" "./ExaGRyPE -nt 500 -ns 2 -l 1"
    ;;
  *)
    echo "Unknown test family $1" >&2
    exit 2
    ;;
esac

if [ "${#failures[@]}" -gt 0 ]; then
  printf 'Failed: %s\n' "${failures[@]}"
  exit 1
fi
