import os
import re
import csv

def extract_stats(root_dir, output_file):
    # Regex patterns
    bias_pattern = re.compile(r"Bias vector:\s*(\[[0-9, ]+\])")
    row_perm_pattern = re.compile(r"Row permutation:\s*(\[[0-9, ]+\])")
    col_perm_pattern = re.compile(r"Column permutation:\s*(\[[0-9, ]+\])")
    score_pattern = re.compile(r"Number of Code Words:\s*(\d+)")

    data = []

    print(f"Scanning directory: {root_dir}")

    for root, dirs, files in os.walk(root_dir):
        for file in files:
            if file.startswith("CodeSize") and file.endswith(".txt"):
                file_path = os.path.join(root, file)
                try:
                    with open(file_path, 'r') as f:
                        # Read only the first 25 lines
                        lines = [f.readline() for _ in range(25)]
                        content = "".join(lines)
                        
                        bias_match = bias_pattern.search(content)
                        row_match = row_perm_pattern.search(content)
                        col_match = col_perm_pattern.search(content)
                        score_match = score_pattern.search(content)

                        if bias_match and row_match and col_match and score_match:
                            bias = bias_match.group(1)
                            row_perm = row_match.group(1)
                            col_perm = col_match.group(1)
                            score = score_match.group(1)
                            
                            data.append({
                                'col_perm': col_perm,
                                'row_perm': row_perm,
                                'bias': bias,
                                'score': score
                            })
                except Exception as e:
                    print(f"Error reading {file_path}: {e}")

    print(f"Found {len(data)} entries. Writing to {output_file}...")

    # Write to CSV
    with open(output_file, 'w', newline='') as csvfile:
        fieldnames = ['col_perm', 'row_perm', 'bias', 'score']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

        writer.writeheader()
        for row in data:
            writer.writerow(row)

    print("Done.")

if __name__ == "__main__":
    # Use absolute or relative path carefully. 
    # Based on context, the script is likely run from /home_nfs/razi.hleihil/IndexGen/
    root_directory = "Test/28-12-25/N11E3CPU"
    output_filename = "history.csv"
    
    extract_stats(root_directory, output_filename)
