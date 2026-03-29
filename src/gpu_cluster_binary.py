import sys
import os
import numpy as np
from numba import cuda, uint64, uint8
import time
import warnings
from numba.core.errors import NumbaPerformanceWarning

# Suppress Numba performance warnings
warnings.simplefilter('ignore', category=NumbaPerformanceWarning)

# -------------------------------------------------------------------------
# CUDA Kernel: Myers' Bit-Parallel (Dense Distance Matrix Output, Binary 64-bit)
# -------------------------------------------------------------------------
@cuda.jit(cache=True)
def compute_distance_matrix_kernel_binary(cen_packed,       # (K,) uint64
                                          vec_packed,       # (N_total,) uint64
                                          L,                # Length
                                          mask,             # bitwise mask
                                          start_row,        # Global Row Offset
                                          start_col,        # Global Col Offset (v_batch_start)
                                          num_rows,         # Rows in current chunk (K)
                                          num_cols,         # Cols in current chunk (v_batch_len)
                                          distance_matrix): # (num_rows, num_cols) of uint8
    
    # Block Configuration: 16x16 threads
    # Each thread computes 1 Center (Row) x 4 Vectors (Columns)
    
    tx = cuda.threadIdx.x # 0..15
    ty = cuda.threadIdx.y # 0..15
    bx = cuda.blockIdx.x
    by = cuda.blockIdx.y
    
    local_row = by * 16 + ty
    local_col_base = bx * 64 + tx * 4

    # ---------------------------------------------------------------------
    # Shared Memory Allocation & Loading
    # ---------------------------------------------------------------------
    smem_row = cuda.shared.array(16, uint64)
    smem_col = cuda.shared.array(64, uint64)

    # Load Centers
    if tx == 0:
        local_r_load = by * 16 + ty
        global_r_load = start_row + local_r_load
        if local_r_load < num_rows:
            smem_row[ty] = cen_packed[global_r_load]
        else:
            smem_row[ty] = 0

    # Load Vectors
    tid = ty * 16 + tx
    if tid < 64:
        local_c_load = bx * 64 + tid
        global_c_load = start_col + local_c_load
        if local_c_load < num_cols:
            smem_col[tid] = vec_packed[global_c_load]
        else:
            smem_col[tid] = 0
            
    cuda.syncthreads()

    # ---------------------------------------------------------------------
    # Compute
    # ---------------------------------------------------------------------
    if local_row >= num_rows:
        return

    row_bits = smem_row[ty]
    peq_1 = row_bits
    peq_0 = (~row_bits) & mask

    c1_bits = smem_col[local_col_base + 0]
    c2_bits = smem_col[local_col_base + 1]
    c3_bits = smem_col[local_col_base + 2]
    c4_bits = smem_col[local_col_base + 3]

    PV_1 = ~uint64(0); MV_1 = uint64(0); score_1 = L
    PV_2 = ~uint64(0); MV_2 = uint64(0); score_2 = L
    PV_3 = ~uint64(0); MV_3 = uint64(0); score_3 = L
    PV_4 = ~uint64(0); MV_4 = uint64(0); score_4 = L

    if L == 64: hb_mask = uint64(1) << uint64(63)
    else:       hb_mask = uint64(1) << uint64(L - 1)

    for k in range(L):
        bit_mask = uint64(1) << uint64(k)
        
        # Col 1
        Eq = peq_1 if (c1_bits & bit_mask) else peq_0
        X = Eq | MV_1
        sum_val = uint64((X & PV_1) + PV_1)
        D0 = (sum_val ^ PV_1) | X
        HN = PV_1 & D0
        HP = MV_1 | ~(PV_1 | D0)
        X2 = (HP << uint64(1)) | uint64(1)
        MV_1 = X2 & D0
        PV_1 = (HN << uint64(1)) | ~(X2 | D0)
        if (HP & hb_mask): score_1 += 1
        if (HN & hb_mask): score_1 -= 1

        # Col 2
        Eq = peq_1 if (c2_bits & bit_mask) else peq_0
        X = Eq | MV_2
        sum_val = uint64((X & PV_2) + PV_2)
        D0 = (sum_val ^ PV_2) | X
        HN = PV_2 & D0
        HP = MV_2 | ~(PV_2 | D0)
        X2 = (HP << uint64(1)) | uint64(1)
        MV_2 = X2 & D0
        PV_2 = (HN << uint64(1)) | ~(X2 | D0)
        if (HP & hb_mask): score_2 += 1
        if (HN & hb_mask): score_2 -= 1

        # Col 3
        Eq = peq_1 if (c3_bits & bit_mask) else peq_0
        X = Eq | MV_3
        sum_val = uint64((X & PV_3) + PV_3)
        D0 = (sum_val ^ PV_3) | X
        HN = PV_3 & D0
        HP = MV_3 | ~(PV_3 | D0)
        X2 = (HP << uint64(1)) | uint64(1)
        MV_3 = X2 & D0
        PV_3 = (HN << uint64(1)) | ~(X2 | D0)
        if (HP & hb_mask): score_3 += 1
        if (HN & hb_mask): score_3 -= 1

        # Col 4
        Eq = peq_1 if (c4_bits & bit_mask) else peq_0
        X = Eq | MV_4
        sum_val = uint64((X & PV_4) + PV_4)
        D0 = (sum_val ^ PV_4) | X
        HN = PV_4 & D0
        HP = MV_4 | ~(PV_4 | D0)
        X2 = (HP << uint64(1)) | uint64(1)
        MV_4 = X2 & D0
        PV_4 = (HN << uint64(1)) | ~(X2 | D0)
        if (HP & hb_mask): score_4 += 1
        if (HN & hb_mask): score_4 -= 1

    # ---------------------------------------------------------------------
    # Dense Store
    # ---------------------------------------------------------------------
    if local_col_base + 0 < num_cols:
        distance_matrix[local_row, local_col_base + 0] = score_1
    if local_col_base + 1 < num_cols:
        distance_matrix[local_row, local_col_base + 1] = score_2
    if local_col_base + 2 < num_cols:
        distance_matrix[local_row, local_col_base + 2] = score_3
    if local_col_base + 3 < num_cols:
        distance_matrix[local_row, local_col_base + 3] = score_4


# -------------------------------------------------------------------------
# Host Code Functions
# -------------------------------------------------------------------------

def pack_binary_sequences(strings):
    """ Pack binary strings ('0' and '1') into flat uint64 array """
    N = len(strings)
    if N == 0: return np.array([], dtype=np.uint64), 0
    L = len(strings[0])
    
    if L > 64:
        raise ValueError(f"[Python GPU] String length L={L} exceeds max 64.")

    huge_string = "".join(strings).encode('ascii')
    arr = np.frombuffer(huge_string, dtype=np.uint8).reshape(N, L) - 48
    
    packed = np.zeros(N, dtype=np.uint64)
    for pos in range(L):
        packed |= arr[:, pos].astype(np.uint64) << np.uint64(pos)
        
    return packed, L

def solve_assignments_cuda_binary(vectors, centers, batch_size=32768):
    K = len(centers)
    N = len(vectors)
    if N == 0 or K == 0: return np.array([], dtype=np.int32)

    vec_packed, L_v = pack_binary_sequences(vectors)
    cen_packed, L_c = pack_binary_sequences(centers)
    
    if L_v != L_c: raise ValueError("Length mismatch between vectors and centers.")
    L = L_v

    print(f"[Python GPU Cluster] K={K} centers, N={N} vectors. BatchSize={batch_size}")
    
    if L == 64: mask = np.uint64(-1)
    else:       mask = np.uint64((1 << L) - 1)
    
    d_cen = cuda.to_device(cen_packed)
    d_vec = cuda.to_device(vec_packed)
    
    assignments = np.zeros(N, dtype=np.int32)
    threadsperblock = (16, 16)

    # Outer Loop: Vector Batches
    for v_batch_start in range(0, N, batch_size):
        v_batch_end = min(v_batch_start + batch_size, N)
        v_batch_len = v_batch_end - v_batch_start
        
        # Dense distance matrix for this local chunk
        d_distance_matrix = cuda.device_array((K, v_batch_len), dtype=np.uint8)
        
        blockspergrid_x = (v_batch_len + 63) // 64
        blockspergrid_y = (K + 15) // 16
        blockspergrid = (blockspergrid_x, blockspergrid_y)

        compute_distance_matrix_kernel_binary[blockspergrid, threadsperblock](
            d_cen, d_vec, L, mask,
            0, v_batch_start, K, v_batch_len, 
            d_distance_matrix
        )
        
        h_distance_matrix = d_distance_matrix.copy_to_host()
        
        # CPU Argmin along the centers axis
        best_centers = np.argmin(h_distance_matrix, axis=0)
        assignments[v_batch_start : v_batch_end] = best_centers.astype(np.int32)
        
        d_distance_matrix = None

    return assignments

def load_lines(filename):
    with open(filename, 'r') as f:
        return [line.strip() for line in f if line.strip()]

def main():
    if len(sys.argv) < 4:
        print("Usage: python gpu_cluster_binary.py <vectors.txt> <centers.txt> <output_assignments.bin> [batch_size]")
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
        assignments = solve_assignments_cuda_binary(vectors, centers, batch_size=batch_size)
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
