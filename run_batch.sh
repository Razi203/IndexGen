#!/bin/bash

# =============================================================================
# Configuration
# =============================================================================
# --- Core Parameters ---
EXECUTABLE="./IndexGen"
WORD_LEN=12
THREADS=8
MAX_PARALLEL_JOBS=4

# --- Static Parameters (from original script) ---
MAX_RUN=0
MIN_GC=0
MAX_GC=0
SAVE_INTERVAL=1800
RESUME="" # Set to "--resume" to resume previous runs

# --- Method-Specific Static Parameters (from original script) ---
# These are passed to all commands; the executable should ignore unused ones.
MIN_HD=0        # Default, overridden by LinearCode loop
VT_A=0          # For VTCode
VT_B=0          # For VTCode
VT_SYND=0       # For Diff_VTCode

# =============================================================================
# Job Management
# =============================================================================
active_jobs=0
pids=()

# Function to wait for a job slot to open
wait_for_job_slot() {
    # Check if we've reached the max number of parallel jobs
    if [ $active_jobs -ge $MAX_PARALLEL_JOBS ]; then
        echo "--- Max parallel jobs ($MAX_PARALLEL_JOBS) reached. Waiting for a job to finish... ---"
        
        # Wait for any single job to finish
        # The -p option is not universally supported, so we'll find the finished PID manually.
        wait -n
        exit_code=$?
        
        finished_pid=0
        
        # Find and remove the finished PID from the array
        # Iterate over indices to safely unset
        for i in "${!pids[@]}"; do
            # Check if the PID is still running. `kill -0` sends a null signal.
            # If the process doesn't exist, kill returns non-zero.
            if ! kill -0 "${pids[$i]}" 2>/dev/null; then
                finished_pid="${pids[$i]}"
                unset 'pids[$i]'
                break # Found one, that's enough for this call
            fi
        done
        
        ((active_jobs--))
        
        if [ $finished_pid -ne 0 ]; then
            if [ $exit_code -eq 0 ]; then
                echo "--- Job $finished_pid finished successfully. Active jobs: $active_jobs ---"
            else
                echo "--- WARNING: Job $finished_pid finished with exit code $exit_code. Active jobs: $active_jobs ---"
            fi
        else
             # This is a safety net in case the PID list gets out of sync
             echo "--- WARNING: A job finished (exit code $exit_code), but couldn't identify its PID. Resyncing list. Active jobs: $active_jobs ---"
             new_pids=()
             for pid in "${pids[@]}"; do
                 # Check if element is non-empty and process is still running
                 if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
                     new_pids+=($pid)
                 fi
             done
             pids=("${new_pids[@]}")
             active_jobs=${#pids[@]} # Correct the count
             echo "--- Resynced PID list. Active jobs: $active_jobs ---"
        fi
    fi
}

# Function to run a command in the background
run_command() {
    local cmd_string="$1"
    local log_file="$2"
    
    # Wait until a slot is free
    wait_for_job_slot
    
    echo "============================================================================="
    echo "Starting new job. Log file: $log_file"
    echo "COMMAND: $cmd_string"
    echo "============================================================================="
    
    # Execute command in the background, redirecting stdout and stderr to log file
    # Using bash -c to correctly handle redirection
    bash -c "$cmd_string" > "$log_file" 2>&1 &
    
    # Store the PID of the new background job
    new_pid=$!
    pids+=($new_pid)
    ((active_jobs++))
    echo "--- Job started with PID $new_pid. Active jobs: $active_jobs ---"
}

# =============================================================================
# Main Execution Loop
# =============================================================================

echo "Starting batch run... (Max $MAX_PARALLEL_JOBS parallel jobs, $THREADS threads each)"

# Loop over specified edit distances
for EDIT_DIST in 3 4 5; do
    echo "--- Preparing jobs for Edit Distance: $EDIT_DIST ---"
    
    # --- 1. Random (3 variations) ---
    for RAND_SIZE in 10000 25000 50000; do
        METHOD="Random"
        # Create a unique directory for this specific run
        OUTPUT_DIR="Results/12.${EDIT_DIST}/${METHOD}_${RAND_SIZE}"
        LOG_FILE="${OUTPUT_DIR}/run.log"
        mkdir -p "$OUTPUT_DIR"
        
        CMD="$EXECUTABLE \
            $RESUME \
            --dir $OUTPUT_DIR \
            --lenStart $WORD_LEN \
            --lenEnd $WORD_LEN \
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
            --rand_candidates $RAND_SIZE \
            --vt_synd $VT_SYND"
            
        run_command "$CMD" "$LOG_FILE"
    done
    
    # --- 2. VTCode ---
    METHOD="VTCode"
    OUTPUT_DIR="Results/12.${EDIT_DIST}/${METHOD}"
    LOG_FILE="${OUTPUT_DIR}/run.log"
    mkdir -p "$OUTPUT_DIR"
    
    CMD="$EXECUTABLE \
        $RESUME \
        --dir $OUTPUT_DIR \
        --lenStart $WORD_LEN \
        --lenEnd $WORD_LEN \
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
        --rand_candidates 0 \
        --vt_synd $VT_SYND"
        
    run_command "$CMD" "$LOG_FILE"

    # --- 3. Diff_VTCode ---
    METHOD="Diff_VTCode"
    OUTPUT_DIR="Results/12.${EDIT_DIST}/${METHOD}"
    LOG_FILE="${OUTPUT_DIR}/run.log"
    mkdir -p "$OUTPUT_DIR"
    
    CMD="$EXECUTABLE \
        $RESUME \
        --dir $OUTPUT_DIR \
        --lenStart $WORD_LEN \
        --lenEnd $WORD_LEN \
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
        --rand_candidates 0 \
        --vt_synd $VT_SYND"
        
    run_command "$CMD" "$LOG_FILE"

    # --- 4. LinearCode (Looping over MIN_HD) ---
    METHOD="LinearCode"
    for CURRENT_MIN_HD in 3 4 5; do
        OUTPUT_DIR="Results/12.${EDIT_DIST}/${METHOD}_HD${CURRENT_MIN_HD}"
        LOG_FILE="${OUTPUT_DIR}/run.log"
        mkdir -p "$OUTPUT_DIR"
        
        CMD="$EXECUTABLE \
            $RESUME \
            --dir $OUTPUT_DIR \
            --lenStart $WORD_LEN \
            --lenEnd $WORD_LEN \
            --editDist $EDIT_DIST \
            --maxRun $MAX_RUN \
            --minGC $MIN_GC \
            --maxGC $MAX_GC \
            --threads $THREADS \
            --saveInterval $SAVE_INTERVAL \
            --method $METHOD \
            --minHD $CURRENT_MIN_HD \
            --vt_a $VT_A \
            --vt_b $VT_B \
            --rand_candidates 0 \
            --vt_synd $VT_SYND"
            
        run_command "$CMD" "$LOG_FILE"
    done

done

# =============================================================================
# Cleanup
# =============================================================================
echo "--- All jobs for all edit distances have been launched. ---"
echo "--- Waiting for the remaining $active_jobs jobs to finish... ---"

# Wait for all remaining background jobs in the 'pids' array to complete
for pid in "${pids[@]}"; do
    if [ -n "$pid" ]; then # Check if pid is not empty
        wait $pid
        exit_code=$?
        if [ $exit_code -eq 0 ]; then
            echo "--- Job $pid finished successfully. ---"
        else
            echo "--- WARNING: Job $pid finished with exit code $exit_code. ---"
        fi
    fi
done

echo "--- All jobs finished. Batch run complete. ---"

