#!/usr/bin/env python3
"""
Script to analyze log files in directory.
Finds the maximum number of code words and creates a visualization.
"""

import os
import re
from pathlib import Path
import matplotlib.pyplot as plt
import numpy as np


def extract_data_from_log(log_path):
    """
    Extract the number of code words and codebook time from a log file.

    Args:
        log_path: Path to the log file

    Returns:
        tuple: (num_codewords, codebook_time) or (None, None) if not found
    """
    try:
        with open(log_path, "r") as f:
            content = f.read()
            # Search for "Number of Code Words:" followed by a number
            codewords_match = re.search(r"Number of Code Words:\s*(\d+)", content)
            # Search for "Codebook Time:" followed by a float
            time_match = re.search(r"Codebook Time:\s*([\d.]+)\s*seconds", content)

            num_codewords = int(codewords_match.group(1)) if codewords_match else None
            codebook_time = float(time_match.group(1)) if time_match else None

            return num_codewords, codebook_time
    except Exception as e:
        print(f"Error reading {log_path}: {e}")
    return None, None


def analyze_perms_directory(base_path="Test/perms/"):
    """
    Analyze all log files in the Test/perms/ directory structure.

    Args:
        base_path: Base directory containing numbered subdirectories

    Returns:
        tuple: (codewords_dict, times_dict) - Mappings of folder names to code words and times
    """
    codewords_results = {}
    times_results = {}
    base_path = Path(base_path)

    if not base_path.exists():
        print(f"Error: Directory {base_path} does not exist!")
        return codewords_results, times_results

    # Iterate through all subdirectories
    for folder in sorted(
        base_path.iterdir(),
        key=lambda x: int(x.name) if x.name.isdigit() else float("inf"),
    ):
        if folder.is_dir():
            log_file = folder / "log.txt"
            if log_file.exists():
                num_codewords, codebook_time = extract_data_from_log(log_file)
                if num_codewords is not None:
                    codewords_results[folder.name] = num_codewords
                    if codebook_time is not None:
                        times_results[folder.name] = codebook_time
                    # print(f"Folder {folder.name}: {num_codewords} code words, {codebook_time}s")
                else:
                    print(f"Warning: Could not extract data from {log_file}")
            else:
                print(f"Warning: No log.txt found in {folder}")

    return codewords_results, times_results


def create_codewords_histogram(results, output_file="codewords_histogram.png"):
    """
    Create a histogram visualization of the code words data.

    Args:
        results: Dictionary mapping folder names to number of code words
        output_file: Path to save the output graph
    """
    if not results:
        print("No data to visualize!")
        return

    # Get codewords values
    codewords = list(results.values())

    # Create histogram
    plt.figure(figsize=(12, 6))
    plt.hist(codewords, bins=30, color="steelblue", edgecolor="black", alpha=0.7)
    plt.xlabel("Number of Code Words", fontsize=12, fontweight="bold")
    plt.ylabel("Frequency", fontsize=12, fontweight="bold")
    plt.title("Distribution of Code Words Counts", fontsize=14, fontweight="bold")
    plt.grid(axis="y", alpha=0.3, linestyle="--")

    # Add vertical lines for mean, max, and min
    plt.axvline(
        np.mean(codewords),
        color="orange",
        linestyle="--",
        linewidth=2,
        label=f"Mean: {np.mean(codewords):,.0f}",
    )
    plt.axvline(
        max(codewords),
        color="darkred",
        linestyle="-",
        linewidth=2,
        label=f"Max: {max(codewords):,}",
    )
    plt.axvline(
        min(codewords),
        color="darkblue",
        linestyle="-",
        linewidth=2,
        label=f"Min: {min(codewords):,}",
    )
    plt.legend()

    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches="tight")
    print(f"\nCode words histogram saved to: {output_file}")


def create_times_histogram(times_results, output_file="codebook_times_histogram.png"):
    """
    Create a histogram visualization of the codebook times data.

    Args:
        times_results: Dictionary mapping folder names to codebook times
        output_file: Path to save the output graph
    """
    if not times_results:
        print("No time data to visualize!")
        return

    # Get times values
    times = list(times_results.values())

    # Create histogram
    plt.figure(figsize=(12, 6))
    plt.hist(times, bins=30, color="green", edgecolor="black", alpha=0.7)
    plt.xlabel("Codebook Time (seconds)", fontsize=12, fontweight="bold")
    plt.ylabel("Frequency", fontsize=12, fontweight="bold")
    plt.title("Distribution of Codebook Times", fontsize=14, fontweight="bold")
    plt.grid(axis="y", alpha=0.3, linestyle="--")

    # Add vertical lines for mean, max, and min
    plt.axvline(
        np.mean(times),
        color="orange",
        linestyle="--",
        linewidth=2,
        label=f"Mean: {np.mean(times):.2f}s",
    )
    plt.axvline(
        max(times),
        color="darkred",
        linestyle="-",
        linewidth=2,
        label=f"Max: {max(times):.2f}s",
    )
    plt.axvline(
        min(times),
        color="darkblue",
        linestyle="-",
        linewidth=2,
        label=f"Min: {min(times):.2f}s",
    )
    plt.legend()

    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches="tight")
    print(f"Codebook times histogram saved to: {output_file}")


def main():
    """Main execution function."""
    path = "Test/28-12-25/N11E3CPU"
    # time_hist_name = "18-11-25-Time-hist.png"
    codewords_hist_name = "N11E3CPU.png"

    print(f"Analyzing log files in {path}...\n")

    # Analyze all log files
    codewords_results, times_results = analyze_perms_directory(path)

    if not codewords_results:
        print("\nNo valid results found!")
        return

    # Find and display the maximum and minimum code words
    max_folder = max(codewords_results.items(), key=lambda x: x[1])
    min_folder = min(codewords_results.items(), key=lambda x: x[1])
    print(f"\n{'=' * 60}")
    print(f"MAXIMUM NUMBER OF CODE WORDS: {max_folder[1]:,}")
    print(f"Found in folder: {max_folder[0]}")
    print(f"\nMINIMUM NUMBER OF CODE WORDS: {min_folder[1]:,}")
    print(f"Found in folder: {min_folder[0]}")
    print(f"{'=' * 60}\n")

    # Create visualizations
    create_codewords_histogram(codewords_results, codewords_hist_name)
    # create_times_histogram(times_results, time_hist_name)

    # Print summary statistics for code words
    codewords_list = list(codewords_results.values())
    print(f"\nCode Words Summary Statistics:")
    print(f"  Total runs analyzed: {len(codewords_list)}")
    print(f"  Maximum: {max(codewords_list):,}")
    print(f"  Minimum: {min(codewords_list):,}")
    print(f"  Mean: {np.mean(codewords_list):,.2f}")
    print(f"  Median: {np.median(codewords_list):,.2f}")
    print(f"  Std Dev: {np.std(codewords_list):,.2f}")

    # # Print summary statistics for codebook times
    # if times_results:
    #     times_list = list(times_results.values())
    #     max_time_folder = max(times_results.items(), key=lambda x: x[1])
    #     min_time_folder = min(times_results.items(), key=lambda x: x[1])

    #     print(f"\nCodebook Time Summary Statistics:")
    #     print(f"  Total runs with time data: {len(times_list)}")
    #     print(f"  Maximum: {max(times_list):.2f} seconds (folder {max_time_folder[0]})")
    #     print(f"  Minimum: {min(times_list):.2f} seconds (folder {min_time_folder[0]})")
    #     print(f"  Mean: {np.mean(times_list):.2f} seconds")
    #     print(f"  Median: {np.median(times_list):.2f} seconds")
    #     print(f"  Std Dev: {np.std(times_list):.2f} seconds")
    # else:
    #     print("\nNo codebook time data found in log files.")

    # Print the log file contents for the maximum codewords folder
    max_log_path = Path(path) / max_folder[0] / "log.txt"
    if max_log_path.exists():
        print(f"\n{'=' * 60}")
        print(f"LOG FILE CONTENTS FOR MAXIMUM CODEWORDS (folder {max_folder[0]}):")
        print(f"{'=' * 60}\n")
        with open(max_log_path, "r") as f:
            print(f.read())
        print(f"\n{'=' * 60}")
    else:
        print(f"\nWarning: Could not find log file at {max_log_path}")


if __name__ == "__main__":
    main()
