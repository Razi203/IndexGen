import os
import csv
import re
import sys

def scan_directory(base_dir):
    # Key: (L, ED, GC_range), Value: {run_val: (MaxCodeSize, Iteration)}
    results = {} 

    # Regex patterns
    l_pattern = re.compile(r"L(\d+)")
    params_pattern = re.compile(r"ED(\d+)_GC([\d\.]+)-([\d\.]+)_Run(\d+)")
    
    codesize_pattern = re.compile(r"CodeSize-(\d+)_.*\.txt")

    if not os.path.exists(base_dir):
        print(f"Error: Directory {base_dir} does not exist.")
        return

    print(f"Scanning {base_dir}...")

    for l_dir in os.listdir(base_dir):
        l_path = os.path.join(base_dir, l_dir)
        if not os.path.isdir(l_path):
            continue
        
        l_match = l_pattern.match(l_dir)
        if not l_match:
            continue
        
        l_val = l_match.group(1)

        for param_dir in os.listdir(l_path):
            param_path = os.path.join(l_path, param_dir)
            if not os.path.isdir(param_path):
                continue

            param_match = params_pattern.match(param_dir)
            if not param_match:
                continue

            ed_val = param_match.group(1)
            gc_min = param_match.group(2)
            gc_max = param_match.group(3)
            run_val = int(param_match.group(4)) # Store run as int for easier checking
            gc_range = f"{gc_min}-{gc_max}"

            # We only care about runs 0, 3, 4 for the specific columns, but let's collect all just in case
            # The user specifically asked for columns for Run 0, 3, 4.
            
            key = (l_val, ed_val, gc_range)
            
            if key not in results:
                results[key] = {}

            if run_val not in results[key]:
                results[key][run_val] = (-1, "")

            for iter_dir in os.listdir(param_path):
                iter_path = os.path.join(param_path, iter_dir)
                if not os.path.isdir(iter_path):
                    continue
                
                # Check for CodeSize file
                for filename in os.listdir(iter_path):
                    size_match = codesize_pattern.match(filename)
                    if size_match:
                        size = int(size_match.group(1))
                        
                        current_max, _ = results[key][run_val]
                        if size > current_max:
                            results[key][run_val] = (size, iter_dir)

    # Write to CSV
    output_file = "summary.csv"
    with open(output_file, 'w', newline='') as csvfile:
        fieldnames = [
            'Length (L)', 'Edit Distance (ED)', 'GC Range', 
            'Run 0 Size', 'Run 3 Size', 'Run 4 Size', 
            'Run 0 Iter', 'Run 3 Iter', 'Run 4 Iter'
        ]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

        writer.writeheader()
        
        # Sort keys for nicer output
        sorted_keys = sorted(results.keys(), key=lambda x: (int(x[0]), int(x[1]), x[2]))

        for key in sorted_keys:
            l, ed, gc = key
            runs_data = results[key]
            
            row = {
                'Length (L)': l,
                'Edit Distance (ED)': ed,
                'GC Range': gc,
                'Run 0 Size': '', 'Run 3 Size': '', 'Run 4 Size': '',
                'Run 0 Iter': '', 'Run 3 Iter': '', 'Run 4 Iter': ''
            }

            for run_id in [0, 3, 4]:
                if run_id in runs_data:
                    size, iteration = runs_data[run_id]
                    if size != -1:
                        row[f'Run {run_id} Size'] = size
                        row[f'Run {run_id} Iter'] = iteration
            
            writer.writerow(row)
    
    print(f"Results written to {output_file}")

if __name__ == "__main__":
    base_directory = "Test/25-11-25"
    # Allow overriding via command line
    if len(sys.argv) > 1:
        base_directory = sys.argv[1]
    
    scan_directory(base_directory)
