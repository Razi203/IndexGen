#!/bin/bash

# =============================================================================
# Batch Parameter Testing for VTCode and Diff_VTCode Methods
# =============================================================================
# This script tests all possible parameter combinations for VTCode and
# Diff_VTCode methods with n=11 and edit distance=3.
#
# VTCode parameters: a in [0, n-1], b in [0, 3]
# Diff_VTCode parameter: syndrome in [0, 3*n - 1]
#
# Results are saved to Test/VT_Params with subfolders for each method and
# parameter combination.
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
BASE_DIR="Test/VT_Params"
mkdir -p "$BASE_DIR"

# =============================================================================
# VTCode Method - Testing all combinations of a and b
# =============================================================================
echo "========================================================================"
echo "Starting VTCode parameter sweep"
echo "========================================================================"

for a in $(seq 0 $((N-1))); do
    for b in $(seq 0 3); do
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
done

echo "========================================================================"
echo "VTCode parameter sweep completed"
echo "========================================================================"
echo ""

# =============================================================================
# Diff_VTCode Method - Testing all syndrome values
# =============================================================================
echo "========================================================================"
echo "Starting Diff_VTCode parameter sweep"
echo "========================================================================"

MAX_SYND=$((3*N - 1))

for synd in $(seq 0 $MAX_SYND); do
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
# Summary
# =============================================================================
echo "========================================================================"
echo "ALL TESTS COMPLETED"
echo "========================================================================"
echo "VTCode runs: $((N * 4)) (a: 0-$((N-1)), b: 0-3)"
echo "Diff_VTCode runs: $((MAX_SYND + 1)) (syndrome: 0-$MAX_SYND)"
echo "Total runs: $(((N * 4) + (MAX_SYND + 1)))"
echo "Results saved in: $BASE_DIR"
echo "========================================================================"
