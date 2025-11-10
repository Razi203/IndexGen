#!/bin/bash

# =============================================================================
# Configuration
# =============================================================================
# Set the parameters for your IndexGen run here.
# Simply change the values on the right-hand side.

# --- Repetition Parameter ---

# Number of times to repeat the execution
NUM_ITERATIONS=3000

# --- Core Parameters ---

# Path to the executable
EXECUTABLE="./IndexGen"

# Output/Resume directory (base directory - iterations will be saved in subdirectories)
OUTPUT_DIR="Test/perms"

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

# 'a' and 'b' parameters (for VTCode)
VT_A=8
VT_B=2

# Number of candidates (for Random)
RAND_CANDIDATES=50000

# Syndrome (for Differential VTCode)
VT_SYND=0

# Min Hamming Distance (for RandomLinear)
RANDLIN_MINHD=3
# Number of candidates (for RandomLinear)
RANDLIN_CANDIDATES=2000000


# =============================================================================
# Execution Loop
# =============================================================================
# This section assembles and runs the command based on the variables above,
# repeating NUM_ITERATIONS times with output in separate subdirectories.

echo "Starting repeated execution with $NUM_ITERATIONS iterations"
echo "============================================================================="
echo ""

for ((i=1; i<=NUM_ITERATIONS; i++)); do
    # Create iteration-specific output directory
    ITERATION_OUTPUT_DIR="$OUTPUT_DIR/$i"
    mkdir -p "$ITERATION_OUTPUT_DIR"

    echo "---"
    echo "Starting iteration $i of $NUM_ITERATIONS"
    echo "Output directory: $ITERATION_OUTPUT_DIR"
    echo "---"

    # Build the command string with iteration-specific output directory
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
        --vt_a $VT_A \
        --vt_b $VT_B \
        --rand_candidates $RAND_CANDIDATES \
        --vt_synd $VT_SYND \
        --randlin_minHD $RANDLIN_MINHD \
        --randlin_candidates $RANDLIN_CANDIDATES"

    # Print the command to the console so you know what's being run
    echo "Executing command:"
    echo "$CMD"
    echo ""

    # Run the command with output redirection
    $CMD >> $ITERATION_OUTPUT_DIR/log.txt 2>&1

    echo ""
    echo "Iteration $i finished."
    echo ""
done

echo "============================================================================="
echo "All $NUM_ITERATIONS iterations finished."
echo "Results are stored in subdirectories 1 through $NUM_ITERATIONS under $OUTPUT_DIR"
