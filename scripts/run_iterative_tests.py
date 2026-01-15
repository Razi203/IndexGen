import json
import os
import subprocess
import random
import argparse
import sys
import copy

# Configuration
BASE_APP_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE_OUTPUT_DIR = os.path.join(BASE_APP_DIR, "Tests/K-Means")
SUBMIT_SCRIPT = os.path.join(BASE_APP_DIR, "scripts/submit_iterative_chain.sh")

# Parameters
K_VALUES = [20, 50, 70, 100]
I_VALUES = [2, 3, 4, 5, 8]
NUM_RUNS = 10

# Base Config Template (Step 1)
# Uses LinearCode with random bias/perms as requested
BASE_CONFIG_1 = {
    "core": {
        "lenStart": 11,
        "lenEnd": 11,
        "editDist": 3
    },
    "constraints": {
        "maxRun": 3,
        "minGC": 0.3,
        "maxGC": 0.7
    },
    "performance": {
        "threads": 16,
        "saveInterval": 3600,
        "use_gpu": True,
        "max_gpu_memory_gb": 8
    },
    "method": {
        "name": "LinearCode",
        "linearCode": {
            "minHD": 3,
            "biasMode": "random",
            "rowPermMode": "random",
            "colPermMode": "random",
            "randomSeed": 0  # Will be set per run
        }
    },
    "clustering": {
        "enabled": True,
        "k": 20,              # Will be set per run
        "clusterConvergence": 3, # Will be set per run
        "verbose": False
    },
    "verify": False
}

# Base Config Template (Step 2)
# FileRead method to refine/standard solve the output of Step 1
BASE_CONFIG_2 = {
    "core": {
        "lenStart": 11,
        "lenEnd": 11,
        "editDist": 3
    },
    "constraints": {
        "maxRun": 3,
        "minGC": 0.3,
        "maxGC": 0.7
    },
    "performance": {
        "threads": 16,
        "saveInterval": 3600,
        "use_gpu": True,
        "max_gpu_memory_gb": 8
    },
    "method": {
        "name": "FileRead",
        "fileRead": {
            "input_file": "" # Will be set per run
        }
    },
    "clustering": {
        "enabled": False
    },
    "verify": False
}

def main():
    parser = argparse.ArgumentParser(description="Submit Iterative K-Means Tests")
    parser.add_argument("--dry-run", action="store_true", help="Generate configs but do not submit jobs")
    args = parser.parse_args()

    # Ensure base output directory exists
    os.makedirs(BASE_OUTPUT_DIR, exist_ok=True)

    total_jobs = len(K_VALUES) * len(I_VALUES) * NUM_RUNS
    job_count = 0

    print(f"Starting submission of {total_jobs} jobs...")
    print(f"Base Output Directory: {BASE_OUTPUT_DIR}")

    for k in K_VALUES:
        for i_val in I_VALUES:
            for r in range(NUM_RUNS):
                job_count += 1
                run_id = f"k{k}_i{i_val}_r{r}"
                run_dir = os.path.join(BASE_OUTPUT_DIR, run_id)
                refined_dir = os.path.join(run_dir, "Refined")
                
                # Make directories
                os.makedirs(run_dir, exist_ok=True)
                os.makedirs(refined_dir, exist_ok=True)
                
                # Setup Config 1
                config1 = copy.deepcopy(BASE_CONFIG_1)
                config1["dir"] = run_dir
                config1["clustering"]["k"] = k
                config1["clustering"]["clusterConvergence"] = i_val
                
                # Generate a random seed for this run
                seed = random.randint(1, 999999)
                config1["method"]["linearCode"]["randomSeed"] = seed
                
                config1_path = os.path.join(run_dir, "config_step1.json")
                with open(config1_path, "w") as f:
                    json.dump(config1, f, indent=4)
                
                # Setup Config 2
                config2 = copy.deepcopy(BASE_CONFIG_2)
                config2["dir"] = refined_dir
                # The input file path will be populated by the submit script dynamically
                config2["method"]["fileRead"]["input_file"] = "PLACEHOLDER_TO_BE_UPDATED_BY_SUBMIT_SCRIPT"
                
                config2_path = os.path.join(run_dir, "config_step2.json")
                with open(config2_path, "w") as f:
                    json.dump(config2, f, indent=4)
                
                # Logs
                log_out = os.path.join(run_dir, "submit.out")
                log_err = os.path.join(run_dir, "submit.err")
                
                # Submit Command
                cmd = [
                    "sbatch",
                    f"--output={log_out}",
                    f"--error={log_err}",
                    f"--job-name=IG_{run_id}",
                    SUBMIT_SCRIPT,
                    config1_path,
                    config2_path
                ]
                
                if args.dry_run:
                    print(f"[Dry Run] would submit: {' '.join(cmd)}")
                else:
                    try:
                        subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                        print(f"[{job_count}/{total_jobs}] Submitted {run_id}")
                    except subprocess.CalledProcessError as e:
                        print(f"[{job_count}/{total_jobs}] ERROR submitting {run_id}: {e}")

    print("submission complete.")

if __name__ == "__main__":
    main()
