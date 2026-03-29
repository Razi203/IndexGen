import numpy as np
from numba import cuda, uint64, int32
import math
import warnings
from numba.core.errors import NumbaPerformanceWarning

# Suppress Numba performance warnings
warnings.simplefilter('ignore', category=NumbaPerformanceWarning)

# -------------------------------------------------------------------------
# CUDA Kernel: Clear Buffer (Reset Counter)
# -------------------------------------------------------------------------
@cuda.jit(cache=True)
def reset_counter_kernel(counter):
    counter[0] = 0

# -------------------------------------------------------------------------
# CUDA Kernel: Myers' Bit-Parallel (Atomic Sparse Output, Binary 64-bit)
# -------------------------------------------------------------------------
@cuda.jit(cache=True)
def compute_chunk_kernel_binary(seq_packed,      # (N,) uint64 array
                                L,               # Length of strings (<=64)
                                threshold,       # Distance threshold D
                                mask,            # Precomputed mask for length L
                                start_row,       # Global Row Offset of this TILE
                                start_col,       # Global Col Offset of this TILE
                                num_rows,        # Number of rows in this TILE
                                num_cols,        # Number of cols in this TILE
                                N,               # Total Number of sequences (Global)
                                global_counter,  # Device array [1] for atomic count
                                edge_buffer,     # Device array [MAX_EDGES, 2] for coords
                                max_edges):      # Capacity of edge_buffer
    
    # Block Configuration: 16x16 threads
    # Each thread computes 1 Row x 4 Columns
    
    tx = cuda.threadIdx.x # 0..15
    ty = cuda.threadIdx.y # 0..15
    bx = cuda.blockIdx.x
    by = cuda.blockIdx.y
    
    block_row_start = by * 16
    block_col_start = bx * 64
    
    # Thread's specific Global Row
    global_row = start_row + block_row_start + ty
    
    # Thread's specific Global Column Start
    local_col_base = tx * 4
    global_col_base = start_col + block_col_start + local_col_base

    # ---------------------------------------------------------------------
    # Shared Memory Allocation & Loading
    # ---------------------------------------------------------------------
    smem_row = cuda.shared.array(16, uint64)
    smem_col = cuda.shared.array(64, uint64)

    # Load Row (Center) bits
    if tx == 0:
        g_r_load = start_row + block_row_start + ty
        if g_r_load < N:
            smem_row[ty] = seq_packed[g_r_load]
        else:
            smem_row[ty] = 0

    # Load Column (Vector) bits
    tid = ty * 16 + tx
    if tid < 64:
        g_c_load = start_col + block_col_start + tid
        if g_c_load < N:
            smem_col[tid] = seq_packed[g_c_load]
        else:
            smem_col[tid] = 0
            
    cuda.syncthreads()

    # ---------------------------------------------------------------------
    # Compute
    # ---------------------------------------------------------------------
    if (block_row_start + ty) >= num_rows:
        return

    row_bits = smem_row[ty]
    # For a binary sequence, PEQ table is simply the sequence bits for '1',
    # and NOT the bits for '0'.
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
    # Atomic Store (SPARSE)
    # ---------------------------------------------------------------------
    
    # Store 1
    gc = global_col_base + 0
    if gc > global_row and gc < N:
        if score_1 < threshold:
            idx = cuda.atomic.add(global_counter, 0, 1)
            if idx < max_edges:
                edge_buffer[idx, 0] = global_row
                edge_buffer[idx, 1] = gc
            
    # Store 2
    gc = global_col_base + 1
    if gc > global_row and gc < N:
        if score_2 < threshold:
            idx = cuda.atomic.add(global_counter, 0, 1)
            if idx < max_edges:
                edge_buffer[idx, 0] = global_row
                edge_buffer[idx, 1] = gc

    # Store 3
    gc = global_col_base + 2
    if gc > global_row and gc < N:
        if score_3 < threshold:
            idx = cuda.atomic.add(global_counter, 0, 1)
            if idx < max_edges:
                edge_buffer[idx, 0] = global_row
                edge_buffer[idx, 1] = gc

    # Store 4
    gc = global_col_base + 3
    if gc > global_row and gc < N:
        if score_4 < threshold:
            idx = cuda.atomic.add(global_counter, 0, 1)
            if idx < max_edges:
                edge_buffer[idx, 0] = global_row
                edge_buffer[idx, 1] = gc

# -------------------------------------------------------------------------
# Host Code
# -------------------------------------------------------------------------

def pack_binary_sequences(strings):
    """
    Pack binary strings ('0' and '1') into a flat 1D uint64 array.
    """
    N = len(strings)
    if N == 0:
        return np.array([], dtype=np.uint64), 0
    L = len(strings[0])
    
    if L > 64:
        raise ValueError(f"[Python GPU] String length L={L} exceeds max supported length 64.")
        
    huge_string = "".join(strings).encode('ascii')
    arr = np.frombuffer(huge_string, dtype=np.uint8).reshape(N, L) - 48
    
    packed = np.zeros(N, dtype=np.uint64)
    for pos in range(L):
        packed |= arr[:, pos].astype(np.uint64) << np.uint64(pos)
        
    return packed, L

def solve_edit_distance_cuda_binary(strings, threshold, tile_size=16384):
    N = len(strings)
    if N == 0: return []

    print("[Python GPU] Preprocessing binary codes for 64-bit uint packing...")
    seq_packed, L = pack_binary_sequences(strings)
    
    if L == 64:
        mask = np.uint64(-1)
    else:
        mask = np.uint64((1 << L) - 1)
        
    print(f"[Python GPU] Transferring packed dataset (N={N}, size={seq_packed.nbytes / (1024*1024):.2f} MB) to GPU...")
    d_seq = cuda.to_device(seq_packed)

    # Sparse output config
    MAX_EDGES_PER_BUFFER = 5_000_000 
    
    print(f"[Python GPU] Allocating Adjacency List for N={N}...")
    adj_list = [[] for _ in range(N)]

    num_streams = 2
    streams = [cuda.stream() for _ in range(num_streams)]
    
    # Buffers
    h_counters = [cuda.pinned_array(1, dtype=np.int32) for _ in range(num_streams)]
    h_edges    = [cuda.pinned_array((MAX_EDGES_PER_BUFFER, 2), dtype=np.int32) for _ in range(num_streams)]
    
    d_counters = [cuda.device_array(1, dtype=np.int32) for _ in range(num_streams)]
    d_edges    = [cuda.device_array((MAX_EDGES_PER_BUFFER, 2), dtype=np.int32) for _ in range(num_streams)]

    threadsperblock = (16, 16)
    blockspergrid_x = (tile_size + 63) // 64
    blockspergrid_y = (tile_size + 15) // 16
    blockspergrid = (blockspergrid_x, blockspergrid_y)

    print(f"[Python GPU] Starting Computation: {N} strings, L={L}, Threshold={threshold}")
    
    tiles = []
    
    # Iterate tiles strictly for the upper triangle
    for t_r in range(0, N, tile_size):
        for t_c in range(t_r, N, tile_size):
            tiles.append((t_r, t_c))
            
    if not tiles:
        return adj_list
        
    active_tasks = [] 
    
    def process_completed_tile(task_info):
        s_idx, _, _ = task_info
        streams[s_idx].synchronize()
        count = h_counters[s_idx][0]
        if count > MAX_EDGES_PER_BUFFER:
            count = MAX_EDGES_PER_BUFFER
        if count > 0:
            valid_edges = h_edges[s_idx][:count]
            for i in range(count):
                r, c = valid_edges[i]
                adj_list[r].append(int(c))
                adj_list[c].append(int(r))

    tile_idx = 0
    while tile_idx < len(tiles) or len(active_tasks) > 0:
        while len(active_tasks) < num_streams and tile_idx < len(tiles):
            stream_id = tile_idx % num_streams
            stream = streams[stream_id]
            t_r, t_c = tiles[tile_idx]
            
            cur_r_len = min(t_r + tile_size, N) - t_r
            cur_c_len = min(t_c + tile_size, N) - t_c
            
            if cur_r_len > 0 and cur_c_len > 0:
                reset_counter_kernel[1, 1, stream](d_counters[stream_id])
                
                compute_chunk_kernel_binary[blockspergrid, threadsperblock, stream](
                    d_seq, L, threshold, mask,
                    t_r, t_c, cur_r_len, cur_c_len, N, 
                    d_counters[stream_id], d_edges[stream_id], MAX_EDGES_PER_BUFFER
                )
                
                d_counters[stream_id].copy_to_host(h_counters[stream_id], stream=stream)
                d_edges[stream_id].copy_to_host(h_edges[stream_id], stream=stream)
                active_tasks.append((stream_id, (t_r, t_c), stream_id))
            
            tile_idx += 1

        if active_tasks:
            task = active_tasks.pop(0)
            process_completed_tile(task)
            
    return adj_list
