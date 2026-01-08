import os
import glob
import json
import subprocess
import time
import ast
import random
import re
import pandas as pd
import numpy as np
import xgboost as xgb
from datetime import datetime

import urllib.request
import urllib.parse


# ==========================================
# CONFIGURATION
# ==========================================
BINARY_PATH = "./IndexGen"       # Path to your C++ executable
HISTORY_FILE = "history.csv"     # Database of all runs

TIMESTAMP = datetime.now().strftime("%H-%M-%S")
BASE_OUTPUT_DIR = "Test/31-12-25/experiments-" + TIMESTAMP  # Where to save run artifacts
BATCH_SIZE = 50                  # How many runs per iteration (keep small for testing)
TOTAL_LOOPS = 100                  # How many times to repeat the whole Train->Gen->Run cycle
THREADS_PER_RUN = 32

# Genetic Algorithm Settings
GA_POPULATION = 20000
GA_GENERATIONS = 50

# Model Settings
MODEL_LEARNING_RATE = 0.001
MODEL_MAX_DEPTH = 30
MODEL_N_ESTIMATORS = 4000

# Problem Constraints
LEN_COL_PERM = 11  # Permutation 0..10
LEN_ROW_PERM = 8   # Permutation 0..7
LEN_BIAS = 11      # Values 0..3


TOPIC = "IndexGen-Naranja"

def pingme(msg="Job Done!"):
    """Sends a notification via ntfy.sh"""
    try:
        url = f"https://ntfy.sh/{TOPIC}"
        data = msg.encode('utf-8')
        req = urllib.request.Request(url, data=data, method='POST')
        with urllib.request.urlopen(req) as response:
            pass # Request successful
    except Exception as e:
        print(f"Warning: Failed to send notification: {e}")

# ==========================================
# 1. DATA MANAGEMENT
# ==========================================
def parse_vector(s):
    """Safely parses string '[1, 2]' or list object to numpy array."""
    if isinstance(s, (list, np.ndarray)):
        return np.array(s)
    if isinstance(s, str):
        try:
            return np.array(ast.literal_eval(s))
        except:
            return np.array([])
    return np.array([])

def load_history():
    if not os.path.exists(HISTORY_FILE):
        print(f"No history file found at {HISTORY_FILE}. Creating empty one.")
        df = pd.DataFrame(columns=['col_perm', 'row_perm', 'bias', 'score'])
        df.to_csv(HISTORY_FILE, index=False)
        return df, None, None

    df = pd.read_csv(HISTORY_FILE)
    if len(df) < 10:
        print("Not enough data to train model yet.")
        return df, None, None

    # Parse vectors
    X_cols = np.stack(df['col_perm'].apply(parse_vector).values)
    X_rows = np.stack(df['row_perm'].apply(parse_vector).values)
    X_bias = np.stack(df['bias'].apply(parse_vector).values)
    
    X = np.hstack([X_cols, X_rows, X_bias])
    y = df['score'].values
    return df, X, y

def save_result(col_perm, row_perm, bias, score):
    """Appends a single result to CSV."""
    new_row = pd.DataFrame([{
        'col_perm': str(col_perm),
        'row_perm': str(row_perm),
        'bias': str(bias),
        'score': score
    }])
    
    # Append to file directly to avoid data loss on crash
    new_row.to_csv(HISTORY_FILE, mode='a', header=not os.path.exists(HISTORY_FILE), index=False)

# ==========================================
# 2. MACHINE LEARNING CORE
# ==========================================
def train_model(X, y):
    print(f"Training Surrogate Model on {len(y)} samples...")
    model = xgb.XGBRegressor(
        n_estimators=800, learning_rate=0.006, max_depth=10, n_jobs=-1, objective='reg:squarederror'
    )
    model.fit(X, y)
    return model

class Chromosome:
    def __init__(self, col_p, row_p, bias):
        self.col_p = np.array(col_p)
        self.row_p = np.array(row_p)
        self.bias = np.array(bias)
        self.predicted_score = 0.0
    
    def to_flat(self):
        return np.hstack([self.col_p, self.row_p, self.bias])

    def mutate(self):
        # Swap Col Perm
        if random.random() < 0.4:
            i, j = np.random.choice(LEN_COL_PERM, 2, replace=False)
            self.col_p[i], self.col_p[j] = self.col_p[j], self.col_p[i]
        # Swap Row Perm
        if random.random() < 0.4:
            i, j = np.random.choice(LEN_ROW_PERM, 2, replace=False)
            self.row_p[i], self.row_p[j] = self.row_p[j], self.row_p[i]
        # Mutate Bias
        if random.random() < 0.4:
            idx = np.random.randint(0, LEN_BIAS)
            self.bias[idx] = np.random.randint(0, 4)

def run_ga(model, best_historical_X):
    print("Running Genetic Search...")
    population = []
    
    # Seed population with best history + random
    seeds = min(len(best_historical_X), 100)
    for i in range(seeds):
        flat = best_historical_X[i]
        c = flat[:LEN_COL_PERM]
        r = flat[LEN_COL_PERM : LEN_COL_PERM + LEN_ROW_PERM]
        b = flat[LEN_COL_PERM + LEN_ROW_PERM:]
        population.append(Chromosome(c, r, b))
        
    while len(population) < GA_POPULATION:
        c = np.random.permutation(LEN_COL_PERM)
        r = np.random.permutation(LEN_ROW_PERM)
        b = np.random.randint(0, 4, size=LEN_BIAS)
        population.append(Chromosome(c, r, b))

    for gen in range(GA_GENERATIONS):
        X_pop = np.array([p.to_flat() for p in population])
        scores = model.predict(X_pop)
        
        # Elitism: Keep top 20%
        top_indices = np.argsort(scores)[-int(GA_POPULATION * 0.2):]
        parents = [population[i] for i in top_indices]
        
        new_pop = list(parents) # Keep parents
        while len(new_pop) < GA_POPULATION:
            p = random.choice(parents)
            child = Chromosome(p.col_p.copy(), p.row_p.copy(), p.bias.copy())
            child.mutate()
            new_pop.append(child)
        population = new_pop
    
    # Return best unique candidates
    X_final = np.array([p.to_flat() for p in population])
    final_scores = model.predict(X_final)
    sorted_idx = np.argsort(final_scores)[::-1]
    
    candidates = []
    hashes = set()
    for idx in sorted_idx:
        p = population[idx]
        p.predicted_score = final_scores[idx]
        h = p.to_flat().tobytes()
        if h not in hashes:
            hashes.add(h)
            candidates.append(p)
        if len(candidates) >= BATCH_SIZE:
            break
            
    return candidates

# ==========================================
# 3. EXECUTION ENGINE
# ==========================================
def generate_config_file(candidate, run_dir):
    config = {
        "dir": run_dir, # Crucial: Tool writes output here
        "core": {"lenStart": 11, "lenEnd": 11, "editDist": 3},
        "constraints": {"maxRun": 0, "minGC": 0, "maxGC": 0},
        "performance": {"threads": THREADS_PER_RUN, "saveInterval": 1800, "use_gpu": False, "max_gpu_memory_gb": 40},
        "method": {
            "name": "LinearCode",
            "linearCode": {
                "minHD": 3,
                "biasMode": "manual", "rowPermMode": "manual", "colPermMode": "manual",
                "bias": candidate.bias.tolist(),
                "rowPerm": candidate.row_p.tolist(),
                "colPerm": candidate.col_p.tolist(),
                "randomSeed": 0
            }
        },
        "verify": False
    }
    
    config_path = os.path.join(run_dir, "config.json")
    with open(config_path, 'w') as f:
        json.dump(config, f, indent=4)
    return config_path

def parse_output_score(run_dir):
    """Finds CodeSize-XXXXX.txt in run_dir and extracts the score."""
    # Pattern to match: CodeSize-0033430_CodeLen...
    search_pattern = os.path.join(run_dir, "CodeSize-*.txt")
    files = glob.glob(search_pattern)
    
    if not files:
        print(f"  [Error] No output file found in {run_dir}")
        return 0

    # Parse filename for score (e.g., CodeSize-0033430_...)
    filename = os.path.basename(files[0])
    match = re.search(r"CodeSize-(\d+)_", filename)
    if match:
        return int(match.group(1))
    
    # Fallback: Read file content if filename check fails
    with open(files[0], 'r') as f:
        content = f.read()
        # You can add logic here if the file contains the number inside
    return 0

def run_candidate(candidate, batch_id, run_id):
    # 1. Setup Directory
    run_dir = os.path.join(BASE_OUTPUT_DIR, f"batch_{batch_id}", f"run_{run_id}")
    os.makedirs(run_dir, exist_ok=True)
    
    # 2. Create Config
    config_path = generate_config_file(candidate, run_dir)
    
    # 3. Run Executable
    print(f"  > Running Batch {batch_id} | Job {run_id}...", end="", flush=True)
    start_time = time.time()
    
    try:
        # Run process silently, capturing stdout if needed for debugging
        result = subprocess.run(
            [BINARY_PATH, "--config", config_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        duration = time.time() - start_time
        
        # 4. Parse Result
        score = parse_output_score(run_dir)
        
        print(f" Done ({duration:.1f}s). Pred: {candidate.predicted_score:.2f} | Score: {score}")
        
        # 5. Save to History
        if score > 0:
            save_result(
                candidate.col_p.tolist(),
                candidate.row_perm.tolist() if hasattr(candidate, 'row_perm') else candidate.row_p.tolist(),
                candidate.bias.tolist(),
                score
            )
            return score
            
    except Exception as e:
        print(f" Failed: {e}")
        return 0

# ==========================================
# MAIN LOOP
# ==========================================
if __name__ == "__main__":
    if not os.path.exists(BINARY_PATH):
        print(f"Error: Executable {BINARY_PATH} not found.")
        # Create a dummy for testing purposes if you run this without the C++ tool
        # with open(BINARY_PATH, 'w') as f: f.write("echo 'Dummy'"); os.chmod(BINARY_PATH, 0o777)
    
    print(f"=== Starting Optimization Loop ({TOTAL_LOOPS} Cycles) ===")
    
    best_score_overall = 0
    best_loop = -1

    for loop in range(TOTAL_LOOPS):
        print(f"\n--- Cycle {loop+1}/{TOTAL_LOOPS} ---")
        
        # 1. Load Data
        df, X, y = load_history()
        
        if X is None:
            # Cold Start: Generate random candidates if no history
            print("Cold start: Generating random candidates.")
            candidates = []
            for _ in range(BATCH_SIZE):
                c = np.random.permutation(LEN_COL_PERM)
                r = np.random.permutation(LEN_ROW_PERM)
                b = np.random.randint(0, 4, size=LEN_BIAS)
                candidates.append(Chromosome(c, r, b))
        else:
            # AI Start: Train and Evolve
            model = train_model(X, y)
            best_idx = np.argsort(y)[-100:]
            candidates = run_ga(model, X[best_idx])
            
        print(f"candidates ready. Executing {len(candidates)} runs...")
        
        # 2. Run Batch
        best_batch_score = 0
        for i, cand in enumerate(candidates):
            score = run_candidate(cand, loop, i)
            best_batch_score = max(best_batch_score, score)
        
        if best_batch_score > best_score_overall:
            best_score_overall = best_batch_score
            best_loop = loop
        
        print(f"Cycle {loop+1} Complete. Best Score: {best_batch_score}")

        if loop % 20 == 0:
            pingme(f"Cycle {loop+1}/{TOTAL_LOOPS} Complete. Best Score so far: {best_score_overall}")
    
    print(f"=== Optimization Complete. Best Score: {best_score_overall} at Loop {best_loop+1} ===")
    pingme(f"=== Optimization Complete. Best Score: {best_score_overall} at Loop {best_loop+1} ===")