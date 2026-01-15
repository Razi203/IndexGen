# Example Configuration Files

This directory contains ready-to-use example configuration files for IndexGen. Copy any file to your working directory and modify as needed.

## Quick Reference

| File | Description | Edit Dist | Constraints | GPU |
|------|-------------|-----------|-------------|-----|
| `all_params_reference.jsonc` | Annotated reference with all options | - | - | - |
| `linear_code_ed3_basic.json` | Basic LinearCode, no constraints | 3 | None | ✓ |
| `linear_code_ed4_constrained.json` | With GC & run constraints | 4 | GC + Run | ✓ |
| `linear_code_ed5_gpu.json` | High edit distance, multi-length | 5 | GC + Run | ✓ |
| `linear_code_random_vectors.json` | Random bias/permutations | 3 | GC + Run | ✓ |
| `linear_code_bias_perms.json` | Manual bias/permutations | 3 | None | ✓ |
| `cpu_only.json` | CPU-only (no GPU) | 3 | GC + Run | ✗ |
| `with_clustering.json` | Cluster-based iterative solving | 3 | GC + Run | ✓ |
| `vtcode_basic.json` | VT Code method | 3 | GC + Run | ✓ |
| `diff_vtcode_basic.json` | Differential VT Code | 3 | GC + Run | ✓ |
| `file_read_example.json` | Read from file | 3 | GC + Run | ✓ |
| `strict_gc_no_runs.json` | Strict GC (45-55%) | 4 | GC only | ✓ |
| `strict_runs_no_gc.json` | Strict runs (max 2) | 4 | Run only | ✓ |
| `multi_length_sweep.json` | Lengths 11-15 sweep | 3 | GC + Run | ✓ |

## Usage

```bash
# Run with an example config
./IndexGen --config config/examples/linear_code_ed3_basic.json

# Copy and customize
cp config/examples/linear_code_ed4_constrained.json my_config.json
# Edit my_config.json as needed
./IndexGen --config my_config.json

# Override specific parameters via CLI
./IndexGen --config my_config.json --clusterConvergence 2 --lenStart 12
```

## Parameter Notes

### Constraints
- `maxRun: 0` = **disabled** (any run length allowed)
- `minGC: 0, maxGC: 0` = **disabled** (any GC content allowed)

### LinearCode Vector Modes
- `default` = zero bias, identity permutation
- `random` = randomly generated (use with `randomSeed` for reproducibility)
- `manual` = user-specified arrays

### GPU vs CPU
- GPU mode requires CUDA-capable NVIDIA GPU and Python environment (`cuda_env`)
- CPU mode works everywhere but is slower for large candidate sets (>10,000)
