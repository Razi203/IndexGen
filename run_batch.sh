#!/bin/bash

# =============================================================================
# Batch Parameter Testing for VTCode, Diff_VTCode, Linear, and Random Methods
# =============================================================================
# This script tests specific parameter combinations:
# - VTCode: (a,b) pairs: (0,0), (3,2), (8,2), (8,0), (3,0), (3,3)
# - Diff_VTCode: syndrome values: 0, 6, 25
# - Linear: distances: 3, 4, 5
# - Random: N values: 10000, 50000
#
# Results are saved to Test/MinMax/Max with subfolders for each method.
# =============================================================================

# --- Configuration ---
EXECUTABLE="./IndexGen"
N=11
EDIT_DIST=3
THREADS=32
SAVE_INTERVAL=1800
MAX_RUN=0
MIN_GC=0
MAX_GC=0

# Create base output directory
BASE_DIR="Test/MinMax/Max"
mkdir -p "$BASE_DIR"

# =============================================================================
# VTCode Method - Testing specific (a, b) pairs
# =============================================================================
echo "========================================================================"
echo "Starting VTCode parameter sweep"
echo "========================================================================"

# Define specific (a, b) pairs to test
declare -a VT_PAIRS=("0 0" "3 2" "8 2" "8 0" "3 0" "3 3")

for pair in "${VT_PAIRS[@]}"; do
    a=$(echo $pair | cut -d' ' -f1)
    b=$(echo $pair | cut -d' ' -f2)

    # Create output directory for this parameter combination
    OUTPUT_DIR="${BASE_DIR}/VTCode/a${a}_b${b}"
    mkdir -p "$OUTPUT_DIR"

    echo "------------------------------------------------------------------------"
    echo "Running VTCode with a=$a, b=$b"
    echo "Output directory: $OUTPUT_DIR"
    echo "------------------------------------------------------------------------"

    # Build and execute command
    CMD="$EXECUTABLE \
        --dir $OUTPUT_DIR \
        --lenStart $N \
        --lenEnd $N \
        --editDist $EDIT_DIST \
        --maxRun $MAX_RUN \
        --minGC $MIN_GC \
        --maxGC $MAX_GC \
        --threads $THREADS \
        --saveInterval $SAVE_INTERVAL \
        --method VTCode \
        --vt_a $a \
        --vt_b $b"

    echo "Executing: $CMD"
    $CMD

    if [ $? -eq 0 ]; then
        echo "✓ Completed VTCode with a=$a, b=$b"
    else
        echo "✗ Failed VTCode with a=$a, b=$b (exit code: $?)"
    fi
    echo ""
done

echo "========================================================================"
echo "VTCode parameter sweep completed"
echo "========================================================================"
echo ""

# =============================================================================
# Diff_VTCode Method - Testing specific syndrome values
# =============================================================================
echo "========================================================================"
echo "Starting Diff_VTCode parameter sweep"
echo "========================================================================"

# Define specific syndrome values to test
declare -a SYNDROMES=(0 6 25)

for synd in "${SYNDROMES[@]}"; do
    # Create output directory for this parameter
    OUTPUT_DIR="${BASE_DIR}/Diff_VTCode/synd${synd}"
    mkdir -p "$OUTPUT_DIR"

    echo "------------------------------------------------------------------------"
    echo "Running Diff_VTCode with syndrome=$synd"
    echo "Output directory: $OUTPUT_DIR"
    echo "------------------------------------------------------------------------"

    # Build and execute command
    CMD="$EXECUTABLE \
        --dir $OUTPUT_DIR \
        --lenStart $N \
        --lenEnd $N \
        --editDist $EDIT_DIST \
        --maxRun $MAX_RUN \
        --minGC $MIN_GC \
        --maxGC $MAX_GC \
        --threads $THREADS \
        --saveInterval $SAVE_INTERVAL \
        --method Diff_VTCode \
        --vt_synd $synd"

    echo "Executing: $CMD"
    $CMD

    if [ $? -eq 0 ]; then
        echo "✓ Completed Diff_VTCode with syndrome=$synd"
    else
        echo "✗ Failed Diff_VTCode with syndrome=$synd (exit code: $?)"
    fi
    echo ""
done

echo "========================================================================"
echo "Diff_VTCode parameter sweep completed"
echo "========================================================================"
echo ""

# =============================================================================
# Linear Method - Testing specific distances
# =============================================================================
echo "========================================================================"
echo "Starting Linear parameter sweep"
echo "========================================================================"

# Define specific distance values to test
declare -a LINEAR_DISTANCES=(3 4 5)

for dist in "${LINEAR_DISTANCES[@]}"; do
    # Create output directory for this parameter
    OUTPUT_DIR="${BASE_DIR}/Linear/dist${dist}"
    mkdir -p "$OUTPUT_DIR"

    echo "------------------------------------------------------------------------"
    echo "Running Linear with distance=$dist"
    echo "Output directory: $OUTPUT_DIR"
    echo "------------------------------------------------------------------------"

    # Build and execute command
    CMD="$EXECUTABLE \
        --dir $OUTPUT_DIR \
        --lenStart $N \
        --lenEnd $N \
        --editDist $EDIT_DIST \
        --maxRun $MAX_RUN \
        --minGC $MIN_GC \
        --maxGC $MAX_GC \
        --threads $THREADS \
        --saveInterval $SAVE_INTERVAL \
        --method LinearCode \
        --minHD $dist"

    echo "Executing: $CMD"
    $CMD

    if [ $? -eq 0 ]; then
        echo "✓ Completed Linear with distance=$dist"
    else
        echo "✗ Failed Linear with distance=$dist (exit code: $?)"
    fi
    echo ""
done

echo "========================================================================"
echo "Linear parameter sweep completed"
echo "========================================================================"
echo ""

# =============================================================================
# Random Method - Testing specific N values
# =============================================================================
echo "========================================================================"
echo "Starting Random parameter sweep"
echo "========================================================================"

# Define specific N values to test
declare -a RANDOM_N_VALUES=(10000 50000)

for n_val in "${RANDOM_N_VALUES[@]}"; do
    # Create output directory for this parameter
    OUTPUT_DIR="${BASE_DIR}/Random/N${n_val}"
    mkdir -p "$OUTPUT_DIR"

    echo "------------------------------------------------------------------------"
    echo "Running Random with N=$n_val"
    echo "Output directory: $OUTPUT_DIR"
    echo "------------------------------------------------------------------------"

    # Build and execute command
    CMD="$EXECUTABLE \
        --dir $OUTPUT_DIR \
        --lenStart $N \
        --lenEnd $N \
        --editDist $EDIT_DIST \
        --maxRun $MAX_RUN \
        --minGC $MIN_GC \
        --maxGC $MAX_GC \
        --threads $THREADS \
        --saveInterval $SAVE_INTERVAL \
        --method Random \
        --rand_candidates $n_val"

    echo "Executing: $CMD"
    $CMD

    if [ $? -eq 0 ]; then
        echo "✓ Completed Random with N=$n_val"
    else
        echo "✗ Failed Random with N=$n_val (exit code: $?)"
    fi
    echo ""
done

echo "========================================================================"
echo "Random parameter sweep completed"
echo "========================================================================"
echo ""

# =============================================================================
# Summary
# =============================================================================
echo "========================================================================"
echo "ALL TESTS COMPLETED"
echo "========================================================================"
echo "VTCode runs: 6 pairs [(0,0), (3,2), (8,2), (8,0), (3,0), (3,3)]"
echo "Diff_VTCode runs: 3 syndromes [0, 6, 25]"
echo "Linear runs: 3 distances [3, 4, 5]"
echo "Random runs: 2 N values [10000, 50000]"
echo "Total runs: 14"
echo "Results saved in: $BASE_DIR"
echo "========================================================================"
