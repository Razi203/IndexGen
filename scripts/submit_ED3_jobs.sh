#!/bin/bash

# Configuration
CONFIG_TEMPLATE="config/config.json"
BASE_OUTPUT_DIR="Test/GC0.4-0.6_Run3/ED3"

# # Go to the root directory of the project assuming script is in IndexGen

mkdir -p "$BASE_OUTPUT_DIR"

# Array of sequence lengths and corresponding execution counts
declare -A RUN_COUNTS
RUN_COUNTS[12]=1000
RUN_COUNTS[13]=100
RUN_COUNTS[14]=10

for N in 12 13 14; do
    RUNS=${RUN_COUNTS[$N]}
    OUT_DIR="$BASE_OUTPUT_DIR/N$N"
    mkdir -p "$OUT_DIR"
    
    # Absolutize OUT_DIR because IndexGen runs from the root or wherever it is called
    ABS_OUT_DIR="$(realpath "$OUT_DIR")"
    CONFIG_PATH="$ABS_OUT_DIR/config_N${N}.json"
    
    echo "Preparing job for N=$N with $RUNS runs in $ABS_OUT_DIR"

    # Use python to generate specific config.json
    python3 -c "
import json
import os

template_path = '$CONFIG_TEMPLATE'
config_out_path = '$CONFIG_PATH'

try:
    with open(template_path, 'r') as f:
        data = json.load(f)
except Exception as e:
    print(f'Error reading template: {e}')
    exit(1)

data['dir'] = '$ABS_OUT_DIR'

if 'core' not in data: data['core'] = {}
data['core']['lenStart'] = $N
data['core']['lenEnd'] = $N
data['core']['editDist'] = 3

if 'constraints' not in data: data['constraints'] = {}
data['constraints']['maxRun'] = 3
data['constraints']['minGC'] = 0.4
data['constraints']['maxGC'] = 0.6

if 'performance' not in data: data['performance'] = {}
data['performance']['use_gpu'] = True

if 'clustering' not in data: data['clustering'] = {}
data['clustering']['enabled'] = False

if 'method' not in data: data['method'] = {}
data['method']['name'] = 'LinearCode'

if 'linearCode' not in data['method']: data['method']['linearCode'] = {}
data['method']['linearCode']['minHD'] = 3
data['method']['linearCode']['biasMode'] = 'random'
data['method']['linearCode']['rowPermMode'] = 'random'
data['method']['linearCode']['colPermMode'] = 'random'

# Remove randomSeed to ensure different seeds for each run
if 'randomSeed' in data['method']['linearCode']:
    del data['method']['linearCode']['randomSeed']

with open(config_out_path, 'w') as f:
    json.dump(data, f, indent=4)
"
    
    # Check if python script succeeded
    if [ $? -ne 0 ]; then
        echo "Failed to generate config for N=$N"
        continue
    fi

    # Create distinct SLURM submission script
    SLURM_SCRIPT="$ABS_OUT_DIR/submit_N${N}.sh"

    cat << EOF > "$SLURM_SCRIPT"
#!/bin/bash
#SBATCH --job-name=IG_N${N}
#SBATCH --cpus-per-task=32
#SBATCH --gres=gpu:1
#SBATCH --time=2-00:00:00

# Ensure we are in the correct directory
cd "$(pwd)"

module load anaconda3  
eval "\$(conda shell.bash hook)"
conda activate cuda_env

echo "=================================================="
echo "Job Started on \$(hostname) at \$(date)"
echo "Target N=$N, Total Runs=$RUNS"
echo "Config used: $CONFIG_PATH"
echo "=================================================="

for ((i=1; i<=$RUNS; i++)); do
    echo ">>> Starting run \$i / $RUNS at \$(date)"
    ./IndexGen --config "$CONFIG_PATH"
done

echo "=================================================="
echo "Job Completed at \$(date)"
EOF

    # Submit the job
    LOG_FILE="$ABS_OUT_DIR/job.log"
    ERR_FILE="$ABS_OUT_DIR/error.log"

    sbatch --output="$LOG_FILE" --error="$ERR_FILE" "$SLURM_SCRIPT"
    echo "Submitted job script: $SLURM_SCRIPT"
    echo "----------------------------------------"
done
