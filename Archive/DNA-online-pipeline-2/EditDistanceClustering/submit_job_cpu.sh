#!/bin/bash

# --- Resource Requests ---
#SBATCH --job-name=DNA_Gen
#SBATCH --cpus-per-task=1
#SBATCH --time=5-00:00:00
#SBATCH --output=exps_results/run.log
#SBATCH --error=exps_results/error.log


# --- Run Your Command ---
./build_and_run.sh
