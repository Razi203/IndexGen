import numpy as np
from numba import cuda, uint32, int32, uint8
import math

# -------------------------------------------------------------------------
# CUDA Kernel: Clear Buffer (Reset Counter)
# -------------------------------------------------------------------------
@cuda.jit
def reset_counter_kernel(counter):
    counter[0] = 0

# -------------------------------------------------------------------------
# CUDA Kernel: Myers' Bit-Parallel (Atomic Sparse Output)
# -------------------------------------------------------------------------
@cuda.jit
def compute_chunk_kernel(peq_flat,       # (N, 256 * n_blocks)
                         seq_transposed, # Transposed Sequence Data (L, N)
                         L,              # Length of strings
                         threshold,      # Distance threshold D
                         start_row,      # Global Row Offset
                         start_col,      # Global Col Offset
                         num_rows,       # Number of rows in this chunk
                         num_cols,       # Number of cols in this chunk
                         N,              # Total Number of sequences
                         global_counter, # Device array [1] for atomic count
                         edge_buffer,    # Device array [MAX_EDGES, 2] for coords
                         max_edges):     # Capacity of edge_buffer
    
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
    # Shared Memory Allocation & Loading (Same as before)
    # ---------------------------------------------------------------------
    smem_peq = cuda.shared.array((16, 4), uint32)
    smem_seq = cuda.shared.array((64, 33), uint8) 

    # Load PEQ
    if tx < 4:
        g_r_load = start_row + block_row_start + ty
        if g_r_load < N:
            smem_peq[ty, tx] = peq_flat[g_r_load * 256 + tx]
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
        
        if g_c_load < N:
            smem_seq[c_idx, char_idx] = seq_transposed[char_idx * N + g_c_load]
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
    # Atomic Store (SPARSE)
    # ---------------------------------------------------------------------
    # Only if edge exists (score < threshold) AND strictly upper triangle
    
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

def pack_and_transpose_sequences(strings):
    """ Optimized packing + TRANSPOSE for "0123" strings. """
    N = len(strings)
    if N == 0: return np.array([], dtype=np.uint8), 0
    L = len(strings[0])
    huge_string = "".join(strings)
    raw_bytes = huge_string.encode('latin1')
    seq_flat = np.frombuffer(raw_bytes, dtype=np.uint8).copy()
    seq_flat -= 48 
    seq_matrix = seq_flat.reshape(N, L)
    return seq_matrix.T.copy().ravel(), L

def solve_edit_distance_cuda(strings, threshold, tile_size=16384):
    N = len(strings)
    if N == 0: return []

    print("Preprocessing data for GPU (Sparse Output)...")
    seq_transposed, L = pack_and_transpose_sequences(strings)
    if L > 32: raise ValueError("This optimized 32-bit kernel only supports lengths <= 32.")
    peq_flat = build_peq_tables(seq_transposed, N, L)

    d_peq = cuda.to_device(peq_flat)
    d_seq = cuda.to_device(seq_transposed)
    
    # Sparse output config
    # 5 million edges per tile buffer (~40MB). 
    # Adjust based on expected density. For random strings + low threshold, this is plenty.
    MAX_EDGES_PER_BUFFER = 5_000_000 
    
    print(f"Allocating Adjacency List for N={N}...")
    adj_list = [[] for _ in range(N)]

    num_streams = 2
    streams = [cuda.stream() for _ in range(num_streams)]
    
    # Buffers now store (ROW, COL) pairs instead of dense matrix
    # Pinned memory for fast transfer
    h_counters = [cuda.pinned_array(1, dtype=np.int32) for _ in range(num_streams)]
    h_edges    = [cuda.pinned_array((MAX_EDGES_PER_BUFFER, 2), dtype=np.int32) for _ in range(num_streams)]
    
    # Device memory
    d_counters = [cuda.device_array(1, dtype=np.int32) for _ in range(num_streams)]
    d_edges    = [cuda.device_array((MAX_EDGES_PER_BUFFER, 2), dtype=np.int32) for _ in range(num_streams)]

    threadsperblock = (16, 16)
    blockspergrid_x = (tile_size + 63) // 64
    blockspergrid_y = (tile_size + 15) // 16
    blockspergrid = (blockspergrid_x, blockspergrid_y)

    print(f"Starting Computation: Sparse N={N}")
    
    tiles = []
    for r_start in range(0, N, tile_size):
        for c_start in range(r_start, N, tile_size):
            tiles.append((r_start, c_start))
            
    active_tasks = [] 

    def process_completed_tile(task_info):
        s_idx, _, _ = task_info
        streams[s_idx].synchronize()
        
        # 1. Read how many edges were found
        count = h_counters[s_idx][0]
        
        if count > MAX_EDGES_PER_BUFFER:
            print(f"WARNING: Edge buffer overflow! Found {count} but cap is {MAX_EDGES_PER_BUFFER}. Results truncated.")
            count = MAX_EDGES_PER_BUFFER
            
        if count > 0:
            # 2. Process edges (Sparse read from pinned memory)
            # Only read the valid 'count' rows
            valid_edges = h_edges[s_idx][:count]
            
            for i in range(count):
                r, c = valid_edges[i]
                adj_list[r].append(int(c))
                adj_list[c].append(int(r)) # Symmetry

    tile_idx = 0
    while tile_idx < len(tiles) or len(active_tasks) > 0:
        while len(active_tasks) < num_streams and tile_idx < len(tiles):
            stream_id = tile_idx % num_streams
            stream = streams[stream_id]
            r_start, c_start = tiles[tile_idx]
            
            # 1. Reset Counter on GPU
            reset_counter_kernel[1, 1, stream](d_counters[stream_id])
            
            r_len = min(r_start + tile_size, N) - r_start
            c_len = min(c_start + tile_size, N) - c_start
            
            # 2. Launch Compute
            compute_chunk_kernel[blockspergrid, threadsperblock, stream](
                d_peq, d_seq, L, threshold,
                r_start, c_start, r_len, c_len, N, 
                d_counters[stream_id], d_edges[stream_id], MAX_EDGES_PER_BUFFER
            )
            
            # 3. Async Copy Back
            # Copy counter first
            d_counters[stream_id].copy_to_host(h_counters[stream_id], stream=stream)
            # Copy edge buffer (We copy the whole buffer because we don't know size yet on host, 
            # unless we sync. Copying 40MB is still way faster than 256MB dense)
            # Optimization: If we trust PCIe speed, copying 40MB is fine.
            d_edges[stream_id].copy_to_host(h_edges[stream_id], stream=stream)
            
            active_tasks.append((stream_id, (r_start, c_start), stream_id))
            tile_idx += 1

        if active_tasks:
            task = active_tasks.pop(0)
            process_completed_tile(task)

    return adj_list