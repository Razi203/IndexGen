import json
import os
import subprocess
import shutil
import time
import sys
import urllib.request
import urllib.parse

# Configuration
CONFIG_TEMPLATE = "config_CPU.json"
BASE_OUTPUT_DIR = "Test/28-12-25/N11E3CPU"
EXECUTABLE = "./IndexGen"
ITERATIONS = 30000
TEMP_CONFIG_FILE = "temp_config.json"
TOPIC = "IndexGen-Newton"

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

        # Note: INDEXGEN_ROOT is auto-detected by the binary from its executable location.
    # No need to set it manually in the environment.
    env = os.environ.copy()
    
    milestone = max(1, ITERATIONS // 10) # 10% milestone, ensure at least 1

    for i in range(ITERATIONS):
        # Define iteration-specific output directory
        iteration_dir = os.path.join(BASE_OUTPUT_DIR, str(i))
        
        # Ensure the directory exists
        os.makedirs(iteration_dir, exist_ok=True)
        
        # Update directory in config
        config_data['dir'] = iteration_dir
        
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
        
        # Check for progress notification (every 10%)
        # Note: (i + 1) because i is 0-indexed
        if (i + 1) % milestone == 0:
            percent = ((i + 1) // milestone) * 10
            msg = f"Progress: {percent}% ({i + 1}/{ITERATIONS})"
            print(f"Sending notification: {msg}")
            pingme(msg)

        print("---")

    # Clean up temporary config file
    if os.path.exists(TEMP_CONFIG_FILE):
        os.remove(TEMP_CONFIG_FILE)

    print("===================================")
    print("All iterations finished.")
    pingme("Job Done! All iterations finished.")

if __name__ == "__main__":
    main()
