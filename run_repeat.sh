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
OUTPUT_DIR="Test/new_bias_perms"

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

# Fixed permutation vectors (used for all runs)
LC_ROW_PERM="4,0,5,1,6,3,7,2"
LC_COL_PERM="8,9,6,10,3,5,0,4,7,2,1"

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
# This script runs in three phases:
# 1. Manual bias vectors: all single non-zero index positions with values 1-3
# 2. Random bias with fixed permutations: 500 iterations
# 3. All random: 1500 iterations with random bias and random permutations

echo "Starting execution with manual bias, random bias, and fully random"
echo "============================================================================="
echo ""

# Counter for output directories
iteration=0

# =============================================================================
# Phase 1: Manual Bias Vectors
# =============================================================================
echo "Phase 1: Running manual bias vectors (single non-zero index)"
echo "---"

# For each position in the bias vector (11 positions)
for position in {0..10}; do
    # For each non-zero value (1, 2, 3 in GF(4))
    for value in {1..3}; do
        # Construct the bias vector with zeros everywhere except at 'position'
        LC_BIAS=""
        for i in {0..10}; do
            if [ $i -eq $position ]; then
                LC_BIAS="${LC_BIAS}${value}"
            else
                LC_BIAS="${LC_BIAS}0"
            fi
            if [ $i -lt 10 ]; then
                LC_BIAS="${LC_BIAS},"
            fi
        done

        # Create iteration-specific output directory
        ITERATION_OUTPUT_DIR="$OUTPUT_DIR/$iteration"
        mkdir -p "$ITERATION_OUTPUT_DIR"

        echo "---"
        echo "Starting iteration $iteration (Manual bias: position $position, value $value)"
        echo "Bias vector: $LC_BIAS"
        echo "Output directory: $ITERATION_OUTPUT_DIR"
        echo "---"

        # Build the command string
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
            --lc_bias_mode manual \
            --lc_row_perm_mode manual \
            --lc_col_perm_mode manual \
            --lc_random_seed 0 \
            --vt_a $VT_A \
            --vt_b $VT_B \
            --rand_candidates $RAND_CANDIDATES \
            --vt_synd $VT_SYND \
            --lc_bias $LC_BIAS \
            --lc_row_perm $LC_ROW_PERM \
            --lc_col_perm $LC_COL_PERM"

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
done

# =============================================================================
# Phase 2: Random Bias Vectors with Fixed Permutations
# =============================================================================
echo "============================================================================="
echo "Phase 2: Running random bias vectors with fixed permutations (500 iterations)"
echo "---"

for ((i=0; i<500; i++)); do
    # Create iteration-specific output directory
    ITERATION_OUTPUT_DIR="$OUTPUT_DIR/$iteration"
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
        --lc_row_perm_mode manual \
        --lc_col_perm_mode manual \
        --lc_random_seed 0 \
        --vt_a $VT_A \
        --vt_b $VT_B \
        --rand_candidates $RAND_CANDIDATES \
        --vt_synd $VT_SYND \
        --lc_row_perm $LC_ROW_PERM \
        --lc_col_perm $LC_COL_PERM"

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

# =============================================================================
# Phase 3: Fully Random Vectors
# =============================================================================
echo "============================================================================="
echo "Phase 3: Running fully random vectors (1500 iterations)"
echo "---"

for ((i=0; i<1500; i++)); do
    # Create iteration-specific output directory
    ITERATION_OUTPUT_DIR="$OUTPUT_DIR/$iteration"
    mkdir -p "$ITERATION_OUTPUT_DIR"

    echo "---"
    echo "Starting iteration $iteration (Fully random run $i)"
    echo "Output directory: $ITERATION_OUTPUT_DIR"
    echo "---"

    # Build the command string with all random
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
        --lc_col_perm_mode random \
        --lc_random_seed 0 \
        --vt_a $VT_A \
        --vt_b $VT_B \
        --rand_candidates $RAND_CANDIDATES \
        --vt_synd $VT_SYND"

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
echo "All iterations finished. Total runs: $iteration"
echo "Results are stored in subdirectories 0 through $((iteration-1)) under $OUTPUT_DIR"
