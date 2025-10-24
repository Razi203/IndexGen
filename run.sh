#!/bin/bash

# =============================================================================
# Configuration
# =============================================================================
# Set the parameters for your IndexGen run here.
# Simply change the values on the right-hand side.

# --- Core Parameters ---

# Path to the executable
EXECUTABLE="./IndexGen"

# Output/Resume directory
OUTPUT_DIR="Results"

# To resume a previous run, uncomment the following line:
# RESUME="--resume"
RESUME=""

# Codeword length range
LEN_START=10
LEN_END=10

# Minimum edit distance
EDIT_DIST=5

# --- Constraint Parameters ---

# Longest allowed homopolymer run (default: 3)
MAX_RUN=0

# GC-content range (0.0 to 1.0)
MIN_GC=0  # default: 0.3
MAX_GC=0  # default: 0.7

# --- Performance Parameters ---

# Number of threads to use
THREADS=16

# Interval in seconds to save progress
SAVE_INTERVAL=1800 # Set to 30 minutes, 80000 is > 22 hours

# --- Method-Specific Parameters ---

# Generation method: LinearCode, VTCode, Random, AllStrings, ProgressiveWave
METHOD="LinearCode"

# Min Hamming Distance (for LinearCode)
MIN_HD=3

# 'a' and 'b' parameters (for VTCode)
VT_A=0
VT_B=0

# Number of seeds (for ProgressiveWave)
WAVE_SEEDS=8

# Candidate pool size (for ProgressiveWave)
WAVE_POOL=50000

# Number of candidates (for Random)
RAND_CANDIDATES=50000


# =============================================================================
# Execution
# =============================================================================
# This section assembles and runs the command based on the variables above.

# Build the command string
CMD="$EXECUTABLE \
    $RESUME \
    --dir $OUTPUT_DIR \
    --lenStart $LEN_START \
    --lenEnd $LEN_END \
    --editDist $EDIT_DIST \
    --maxRun $MAX_RUN \
    --minGC $MIN_GC \
    --maxGC $MAX_GC \
    --threads $THREADS \
    --saveInterval $SAVE_INTERVAL \
    --method $METHOD \
    --minHD $MIN_HD \
    --vt_a $VT_A \
    --vt_b $VT_B \
    --wave_seeds $WAVE_SEEDS \
    --wave_pool $WAVE_POOL \
    --rand_candidates $RAND_CANDIDATES"

# Print the command to the console so you know what's being run
echo "Executing command:"
echo "$CMD"
echo "---"

# Run the command
$CMD

echo "---"
echo "Execution finished."
