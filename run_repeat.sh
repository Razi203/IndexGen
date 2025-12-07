#!/bin/bash

# =============================================================================
# Configuration
# =============================================================================
# Set the parameters for your IndexGen run here.
# Simply change the values on the right-hand side.

# --- Core Parameters ---

# Path to the executable
EXECUTABLE="./IndexGen"

# Output/Resume directory (base directory - iterations will be saved in subdirectories)
OUTPUT_DIR="Test/7-12-25"

# To resume a previous run, uncomment the following line:
# RESUME="--resume"
RESUME=""

# Codeword length range
LEN_START=12
LEN_END=12

# Minimum edit distance
EDIT_DIST=4

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
MIN_HD=4


# =============================================================================
# Execution
# =============================================================================
# This script runs in two phases:
# 1. Manual bias vectors: all single non-zero index positions with values 1-3
# 2. Random bias with default permutations: 500 iterations

echo "Starting execution with manual bias and random bias"
echo "============================================================================="
echo ""

# # =============================================================================
# # Phase 1: Manual Bias Vectors
# # =============================================================================
# echo "Phase 1: Running manual bias vectors (single non-zero index)"
# echo "---"

# # Counter for Phase 1
# iteration=0

# # For each position in the bias vector (11 positions)
# for position in {0..10}; do
#     # For each non-zero value (1, 2, 3 in GF(4))
#     for value in {1..3}; do
#         # Construct the bias vector with zeros everywhere except at 'position'
#         LC_BIAS=""
#         for i in {0..10}; do
#             if [ $i -eq $position ]; then
#                 LC_BIAS="${LC_BIAS}${value}"
#             else
#                 LC_BIAS="${LC_BIAS}0"
#             fi
#             if [ $i -lt 10 ]; then
#                 LC_BIAS="${LC_BIAS},"
#             fi
#         done

#         # Create iteration-specific output directory
#         ITERATION_OUTPUT_DIR="$OUTPUT_DIR/org_unit/$iteration"
#         mkdir -p "$ITERATION_OUTPUT_DIR"

#         echo "---"
#         echo "Starting iteration $iteration (Manual bias: position $position, value $value)"
#         echo "Bias vector: $LC_BIAS"
#         echo "Output directory: $ITERATION_OUTPUT_DIR"
#         echo "---"

#         # Build the command string
#         CMD="$EXECUTABLE \
#             $RESUME \
#             --dir $ITERATION_OUTPUT_DIR \
#             --lenStart $LEN_START \
#             --lenEnd $LEN_END \
#             --editDist $EDIT_DIST \
#             --maxRun $MAX_RUN \
#             --minGC $MIN_GC \
#             --maxGC $MAX_GC \
#             --threads $THREADS \
#             --saveInterval $SAVE_INTERVAL \
#             --method $METHOD \
#             --minHD $MIN_HD \
#             --lc_bias_mode manual \
#             --lc_bias $LC_BIAS"

#         # Print the command to the console
#         echo "Executing command:"
#         echo "$CMD"
#         echo ""

#         # Run the command with output redirection
#         $CMD >> $ITERATION_OUTPUT_DIR/log.txt 2>&1

#         echo ""
#         echo "Iteration $iteration finished."
#         echo ""

#         # Increment iteration counter
#         ((iteration++))
#     done
# done

# =============================================================================
# Phase 2: Random Bias Vectors with Default Permutations
# =============================================================================
echo "============================================================================="
echo "Phase 2: Running random bias vectors with default permutations (500 iterations)"
echo "---"

# Reset counter for Phase 2
iteration=0

for ((i=0; i<500; i++)); do
    # Create iteration-specific output directory
    ITERATION_OUTPUT_DIR="$OUTPUT_DIR/random-12-4/$iteration"
    mkdir -p "$ITERATION_OUTPUT_DIR"

    echo "---"
    echo "Starting iteration $iteration (Random bias run $i)"
    echo "Output directory: $ITERATION_OUTPUT_DIR"
    echo "---"

    # Build the command string with random bias
    CMD="$EXECUTABLE \
        $RESUME \
        --dir $ITERATION_OUTPUT_DIR \
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
        --lc_bias_mode random \
        --lc_row_perm_mode random \
        --lc_col_perm_mode random"

    # Print the command to the console
    echo "Executing command:"
    echo "$CMD"
    echo ""

    # Run the command with output redirection
    $CMD >> $ITERATION_OUTPUT_DIR/log.txt 2>&1

    echo ""
    echo "Iteration $iteration finished."
    echo ""

    # Increment iteration counter
    ((iteration++))
done

echo "============================================================================="
echo "All iterations finished."
echo "Phase 1 results are stored in: $OUTPUT_DIR/phase1/"
echo "Phase 2 results are stored in: $OUTPUT_DIR/phase2/"
