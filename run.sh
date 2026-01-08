#!/bin/bash

# =============================================================================
# Configuration
# =============================================================================
# Set the parameters for your IndexGen run here.
# Simply change the values on the right-hand side.

function pingme() {
    # Usage: pingme "Optional Message"
    # If no message is provided, defaults to "Job Done!"
    local topic="IndexGen-Newton" 
    local msg="${1:-Job Done!}"
    curl -d "$msg" ntfy.sh/$topic
}


# Set INDEXGEN_ROOT to the directory containing this script
export INDEXGEN_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"


# To resume a previous run, uncomment the following line:
# RESUME="--resume"
RESUME=""

DIR="Test/28-12-25/N16E3"

CMD="time ./IndexGen --config config.json --dir $DIR"

# Print the command to the console so you know what's being run
echo "Executing command:"
echo "$CMD"
echo "---"

# Run the command
$CMD

if [ $? -eq 0 ]; then
    pingme "Job Succeeded!"
else
    pingme "Job Failed!"
fi

echo "---"
echo "Execution finished."