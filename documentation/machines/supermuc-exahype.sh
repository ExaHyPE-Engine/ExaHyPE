#!/bin/bash
# ==============================================================================================
# Installation and Batch Script for ExaHyPE on SuperMUC
# ==============================================================================================
set -eu

# ==============================================================================================
# USER PARAMETERS
# ==============================================================================================
REPO_URL="https://gitlab.lrz.de/hpcsoftware/Peano.git"
BRANCH="muc/exahype"
DIR_NAME="Peano"

# Change to the application you want to build and run
EXAHYPE_APPLICATION=""
EXAHYPE_EXECUTABLE_NAME=""
EXAHYPE_DIMENSIONS=2
EXAHYPE_GRID_LEVELS=5
EXAHYPE_SOLUTION_NUM_SNAPSHOTS=100
EXAHYPE_SOLUTION_FOLDER="solutions"
EXAHYPE_SOLUTION_PATCH_FILE=""

SLURM_ACCOUNT=""
SLURM_PARTITION="general"

# Build Configuration
BUILD_TIME="00:20:00"
BUILD_NODES=1
BUILD_RANKS=1
BUILD_THREADS=32

# Run Configuration
USE_INTERACTIVE_SLURM=true
RUN_NUM_NODES=4
RUN_NUM_RANKS=8
RUN_NUM_THREADS=8
RUN_TIME=02:00:00
# ==============================================================================================

echo ">>> Starting ExaHyPE Setup..."

PEANO_ROOT="$(pwd)/$DIR_NAME"
echo ">>> PEANO_ROOT is set to: $PEANO_ROOT"

export http_proxy="http://localhost:8000"
export https_proxy="http://localhost:8000"
export HTTP_PROXY="http://localhost:8000"
export HTTPS_PROXY="http://localhost:8000"

APP_FULL_PATH="$PEANO_ROOT/$EXAHYPE_APPLICATION"
APP_DIR=$(dirname "$APP_FULL_PATH")
APP_EXEC="$APP_DIR/$EXAHYPE_EXECUTABLE_NAME"

MODULE_LOAD_COMMANDS='
module switch stack/24.5.0
module load gcc/14.3.0
module load intel-toolkit
module load python/3.13.7
module load cmake
module load ninja
module load hdf5/1.10.11-intel25-impi
module load netcdf-c/4.9.2-intel25-impi
'

load_modules() {
    eval "$MODULE_LOAD_COMMANDS"
}

load_modules

# ==============================================================================================
# PHASE 1: DOWNLOAD & VENV
# ==============================================================================================

echo ">>> Setting up temporary local Git LFS..."
LFS_VERSION="v3.4.0"
LFS_TEMP_DIR="tmp_git_lfs"
rm -rf "$LFS_TEMP_DIR"
mkdir -p "$LFS_TEMP_DIR"
echo "    Downloading Git LFS $LFS_VERSION..."
wget -q --show-progress "https://github.com/git-lfs/git-lfs/releases/download/${LFS_VERSION}/git-lfs-linux-amd64-${LFS_VERSION}.tar.gz" -O "$LFS_TEMP_DIR/lfs.tar.gz"
tar -xf "$LFS_TEMP_DIR/lfs.tar.gz" -C "$LFS_TEMP_DIR"
export PATH="$(pwd)/$LFS_TEMP_DIR/git-lfs-${LFS_VERSION#v}:$PATH"
git lfs install

if [ -d "$DIR_NAME" ]; then
    echo ">>> Directory '$DIR_NAME' already exists."
    echo ">>> Entering directory and pulling latest changes..."
    cd "$DIR_NAME"
    git checkout "$BRANCH"
    git pull
    git lfs pull
    cd ..
else
    echo ">>> Cloning repository branch: $BRANCH..."
    GIT_CLONE_PROTECTION_ACTIVE=false git clone "$REPO_URL" -b "$BRANCH"
fi

echo ">>> Removing temporary local Git LFS..."
git lfs uninstall
rm -rf "$LFS_TEMP_DIR"

cd "$DIR_NAME"

echo ">>> Setting up Python virtual environment..."

if [ ! -d "venv" ]; then
    echo "   >>> Creating new virtual environment..."
    python3 -m venv "venv"
    echo "   >>> Activating environment..."
    source "venv/bin/activate"
    echo "   >>> Upgrading pip and installing dependencies..."
    pip install --upgrade pip
    pip install .
else
    echo "   >>> Existing virtual environment detected. Skipping pip install."
    echo "   >>> Activating environment..."
    source "venv/bin/activate"
fi

# ==============================================================================================
# PHASE 2: COMPILATION & GENERATION
# ==============================================================================================

# Check if the ExaHyPE application executable already exists
if [ -x "$APP_EXEC" ]; then
    echo "=========================================="
    echo ">>> Existing ExaHyPE executable detected."
    echo "   >>> Location: $APP_EXEC"
    echo "   >>> Skipping build phase."
    echo "=========================================="
else
    echo ">>> No existing executable found at $APP_EXEC"
    echo "   >>> Proceeding with build phase..."

    BUILD_SCRIPT="_build_task.sh"

    cat << EOF > $BUILD_SCRIPT
#!/bin/bash
set -eu
echo ">>> [BUILD JOB] Loading Modules..."
$MODULE_LOAD_COMMANDS

echo ">>> [BUILD JOB] Activating Venv..."
source "$PEANO_ROOT/venv/bin/activate"

echo ">>> [BUILD JOB] Creating build directory..."
mkdir -p build
cd build

export CC=icx
export CXX=icpx

echo ">>> [BUILD JOB] Running CMake with Ninja..."
cmake .. -GNinja \
    -DPEANO_WITH_MPI=ON \
    -DPEANO_WITH_MULTITHREADING=ON \
    -DPEANO_WITH_HDF5=ON \
    -DPEANO_WITH_NETCDF=ON

echo ">>> [BUILD JOB] Building with Ninja (using \$SLURM_CPUS_PER_TASK threads)..."
ninja -j \$SLURM_CPUS_PER_TASK

APP_FULL_PATH="$APP_FULL_PATH"
APP_DIR="$APP_DIR"
APP_SCRIPT=\$(basename "\$APP_FULL_PATH")

echo ">>> [BUILD JOB] Entering Application directory: \$APP_DIR"
if [ ! -f "\$APP_FULL_PATH" ]; then
    echo "Error: Application script not found at \$APP_FULL_PATH"
    exit 1
fi
cd "\$APP_DIR"

echo ">>> [BUILD JOB] Running ExaHyPE Python generation script: \$APP_SCRIPT"
python3 "\$APP_SCRIPT" -ns $EXAHYPE_SOLUTION_NUM_SNAPSHOTS -md $EXAHYPE_GRID_LEVELS -d $EXAHYPE_DIMENSIONS

echo ">>> [BUILD JOB] Build phase complete."
EOF

    chmod +x $BUILD_SCRIPT

    echo "=========================================="
    echo ">>> Submitting BUILD JOB to $SLURM_PARTITION..."
    echo "=========================================="

    salloc --nodes=$BUILD_NODES \
           --ntasks=$BUILD_RANKS \
           --cpus-per-task=$BUILD_THREADS \
           --account=$SLURM_ACCOUNT \
           --partition=$SLURM_PARTITION \
           --time=$BUILD_TIME \
           srun ./$BUILD_SCRIPT

    rm -f $BUILD_SCRIPT
fi

# ==============================================================================================
# PHASE 3: EXECUTION & POST-PROCESSING
# ==============================================================================================

cd "$APP_DIR"

if [ ! -f "./$EXAHYPE_EXECUTABLE_NAME" ]; then
    echo "Warning: ./$EXAHYPE_EXECUTABLE_NAME not found."
    echo "Looking for any executable created recently..."
    EXAHYPE_EXECUTABLE_NAME=$(ls -t | grep -v "\." | head -n 1)
    echo "Found: $EXAHYPE_EXECUTABLE_NAME"
fi

echo "=========================================="
echo ">>> Application Build Complete!"
echo "   >>> Executable location: $(pwd)"
echo "=========================================="

VISUALISATION_COMMANDS="
if [ \"$EXAHYPE_SOLUTION_NUM_SNAPSHOTS\" -gt 0 ]; then
    echo \"==========================================\"
    echo \">>> [RUN JOB] Converting to VTU...\"
    python3 \"$PEANO_ROOT/python/visualisation/convert-to-vtu.py\" \"$EXAHYPE_SOLUTION_FOLDER/$EXAHYPE_SOLUTION_PATCH_FILE\" -j 32
    echo \">>> [RUN JOB] Conversion finished.\"

    echo \"==========================================\"
    echo \">>> [RUN JOB] Creating animation...\"
    python3 \"$PEANO_ROOT/python/visualisation/convert-to-png.py\" \"$EXAHYPE_SOLUTION_FOLDER/$EXAHYPE_SOLUTION_PATCH_FILE\" -j 32 --dpi 600

    ANIMATION_OUT=\"$HOME/${EXAHYPE_EXECUTABLE_NAME}.mp4\"

    echo \">>> [RUN JOB] Saving animation to: \$ANIMATION_OUT\"
    python3 \"$PEANO_ROOT/python/visualisation/create-animation-from-png.py\" --input-dir \"$EXAHYPE_SOLUTION_FOLDER\" --output \"\$ANIMATION_OUT\" --fps 24
    echo \">>> [RUN JOB] Creation finished.\"
fi
"

if [ "$USE_INTERACTIVE_SLURM" = true ]; then
    if [ -z "$SLURM_ACCOUNT" ]; then
        echo "ERROR: SLURM_ACCOUNT variable is empty."
        exit 1
    fi

    echo "=========================================="
    echo ">>> Submitting RUN JOB to $SLURM_PARTITION..."
    echo "=========================================="

    RUN_SCRIPT="_run_and_visualise.sh"

    cat << EOF > $RUN_SCRIPT
#!/bin/bash
set -eu
echo ">>> [RUN JOB] Loading Modules..."
$MODULE_LOAD_COMMANDS

echo ">>> [RUN JOB] Activating Venv..."
source "$PEANO_ROOT/venv/bin/activate"

export OMP_PLACES=cores
export OMP_PROC_BIND=close
export OMP_NUM_THREADS=$RUN_NUM_THREADS

cd "$PEANO_ROOT/build"
# ctest --verbose

cd "$APP_DIR"
rm -rf solutions
mkdir -p solutions

echo ">>> [RUN JOB] Starting Simulation ($EXAHYPE_EXECUTABLE_NAME)..."
srun --ntasks-per-node=$RUN_NUM_RANKS --cpus-per-task=$RUN_NUM_THREADS ./$EXAHYPE_EXECUTABLE_NAME

$VISUALISATION_COMMANDS
echo ">>> [RUN JOB] complete."
EOF

    chmod +x $RUN_SCRIPT

    salloc --nodes=$RUN_NUM_NODES \
           --account=$SLURM_ACCOUNT \
           --partition=$SLURM_PARTITION \
           --time=$RUN_TIME \
           ./$RUN_SCRIPT

    rm -f $RUN_SCRIPT
else
    if [ -z "$SLURM_ACCOUNT" ]; then
        echo "ERROR: SLURM_ACCOUNT variable is empty."
        exit 1
    fi

    BATCH_FILE="_run_and_visualise.slurm"

    echo "=========================================="
    echo ">>> Generating Batch Script: $BATCH_FILE"
    echo "=========================================="

    cat << EOF > "$BATCH_FILE"
#!/bin/bash
#SBATCH --job-name=$EXAHYPE_EXECUTABLE_NAME
#SBATCH --output=$HOME/${EXAHYPE_EXECUTABLE_NAME}_%j.out
#SBATCH --error=$HOME/${EXAHYPE_EXECUTABLE_NAME}_%j.err
#SBATCH --nodes=$RUN_NUM_NODES
#SBATCH --ntasks-per-node=$RUN_NUM_RANKS
#SBATCH --cpus-per-task=$RUN_NUM_THREADS
#SBATCH --time=$RUN_TIME
#SBATCH --account=$SLURM_ACCOUNT
#SBATCH --partition=$SLURM_PARTITION
#SBATCH --exclusive
#SBATCH --requeue

set -eu

echo ">>> [BATCH JOB] Job ID: \$SLURM_JOB_ID"
echo ">>> [BATCH JOB] Hostname: \$(hostname)"
echo ">>> [BATCH JOB] Loading Modules..."
$MODULE_LOAD_COMMANDS

echo ">>> [BATCH JOB] Activating Venv..."
source "$PEANO_ROOT/venv/bin/activate"

cd "$APP_DIR"
rm -rf solutions
mkdir -p solutions

export OMP_NUM_THREADS=\$SLURM_CPUS_PER_TASK
export OMP_PLACES=cores
export OMP_PROC_BIND=close

echo ">>> [BATCH JOB] Starting Simulation ($EXAHYPE_EXECUTABLE_NAME)..."
srun ./$EXAHYPE_EXECUTABLE_NAME

$VISUALISATION_COMMANDS
echo ">>> [BATCH JOB] Finished."
EOF

    echo "=========================================="
    echo ">>> Submitting BATCH JOB to queue..."
    echo "=========================================="
    sbatch "$BATCH_FILE"
    echo ">>> Job submitted. Check status with: squeue --me"
    rm "$BATCH_FILE"
fi
