import json
import os
import subprocess
import shutil
import time
import sys

# Configuration
CONFIG_TEMPLATE = "random_bias_config.json"
BASE_OUTPUT_DIR = "Test/21-12-25/random-repeat-minhd-2"
EXECUTABLE = "./IndexGen"
ITERATIONS = 400
TEMP_CONFIG_FILE = "temp_config.json"

def main():
    # Determine the directory where this script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Change current working directory to the script directory
    # This ensures ./IndexGen and config files are found relative to the script
    os.chdir(script_dir)

    print("Starting execution with Python script")
    print("===================================")
    print(f"Working Directory: {os.getcwd()}")
    print(f"Template Config: {CONFIG_TEMPLATE}")
    print(f"Output Base Dir: {BASE_OUTPUT_DIR}")
    print(f"Iterations: {ITERATIONS}")
    print("-----------------------------------")

    # Load the template configuration
    try:
        with open(CONFIG_TEMPLATE, 'r') as f:
            config_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: Template config file '{CONFIG_TEMPLATE}' not found.")
        return
    except json.JSONDecodeError:
        print(f"Error: Failed to decode JSON from '{CONFIG_TEMPLATE}'.")
        return

    
    # Set environment variable for INDEXGEN_ROOT so C++ binary can find scripts
    # even after changing its internal working directory
    env = os.environ.copy()
    env["INDEXGEN_ROOT"] = script_dir
    

    for i in range(ITERATIONS):
        # Define iteration-specific output directory
        iteration_dir = os.path.join(BASE_OUTPUT_DIR, str(i))
        
        # Ensure the directory exists
        os.makedirs(iteration_dir, exist_ok=True)
        
        # Update directory in config
        config_data['dir'] = iteration_dir
        config_data['method']['linearCode']['minHD'] = 2
        
        # Write to temporary config file
        with open(TEMP_CONFIG_FILE, 'w') as f:
            json.dump(config_data, f, indent=4)
            
        print(f"Starting iteration {i}")
        print(f"Output directory: {iteration_dir}")

        # Open log file for redirection
        log_path = os.path.join(iteration_dir, "log.txt")
        try:
            with open(log_path, "w") as log_file:
                # Construct command
                cmd = [EXECUTABLE, "--config", TEMP_CONFIG_FILE]
                
                # Run the command with cwd set to script_dir explicitly
                subprocess.run(cmd, stdout=log_file, stderr=subprocess.STDOUT, check=False, cwd=script_dir, env=env)
        except Exception as e:
            print(f"Error executing iteration {i}: {e}")

        print(f"Iteration {i} finished.")
        print("---")

    # Clean up temporary config file
    if os.path.exists(TEMP_CONFIG_FILE):
        os.remove(TEMP_CONFIG_FILE)

    print("===================================")
    print("All iterations finished.")

if __name__ == "__main__":
    main()
