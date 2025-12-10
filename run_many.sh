#!/bin/bash

# function to send notifications
function pingme() {
    local msg="${1:-Job Done!}"
    curl -d "$msg" ntfy.sh/IndexGen-Newton
}


# Set INDEXGEN_ROOT to the directory containing this script
export INDEXGEN_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"


# =============================================================================
# Configuration
# =============================================================================

# Executable path
EXECUTABLE="./IndexGen"

# Base Output Directory
BASE_OUTPUT_DIR="Test/25-11-25"

# Number of iterations per configuration
NUM_ITERATIONS=5

# Parameter Ranges
LENGTHS=(11 12 13 14)
EDIT_DISTS=(5)
MAX_RUNS=(0 3 4)

# GC Combinations "MIN,MAX"
GC_COMBS=("0,0" "0.3,0.7")

# Base Vectors (for Length 14)
BASE_LC_BIAS="3,0,2,3,1,1,1,3,2,3,3,0,0,0"
BASE_LC_ROW_PERM="4,0,5,1,2,3,6,7,8"
BASE_LC_COL_PERM="8,9,6,10,3,5,0,4,7,2,1,11,12,13"

# Other Fixed Parameters
METHOD="LinearCode"
MIN_HD=4
LC_BIAS_MODE="manual"
LC_ROW_PERM_MODE="manual"
LC_COL_PERM_MODE="manual"
LC_RANDOM_SEED=0 

THREADS=32
SAVE_INTERVAL=1800

# Dry run mode (set to true to test without running)
DRY_RUN=false

# =============================================================================
# Helper Functions
# =============================================================================

# Function to truncate a CSV string to n elements
truncate_csv() {
    local csv="$1"
    local count="$2"
    echo "$csv" | cut -d, -f1-"$count"
}

# Function to count elements in CSV
count_csv() {
    local csv="$1"
    echo "$csv" | tr ',' '\n' | wc -l
}

# =============================================================================
# Main Execution
# =============================================================================

for LEN in "${LENGTHS[@]}"; do
    # Determine iterations based on length
    case $LEN in
        11) NUM_ITERATIONS=100 ;;
        12) NUM_ITERATIONS=50 ;;
        13) NUM_ITERATIONS=20 ;;
        14) NUM_ITERATIONS=10 ;;
        *) NUM_ITERATIONS=1 ;;
    esac

    # Calculate truncation
    DIFF=$((14 - LEN))
    
    # Truncate Vectors
    # Calculate original lengths
    LEN_BIAS=$(count_csv "$BASE_LC_BIAS")
    LEN_ROW=$(count_csv "$BASE_LC_ROW_PERM")
    LEN_COL=$(count_csv "$BASE_LC_COL_PERM")
    
    # New lengths
    NEW_LEN_BIAS=$((LEN_BIAS - DIFF))
    NEW_LEN_ROW=$((LEN_ROW - DIFF))
    NEW_LEN_COL=$((LEN_COL - DIFF))
    
    CUR_BIAS=$(truncate_csv "$BASE_LC_BIAS" "$NEW_LEN_BIAS")
    CUR_ROW_PERM=$(truncate_csv "$BASE_LC_ROW_PERM" "$NEW_LEN_ROW")
    CUR_COL_PERM=$(truncate_csv "$BASE_LC_COL_PERM" "$NEW_LEN_COL")
    
    for ED in "${EDIT_DISTS[@]}"; do
        for GC_COMB in "${GC_COMBS[@]}"; do
            IFS=',' read -r MIN_GC MAX_GC <<< "$GC_COMB"
            
            for MAX_RUN in "${MAX_RUNS[@]}"; do
                
                # Configuration Loop
                
                for (( i=1; i<=NUM_ITERATIONS; i++ )); do
                    
                    # Determine Mode
                    # First iteration is manual
                    # Second iteration is manual perms, random bias
                    # The rest are random
                    if (( i == 1 )); then
                        CUR_BIAS_MODE="manual"
                        CUR_ROW_PERM_MODE="manual"
                        CUR_COL_PERM_MODE="manual"
                        MODE_SUFFIX=""
                    elif (( i == 2 )); then
                        CUR_BIAS_MODE="random"
                        CUR_ROW_PERM_MODE="manual"
                        CUR_COL_PERM_MODE="manual"
                        MODE_SUFFIX="_RandomBias"
                    else
                        CUR_BIAS_MODE="random"
                        CUR_ROW_PERM_MODE="random"
                        CUR_COL_PERM_MODE="random"
                        MODE_SUFFIX="_Random"
                    fi

                    # Define Output Directory
                    # Structure: Test/25-11-25/L{len}/ED{ed}_GC{min}-{max}_Run{run}/Iter{i}
                    SUBDIR="L${LEN}/ED${ED}_GC${MIN_GC}-${MAX_GC}_Run${MAX_RUN}/Iter${i}${MODE_SUFFIX}"
                    OUTPUT_DIR="${BASE_OUTPUT_DIR}/${SUBDIR}"
                    
                    if [ "$DRY_RUN" = false ]; then
                        mkdir -p "$OUTPUT_DIR"
                    fi
                    
                    # Generate Random Seed (use iteration + random to ensure uniqueness)
                    SEED=$((LC_RANDOM_SEED + i + RANDOM))
                    
                    CMD="$EXECUTABLE \
                        --dir $OUTPUT_DIR \
                        --lenStart $LEN \
                        --lenEnd $LEN \
                        --editDist $ED \
                        --maxRun $MAX_RUN \
                        --minGC $MIN_GC \
                        --maxGC $MAX_GC \
                        --threads $THREADS \
                        --saveInterval $SAVE_INTERVAL \
                        --method $METHOD \
                        --minHD $MIN_HD \
                        --lc_bias_mode $CUR_BIAS_MODE \
                        --lc_row_perm_mode $CUR_ROW_PERM_MODE \
                        --lc_col_perm_mode $CUR_COL_PERM_MODE \
                        --lc_random_seed $SEED"

                    # Add manual vectors individually based on mode
                    if [ "$CUR_BIAS_MODE" == "manual" ]; then
                        CMD="$CMD --lc_bias $CUR_BIAS"
                    fi
                    if [ "$CUR_ROW_PERM_MODE" == "manual" ]; then
                        CMD="$CMD --lc_row_perm $CUR_ROW_PERM"
                    fi
                    if [ "$CUR_COL_PERM_MODE" == "manual" ]; then
                        CMD="$CMD --lc_col_perm $CUR_COL_PERM"
                    fi
                        
                    echo "Running: L=$LEN ED=$ED GC=$MIN_GC-$MAX_GC Run=$MAX_RUN Iter=$i Mode=$CUR_BIAS_MODE Perm=$CUR_ROW_PERM_MODE"
                    if [ "$DRY_RUN" = true ]; then
                        echo "  CMD: $CMD"
                        if [ "$CUR_BIAS_MODE" == "manual" ]; then
                            echo "    Bias: $CUR_BIAS"
                        fi
                        if [ "$CUR_ROW_PERM_MODE" == "manual" ]; then
                            echo "    Row : $CUR_ROW_PERM"
                        fi
                        if [ "$CUR_COL_PERM_MODE" == "manual" ]; then
                            echo "    Col : $CUR_COL_PERM"
                        fi
                    else
                        $CMD > "${OUTPUT_DIR}/run.log" 2>&1
                    fi
                    
                done
                
                # Notify after completion of this configuration set
                if [ "$DRY_RUN" = false ]; then
                    pingme "Completed: L=$LEN ED=$ED GC=$MIN_GC-$MAX_GC Run=$MAX_RUN ($NUM_ITERATIONS iterations)"
                fi
            done
        done
    done
done

pingme "All runs completed."
echo "All runs completed."
