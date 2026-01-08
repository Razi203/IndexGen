#!/bin/bash

# --- Resource Requests ---
#SBATCH --job-name=DNA_Gen
#SBATCH --cpus-per-task=2
#SBATCH --gres=gpu:A40:1
#SBATCH --time=5-00:00:00
#SBATCH --output=Test/28-12-25/N16E3/run.log
#SBATCH --error=Test/28-12-25/N16E3/error.log

# --- 1. Load the Anaconda/Miniconda Module ---
# Most clusters require you to load a module before using conda.
# Common names are 'anaconda3', 'miniconda3', or 'python'.
# If you aren't sure, try running 'module spider conda' in your terminal first.
module load anaconda3  

# --- 2. Initialize Conda for the Shell ---
# This step ensures 'conda activate' works inside a script.
# (Replace the path below with the output of 'echo $CONDA_EXE' if this fails)
eval "$(conda shell.bash hook)"

# --- 3. Activate Your Environment ---
conda activate cuda_env

# --- 4. Verify (Optional but Recommended) ---
echo "Active Conda Environment: $CONDA_DEFAULT_ENV"
echo "Python Path: $(which python)"

# --- 5. Run Your Command ---
./run.sh