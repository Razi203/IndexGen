#!/bin/bash

# =============================================================================
# Configuration
# =============================================================================
# Set the parameters for your IndexGen run here.
# Simply change the values on the right-hand side.

function pingme() {
    # Usage: pingme "Optional Message"
    # If no message is provided, defaults to "Job Done!"
    local topic="IndexGen-MRPC" 
    local msg="${1:-Job Done!}"
    curl -d "$msg" ntfy.sh/$topic
}


# Note: INDEXGEN_ROOT is auto-detected by the binary from its executable location.
# You can optionally set it manually to override: export INDEXGEN_ROOT="/path/to/project"


# To resume a previous run, uncomment the following line:
# RESUME="--resume"
RESUME=""

DIR="Tests/K-Means/run5"

# Use specific python environment

source "$(conda info --base)/etc/profile.d/conda.sh"
conda activate cuda_env

# Check if a config file was provided as an argument
CONFIG_FILE=${1:-config/config.json}

CMD="./IndexGen --config $CONFIG_FILE --dir $DIR"

# Print the command to the console so you know what's being run
echo "Executing command:"
echo "$CMD"
echo "---"

mkdir -p $DIR
# Run the command
time $CMD > $DIR/output2.log 2>&1

if [ $? -eq 0 ]; then
    pingme "Job Succeeded!"
else
    pingme "Job Failed!"
fi

echo "---"
echo "Execution finished."