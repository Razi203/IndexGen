import sys

def calculate_batch_size(max_gpu_memory_gb):
    """
    Replicates logic from cuda_edit_distance.py to debug memory scaling.
    """
    limit_bytes = max_gpu_memory_gb * 1024**3
    buffer_bytes = 200 * 1024**2 # 200 MB reserve for buffers/overhead
    
    mem_per_row = 1024 + 64 # Peaucellier + Seq + margin
    
    available_for_batch = limit_bytes - buffer_bytes
    if available_for_batch < 0: available_for_batch = 100 * 1024**2 
    
    batch_size = int(available_for_batch // mem_per_row)
    
    # Align to 64
    batch_size = (batch_size // 64) * 64
    if batch_size < 1024: batch_size = 1024
    
    return batch_size, available_for_batch, mem_per_row

def main():
    if len(sys.argv) < 2:
        print("Usage: python debug_memory_calc.py <max_gpu_memory_gb>")
        sys.exit(1)
        
    try:
        max_gb = float(sys.argv[1])
    except ValueError:
        print("Error: max_gpu_memory_gb must be a float")
        sys.exit(1)

    print(f"--- Debugging Memory Calculation for {max_gb} GB ---")
    
    batch_size, available, per_row = calculate_batch_size(max_gb)
    
    print(f"Available Bytes: {available:20_} ({available/1024**3:.3f} GB)")
    print(f"Bytes Per Row:   {per_row:20_}")
    print(f"Calc Batch Size: {batch_size:20_}")
    
    # Expected VRAM Usage per batch
    expected_usage = batch_size * per_row + 200*1024**2
    print(f"Expected Usage:  {expected_usage:20_} ({expected_usage/1024**3:.3f} GB)")
    
    print("-" * 50)
    print("If this Batch Size is used, and actual usage is ~400MB, check if N (total strings) < Batch Size.")

if __name__ == "__main__":
    main()
