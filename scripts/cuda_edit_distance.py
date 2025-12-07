import numpy as np
from numba import cuda, uint32, int32, uint8
import math
import warnings
from numba.core.errors import NumbaPerformanceWarning

# Suppress Numba performance warnings
warnings.simplefilter('ignore', category=NumbaPerformanceWarning)

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
def compute_chunk_kernel(peq_flat,       # (BATCH_ROWS, 256)
                         seq_transposed, # (L, BATCH_COLS) flattened
                         L,              # Length of strings
                         threshold,      # Distance threshold D
                         start_row,      # Global Row Offset of this TILE
                         start_col,      # Global Col Offset of this TILE
                         num_rows,       # Number of rows in this TILE
                         num_cols,       # Number of cols in this TILE
                         N,              # Total Number of sequences (Global)
                         global_counter, # Device array [1] for atomic count
                         edge_buffer,    # Device array [MAX_EDGES, 2] for coords
                         max_edges,      # Capacity of edge_buffer
                         peq_batch_start, # Global index of the first row in peq_flat
                         seq_batch_start, # Global index of the first col in seq_transposed
                         seq_batch_width):# Width of the current sequence batch (for stride)
    
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
    smem_peq = cuda.shared.array((16, 4), uint32)
    smem_seq = cuda.shared.array((64, 33), uint8) 

    # Load PEQ
    if tx < 4:
        g_r_load = start_row + block_row_start + ty
        if g_r_load < N:
            # Map global row to local PEQ index
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
        
        if g_c_load < N:
            # Map global col to local Seq index
            local_c = g_c_load - seq_batch_start
            # Sequence Batch is flattened (L, seq_batch_width). 
            # Stride is seq_batch_width.
            # Index = char_idx * seq_batch_width + local_c
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
        # We need to access smem_seq in a stride=1 way if possible, but c1..c4 are from different columns?
        # local_col_base = tx * 4. 
        # c1 is from column (local_col_base + 0). 
        # smem_seq shape (64, 33). row=col_idx, col=char_idx.
        # So smem_seq[col_idx, k]. This is correct.
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

def pack_sequences(strings):
    """ Pack strings into flat 'latin1' bytes, no transpose yet. """
    N = len(strings)
    if N == 0: return np.array([], dtype=np.uint8), 0
    L = len(strings[0])
    huge_string = "".join(strings)
    raw_bytes = huge_string.encode('latin1')
    seq_flat = np.frombuffer(raw_bytes, dtype=np.uint8).copy()
    seq_flat -= 48 
    return seq_flat, L, N

def transpose_batch(seq_flat, batch_start, batch_size, L, N_total):
    """ 
    Extract a batch of sequences from the flat array and TRANSPOSE them.
    Returns: flattened array of shape (batch_size, L).T -> (L, batch_size) 
    """
    if batch_size == 0: return np.zeros(0, dtype=np.uint8)
    
    # seq_flat is (N_total * L)
    start_idx = batch_start * L
    end_idx = start_idx + batch_size * L
    
    # Extract chunk
    chunk = seq_flat[start_idx:end_idx]
    
    # Reshape to (BatchSize, L)
    seq_matrix = chunk.reshape(batch_size, L)
    
    # Transpose to (L, BatchSize)
    # Ravel for GPU (col-major logically)
    return seq_matrix.T.copy().ravel()

def solve_edit_distance_cuda(strings, threshold, tile_size=16384, max_gpu_memory_gb=9.0):
    N = len(strings)
    if N == 0: return []

    print(f"[Python GPU] Preprocessing data for GPU (Memory Limit: {max_gpu_memory_gb} GB)...")
    
    # 1. Flatten all strings on Host (This still requires Host RAM)
    seq_flat_all, L, _ = pack_sequences(strings)
    
    if L > 32: raise ValueError("[Python GPU] This optimized 32-bit kernel only supports lengths <= 32.")

    # 2. Calculate Batch Size
    # Memory Usage Formula:
    # PEQ (per item) = 256 * 4 bytes = 1 KB
    # Seq Transposed (per item) = L (32) bytes = 0.03 KB
    # Buffer Overhead = 80 MB (fixed)
    # Safely: BatchSize * 1.1 KB < (Limit - 100MB)
    
    limit_bytes = max_gpu_memory_gb * 1024**3
    buffer_bytes = 200 * 1024**2 # 200 MB reserve for buffers/overhead
    
    mem_per_row = 1024 + 64 # Peaucellier + Seq + margin
    
    available_for_batch = limit_bytes - buffer_bytes
    if available_for_batch < 0: available_for_batch = 100 * 1024**2 # Minimal fallback
    
    # We need PEQ for Batch_Rows and Seq for Batch_Cols. 
    # If we assume we process square batches BatchSize x BatchSize.
    # We load BatchSize items (PEQ) and BatchSize items (Seq).
    # Total items loaded: 2 * BatchSize?
    # Actually, we can just load one Batch of PEQ (Rows) and iterate over chunks of Cols.
    # The 'Col Chunk' can be smaller if needed, but simplest is square blocks.
    
    batch_size = int(available_for_batch // mem_per_row)
    # Clamp batch size to reasonable limits / alignment
    batch_size = min(batch_size, N)
    batch_size = ((batch_size + 63) // 64) * 64 # Align to 64 (Ceiling)
    if batch_size < 1024: batch_size = 1024
    
    print(f"[Python GPU] Calculated Batch Size: {batch_size} sequences (Target VRAM Usage per batch)")

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

    print(f"[Python GPU] Starting Computation: {N} strings, BatchSize={batch_size}")
    
    # ---------------------------------------------------------------------
    # Outer Loop: Row Batches (PEQ loaded once per row-batch)
    # ---------------------------------------------------------------------
    for r_batch_start in range(0, N, batch_size):
        r_batch_end = min(r_batch_start + batch_size, N)
        r_batch_len = r_batch_end - r_batch_start
        
        # 1. Build and Upload PEQ for this Row Batch
        # We need to extract the 'transposed' version of this batch just for PEQ build
        # or build PEQ from the flat linear strings?
        # Our build_peq_tables expects transposed input. 
        batch_seq_transposed = transpose_batch(seq_flat_all, r_batch_start, r_batch_len, L, N)
        peq_flat_batch = build_peq_tables(batch_seq_transposed, r_batch_len, L)
        d_peq = cuda.to_device(peq_flat_batch) # Moved to GPU
        
        # -----------------------------------------------------------------
        # Inner Loop: Col Batches (Seq loaded once per col-batch)
        # -----------------------------------------------------------------
        # Optimization: We only care about upper triangle (c > r).
        # So begin c_batch from r_batch_start.
        
        for c_batch_start in range(r_batch_start, N, batch_size):
            c_batch_end = min(c_batch_start + batch_size, N)
            c_batch_len = c_batch_end - c_batch_start
            
            # 2. Build and Upload Seq for this Col Batch
            # (Reuse batch_seq_transposed if r_batch == c_batch? Optimization for later)
            if c_batch_start == r_batch_start and r_batch_len == c_batch_len:
                 d_seq = cuda.to_device(batch_seq_transposed)
            else:
                 c_seq_transposed = transpose_batch(seq_flat_all, c_batch_start, c_batch_len, L, N)
                 d_seq = cuda.to_device(c_seq_transposed)
                 
            # -------------------------------------------------------------
            # Tiling Loop: Process this (RowBatch x ColBatch) block
            # -------------------------------------------------------------
            # We tile within the global coordinates, but restricted to this batch intersection
            
            tiles = []
            
            # Iterate tiles within this batch window
            # Range: [r_batch_start, r_batch_end)
            for t_r in range(r_batch_start, r_batch_end, tile_size):
                # Ensure we only process upper triangle
                # Start col must be at least t_r (actually max(t_r, c_batch_start))
                start_c_scan = max(t_r, c_batch_start)
                
                for t_c in range(start_c_scan, c_batch_end, tile_size):
                    tiles.append((t_r, t_c))
            
            if not tiles: continue
            
            active_tasks = [] 
            
            # Helper to process results
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
                    
                    # Compute lengths for this specific tile
                    # Must be clamped to both Batch End and Global N (redundant but safe)
                    cur_r_len = min(t_r + tile_size, r_batch_end) - t_r
                    cur_c_len = min(t_c + tile_size, c_batch_end) - t_c
                    
                    if cur_r_len > 0 and cur_c_len > 0:
                        reset_counter_kernel[1, 1, stream](d_counters[stream_id])
                        
                        compute_chunk_kernel[blockspergrid, threadsperblock, stream](
                            d_peq, d_seq, L, threshold,
                            t_r, t_c, cur_r_len, cur_c_len, N, 
                            d_counters[stream_id], d_edges[stream_id], MAX_EDGES_PER_BUFFER,
                            r_batch_start, c_batch_start, c_batch_len
                        )
                        d_counters[stream_id].copy_to_host(h_counters[stream_id], stream=stream)
                        d_edges[stream_id].copy_to_host(h_edges[stream_id], stream=stream)
                        active_tasks.append((stream_id, (t_r, t_c), stream_id))
                    
                    tile_idx += 1

                if active_tasks:
                    task = active_tasks.pop(0)
                    process_completed_tile(task)
            
            # Cleanup Device Memory for this Col Batch
            d_seq = None # Allow GC/Recycle
            
        # Cleanup Device Memory for this Row Batch
        d_peq = None

    return adj_list