import json
import os
import subprocess
import shutil
import time
import sys
import urllib.request
import urllib.parse
import glob
import re
import argparse

# Configuration
CONFIG_TEMPLATE = "random_bias_config.json"
BASE_OUTPUT_DIR = "Test/Combinations_Len15"
SUBMIT_SCRIPT = "submit_job.sh"
TOPIC = "IndexGen-Newton"

# Parameters
LENGTH = 15
MAX_RUNS = [0, 3, 4]
GC_RANGES = [(0.0, 0.0), (0.3, 0.7)]
# Format: (editDist, minHD)
ED_PARAMS = [(3, 3), (4, 3), (5, 4)]
# Format: (mode_name, biasMode, rowPermMode, colPermMode)
MODES = [
    ("Default", "default", "identity", "identity"),
    ("Random", "random", "random", "random")
]

def pingme(msg="Job Done!"):
    """Sends a notification via ntfy.sh"""
    try:
        url = f"https://ntfy.sh/{TOPIC}"
        data = msg.encode('utf-8')
        req = urllib.request.Request(url, data=data, method='POST')
        with urllib.request.urlopen(req) as response:
            pass # Request successful
    except Exception as e:
        print(f"Warning: Failed to send notification: {e}")

def get_code_size(directory):
    """Finds the CodeSize file and returns the size."""
    try:
        files = glob.glob(os.path.join(directory, "CodeSize-#*_*.txt"))
        if files:
            # Assuming the format is CodeSize-#<size>_...
            filename = os.path.basename(files[0])
            # Extract size between '#' and '_'
            parts = filename.split('_')
            prologue = parts[0] # CodeSize-#<size>
            if '#' in prologue:
                size_str = prologue.split('#')[1]
                return size_str
    except Exception as e:
        print(f"Error finding code size: {e}")
    return "Unknown"

def format_time(seconds):
    hours, rem = divmod(seconds, 3600)
    minutes, secs = divmod(rem, 60)
    return f"{int(hours)}h {int(minutes)}m {int(secs)}s"

def do_report(directory, duration):
    """Callback function used by the worker job to report results."""
    code_size = get_code_size(directory)
    status = "SUCCESS" if code_size != "Unknown" else "FAILED/UNKNOWN"
    time_str = format_time(duration)
    dir_name = os.path.basename(directory)
    
    msg = f"{status} | Size: {code_size} | Time: {time_str}\nParams: {dir_name}"
    print(f"Reporting: {msg}")
    pingme(msg)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", action="store_true", help="Run in report mode")
    parser.add_argument("--dir", help="Directory check for results")
    parser.add_argument("--time", type=int, help="Duration in seconds")
    args = parser.parse_args()

    # Determine the directory where this script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # Change current working directory to the script directory
    os.chdir(script_dir)

    if args.report:
        if not args.dir or args.time is None:
            print("Error: --dir and --time are required for --report mode")
            return
        do_report(args.dir, args.time)
        return

    # Normal execution: Submit jobs
    print("Starting execution - Queue All Mode")
    print("===================================")
    print(f"Working Directory: {os.getcwd()}")
    print(f"Template Config: {CONFIG_TEMPLATE}")
    print(f"Output Base Dir: {BASE_OUTPUT_DIR}")
    print("-----------------------------------")

    # Load the template configuration
    try:
        with open(CONFIG_TEMPLATE, 'r') as f:
            base_config = json.load(f)
    except FileNotFoundError:
        print(f"Error: Template config file '{CONFIG_TEMPLATE}' not found.")
        return
    except json.JSONDecodeError:
        print(f"Error: Failed to decode JSON from '{CONFIG_TEMPLATE}'.")
        return

    run_count = 0
    total_runs = len(MAX_RUNS) * len(GC_RANGES) * len(ED_PARAMS) * len(MODES)

    for max_run in MAX_RUNS:
        for min_gc, max_gc in GC_RANGES:
            for ed, min_hd in ED_PARAMS:
                for mode_name, bias_mode, row_perm, col_perm in MODES:
                    run_count += 1
                    
                    dir_name = f"MR{max_run}_GC{min_gc}-{max_gc}_ED{ed}_mHD{min_hd}_{mode_name}"
                    iteration_dir = os.path.join(BASE_OUTPUT_DIR, dir_name)
                    
                    # Ensure the directory exists
                    os.makedirs(iteration_dir, exist_ok=True)
                    
                    # Prepare config
                    config = base_config.copy()
                    config['dir'] = iteration_dir
                    
                    config['core']['lenStart'] = LENGTH
                    config['core']['lenEnd'] = LENGTH
                    config['core']['editDist'] = ed
                    
                    config['constraints']['maxRun'] = max_run
                    config['constraints']['minGC'] = min_gc
                    config['constraints']['maxGC'] = max_gc
                    
                    if 'method' not in config: config['method'] = {}
                    if 'linearCode' not in config['method']: config['method']['linearCode'] = {}
                    
                    config['method']['linearCode']['minHD'] = min_hd
                    config['method']['linearCode']['biasMode'] = bias_mode
                    config['method']['linearCode']['rowPermMode'] = row_perm
                    config['method']['linearCode']['colPermMode'] = col_perm
                    
                    # Write run-specific config file
                    config_file_path = os.path.join(iteration_dir, "config.json")
                    with open(config_file_path, 'w') as f:
                        json.dump(config, f, indent=4)
                        
                    log_path = os.path.join(iteration_dir, "log.txt")
                    err_path = os.path.join(iteration_dir, "error.log")
                    
                    # Submit to sbatch
                    job_name = f"IG_{dir_name}" 
                    cmd = [
                        "sbatch",
                        f"--output={log_path}",
                        f"--error={err_path}",
                        f"--job-name={job_name}",
                        SUBMIT_SCRIPT,
                        config_file_path
                    ]
                    
                    try:
                        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
                        print(f"[{run_count}/{total_runs}] Submitted {dir_name}: {result.stdout.strip()}")
                    except subprocess.CalledProcessError as e:
                        print(f"[{run_count}/{total_runs}] Error submitting {dir_name}: {e.stderr}")

    print("===================================")
    print(f"All {total_runs} jobs submitted.")
    print("You can check status with 'squeue'.")
    pingme(f"All {total_runs} jobs queued successfully.")

if __name__ == "__main__":
    main()
