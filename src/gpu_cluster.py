import sys
import os
import numpy as np
from numba import cuda, uint32, uint8
import math
import warnings
import time
from numba.core.errors import NumbaPerformanceWarning

# Suppress Numba performance warnings
warnings.simplefilter('ignore', category=NumbaPerformanceWarning)

# -------------------------------------------------------------------------
# CUDA Kernel: Myers' Bit-Parallel (Dense Distance Matrix Output)
# -------------------------------------------------------------------------
@cuda.jit
def compute_distance_matrix_kernel(peq_flat,        # (K, 256)
                                   seq_transposed,  # (L, BATCH_COLS) flattened
                                   L,               # Length of strings
                                   start_row,       # Global Row Offset of this TILE
                                   start_col,       # Global Col Offset of this TILE
                                   num_rows,        # Number of rows in this TILE
                                   num_cols,        # Number of cols in this TILE
                                   K,               # Total Number of centers
                                   N_batch,         # Number of vectors in current batch
                                   distance_matrix, # Device array (num_rows, num_cols) of uint8
                                   peq_batch_start, # Global index of first row in peq_flat
                                   seq_batch_start, # Global index of first col in seq_transposed
                                   seq_batch_width):# Width of current seq batch
    
    # Block Configuration: 16x16 threads
    # Each thread computes 1 Center (Row) x 4 Vectors (Columns)
    
    tx = cuda.threadIdx.x # 0..15
    ty = cuda.threadIdx.y # 0..15
    bx = cuda.blockIdx.x
    by = cuda.blockIdx.y
    
    block_row_start = by * 16
    block_col_start = bx * 64
    
    # Thread's specific Global Row (Center index)
    global_row = start_row + block_row_start + ty
    
    # Thread's specific Global Column Start (Vector index in batch)
    local_col_base = tx * 4
    global_col_base = start_col + block_col_start + local_col_base

    # ---------------------------------------------------------------------
    # Shared Memory Allocation & Loading
    # ---------------------------------------------------------------------
    smem_peq = cuda.shared.array((16, 4), uint32)
    smem_seq = cuda.shared.array((64, 33), uint8) 

    # Load PEQ
    if tx < 4:
        g_r_load = start_row + block_row_start + ty
        if g_r_load < K:
            local_r = g_r_load - peq_batch_start
            smem_peq[ty, tx] = peq_flat[local_r * 256 + tx]
        else:
            smem_peq[ty, tx] = 0

    # Load Sequence
    tid = ty * 16 + tx
    total_threads = 256
    total_items = 64 * L
    
    for i in range(tid, total_items, total_threads):
        c_idx = i // L         
        char_idx = i % L       
        g_c_load = start_col + block_col_start + c_idx
        
        if g_c_load < N_batch:
            local_c = g_c_load - seq_batch_start
            smem_seq[c_idx, char_idx] = seq_transposed[char_idx * seq_batch_width + local_c]
        else:
            smem_seq[c_idx, char_idx] = 99 
            
    cuda.syncthreads()

    # ---------------------------------------------------------------------
    # Compute
    # ---------------------------------------------------------------------
    if (block_row_start + ty) >= num_rows:
        return

    loc_peq_0 = smem_peq[ty, 0]
    loc_peq_1 = smem_peq[ty, 1]
    loc_peq_2 = smem_peq[ty, 2]
    loc_peq_3 = smem_peq[ty, 3]

    PV_1 = uint32(0xFFFFFFFF); MV_1 = uint32(0); score_1 = L
    PV_2 = uint32(0xFFFFFFFF); MV_2 = uint32(0); score_2 = L
    PV_3 = uint32(0xFFFFFFFF); MV_3 = uint32(0); score_3 = L
    PV_4 = uint32(0xFFFFFFFF); MV_4 = uint32(0); score_4 = L

    if L == 32: hb_mask = uint32(1) << 31
    else:       hb_mask = uint32(1) << (L - 1)

    for k in range(L):
        c1 = smem_seq[local_col_base + 0, k]
        c2 = smem_seq[local_col_base + 1, k]
        c3 = smem_seq[local_col_base + 2, k]
        c4 = smem_seq[local_col_base + 3, k]

        # Col 1
        Eq = loc_peq_3
        if c1 == 0: Eq = loc_peq_0
        elif c1 == 1: Eq = loc_peq_1
        elif c1 == 2: Eq = loc_peq_2
        X = Eq | MV_1
        sum_val = uint32((X & PV_1) + PV_1)
        D0 = (sum_val ^ PV_1) | X
        HN = PV_1 & D0
        HP = MV_1 | ~(PV_1 | D0)
        X2 = (HP << 1) | uint32(1)
        MV_1 = X2 & D0
        PV_1 = (HN << 1) | ~(X2 | D0)
        if (HP & hb_mask): score_1 += 1
        if (HN & hb_mask): score_1 -= 1

        # Col 2
        Eq = loc_peq_3
        if c2 == 0: Eq = loc_peq_0
        elif c2 == 1: Eq = loc_peq_1
        elif c2 == 2: Eq = loc_peq_2
        X = Eq | MV_2
        sum_val = uint32((X & PV_2) + PV_2)
        D0 = (sum_val ^ PV_2) | X
        HN = PV_2 & D0
        HP = MV_2 | ~(PV_2 | D0)
        X2 = (HP << 1) | uint32(1)
        MV_2 = X2 & D0
        PV_2 = (HN << 1) | ~(X2 | D0)
        if (HP & hb_mask): score_2 += 1
        if (HN & hb_mask): score_2 -= 1

        # Col 3
        Eq = loc_peq_3
        if c3 == 0: Eq = loc_peq_0
        elif c3 == 1: Eq = loc_peq_1
        elif c3 == 2: Eq = loc_peq_2
        X = Eq | MV_3
        sum_val = uint32((X & PV_3) + PV_3)
        D0 = (sum_val ^ PV_3) | X
        HN = PV_3 & D0
        HP = MV_3 | ~(PV_3 | D0)
        X2 = (HP << 1) | uint32(1)
        MV_3 = X2 & D0
        PV_3 = (HN << 1) | ~(X2 | D0)
        if (HP & hb_mask): score_3 += 1
        if (HN & hb_mask): score_3 -= 1

        # Col 4
        Eq = loc_peq_3
        if c4 == 0: Eq = loc_peq_0
        elif c4 == 1: Eq = loc_peq_1
        elif c4 == 2: Eq = loc_peq_2
        X = Eq | MV_4
        sum_val = uint32((X & PV_4) + PV_4)
        D0 = (sum_val ^ PV_4) | X
        HN = PV_4 & D0
        HP = MV_4 | ~(PV_4 | D0)
        X2 = (HP << 1) | uint32(1)
        MV_4 = X2 & D0
        PV_4 = (HN << 1) | ~(X2 | D0)
        if (HP & hb_mask): score_4 += 1
        if (HN & hb_mask): score_4 -= 1

    # ---------------------------------------------------------------------
    # Dense Store
    # ---------------------------------------------------------------------
    
    # Store 1
    gc = global_col_base + 0
    if gc < N_batch:
        distance_matrix[start_row + block_row_start + ty, gc] = score_1
            
    # Store 2
    gc = global_col_base + 1
    if gc < N_batch:
        distance_matrix[start_row + block_row_start + ty, gc] = score_2

    # Store 3
    gc = global_col_base + 2
    if gc < N_batch:
        distance_matrix[start_row + block_row_start + ty, gc] = score_3

    # Store 4
    gc = global_col_base + 3
    if gc < N_batch:
        distance_matrix[start_row + block_row_start + ty, gc] = score_4


# -------------------------------------------------------------------------
# Host Code Functions
# -------------------------------------------------------------------------

def build_peq_tables(seq_flat, N, L):
    """ Vectorized PEQ table build (UINT32) """
    peq_table = np.zeros(N * 256, dtype=np.uint32)
    seq_matrix_T = seq_flat.reshape(L, N)
    range_N = np.arange(N, dtype=np.uint64)
    
    for pos in range(L):
        chars = seq_matrix_T[pos, :] 
        bit_val = np.uint32(1) << np.uint32(pos)
        indices = (range_N * 256) + chars.astype(np.uint64)
        peq_table[indices] |= bit_val
            
    return peq_table

def pack_sequences(strings):
    """ Pack strings into flat 'latin1' bytes """
    N = len(strings)
    if N == 0: return np.array([], dtype=np.uint8), 0, 0
    L = len(strings[0])
    huge_string = "".join(strings)
    raw_bytes = huge_string.encode('latin1')
    seq_flat = np.frombuffer(raw_bytes, dtype=np.uint8).copy()
    seq_flat -= 48  # '0' is 48 in ascii. Converts '0'->0, '1'->1, etc.
    return seq_flat, L, N

def transpose_batch(seq_flat, batch_start, batch_size, L, N_total):
    """ Extract a batch and TRANSPOSE """
    if batch_size == 0: return np.zeros(0, dtype=np.uint8)
    start_idx = batch_start * L
    end_idx = start_idx + batch_size * L
    chunk = seq_flat[start_idx:end_idx]
    seq_matrix = chunk.reshape(batch_size, L)
    return seq_matrix.T.copy().ravel()

def solve_assignments_cuda(vectors, centers, batch_size=32768):
    K = len(centers)
    N = len(vectors)
    if N == 0 or K == 0: return np.array([], dtype=np.int32)

    vec_flat, L_v, _ = pack_sequences(vectors)
    cen_flat, L_c, _ = pack_sequences(centers)
    if L_v != L_c: raise ValueError("Length mismatch between vectors and centers.")
    L = L_v
    if L > 32: raise ValueError("Length > 32 not supported by kernel.")

    print(f"[Python GPU Cluster] K={K} centers, N={N} vectors. BatchSize={batch_size}")
    
    # 1. Prepare Centers (PEQ built once)
    c_seq_transposed = transpose_batch(cen_flat, 0, K, L, K)
    peq_flat_centers = build_peq_tables(c_seq_transposed, K, L)
    d_peq = cuda.to_device(peq_flat_centers)
    
    assignments = np.zeros(N, dtype=np.int32)
    
    threadsperblock = (16, 16)

    # We evaluate all K centers against BATCH_SIZE vectors in each loop
    # Tiling over K is usually not needed because K is small, but we follow the identical tile logic.
    tile_size_R = K
    tile_size_C = batch_size

    # Outer Loop: Vector Batches
    for v_batch_start in range(0, N, batch_size):
        v_batch_end = min(v_batch_start + batch_size, N)
        v_batch_len = v_batch_end - v_batch_start
        
        v_seq_transposed = transpose_batch(vec_flat, v_batch_start, v_batch_len, L, N)
        d_seq = cuda.to_device(v_seq_transposed)
        
        # Dense distance matrix for this batch
        d_distance_matrix = cuda.device_array((K, v_batch_len), dtype=np.uint8)
        
        blockspergrid_x = (v_batch_len + 63) // 64
        blockspergrid_y = (K + 15) // 16
        blockspergrid = (blockspergrid_x, blockspergrid_y)

        compute_distance_matrix_kernel[blockspergrid, threadsperblock](
            d_peq, d_seq, L, 
            0, 0, K, v_batch_len, 
            K, v_batch_len, 
            d_distance_matrix,
            0, 0, v_batch_len
        )
        
        h_distance_matrix = d_distance_matrix.copy_to_host()
        
        # CPU Argmin along the centers axis (axis=0)
        best_centers = np.argmin(h_distance_matrix, axis=0)
        assignments[v_batch_start : v_batch_end] = best_centers.astype(np.int32)
        
        # Free GPU mem for next batch
        d_distance_matrix = None
        d_seq = None

    return assignments

def load_lines(filename):
    with open(filename, 'r') as f:
        return [line.strip() for line in f if line.strip()]

def main():
    if len(sys.argv) < 4:
        print("Usage: python gpu_cluster.py <vectors.txt> <centers.txt> <output_assignments.bin> [batch_size]")
        sys.exit(1)

    vectors_file = sys.argv[1]
    centers_file = sys.argv[2]
    output_file = sys.argv[3]
    
    batch_size = 32768
    if len(sys.argv) > 4:
        batch_size = int(sys.argv[4])
        
    start_t = time.time()
    vectors = load_lines(vectors_file)
    centers = load_lines(centers_file)
    load_t = time.time() - start_t
    print(f"[Python GPU Cluster] Loaded data in {load_t:.4f}s")

    start_t = time.time()
    try:
        assignments = solve_assignments_cuda(vectors, centers, batch_size=batch_size)
    except Exception as e:
        print(f"[Error] CUDA computation failed: {e}")
        sys.exit(1)
    cuda_time = time.time() - start_t
    print(f"[Python GPU Cluster] CUDA Compute finished in {cuda_time:.4f}s")
    
    start_t = time.time()
    with open(output_file, 'wb') as f_out:
        f_out.write(assignments.tobytes())
    write_t = time.time() - start_t
    print(f"[Python GPU Cluster] Wrote assignments to {output_file} in {write_t:.4f}s")

if __name__ == "__main__":
    main()
