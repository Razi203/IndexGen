#!/bin/bash

# --- Resource Requests ---
#SBATCH --job-name=DNA_Gen
#SBATCH --cpus-per-task=2
#SBATCH --gres=gpu:A40:1
#SBATCH --time=2-00:00:00

# Note: --output and --error should be passed via sbatch command line arguments 
# to ensure logs go to the specific run directory.

# --- 1. Load the Anaconda/Miniconda Module ---
module load anaconda3  

# --- 2. Initialize Conda for the Shell ---
eval "$(conda shell.bash hook)"

# --- 3. Activate Your Environment ---
conda activate cuda_env

# --- 4. Verify ---
echo "Active Conda Environment: $CONDA_DEFAULT_ENV"
echo "Python Path: $(which python)"
echo "Running IndexGen with Config: $1"

CONFIG_FILE="$1"
# Get the directory of the config file to pass to report
RUN_DIR=$(dirname "$CONFIG_FILE")

# --- 5. Run Your Command ---
START_TIME=$(date +%s)

./IndexGen --config "$CONFIG_FILE"

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

# --- 6. Report Back ---
# Call the main script in report mode (assuming run_repeat.py is in the current dir or INDEXGEN_ROOT)
# We assume slurm runs in the submission directory by default unless changed.
python3 run_repeat.py --report --dir "$RUN_DIR" --time "$DURATION"