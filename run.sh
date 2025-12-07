#!/bin/bash

# =============================================================================
# Configuration
# =============================================================================
# Set the parameters for your IndexGen run here.
# Simply change the values on the right-hand side.

function pingme() {
    # Usage: pingme "Optional Message"
    # If no message is provided, defaults to "Job Done!"
    local topic="IndexGen-Lambda" 
    local msg="${1:-Job Done!}"
    curl -d "$msg" ntfy.sh/$topic
}


# Set INDEXGEN_ROOT to the directory containing this script
export INDEXGEN_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"


# To resume a previous run, uncomment the following line:
# RESUME="--resume"
RESUME=""

CMD="./IndexGen --config config.json"

# Print the command to the console so you know what's being run
echo "Executing command:"
echo "$CMD"
echo "---"

# Run the command
$CMD

echo "---"
echo "Execution finished."
# pingme "Job Done!"
