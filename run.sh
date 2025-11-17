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
OUTPUT_DIR="TEMP"

# To resume a previous run, uncomment the following line:
# RESUME="--resume"
RESUME=""

# Codeword length range
LEN_START=11
LEN_END=11

# Minimum edit distance
EDIT_DIST=3

# --- Constraint Parameters ---

# Longest allowed homopolymer run (default: 3)
MAX_RUN=0

# GC-content range (0.0 to 1.0)
MIN_GC=0  # default: 0.3
MAX_GC=0  # default: 0.7

# --- Performance Parameters ---

# Number of threads to use
THREADS=32

# Interval in seconds to save progress
SAVE_INTERVAL=1800 # Set to 30 minutes, 80000 is > 22 hours

# --- Method-Specific Parameters ---

# Generation method: LinearCode, VTCode, Random, Diff_VTCode
METHOD="LinearCode"

# Min Hamming Distance (for LinearCode)
MIN_HD=3

# LinearCode vector control parameters
# Bias vector mode: default, random, manual
LC_BIAS_MODE="manual"
# Row permutation mode: identity, random, manual
LC_ROW_PERM_MODE="identity"
# Column permutation mode: identity, random, manual
LC_COL_PERM_MODE="identity"
# Bias vector (CSV, GF(4) values 0-3) - only used when LC_BIAS_MODE=manual
LC_BIAS="1,0,0,0,0,0,0,0,0,0,0"
# Row permutation (CSV, 0-indexed) - only used when LC_ROW_PERM_MODE=manual
LC_ROW_PERM=""
# Column permutation (CSV, 0-indexed) - only used when LC_COL_PERM_MODE=manual
LC_COL_PERM=""
# Random seed for LinearCode vectors
LC_RANDOM_SEED=0

# 'a' and 'b' parameters (for VTCode)
VT_A=8
VT_B=2

# Number of candidates (for Random)
RAND_CANDIDATES=50000

# Syndrome (for Differential VTCode)
VT_SYND=0


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
    --lc_bias_mode $LC_BIAS_MODE \
    --lc_row_perm_mode $LC_ROW_PERM_MODE \
    --lc_col_perm_mode $LC_COL_PERM_MODE \
    --lc_random_seed $LC_RANDOM_SEED \
    --vt_a $VT_A \
    --vt_b $VT_B \
    --rand_candidates $RAND_CANDIDATES \
    --vt_synd $VT_SYND"

# Add optional manual vectors if provided
if [ -n "$LC_BIAS" ]; then
    CMD="$CMD --lc_bias $LC_BIAS"
fi
if [ -n "$LC_ROW_PERM" ]; then
    CMD="$CMD --lc_row_perm $LC_ROW_PERM"
fi
if [ -n "$LC_COL_PERM" ]; then
    CMD="$CMD --lc_col_perm $LC_COL_PERM"
fi

# Print the command to the console so you know what's being run
echo "Executing command:"
echo "$CMD"
echo "---"

# Run the command
$CMD

echo "---"
echo "Execution finished."
