import os
import sys

# --- CONFIGURATION ---
# 1. Set the main directory you want to scan
START_DIRECTORY = "Test/21-12-25/random-repeat-minhd-3"

# 2. Set the names of subfolders to EXCLUDE
#    Any folder with these names will be skipped.
EXCLUDED_FOLDERS = {
    "venv",
    ".git",
    "__pycache__",
    "build",
    "node_modules",
    "Old",
    "VT2",
}

# 3. Set the name for the final combined output file
OUTPUT_FILE = "Results/Results-21-12-25.txt"

# 4. Define the start of the delimiter line
DELIMITER_START = "=="
# --- END CONFIGURATION ---


def process_directory(root_dir, exclude_set, output_filename, delimiter):
    """
    Walks a directory, skipping specified subfolders, and extracts
    content from files based on a delimiter.
    """

    # Get absolute paths for clearer logging
    abs_root = os.path.abspath(root_dir)
    abs_output = os.path.abspath(output_filename)

    # --- Safety Checks ---
    if not os.path.isdir(abs_root):
        print(f"Error: Start directory not found: {abs_root}")
        sys.exit(1)

    if os.path.abspath(os.path.dirname(abs_output)) == abs_root:
        print("Warning: Output file is in the root scan directory.")
        print("The script will skip processing the output file itself.")

    print(f"Scanning: {abs_root}")
    print(f"Excluding: {', '.join(exclude_set) or 'None'}")
    print(f"Outputting to: {abs_output}\n")

    extracted_count = 0

    # Open the output file once in 'write' mode to clear it
    try:
        with open(output_filename, "w", encoding="utf-8") as outfile:
            # os.walk(..., topdown=True) is default, but explicit is clear.
            # It allows us to modify 'dirnames' in-place to prune the search.
            for dirpath, dirnames, filenames in os.walk(abs_root, topdown=True):
                # --- This is the exclusion logic ---
                # We modify 'dirnames' in-place. os.walk will then skip
                # recursing into any directories we remove.
                dirnames[:] = [d for d in dirnames if d not in exclude_set]

                for filename in filenames:
                    file_path = os.path.join(dirpath, filename)

                    # --- Skip processing the output file itself ---
                    if os.path.abspath(file_path) == abs_output:
                        continue

                    try:
                        with open(file_path, "r", encoding="utf-8") as infile:
                            header_lines = []
                            found_delimiter = False

                            for line in infile:
                                # Add the line *first*
                                header_lines.append(line)

                                # Check if this *is* the delimiter line
                                if line.strip().startswith(delimiter):
                                    found_delimiter = True
                                    break  # Stop after including the delimiter

                            # Only write if we found a file with the delimiter
                            if found_delimiter:
                                header_to_write = "".join(header_lines)
                                outfile.write(header_to_write)
                                # Add an extra newline for separation,
                                # as seen in your example.
                                outfile.write("\n")
                                extracted_count += 1
                                print(f"Processed: {file_path}")

                    except (IOError, UnicodeDecodeError) as e:
                        # Skip files we can't read (e.g., binary, permissions)
                        print(f"Skipping file (cannot read): {file_path} - {e}")
                        continue

    except IOError as e:
        print(f"Error: Could not write to output file: {abs_output} - {e}")
        sys.exit(1)

    print("\n--- Done ---")
    print(f"Successfully processed and combined headers from {extracted_count} files.")


# --- Main execution ---
if __name__ == "__main__":
    # Check if the user has updated the placeholder path
    if START_DIRECTORY == "path/to/your/main/directory":
        print("Error: Please edit the script and set the 'START_DIRECTORY' variable.")
        sys.exit(1)

    process_directory(START_DIRECTORY, EXCLUDED_FOLDERS, OUTPUT_FILE, DELIMITER_START)
